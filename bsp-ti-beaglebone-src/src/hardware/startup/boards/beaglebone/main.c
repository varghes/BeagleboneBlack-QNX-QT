/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */


#include "startup.h"
#include <arm/omap.h>
#include <arm/am335x.h>
#include <arm/beaglebone.h>

#define SHOW_CAPEMUX_CHOICES			1
#define DONT_SHOW_CAPEMUX_CHOICES		0

#define UWORD32					uint32_t
#define WR_MEM_32(addr, data) 	out32(addr, data)
#define RD_MEM_32(addr) 		in32(addr)

extern struct callout_rtn   reboot_am335x;
extern void init_qtime_am335x(void);
extern void init_pinmux(int ethif);
extern void init_pinmux_capes(int profile, int showchoices);
extern void init_edma(void);
extern void get_boardid_i2c(BEAGLEBONE_ID *boneid);
extern unsigned long detect_frequency_using_power();

#define delay(x) {volatile unsigned _delay = x; while (_delay--); } /* 5+ cycles per loop */
#define delay_cy(x) {unsigned _delay = ((x+1)>>1); while (_delay--) asm volatile (""); }  /* ~2 cycles per loop at 720MHz, x in 1/720us units (720MHz clock) */

const struct callout_slot callouts[] = {
    { CALLOUT_SLOT( reboot, _am335x ) },
};

const struct debug_device debug_devices[] = {
    {   "8250",
        {   "0x44E09000^2.0.48000000.16",   // UART0, use the baud rate set by boot loader
        },
        init_omap,
        put_omap,
        {   &display_char_8250,
            &poll_key_8250,
            &break_detect_8250,
        }
    },
};

void Print_PLL_Config(char *str, UWORD32 CLKIN, UWORD32 cm_clkmode, UWORD32 cm_clksel, UWORD32 cm_div_m2)
{
	UWORD32 N,M,M2,mult,freq,clkmode,clksel,div_m2;

	clkmode = RD_MEM_32(cm_clkmode);
	clksel  = RD_MEM_32(cm_clksel);
	div_m2  = RD_MEM_32(cm_div_m2);

	M  = (clksel>>8)&0x7FF;		// Multiplier bits 18-8
	N  = (clksel>>0)&0x03F;		// Divisor bits 6-0
	M2 = (div_m2>>0)&0x1F;		// Post-divisor bits 4-0
	mult = (CLKIN * M) / (N+1);
	freq = mult / M2;
	switch (clkmode&7)
	{
	case  4: kprintf("%s DPLL in MN Bypass mode\n", str); break;
	case  5: kprintf("%s DPLL in Idle Bypass Low Power mode\n", str); break;
	case  6: kprintf("%s DPLL in Idle Bypass Fast Relock mode\n", str); break;
	case  7: kprintf("%s DPLL in Lock mode:\n", str);
			 kprintf("  %s clock %d Mhz [%d/%d]\n", str, freq, mult, M2);
			 break;
	default: kprintf("%s DPLL in Reserved mode\n", str); break;
	}
}

void Print_Core_PLL_Config(char *str, UWORD32 CLKIN, UWORD32 cm_clkmode, UWORD32 cm_clksel)
{
	UWORD32 N,M,M4,M5,M6,mult,clkmode,clksel,div_m4,div_m5,div_m6;

	clkmode = RD_MEM_32(cm_clkmode);
	clksel  = RD_MEM_32(cm_clksel);
	div_m4  = RD_MEM_32(AM335X_CM_DIV_M4_DPLL_CORE);
	div_m5  = RD_MEM_32(AM335X_CM_DIV_M5_DPLL_CORE);
	div_m6  = RD_MEM_32(AM335X_CM_DIV_M6_DPLL_CORE);

	M  = (clksel>>8)&0x7FF;		// Multiplier bits 18-8
	N  = (clksel>>0)&0x03F;		// Divisor bits 6-0
	M4 = (div_m4>>0)&0x1F;		// Post-divisor bits 4-0
	M5 = (div_m5>>0)&0x1F;		// Post-divisor bits 4-0
	M6 = (div_m6>>0)&0x1F;		// Post-divisor bits 4-0
	mult = (CLKIN * M) / (N+1);
	switch (clkmode&7)
	{
	case  4: kprintf("%s DPLL in MN Bypass mode\n", str); break;
	case  5: kprintf("%s DPLL in Idle Bypass Low Power mode\n", str); break;
	case  6: kprintf("%s DPLL in Idle Bypass Fast Relock mode\n", str); break;
	case  7: kprintf("%s DPLL in Lock mode:\n", str);
			 kprintf("  M4 %s clock %d Mhz [%d/%d]\n", str, mult/M4, mult, M4);
			 kprintf("  M5 %s clock %d Mhz [%d/%d]\n", str, mult/M5, mult, M5);
			 kprintf("  M6 %s clock %d Mhz [%d/%d]\n", str, mult/M6, mult, M6);
			 break;
	default: kprintf("%s DPLL in Reserved mode\n", str); break;
	}
}

static void display_PLLs(void)
{
	Print_PLL_Config     ("DDR ", 24, AM335X_CM_CLKMODE_DPLL_DDR , AM335X_CM_CLKSEL_DPLL_DDR , AM335X_CM_DIV_M2_DPLL_DDR );
	Print_PLL_Config     ("Disp", 24, AM335X_CM_CLKMODE_DPLL_DISP, AM335X_CM_CLKSEL_DPLL_DISP, AM335X_CM_DIV_M2_DPLL_DISP);
	Print_PLL_Config     ("MPU ", 24, AM335X_CM_CLKMODE_DPLL_MPU , AM335X_CM_CLKSEL_DPLL_MPU , AM335X_CM_DIV_M2_DPLL_MPU );
	Print_PLL_Config     ("PER ", 24, AM335X_CM_CLKMODE_DPLL_PER , AM335X_CM_CLKSEL_DPLL_PER , AM335X_CM_DIV_M2_DPLL_PER );
	Print_Core_PLL_Config("CORE", 24, AM335X_CM_CLKMODE_DPLL_CORE, AM335X_CM_CLKSEL_DPLL_CORE);
}

#if DBG
static void display_clocks(char *text)
{
	uint32_t r;

	kprintf(text);

	r = in32(AM335X_CM_PER_L4LS_CLKSTCTRL);
	kprintf("AM335X_CM_PER_L4LS_CLKSTCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L3S_CLKSTCTRL);
	kprintf("AM335X_CM_PER_L3S_CLKSTCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L4FW_CLKSTCTRL);
	kprintf("AM335X_CM_PER_L4FW_CLKSTCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L3_CLKSTCTRL);
	kprintf("AM335X_CM_PER_L3_CLKSTCTRL %x\n", r);
	r = in32(AM335X_CM_PER_PCIE_CLKCTRL);
	kprintf("AM335X_CM_PER_PCIE_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_CPGMAC0_CLKCTRL);
	kprintf("AM335X_CM_PER_CPGMAC0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_LCDC_CLKCTRL);
	kprintf("AM335X_CM_PER_LCDC_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_USB0_CLKCTRL);
	kprintf("AM335X_CM_PER_USB0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MLB_CLKCTRL);
	kprintf("AM335X_CM_PER_MLB_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TPTC0_CLKCTRL);
	kprintf("AM335X_CM_PER_TPTC0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_EMIF_CLKCTRL);
	kprintf("AM335X_CM_PER_EMIF_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_OCMCRAM_CLKCTRL);
	kprintf("AM335X_CM_PER_OCMCRAM_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_GPMC_CLKCTRL);
	kprintf("AM335X_CM_PER_GPMC_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MCASP0_CLKCTRL);
	kprintf("AM335X_CM_PER_MCASP0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_UART5_CLKCTRL);
	kprintf("AM335X_CM_PER_UART5_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MMC0_CLKCTRL);
	kprintf("AM335X_CM_PER_MMC0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_ELM_CLKCTRL);
	kprintf("AM335X_CM_PER_ELM_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_I2C2_CLKCTRL);
	kprintf("AM335X_CM_PER_I2C2_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_I2C1_CLKCTRL);
	kprintf("AM335X_CM_PER_I2C1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_SPI0_CLKCTRL);
	kprintf("AM335X_CM_PER_SPI0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_SPI1_CLKCTRL);
	kprintf("AM335X_CM_PER_SPI1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L4LS_CLKCTRL);
	kprintf("AM335X_CM_PER_L4LS_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L4FW_CLKCTRL);
	kprintf("AM335X_CM_PER_L4FW_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MCASP1_CLKCTRL);
	kprintf("AM335X_CM_PER_MCASP1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_UART1_CLKCTRL);
	kprintf("AM335X_CM_PER_UART1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_UART2_CLKCTRL);
	kprintf("AM335X_CM_PER_UART2_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_UART3_CLKCTRL);
	kprintf("AM335X_CM_PER_UART3_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_UART4_CLKCTRL);
	kprintf("AM335X_CM_PER_UART4_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TIMER7_CLKCTRL);
	kprintf("AM335X_CM_PER_TIMER7_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TIMER2_CLKCTRL);
	kprintf("AM335X_CM_PER_TIMER2_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TIMER3_CLKCTRL);
	kprintf("AM335X_CM_PER_TIMER3_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TIMER4_CLKCTRL);
	kprintf("AM335X_CM_PER_TIMER4_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MCASP2_CLKCTRL);
	kprintf("AM335X_CM_PER_MCASP2_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_RNG_CLKCTRL);
	kprintf("AM335X_CM_PER_RNG_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_AES0_CLKCTRL);
	kprintf("AM335X_CM_PER_AES0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_AES1_CLKCTRL);
	kprintf("AM335X_CM_PER_AES1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_DES_CLKCTRL);
	kprintf("AM335X_CM_PER_DES_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_SHA0_CLKCTRL);
	kprintf("AM335X_CM_PER_SHA0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_PKA_CLKCTRL);
	kprintf("AM335X_CM_PER_PKA_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_GPIO6_CLKCTRL);
	kprintf("AM335X_CM_PER_GPIO6_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_GPIO1_CLKCTRL);
	kprintf("AM335X_CM_PER_GPIO1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_GPIO2_CLKCTRL);
	kprintf("AM335X_CM_PER_GPIO2_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_GPIO3_CLKCTRL);
	kprintf("AM335X_CM_PER_GPIO3_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_GPIO4_CLKCTRL);
	kprintf("AM335X_CM_PER_GPIO4_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TPCC_CLKCTRL);
	kprintf("AM335X_CM_PER_TPCC_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_DCAN0_CLKCTRL);
	kprintf("AM335X_CM_PER_DCAN0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_DCAN1_CLKCTRL);
	kprintf("AM335X_CM_PER_DCAN1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_EPWMSS1_CLKCTRL);
	kprintf("AM335X_CM_PER_EPWMSS1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_EMIF_FW_CLKCTRL);
	kprintf("AM335X_CM_PER_EMIF_FW_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L3_INSTR_CLKCTRL);
	kprintf("AM335X_CM_PER_L3_INSTR_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L3_CLKCTRL);
	kprintf("AM335X_CM_PER_L3_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_IEEE5000_CLKCTRL);
	kprintf("AM335X_CM_PER_IEEE5000_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_ICSS_CLKCTRL);
	kprintf("AM335X_CM_PER_ICSS_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TIMER5_CLKCTRL);
	kprintf("AM335X_CM_PER_TIMER5_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TIMER6_CLKCTRL);
	kprintf("AM335X_CM_PER_TIMER6_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MMC1_CLKCTRL);
	kprintf("AM335X_CM_PER_MMC1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MMC2_CLKCTRL);
	kprintf("AM335X_CM_PER_MMC2_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TPTC1_CLKCTRL);
	kprintf("AM335X_CM_PER_TPTC1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_TPTC2_CLKCTRL);
	kprintf("AM335X_CM_PER_TPTC2_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_GPIO5_CLKCTRL);
	kprintf("AM335X_CM_PER_GPIO5_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_SPINLOCK_CLKCTRL);
	kprintf("AM335X_CM_PER_SPINLOCK_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MAILBOX0_CLKCTRL);
	kprintf("AM335X_CM_PER_MAILBOX0_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L4HS_CLKSTCTRL);
	kprintf("AM335X_CM_PER_L4HS_CLKSTCTRL %x\n", r);
	r = in32(AM335X_CM_PER_L4HS_CLKCTRL);
	kprintf("AM335X_CM_PER_L4HS_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_OCPWP_L3_CLKSTCT);
	kprintf("AM335X_CM_PER_OCPWP_L3_CLKSTCT %x\n", r);
	r = in32(AM335X_CM_PER_OCPWP_CLKCTRL);
	kprintf("AM335X_CM_PER_OCPWP_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_MAILBOX1_CLKCTRL);
	kprintf("AM335X_CM_PER_MAILBOX1_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_ICSS_CLKSTCTRL);
	kprintf("AM335X_CM_PER_ICSS_CLKSTCTRL %x\n", r);
	r = in32(AM335X_CM_PER_CPSW_CLKSTCTRL);
	kprintf("AM335X_CM_PER_CPSW_CLKSTCTRL %x\n", r);
	r = in32(AM335X_CM_PER_LCDC_CLKSTCTRL);
	kprintf("AM335X_CM_PER_LCDC_CLKSTCTRL %x\n", r);
	r = in32(AM335X_CM_PER_CLKDIV32K_CLKCTRL);
	kprintf("AM335X_CM_PER_CLKDIV32K_CLKCTRL %x\n", r);
	r = in32(AM335X_CM_PER_CLK_24MHZ_CLKSTCT);
	kprintf("AM335X_CM_PER_CLK_24MHZ_CLKSTCT %x\n", r);


	r = in32(AM335X_CLKSEL_TIMER7_CLK);
	kprintf("AM335X_CLKSEL_TIMER7_CLK %x\n", r);
	r = in32(AM335X_CLKSEL_TIMER2_CLK);
	kprintf("AM335X_CLKSEL_TIMER2_CLK %x\n", r);
	r = in32(AM335X_CLKSEL_TIMER3_CLK);
	kprintf("AM335X_CLKSEL_TIMER3_CLK %x\n", r);
	r = in32(AM335X_CLKSEL_TIMER4_CLK);
	kprintf("AM335X_CLKSEL_TIMER4_CLK %x\n", r);
	r = in32(AM335X_CM_MAC_CLKSEL);
	kprintf("AM335X_CM_MAC_CLKSEL %x\n", r);
	r = in32(AM335X_CLKSEL_TIMER5_CLK);
	kprintf("AM335X_CLKSEL_TIMER5_CLK %x\n", r);
	r = in32(AM335X_CLKSEL_TIMER6_CLK);
	kprintf("AM335X_CLKSEL_TIMER6_CLK %x\n", r);
	r = in32(AM335X_CM_CPTS_RFT_CLKSEL);
	kprintf("AM335X_CM_CPTS_RFT_CLKSEL %x\n", r);
	r = in32(AM335X_CLKSEL_TIMER1MS_CLK);
	kprintf("AM335X_CLKSEL_TIMER1MS_CLK %x\n", r);
	r = in32(AM335X_CLKSEL_GFX_FCLK);
	kprintf("AM335X_CLKSEL_GFX_FCLK %x\n", r);
	r = in32(AM335X_CLKSEL_ICSS_OCP_CLK);
	kprintf("AM335X_CLKSEL_ICSS_OCP_CLK %x\n", r);
	r = in32(AM335X_CLKSEL_LCDC_PIXEL_CLK);
	kprintf("AM335X_CLKSEL_LCDC_PIXEL_CLK %x\n", r);
	r = in32(AM335X_CLKSEL_WDT1_CLK);
	kprintf("AM335X_CLKSEL_WDT1_CLK %x\n", r);
	r = in32(AM335X_CLKSEL_GPIO0_DBCLK);
	kprintf("AM335X_CLKSEL_GPIO0_DBCLK %x\n", r);

	r = in32(AM335X_CM_AUTOIDLE_DPLL_DISP);
	kprintf("AM335X_CM_AUTOIDLE_DPLL_DISP %x\n", r);
	r = in32(AM335X_CM_IDLEST_DPLL_DISP);
	kprintf("AM335X_CM_IDLEST_DPLL_DISP %x\n", r);
	r = in32(AM335X_CM_SSC_DELTAMSTEP_DPLL_DISP);
	kprintf("AM335X_CM_SSC_DELTAMSTEP_DPLL_DISP %x\n", r);
	r = in32(AM335X_CM_SSC_MODFREQDIV_DPLL_DISP);
	kprintf("AM335X_CM_SSC_MODFREQDIV_DPLL_DISP %x\n", r);
	r = in32(AM335X_CM_CLKSEL_DPLL_DISP);
	kprintf("AM335X_CM_CLKSEL_DPLL_DISP %x\n", r);
	r = in32(AM335X_CM_CLKMODE_DPLL_DISP);
	kprintf("AM335X_CM_CLKMODE_DPLL_DISP %x\n", r);
	r = in32(AM335X_CM_DIV_M2_DPLL_DISP);
	kprintf("AM335X_CM_DIV_M2_DPLL_DISP %x\n", r);

	r = in32(AM335X_CM_AUTOIDLE_DPLL_DDR);
	kprintf("AM335X_CM_AUTOIDLE_DPLL_DDR %x\n", r);
	r = in32(AM335X_CM_IDLEST_DPLL_DDR);
	kprintf("AM335X_CM_IDLEST_DPLL_DDR %x\n", r);
	r = in32(AM335X_CM_SSC_DELTAMSTEP_DPLL_DDR);
	kprintf("AM335X_CM_SSC_DELTAMSTEP_DPLL_DDR %x\n", r);
	r = in32(AM335X_CM_SSC_MODFREQDIV_DPLL_DDR);
	kprintf("AM335X_CM_SSC_MODFREQDIV_DPLL_DDR %x\n", r);
	r = in32(AM335X_CM_CLKSEL_DPLL_DDR);
	kprintf("AM335X_CM_CLKSEL_DPLL_DDR %x\n", r);
	r = in32(AM335X_CM_CLKMODE_DPLL_DDR);
	kprintf("AM335X_CM_CLKMODE_DPLL_DDR %x\n", r);
	r = in32(AM335X_CM_DIV_M2_DPLL_DDR);
	kprintf("AM335X_CM_DIV_M2_DPLL_DDR %x\n", r);

	r = in32(AM335X_CM_AUTOIDLE_DPLL_MPU);
	kprintf("AM335X_CM_AUTOIDLE_DPLL_MPU %x\n", r);
	r = in32(AM335X_CM_IDLEST_DPLL_MPU);
	kprintf("AM335X_CM_IDLEST_DPLL_MPU %x\n", r);
	r = in32(AM335X_CM_SSC_DELTAMSTEP_DPLL_MPU);
	kprintf("AM335X_CM_SSC_DELTAMSTEP_DPLL_MPU %x\n", r);
	r = in32(AM335X_CM_SSC_MODFREQDIV_DPLL_MPU);
	kprintf("AM335X_CM_SSC_MODFREQDIV_DPLL_MPU %x\n", r);
	r = in32(AM335X_CM_CLKSEL_DPLL_MPU);
	kprintf("AM335X_CM_CLKSEL_DPLL_MPU %x\n", r);
	r = in32(AM335X_CM_CLKMODE_DPLL_MPU);
	kprintf("AM335X_CM_CLKMODE_DPLL_MPU %x\n", r);
	r = in32(AM335X_CM_DIV_M2_DPLL_MPU);
	kprintf("AM335X_CM_DIV_M2_DPLL_MPU %x\n", r);
	r = in32(CM_WKUP_ADC_TSC_CLKCTRL);
	kprintf("CM_WKUP_ADC_TSC_CLKCTRL %x\n", r);

	r = in32(AM335X_GPIO0_BASE+GPIO_CTRL);
	kprintf("AM335X_GPIO0_BASE %x\n", r);
	r = in32(AM335X_GPIO1_BASE+GPIO_CTRL);
	kprintf("AM335X_GPIO1_BASE %x\n", r);
	r = in32(AM335X_GPIO2_BASE+GPIO_CTRL);
	kprintf("AM335X_GPIO2_BASE %x\n", r);
	r = in32(AM335X_GPIO3_BASE+GPIO_CTRL);
	kprintf("AM335X_GPIO3_BASE %x\n", r);

	/* Read CPSW */
	r = in32(0x4A100000);
	kprintf("cpsw id %x\n", r);
}
#endif

void Peripheral_PLL_Config(uint32_t N, uint32_t M, uint32_t M2)
{
	uint32_t clkmode, clksel, div_m2;

	clkmode = in32(AM335X_CM_CLKMODE_DPLL_PER);
	clksel = in32(AM335X_CM_CLKSEL_DPLL_PER);
	div_m2 = in32(AM335X_CM_DIV_M2_DPLL_PER);

	/* Set the PLL to bypass Mode */
	out32(AM335X_CM_CLKMODE_DPLL_PER, DPLL_MN_BYP_MODE);

	while(in32(AM335X_CM_IDLEST_DPLL_PER) != ST_MN_BYPASS); // correct register?

	clksel = clksel & (~DPLL_MULT_MASK);
	clksel = clksel | (DPLL_MULT(M) | DPLL_DIV(N));
	out32(AM335X_CM_CLKSEL_DPLL_PER, clksel);

	div_m2 = div_m2 & ~DPLL_CLKOUT_DIV_MASK;
	div_m2 = div_m2 | M2;
	out32(AM335X_CM_DIV_M2_DPLL_PER, div_m2);

	clkmode = clkmode | DPLL_LOCK_MODE;
	out32(AM335X_CM_CLKMODE_DPLL_PER, clkmode);

	while(in32(AM335X_CM_IDLEST_DPLL_PER) != 0x1);

}

void CORE_PLL_Config(UWORD32 CLKIN, UWORD32 N,UWORD32 M,UWORD32 M4,UWORD32 M5,UWORD32 M6)
{
	UWORD32 ref_clk,clk_out4 = 0;
	UWORD32 clkmode,clksel,div_m4,div_m5,div_m6;
	UWORD32 clk_out5,clk_out6;

#if DBG
	kprintf("\t **** CORE PLL Config is in progress ..........\n");
#endif

	ref_clk  = CLKIN/(N+1);
	clk_out4 = (ref_clk*M)*2/M4;
	clk_out5 = (ref_clk*M)*2/M5;
	clk_out6 = (ref_clk*M)*2/M6;

	clkmode = RD_MEM_32(AM335X_CM_CLKMODE_DPLL_CORE);
	clksel  = RD_MEM_32(AM335X_CM_CLKSEL_DPLL_CORE);
	div_m4  = RD_MEM_32(AM335X_CM_DIV_M4_DPLL_CORE);
	div_m5  = RD_MEM_32(AM335X_CM_DIV_M5_DPLL_CORE);
	div_m6  = RD_MEM_32(AM335X_CM_DIV_M6_DPLL_CORE);

	clkmode = (clkmode&0xfffffff8)|0x00000004;
	WR_MEM_32(AM335X_CM_CLKMODE_DPLL_CORE, clkmode);
	while((RD_MEM_32(AM335X_CM_IDLEST_DPLL_CORE) & 0x00000100 )!=0x00000100);

	clksel = ((M) <<0x8) | (N );
	WR_MEM_32(AM335X_CM_CLKSEL_DPLL_CORE,clksel);
	div_m4 = M4;
	WR_MEM_32(AM335X_CM_DIV_M4_DPLL_CORE,div_m4);
	div_m5 = M5;
	WR_MEM_32(AM335X_CM_DIV_M5_DPLL_CORE,div_m5);
	div_m6 = M6;
	WR_MEM_32(AM335X_CM_DIV_M6_DPLL_CORE,div_m6);
	clkmode =(clkmode&0xfffffff8)|0x00000007;
	WR_MEM_32(AM335X_CM_CLKMODE_DPLL_CORE,clkmode);

#if DBG
	kprintf("\t **** CORE PLL Config is in progress being Locked ..........	\n");
#endif

	while ((RD_MEM_32(AM335X_CM_IDLEST_DPLL_CORE) & 0x00000001 )!=0x00000001);

#if DBG
	kprintf("CLK_OUT4 : %d \n", clk_out4);
	kprintf("CLK_OUT5 : %d \n", clk_out5);
	kprintf("CLK_OUT6 : %d \n", clk_out6);
	kprintf("\t **** CORE PLL Config is DONE .......... \n");
#endif
}

void Display_PLL_Config(UWORD32 CLKIN, UWORD32 N, UWORD32 M, UWORD32 M2)
{
	UWORD32 ref_clk,clk_out2 = 0;
	UWORD32 clkmode,clksel,div_m2;

#if DBG
	kprintf("\t **** DISP PLL Config is in progress ..........\n");
#endif

	ref_clk  = CLKIN/(N+1);
	clk_out2 = (ref_clk*M)*2/M2;

	clkmode = RD_MEM_32(AM335X_CM_CLKMODE_DPLL_DISP);
	clksel  = RD_MEM_32(AM335X_CM_CLKSEL_DPLL_DISP);
	div_m2  = RD_MEM_32(AM335X_CM_DIV_M2_DPLL_DISP);

	clkmode = (clkmode&0xfffffff8)|0x00000004;
	WR_MEM_32(AM335X_CM_CLKMODE_DPLL_DISP, clkmode);
	while ((RD_MEM_32(AM335X_CM_IDLEST_DPLL_DISP) & 0x00000100 )!=0x00000100);

	clksel = ((M) <<0x8) | (N );
	WR_MEM_32(AM335X_CM_CLKSEL_DPLL_DISP, clksel);

	div_m2 = M2;
	WR_MEM_32(AM335X_CM_DIV_M2_DPLL_DISP, div_m2);

	clkmode =(clkmode&0xfffffff8)|0x00000007;
	WR_MEM_32(AM335X_CM_CLKMODE_DPLL_DISP,clkmode);

#if DBG
	kprintf("\t **** DISP PLL Config is in progress being Locked ..........	\n");
#endif

	while ((RD_MEM_32(AM335X_CM_IDLEST_DPLL_DISP) & 0x00000001 )!=0x00000001);

#if DBG
	kprintf("CLK_OUT2 : %d \n", clk_out2);
	kprintf("\t **** DISP PLL Config is DONE .......... \n");
#endif
}

void DDR_PLL_Config(UWORD32 CLKIN, UWORD32 N, UWORD32 M, UWORD32 M2)
{
	UWORD32 ref_clk,clk_out2 = 0;
	UWORD32 clkmode,clksel,div_m2;

	kprintf("\t **** DDR PLL Config is in progress ..........\n");

	ref_clk  = CLKIN/(N+1);
	clk_out2 = (ref_clk*M)*2/M2;

	clkmode = RD_MEM_32(AM335X_CM_CLKMODE_DPLL_DDR);
	clksel  = RD_MEM_32(AM335X_CM_CLKSEL_DPLL_DDR);
	div_m2  = RD_MEM_32(AM335X_CM_DIV_M2_DPLL_DDR);

	clkmode = (clkmode&0xfffffff8)|0x00000004;
	WR_MEM_32(AM335X_CM_CLKMODE_DPLL_DDR, clkmode);
	while ((RD_MEM_32(AM335X_CM_IDLEST_DPLL_DDR) & 0x00000100 )!=0x00000100);

	clksel = ((M) <<0x8) | (N );
	WR_MEM_32(AM335X_CM_CLKSEL_DPLL_DDR, clksel);

	div_m2 = M2;
	WR_MEM_32(AM335X_CM_DIV_M2_DPLL_DDR, div_m2);

	clkmode =(clkmode&0xfffffff8)|0x00000007;
	WR_MEM_32(AM335X_CM_CLKMODE_DPLL_DDR,clkmode);

#if DBG
	kprintf("\t **** DDR PLL Config is in progress being Locked ..........	\n");
#endif

	while ((RD_MEM_32(AM335X_CM_IDLEST_DPLL_DDR) & 0x00000001 )!=0x00000001);

#if DBG
	kprintf("CLK_OUT2 : %d \n", clk_out2);
	kprintf("\t **** DDR PLL Config is DONE .......... \n");
#endif
}


/*
 * Enable clocks
 */
static void init_clocks(void)
{
	/* GPIO */
	out32(AM335X_CM_PER_GPIO1_CLKCTRL, 2);
	out32(AM335X_CM_PER_GPIO2_CLKCTRL, 2);
	out32(AM335X_CM_PER_GPIO3_CLKCTRL, 2);

	/* LCD */
	out32(AM335X_CM_PER_OCPWP_CLKCTRL, 2);
	out32(AM335X_CM_PER_LCDC_CLKCTRL, 2);
	out32(AM335X_CM_PER_LCDC_CLKSTCTRL, 2);
	out32(AM335X_CM_PER_OCMCRAM_CLKCTRL, 2);
	out32(AM335X_CLKSEL_LCDC_PIXEL_CLK, 0);

	out32(AM335X_CM_PER_L4LS_CLKCTRL, 2);
	out32(AM335X_CM_PER_L4FW_CLKCTRL, 2);

	out32(AM335X_CM_PER_I2C1_CLKCTRL, 2);
	out32(AM335X_CM_PER_I2C2_CLKCTRL, 2);

	out32(AM335X_CM_PER_MLB_CLKCTRL, 2);
	out32(AM335X_CM_PER_TPTC0_CLKCTRL, 2);
	out32(AM335X_CM_PER_TPTC1_CLKCTRL, 2);
	out32(AM335X_CM_PER_TPTC2_CLKCTRL, 2);
	out32(AM335X_CM_PER_TIMER2_CLKCTRL, 2);
	out32(AM335X_CM_PER_TIMER3_CLKCTRL, 2);
	out32(AM335X_CM_PER_TIMER4_CLKCTRL, 2);
	out32(AM335X_CM_PER_TIMER5_CLKCTRL, 2);
	out32(AM335X_CM_PER_TIMER6_CLKCTRL, 2);
	out32(AM335X_CM_PER_TIMER7_CLKCTRL, 2);
	out32(AM335X_CM_PER_TPCC_CLKCTRL, 2);
	out32(AM335X_CM_PER_EPWMSS1_CLKCTRL, 2);
	out32(AM335X_CM_PER_SPINLOCK_CLKCTRL, 2);
	out32(AM335X_CM_PER_MAILBOX0_CLKCTRL, 2);
	out32(AM335X_CM_PER_UART1_CLKCTRL, 2);
	out32(AM335X_CM_PER_UART2_CLKCTRL, 2);
	out32(AM335X_CM_PER_UART3_CLKCTRL, 2);
	out32(AM335X_CM_PER_UART4_CLKCTRL, 2);
	out32(AM335X_CM_PER_UART5_CLKCTRL, 2);

	/* GPMC */
	out32(AM335X_CM_PER_GPMC_CLKCTRL, 2);

	/* MMC0 */
	out32(AM335X_CM_PER_MMC0_CLKCTRL, 2);

	/* MMC1 */
	out32(AM335X_CM_PER_MMC1_CLKCTRL, 2);

	/* USB */
	out32(AM335X_CM_PER_USB0_CLKCTRL, 2);

	/* Ethernet */
	out32(AM335X_CM_PER_CPSW_CLKSTCTRL, 2);
	out32(AM335X_CM_PER_CPGMAC0_CLKCTRL, 2);

	/* CAN */
	out32(AM335X_CM_PER_DCAN0_CLKCTRL, 2);
	out32(AM335X_CM_PER_DCAN1_CLKCTRL, 2);

	/* SPI */
	out32(AM335X_CM_PER_SPI0_CLKCTRL, 2);
	out32(AM335X_CM_PER_SPI1_CLKCTRL, 2);

	//Display_PLL_Config(24000000, 23, 200, 1);
	Display_PLL_Config(24000000, 23, 300, 1);

	/* MCASP0 */
	out32(AM335X_CM_PER_MCASP0_CLKCTRL, 2);

	/* MCASP1 */
	out32(AM335X_CM_PER_MCASP1_CLKCTRL, 2);

	/* TSCADC */
	out32(CM_WKUP_ADC_TSC_CLKCTRL, 2);

	/* Enable 960MHz clock from peripheral PLL to USB PHY:
	 * CLKDCOLDO = (M / (N+1)) * CLKINP
	 * Master oscillator is 24MHz so:
	 * 960000000 = (960 / (24)) * 24000000
	 *
	 * Use M2 of 5 to get divided output of 192MHz
	 */
	Peripheral_PLL_Config(23, 960, 5);
}

/*
 * main()
 *  Startup program executing out of RAM
 *
 * 1. It gathers information about the system and places it in a structure
 *    called the system page. The kernel references this structure to
 *    determine everything it needs to know about the system. This structure
 *    is also available to user programs (read only if protection is on)
 *    via _syspage->.
 *
 * 2. It (optionally) turns on the MMU and starts the next program
 *    in the image file system.
 */
int
main(int argc, char **argv, char **envv)
{
    int				opt;
	char			*p;
	paddr_t			resmem_addr[32];
	size_t			resmem_size[32];
	int				dsp_mem_count = 0;
	paddr_t			linkmem_addr = 0;
	size_t			linkmem_size = 0;
	int				link_present = 0;
    BEAGLEBONE_ID	boneid;

    add_callout_array(callouts, sizeof(callouts));

    while ((opt = getopt(argc, argv, COMMON_OPTIONS_STRING "L:x:pglsW")) != -1) {
        switch (opt) {
			case 'x':
				resmem_addr[dsp_mem_count] = getsize(optarg, &p);
				if (*p == ',')
				{
					resmem_size[dsp_mem_count] = getsize(p + 1, &p);
				}
				dsp_mem_count++;
				break;
			case 'L': linkmem_addr = getsize(optarg, &p); 
				if (*p == ',') { 
					linkmem_size = getsize(p + 1, &p); 
				}
				link_present = 1; 
				break; 
            default:
                handle_common_option(opt);
                break;
        }
    }

    /*
     * Initialize debugging output
     */
    select_debug(debug_devices, sizeof(debug_devices));

#if DBG
    display_clocks("-- clock before init -------\n");
#endif

    init_clocks();

#if DBG
    display_clocks("-- clock after init -------\n");
#endif

    init_pinmux(GMII);

    display_PLLs();

    // Detect beaglebone and capes
    get_boardid_i2c(&boneid);

    // This BSP assumes no capes. If there are any capes, select a profile
    // for init_pinmux_capes and implement the profile for your cape. Some
    // example code is in init_pinmux.c to start with, and replace '0' with
    // your profile number (likely to be profile '1'). Profile '0' is the
    // default profile, eg no cape present, and initialises the pinmux for
    // uart1-5, spi1, and i2c1-2 to be available on the cape connectors.
#if DBG
    init_pinmux_capes(0 /*profile*/, SHOW_CAPEMUX_CHOICES);
#else
    init_pinmux_capes(0 /*profile*/, DONT_SHOW_CAPEMUX_CHOICES);
#endif


    /*
     * Collect information on all free RAM in the system
     */
    init_raminfo();

	init_edma();

    /*
     * Set CPU frequency
     */

	if (cpu_freq == 0)
	{
		cpu_freq = detect_frequency_using_power();
	}

    /* 
     * Remove RAM used by modules in the image
     */
    alloc_ram(shdr->ram_paddr, shdr->ram_size, 1);

    if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)
    {
        init_mmu();
    }

    init_intrinfo();

    init_qtime_am335x();

    init_cacheattr();

    init_cpuinfo();

    init_hwinfo();

    add_typed_string(_CS_MACHINE, "BeagleBone");

    /*
     * Load bootstrap executables in the image file system and Initialise
     * various syspage pointers. This must be the _last_ initialisation done
     * before transferring control to the next program.
     */
    init_system_private();

    /*
     * This is handy for debugging a new version of the startup program.
     * Commenting this line out will save a great deal of code.
     */
    print_syspage();

    kprintf("Jumping to QNX\n");

    return 0;
}


__SRCVERSION( "$URL$ $Rev$" );
