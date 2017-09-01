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
#include <arm/am335x.h>
#include <arm/beaglebone.h>

#define conf_gpmc_ad0			(AM335X_CTRL_BASE+0x0800)
#define conf_gpmc_ad1			(AM335X_CTRL_BASE+0x0804)
#define conf_gpmc_ad2			(AM335X_CTRL_BASE+0x0808)
#define conf_gpmc_ad3			(AM335X_CTRL_BASE+0x080C)
#define conf_gpmc_ad4			(AM335X_CTRL_BASE+0x0810)
#define conf_gpmc_ad5			(AM335X_CTRL_BASE+0x0814)
#define conf_gpmc_ad6			(AM335X_CTRL_BASE+0x0818)
#define conf_gpmc_ad7			(AM335X_CTRL_BASE+0x081C)
#define conf_gpmc_ad8			(AM335X_CTRL_BASE+0x0820)
#define conf_gpmc_ad9			(AM335X_CTRL_BASE+0x0824)
#define conf_gpmc_ad10			(AM335X_CTRL_BASE+0x0828)
#define conf_gpmc_ad11			(AM335X_CTRL_BASE+0x082C)
#define conf_gpmc_ad12			(AM335X_CTRL_BASE+0x0830)
#define conf_gpmc_ad13			(AM335X_CTRL_BASE+0x0834)
#define conf_gpmc_ad14			(AM335X_CTRL_BASE+0x0838)
#define conf_gpmc_ad15			(AM335X_CTRL_BASE+0x083C)
#define conf_gpmc_a0			(AM335X_CTRL_BASE+0x0840)
#define conf_gpmc_a1			(AM335X_CTRL_BASE+0x0844)
#define conf_gpmc_a2			(AM335X_CTRL_BASE+0x0848)
#define conf_gpmc_a3			(AM335X_CTRL_BASE+0x084C)
#define conf_gpmc_a4			(AM335X_CTRL_BASE+0x0850)
#define conf_gpmc_a5			(AM335X_CTRL_BASE+0x0854)
#define conf_gpmc_a6			(AM335X_CTRL_BASE+0x0858)
#define conf_gpmc_a7			(AM335X_CTRL_BASE+0x085C)
#define conf_gpmc_a8			(AM335X_CTRL_BASE+0x0860)
#define conf_gpmc_a9			(AM335X_CTRL_BASE+0x0864)
#define conf_gpmc_a10			(AM335X_CTRL_BASE+0x0868)
#define conf_gpmc_a11			(AM335X_CTRL_BASE+0x086C)
#define conf_gpmc_wait0			(AM335X_CTRL_BASE+0x0870)
#define conf_gpmc_wpn			(AM335X_CTRL_BASE+0x0874)
#define conf_gpmc_be1n			(AM335X_CTRL_BASE+0x0878)
#define conf_gpmc_csn0			(AM335X_CTRL_BASE+0x087C)
#define conf_gpmc_csn1			(AM335X_CTRL_BASE+0x0880)
#define conf_gpmc_csn2			(AM335X_CTRL_BASE+0x0884)
#define conf_gpmc_csn3			(AM335X_CTRL_BASE+0x0888)
#define conf_gpmc_clk			(AM335X_CTRL_BASE+0x088C)
#define conf_gpmc_advn_ale		(AM335X_CTRL_BASE+0x0890)
#define conf_gpmc_oen_ren		(AM335X_CTRL_BASE+0x0894)
#define conf_gpmc_wen			(AM335X_CTRL_BASE+0x0898)
#define conf_gpmc_be0n_cle		(AM335X_CTRL_BASE+0x089C)
#define conf_lcd_data0			(AM335X_CTRL_BASE+0x08A0)
#define conf_lcd_data1			(AM335X_CTRL_BASE+0x08A4)
#define conf_lcd_data2			(AM335X_CTRL_BASE+0x08A8)
#define conf_lcd_data3			(AM335X_CTRL_BASE+0x08AC)
#define conf_lcd_data4			(AM335X_CTRL_BASE+0x08B0)
#define conf_lcd_data5			(AM335X_CTRL_BASE+0x08B4)
#define conf_lcd_data6			(AM335X_CTRL_BASE+0x08B8)
#define conf_lcd_data7			(AM335X_CTRL_BASE+0x08BC)
#define conf_lcd_data8			(AM335X_CTRL_BASE+0x08C0)
#define conf_lcd_data9			(AM335X_CTRL_BASE+0x08C4)
#define conf_lcd_data10			(AM335X_CTRL_BASE+0x08C8)
#define conf_lcd_data11			(AM335X_CTRL_BASE+0x08CC)
#define conf_lcd_data12			(AM335X_CTRL_BASE+0x08D0)
#define conf_lcd_data13			(AM335X_CTRL_BASE+0x08D4)
#define conf_lcd_data14			(AM335X_CTRL_BASE+0x08D8)
#define conf_lcd_data15			(AM335X_CTRL_BASE+0x08DC)
#define conf_lcd_vsync			(AM335X_CTRL_BASE+0x08E0)
#define conf_lcd_hsync			(AM335X_CTRL_BASE+0x08E4)
#define conf_lcd_pclk			(AM335X_CTRL_BASE+0x08E8)
#define conf_lcd_ac_bias_en		(AM335X_CTRL_BASE+0x08EC)
#define conf_mmc0_dat3			(AM335X_CTRL_BASE+0x08F0)
#define conf_mmc0_dat2			(AM335X_CTRL_BASE+0x08F4)
#define conf_mmc0_dat1			(AM335X_CTRL_BASE+0x08F8)
#define conf_mmc0_dat0			(AM335X_CTRL_BASE+0x08FC)
#define conf_mmc0_clk			(AM335X_CTRL_BASE+0x0900)
#define conf_mmc0_cmd			(AM335X_CTRL_BASE+0x0904)
#define conf_mii1_col			(AM335X_CTRL_BASE+0x0908)
#define conf_mii1_crs			(AM335X_CTRL_BASE+0x090C)
#define conf_mii1_rxerr			(AM335X_CTRL_BASE+0x0910)
#define conf_mii1_txen			(AM335X_CTRL_BASE+0x0914)
#define conf_mii1_rxdv			(AM335X_CTRL_BASE+0x0918)
#define conf_mii1_txd3			(AM335X_CTRL_BASE+0x091C)
#define conf_mii1_txd2			(AM335X_CTRL_BASE+0x0920)
#define conf_mii1_txd1			(AM335X_CTRL_BASE+0x0924)
#define conf_mii1_txd0			(AM335X_CTRL_BASE+0x0928)
#define conf_mii1_txclk			(AM335X_CTRL_BASE+0x092C)
#define conf_mii1_rxclk			(AM335X_CTRL_BASE+0x0930)
#define conf_mii1_rxd3			(AM335X_CTRL_BASE+0x0934)
#define conf_mii1_rxd2			(AM335X_CTRL_BASE+0x0938)
#define conf_mii1_rxd1			(AM335X_CTRL_BASE+0x093C)
#define conf_mii1_rxd0			(AM335X_CTRL_BASE+0x0940)
#define conf_rmii1_refclk		(AM335X_CTRL_BASE+0x0944)
#define conf_mdio_data			(AM335X_CTRL_BASE+0x0948)
#define conf_mdio_clk			(AM335X_CTRL_BASE+0x094C)
#define conf_spi0_sclk			(AM335X_CTRL_BASE+0x0950)
#define conf_spi0_d0			(AM335X_CTRL_BASE+0x0954)
#define conf_spi0_d1			(AM335X_CTRL_BASE+0x0958)
#define conf_spi0_cs0			(AM335X_CTRL_BASE+0x095C)
#define conf_spi0_cs1			(AM335X_CTRL_BASE+0x0960)
#define conf_ecap0_in_pwm0_out	(AM335X_CTRL_BASE+0x0964)
#define conf_uart0_ctsn			(AM335X_CTRL_BASE+0x0968)
#define conf_uart0_rtsn			(AM335X_CTRL_BASE+0x096C)
#define conf_uart0_rxd			(AM335X_CTRL_BASE+0x0970)
#define conf_uart0_txd			(AM335X_CTRL_BASE+0x0974)
#define conf_uart1_ctsn			(AM335X_CTRL_BASE+0x0978)
#define conf_uart1_rtsn			(AM335X_CTRL_BASE+0x097C)
#define conf_uart1_rxd			(AM335X_CTRL_BASE+0x0980)
#define conf_uart1_txd			(AM335X_CTRL_BASE+0x0984)
#define conf_i2c0_sda			(AM335X_CTRL_BASE+0x0988)
#define conf_i2c0_scl			(AM335X_CTRL_BASE+0x098C)
#define conf_mcasp0_aclkx		(AM335X_CTRL_BASE+0x0990)
#define conf_mcasp0_fsx			(AM335X_CTRL_BASE+0x0994)
#define conf_mcasp0_axr0		(AM335X_CTRL_BASE+0x0998)
#define conf_mcasp0_ahclkr		(AM335X_CTRL_BASE+0x099C)
#define conf_mcasp0_aclkr		(AM335X_CTRL_BASE+0x09A0)
#define conf_mcasp0_fsr			(AM335X_CTRL_BASE+0x09A4)
#define conf_mcasp0_axr1		(AM335X_CTRL_BASE+0x09A8)
#define conf_mcasp0_ahclkx		(AM335X_CTRL_BASE+0x09AC)
#define conf_xdma_event_intr0	(AM335X_CTRL_BASE+0x09B0)
#define conf_xdma_event_intr1	(AM335X_CTRL_BASE+0x09B4)
#define conf_nresetin_out		(AM335X_CTRL_BASE+0x09B8)
#define conf_porz				(AM335X_CTRL_BASE+0x09BC)
#define conf_nnmi				(AM335X_CTRL_BASE+0x09C0)
#define conf_osc0_in			(AM335X_CTRL_BASE+0x09C4)
#define conf_osc0_out			(AM335X_CTRL_BASE+0x09C8)
#define conf_rsvd1				(AM335X_CTRL_BASE+0x09CC)
#define conf_tms				(AM335X_CTRL_BASE+0x09D0)
#define conf_tdi				(AM335X_CTRL_BASE+0x09D4)
#define conf_tdo				(AM335X_CTRL_BASE+0x09D8)
#define conf_tck				(AM335X_CTRL_BASE+0x09DC)
#define conf_ntrst				(AM335X_CTRL_BASE+0x09E0)
#define conf_emu0				(AM335X_CTRL_BASE+0x09E4)
#define conf_emu1				(AM335X_CTRL_BASE+0x09E8)
#define conf_osc1_in			(AM335X_CTRL_BASE+0x09EC)
#define conf_osc1_out			(AM335X_CTRL_BASE+0x09F0)
#define conf_pmic_power_en		(AM335X_CTRL_BASE+0x09F4)
#define conf_rtc_porz			(AM335X_CTRL_BASE+0x09F8)
#define conf_rsvd2				(AM335X_CTRL_BASE+0x09FC)
#define conf_ext_wakeup			(AM335X_CTRL_BASE+0x0A00)
#define conf_enz_kaldo_1p8v		(AM335X_CTRL_BASE+0x0A04)
#define conf_usb0_dm			(AM335X_CTRL_BASE+0x0A08)
#define conf_usb0_dp			(AM335X_CTRL_BASE+0x0A0C)
#define conf_usb0_ce			(AM335X_CTRL_BASE+0x0A10)
#define conf_usb0_id			(AM335X_CTRL_BASE+0x0A14)
#define conf_usb0_vbus			(AM335X_CTRL_BASE+0x0A18)
#define conf_usb0_drvvbus		(AM335X_CTRL_BASE+0x0A1C)
#define conf_usb1_dm			(AM335X_CTRL_BASE+0x0A20)
#define conf_usb1_dp			(AM335X_CTRL_BASE+0x0A24)
#define conf_usb1_ce			(AM335X_CTRL_BASE+0x0A28)
#define conf_usb1_id			(AM335X_CTRL_BASE+0x0A2C)
#define conf_usb1_vbus			(AM335X_CTRL_BASE+0x0A30)
#define conf_usb1_drvvbus		(AM335X_CTRL_BASE+0x0A34)
#define conf_ddr_resetn			(AM335X_CTRL_BASE+0x0A38)
#define conf_ddr_csn0			(AM335X_CTRL_BASE+0x0A3C)
#define conf_ddr_cke			(AM335X_CTRL_BASE+0x0A40)
#define conf_ddr_ck				(AM335X_CTRL_BASE+0x0A44)
#define conf_ddr_nck			(AM335X_CTRL_BASE+0x0A48)
#define conf_ddr_casn			(AM335X_CTRL_BASE+0x0A4C)
#define conf_ddr_rasn			(AM335X_CTRL_BASE+0x0A50)
#define conf_ddr_wen			(AM335X_CTRL_BASE+0x0A54)
#define conf_ddr_ba0			(AM335X_CTRL_BASE+0x0A58)
#define conf_ddr_ba1			(AM335X_CTRL_BASE+0x0A5C)
#define conf_ddr_ba2			(AM335X_CTRL_BASE+0x0A60)
#define conf_ddr_a0				(AM335X_CTRL_BASE+0x0A64)
#define conf_ddr_a1				(AM335X_CTRL_BASE+0x0A68)
#define conf_ddr_a2				(AM335X_CTRL_BASE+0x0A6C)
#define conf_ddr_a3				(AM335X_CTRL_BASE+0x0A70)
#define conf_ddr_a4				(AM335X_CTRL_BASE+0x0A74)
#define conf_ddr_a5				(AM335X_CTRL_BASE+0x0A78)
#define conf_ddr_a6				(AM335X_CTRL_BASE+0x0A7C)
#define conf_ddr_a7				(AM335X_CTRL_BASE+0x0A80)
#define conf_ddr_a8				(AM335X_CTRL_BASE+0x0A84)
#define conf_ddr_a9				(AM335X_CTRL_BASE+0x0A88)
#define conf_ddr_a10			(AM335X_CTRL_BASE+0x0A8C)
#define conf_ddr_a11			(AM335X_CTRL_BASE+0x0A90)
#define conf_ddr_a12			(AM335X_CTRL_BASE+0x0A94)
#define conf_ddr_a13			(AM335X_CTRL_BASE+0x0A98)
#define conf_ddr_a14			(AM335X_CTRL_BASE+0x0A9C)
#define conf_ddr_a15			(AM335X_CTRL_BASE+0x0AA0)
#define conf_ddr_odt			(AM335X_CTRL_BASE+0x0AA4)
#define conf_ddr_d0				(AM335X_CTRL_BASE+0x0AA8)
#define conf_ddr_d1				(AM335X_CTRL_BASE+0x0AAC)
#define conf_ddr_d2				(AM335X_CTRL_BASE+0x0AB0)
#define conf_ddr_d3				(AM335X_CTRL_BASE+0x0AB4)
#define conf_ddr_d4				(AM335X_CTRL_BASE+0x0AB8)
#define conf_ddr_d5				(AM335X_CTRL_BASE+0x0ABC)
#define conf_ddr_d6				(AM335X_CTRL_BASE+0x0AC0)
#define conf_ddr_d7				(AM335X_CTRL_BASE+0x0AC4)
#define conf_ddr_d8				(AM335X_CTRL_BASE+0x0AC8)
#define conf_ddr_d9				(AM335X_CTRL_BASE+0x0ACC)
#define conf_ddr_d10			(AM335X_CTRL_BASE+0x0AD0)
#define conf_ddr_d11			(AM335X_CTRL_BASE+0x0AD4)
#define conf_ddr_d12			(AM335X_CTRL_BASE+0x0AD8)
#define conf_ddr_d13			(AM335X_CTRL_BASE+0x0ADC)
#define conf_ddr_d14			(AM335X_CTRL_BASE+0x0AE0)
#define conf_ddr_d15			(AM335X_CTRL_BASE+0x0AE4)
#define conf_ddr_dqm0			(AM335X_CTRL_BASE+0x0AE8)
#define conf_ddr_dqm1			(AM335X_CTRL_BASE+0x0AEC)
#define conf_ddr_dqs0			(AM335X_CTRL_BASE+0x0AF0)
#define conf_ddr_dqsn0			(AM335X_CTRL_BASE+0x0AF4)
#define conf_ddr_dqs1			(AM335X_CTRL_BASE+0x0AF8)
#define conf_ddr_dqsn1			(AM335X_CTRL_BASE+0x0AFC)
#define conf_ddr_vref			(AM335X_CTRL_BASE+0x0B00)
#define conf_ddr_vtp			(AM335X_CTRL_BASE+0x0B04)
#define conf_ddr_strben0		(AM335X_CTRL_BASE+0x0B08)
#define conf_ddr_strben1		(AM335X_CTRL_BASE+0x0B0C)
#define conf_ain7				(AM335X_CTRL_BASE+0x0B10)
#define conf_ain6				(AM335X_CTRL_BASE+0x0B14)
#define conf_ain5				(AM335X_CTRL_BASE+0x0B18)
#define conf_ain4				(AM335X_CTRL_BASE+0x0B1C)
#define conf_ain3				(AM335X_CTRL_BASE+0x0B20)
#define conf_ain2				(AM335X_CTRL_BASE+0x0B24)
#define conf_ain1				(AM335X_CTRL_BASE+0x0B28)
#define conf_ain0				(AM335X_CTRL_BASE+0x0B2C)
#define conf_vrefp				(AM335X_CTRL_BASE+0x0B30)
#define conf_vrefn				(AM335X_CTRL_BASE+0x0B34)
#define conf_avdd				(AM335X_CTRL_BASE+0x0B3C)
#define conf_avss				(AM335X_CTRL_BASE+0x0B3C)
#define conf_iforce				(AM335X_CTRL_BASE+0x0B40)
#define conf_vsense				(AM335X_CTRL_BASE+0x0B44)
#define conf_testout			(AM335X_CTRL_BASE+0x0B48)
#define cqdetect_status			(AM335X_CTRL_BASE+0x0E00)
#define ddr_io_ctrl				(AM335X_CTRL_BASE+0x0E04)
#define vtp_ctrl				(AM335X_CTRL_BASE+0x0E0C)
#define vref_ctrl				(AM335X_CTRL_BASE+0x0E14)
#define tpcc_evt_mux_0_3		(AM335X_CTRL_BASE+0x0F90)
#define tpcc_evt_mux_4_7		(AM335X_CTRL_BASE+0x0F94)
#define tpcc_evt_mux_8_11		(AM335X_CTRL_BASE+0x0F98)
#define tpcc_evt_mux_12_15		(AM335X_CTRL_BASE+0x0F9C)
#define tpcc_evt_mux_16_19		(AM335X_CTRL_BASE+0x0FA0)
#define tpcc_evt_mux_20_23		(AM335X_CTRL_BASE+0x0FA4)
#define tpcc_evt_mux_24_27		(AM335X_CTRL_BASE+0x0FA8)
#define tpcc_evt_mux_28_31		(AM335X_CTRL_BASE+0x0FAC)
#define tpcc_evt_mux_32_35		(AM335X_CTRL_BASE+0x0FB0)
#define tpcc_evt_mux_36_39		(AM335X_CTRL_BASE+0x0FB4)
#define tpcc_evt_mux_40_43		(AM335X_CTRL_BASE+0x0FB8)
#define tpcc_evt_mux_44_47		(AM335X_CTRL_BASE+0x0FBC)
#define tpcc_evt_mux_48_51		(AM335X_CTRL_BASE+0x0FC0)
#define tpcc_evt_mux_52_55		(AM335X_CTRL_BASE+0x0FC4)
#define tpcc_evt_mux_56_59		(AM335X_CTRL_BASE+0x0FC8)
#define tpcc_evt_mux_60_63		(AM335X_CTRL_BASE+0x0FCC)
#define timer_evt_capt			(AM335X_CTRL_BASE+0x0FD0)
#define ecap_evt_capt			(AM335X_CTRL_BASE+0x0FD4)
#define adc_evt_capt			(AM335X_CTRL_BASE+0x0FD8)
#define reset_iso				(AM335X_CTRL_BASE+0x1000)
#define ddr_cke_ctrl			(AM335X_CTRL_BASE+0x131C)
#define sma2 Section			(AM335X_CTRL_BASE+0x1320)
#define m3_txev_eoi				(AM335X_CTRL_BASE+0x1324)
#define ipc_msg_reg0			(AM335X_CTRL_BASE+0x1328)
#define ipc_msg_reg1			(AM335X_CTRL_BASE+0x132C)
#define ipc_msg_reg2			(AM335X_CTRL_BASE+0x1330)
#define ipc_msg_reg3			(AM335X_CTRL_BASE+0x1334)
#define ipc_msg_reg4			(AM335X_CTRL_BASE+0x1338)
#define ipc_msg_reg5			(AM335X_CTRL_BASE+0x133C)
#define ipc_msg_reg6			(AM335X_CTRL_BASE+0x1340)
#define ipc_msg_reg7			(AM335X_CTRL_BASE+0x1344)
#define ddr_cmd0_ioctrl			(AM335X_CTRL_BASE+0x1404)
#define ddr_cmd1_ioctrl			(AM335X_CTRL_BASE+0x1408)
#define ddr_cmd2_ioctrl			(AM335X_CTRL_BASE+0x140C)
#define ddr_data0_ioctrl		(AM335X_CTRL_BASE+0x1440)
#define ddr_data1_ioctrl		(AM335X_CTRL_BASE+0x1444)

#define SLEWCTRL	(0x1 << 6)
#define	RXACTIVE	(0x1 << 5)
#define	PULLUP_EN	(0x1 << 4) /* Pull UP Selection */
#define PULLUDEN	(0x0 << 3) /* Pull up enabled */
#define PULLUDDIS	(0x1 << 3) /* Pull up disabled */
#define MODE(val)	val

// Pinmux for BeagleBoard
static void init_uart0_pin_mux(void)
{
	out32(conf_uart0_rxd        , (MODE(0) | PULLUP_EN | RXACTIVE | PULLUDEN));	/* UART0_RXD [UART0_TXD /SPI1_CS1 /DCAN0_RX/I2C2_SCL/ECAP1_IN_PWM1_OUT/PR1_PRU1_PRU_R30_15/PR1_PRU1_PRU_R31_15/GPIO1_11] */
	out32(conf_uart0_txd        , (MODE(0) | PULLUDEN ));				/* UART0_TXD [UART0_RXD /SPI1_CS0 /DCAN0_TX/I2C2_SDA/ECAP2_IN_PWM2_OUT/PR1_PRU1_PRU_R30_14/PR1_PRU1_PRU_R31_14/GPIO1_10] */
	out32(conf_uart0_ctsn        , (MODE(0) | RXACTIVE | PULLUDEN));	/* UART0_CTSN [UART0_CTSN/UART4_RXD/DCAN1_TX/I2C1_SDA/SPI1_D0/TIMER7/PR1_EDC_SYNC0_OUT/GPIO1_8] */
	out32(conf_uart0_rtsn        , (MODE(0) | PULLUP_EN | PULLUDEN));				/* UART0_RTSN [UART0_RTSN/UART4_TXD/DCAN1_RX/I2C1_SCL/SPI1_D1/SPI1_CS0/PR1_EDC_SYNC1_OUT/GPIO1_9] */
}

static void init_i2c0_pin_mux(void)
{
	out32(conf_i2c0_sda         , (MODE(0) | RXACTIVE | PULLUDEN | SLEWCTRL));	/* I2C_DATA  I2C0_SDA/TIMER4/UART2_CTSN/ECAP2_IN_PWM2_OUT////GPIO3_5 */
	out32(conf_i2c0_scl         , (MODE(0) | RXACTIVE | PULLUDEN | SLEWCTRL));	/* I2C_SCLK  I2C0_SCL/TIMER7/UART2_RTSN/ECAP1_IN_PWM1_OUT////GPIO3_6 */
}

static void init_mmc0_pin_mux(void)
{
	out32(conf_mmc0_dat3        , (MODE(0) | RXACTIVE | PULLUP_EN));	/* MMC0_DAT3  [MMC0_DAT3/GPMC_A20 /UART4_CTSN/TIMER5   /UART1_DCDN/PR1_PRU0_PRU_R30_8 /PR1_PRU0_PRU_R31_8 /GPIO2_26] */
	out32(conf_mmc0_dat2        , (MODE(0) | RXACTIVE | PULLUP_EN));	/* MMC0_DAT2  [MMC0_DAT2/GPMC_A21 /UART4_RTSN/TIMER6   /UART1_DSRN/PR1_PRU0_PRU_R30_9 /PR1_PRU0_PRU_R31_9 /GPIO2_27] */
	out32(conf_mmc0_dat1        , (MODE(0) | RXACTIVE | PULLUP_EN));	/* MMC0_DAT1  [MMC0_DAT1/GPMC_A22 /UART5_CTSN/UART3_RXD/UART1_DTRN/PR1_PRU0_PRU_R30_10/PR1_PRU0_PRU_R31_10/GPIO2_28] */
	out32(conf_mmc0_dat0        , (MODE(0) | RXACTIVE | PULLUP_EN));	/* MMC0_DAT0  [MMC0_DAT0/GPMC_A23 /UART5_RTSN/UART3_TXD/UART1_RIN /PR1_PRU0_PRU_R30_11/PR1_PRU0_PRU_R31_11/GPIO2_29] */
	out32(conf_mmc0_clk         , (MODE(0) | RXACTIVE | PULLUP_EN));	/* MMC0_CLK   [MMC0_CLK /GPMC_A24 /UART3_CTSN/UART2_RXD/DCAN1_TX  /PR1_PRU0_PRU_R30_12/PR1_PRU0_PRU_R31_12/GPIO2_30] */
	out32(conf_mmc0_cmd         , (MODE(0) | RXACTIVE | PULLUP_EN));	/* MMC0_CMD   [MMC0_CMD /GPMC_A25 /UART3_RTSN/UART2_TXD/DCAN1_RX  /PR1_PRU0_PRU_R30_13/PR1_PRU0_PRU_R31_13/GPIO2_31] */
	out32(conf_spi0_cs1         , (MODE(5) | RXACTIVE | PULLUP_EN));	/* MMC0_CD    [SPI0_CS1 /UART3_RXD/ECAP1_IN_PWM1_OUT   /MMC0_POW  /XDMA_EVENT_INTR2   /MMC0_SDCD/EMU4     /GPIO0_6]  */
}

static void init_leds_pin_mux(void)
{
	out32(conf_gpmc_a5          , MODE(7));	/* USR0 [GPMC_A5/GMII2_TXD0 /RGMII2_TD0 /RMII2_TXD0/GPMC_A21/PR1_MII1_RXD3/EQEP1B_IN  /GPIO1_21] */
	out32(conf_gpmc_a6          , MODE(7));	/* USR1 [GPMC_A6/GMII2_TXCLK/RGMII2_TCLK/MMC2_DAT4 /GPMC_A22/PR1_MII1_RXD2/EQEP1_INDEX/GPIO1_22] */
	out32(conf_gpmc_a7          , MODE(7));	/* USR2 [GPMC_A7/GMII2_RXCLK/RGMII2_RCLK/MMC2_DAT5/GPMC_A23/PR1_MII1_RXD1/EQEP1_STROBE/GPIO1_23] */
	out32(conf_gpmc_a8          , MODE(7));	/* USR3 [GPMC_A8/GMII2_RXD3 /RGMII2_RD3 /MMC2_DAT6/GPMC_A24/PR1_MII1_RXD0/MCASP0_ACLKX/GPIO1_24] */
}

static void init_rgmii1_pin_mux(void)
{
	out32(conf_mii1_txclk       , MODE(2));							/* RGMII1_TCLK [GMII1_TXCLK /UART2_RXD       /RGMII1_TCLK/MMC0_DAT7    /MMC1_DAT0    /UART1_DCDN  /MCASP0_ACLKX /GPIO3_9 ] */
	out32(conf_mii1_txd0        , MODE(2));							/* RGMII1_TD0  [GMII1_TXD0  /RMII1_TXD0      /RGMII1_TD0 /MCASP1_AXR2  /MCASP1_ACLKR /EQEP0B_IN   /MMC1_CLK     /GPIO0_28] */
	out32(conf_mii1_txd1        , MODE(2));							/* RGMII1_TD1  [GMII1_TXD1  /RMII1_TXD1      /RGMII1_TD1 /MCASP1_FSR   /MCASP1_AXR1  /EQEP0A_IN   /MMC1_CMD     /GPIO0_2 ] */
	out32(conf_mii1_txd2        , MODE(2));							/* RGMII1_TD2  [GMII1_TXD2  /DCAN0_RX        /RGMII1_TD2 /UART4_TXD    /MCASP1_AXR0  /MMC2_DAT2   /MCASP0_AHCLKX/GPIO0_17] */
	out32(conf_mii1_txd3        , MODE(2));							/* RGMII1_TD3  [GMII1_TXD3  /DCAN0_TX        /RGMII1_TD3 /UART4_RXD    /MCASP1_FSX   /MMC2_DAT1   /MCASP0_FSR   /GPIO0_16] */
	out32(conf_mii1_txen        , MODE(2));							/* RGMII1_TCTL [GMII1_TXEN  /RMII1_TXEN      /RGMII1_TCTL/TIMER4       /MCASP1_AXR0  /EQEP0_INDEX /MMC2_CMD     /GPIO3_3 ] */
																	/*             [GMII1_CRS   /RMII1_CRS_DV    /SPI1_D0    /I2C1_SDA     /MCASP1_ACLKX /UART5_CTSN  /UART2_RXD    /GPIO3_1 ] */
																	/*             [GMII1_COL   /RMII2_REFCLK    /SPI1_SCLK  /UART5_RXD    /MCASP1_AXR2  /MMC2_DAT3   /MCASP0_AXR2  /GPIO3_0 ] */
	out32(conf_mii1_rxclk       , MODE(2) | RXACTIVE);				/* RGMII1_RCLK [GMII1_RXCLK /UART2_TXD       /RGMII1_RCLK/MMC0_DAT6    /MMC1_DAT1    /UART1_DSRN  /MCASP0_FSX   /GPIO3_10] */
	out32(conf_mii1_rxd0        , MODE(2) | RXACTIVE);				/* RGMII1_RD0  [GMII1_RXD0  /RMII1_RXD0      /RGMII1_RD0 /MCASP1_AHCLKX/MCASP1_AHCLKR/MCASP1_ACLKR/MCASP0_AXR3  /GPIO2_21] */
	out32(conf_mii1_rxd1        , MODE(2) | RXACTIVE);				/* RGMII1_RD1  [GMII1_RXD1  /RMII1_RXD1      /RGMII1_RD1 /MCASP1_AXR3  /MCASP1_FSR   /EQEP0_STROBE/MMC2_CLK     /GPIO2_20] */
	out32(conf_mii1_rxd2        , MODE(2) | RXACTIVE);				/* RGMII1_RD2  [GMII1_RXD2  /UART3_TXD       /RGMII1_RD2 /MMC0_DAT4    /MMC1_DAT3    /UART1_RIN   /MCASP0_AXR1  /GPIO2_19] */
	out32(conf_mii1_rxd3        , MODE(2) | RXACTIVE);				/* RGMII1_RD3  [GMII1_RXD3  /UART3_RXD       /RGMII1_RD3 /MMC0_DAT5    /MMC1_DAT2    /UART1_DTRN  /MCASP0_AXR0  /GPIO2_18] */
																	/*             [GMII1_RXERR /RMII1_RXERR     /SPI1_D1    /I2C1_SCL     /MCASP1_FSX   /UART5_RTSN  /UART2_TXD    /GPIO3_2 ] */
	out32(conf_mii1_rxdv        , MODE(2) | RXACTIVE | PULLUP_EN);	/* RGMII1_RCTL [GMII1_RXDV  /LCD_MEMORY_CLK  /RGMII1_RCTL/UART5_TXD    /MCASP1_ACLKX /MMC2_DAT0   /MCASP0_ACLKR /GPIO3_4 ] */
																	/*             [RMII1_REFCLK/XDMA_EVENT_INTR2/SPI1_CS0   /UART5_TXD    /MCASP1_AXR3  /MMC0_POW    /MCASP1_AHCLKX/GPIO0_29] */
	out32(conf_mdio_data        , MODE(0) | RXACTIVE | PULLUP_EN);	/* MDIO_DATA   [MDIO_CLK    /TIMER5          /UART5_TXD  /UART3_RTSN   /MMC0_SDWP    /MMC1_CLK    /MMC2_CLK     /GPIO0_1 ] */
	out32(conf_mdio_clk         , MODE(0) | PULLUP_EN);				/* MDIO_CLK    [MDIO_DATA   /TIMER6          /UART5_RXD  /UART3_CTSN   /MMC0_SDCD    /MMC1_CMD    /MMC2_CMD     /GPIO0_0 ] */
}

static void init_gmii1_pin_mux(void)
{
	out32(conf_mii1_txclk       , MODE(0) | RXACTIVE);				/* GMII1_TXCLK [GMII1_TXCLK /UART2_RXD       /RGMII1_TCLK/MMC0_DAT7    /MMC1_DAT0    /UART1_DCDN  /MCASP0_ACLKX /GPIO3_9 ] */
	out32(conf_mii1_txd0        , MODE(0));							/* GMII1_TXD0  [GMII1_TXD0  /RMII1_TXD0      /RGMII1_TD0 /MCASP1_AXR2  /MCASP1_ACLKR /EQEP0B_IN   /MMC1_CLK     /GPIO0_28] */
	out32(conf_mii1_txd1        , MODE(0));							/* GMII1_TXD1  [GMII1_TXD1  /RMII1_TXD1      /RGMII1_TD1 /MCASP1_FSR   /MCASP1_AXR1  /EQEP0A_IN   /MMC1_CMD     /GPIO0_2 ] */
	out32(conf_mii1_txd2        , MODE(0));							/* GMII1_TXD2  [GMII1_TXD2  /DCAN0_RX        /RGMII1_TD2 /UART4_TXD    /MCASP1_AXR0  /MMC2_DAT2   /MCASP0_AHCLKX/GPIO0_17] */
	out32(conf_mii1_txd3        , MODE(0));							/* GMII1_TXD3  [GMII1_TXD3  /DCAN0_TX        /RGMII1_TD3 /UART4_RXD    /MCASP1_FSX   /MMC2_DAT1   /MCASP0_FSR   /GPIO0_16] */
	out32(conf_mii1_txen        , MODE(0));							/* GMII1_TXEN  [GMII1_TXEN  /RMII1_TXEN      /RGMII1_TCTL/TIMER4       /MCASP1_AXR0  /EQEP0_INDEX /MMC2_CMD     /GPIO3_3 ] */
	out32(conf_mii1_crs         , MODE(0));							/* GMII1_CRS   [GMII1_CRS   /RMII1_CRS_DV    /SPI1_D0    /I2C1_SDA     /MCASP1_ACLKX /UART5_CTSN  /UART2_RXD    /GPIO3_1 ] */
	out32(conf_mii1_col         , MODE(0));							/* GMII1_COL   [GMII1_COL   /RMII2_REFCLK    /SPI1_SCLK  /UART5_RXD    /MCASP1_AXR2  /MMC2_DAT3   /MCASP0_AXR2  /GPIO3_0 ] */
	out32(conf_mii1_rxclk       , MODE(0) | RXACTIVE);				/* GMII1_RXCLK [GMII1_RXCLK /UART2_TXD       /RGMII1_RCLK/MMC0_DAT6    /MMC1_DAT1    /UART1_DSRN  /MCASP0_FSX   /GPIO3_10] */
	out32(conf_mii1_rxd0        , MODE(0) | RXACTIVE);				/* GMII1_RXD0  [GMII1_RXD0  /RMII1_RXD0      /RGMII1_RD0 /MCASP1_AHCLKX/MCASP1_AHCLKR/MCASP1_ACLKR/MCASP0_AXR3  /GPIO2_21] */
	out32(conf_mii1_rxd1        , MODE(0) | RXACTIVE);				/* GMII1_RXD1  [GMII1_RXD1  /RMII1_RXD1      /RGMII1_RD1 /MCASP1_AXR3  /MCASP1_FSR   /EQEP0_STROBE/MMC2_CLK     /GPIO2_20] */
	out32(conf_mii1_rxd2        , MODE(0) | RXACTIVE);				/* GMII1_RXD2  [GMII1_RXD2  /UART3_TXD       /RGMII1_RD2 /MMC0_DAT4    /MMC1_DAT3    /UART1_RIN   /MCASP0_AXR1  /GPIO2_19] */
	out32(conf_mii1_rxd3        , MODE(0) | RXACTIVE);				/* GMII1_RXD3  [GMII1_RXD3  /UART3_RXD       /RGMII1_RD3 /MMC0_DAT5    /MMC1_DAT2    /UART1_DTRN  /MCASP0_AXR0  /GPIO2_18] */
	out32(conf_mii1_rxerr       , MODE(0) | RXACTIVE);				/* GMII1_RXERR [GMII1_RXERR /RMII1_RXERR     /SPI1_D1    /I2C1_SCL     /MCASP1_FSX   /UART5_RTSN  /UART2_TXD    /GPIO3_2 ] */
	out32(conf_mii1_rxdv        , MODE(0) | RXACTIVE);				/* GMII1_RXDV  [GMII1_RXDV  /LCD_MEMORY_CLK  /RGMII1_RCTL/UART5_TXD    /MCASP1_ACLKX /MMC2_DAT0   /MCASP0_ACLKR /GPIO3_4 ] */
																	/*             [RMII1_REFCLK/XDMA_EVENT_INTR2/SPI1_CS0   /UART5_TXD    /MCASP1_AXR3  /MMC0_POW    /MCASP1_AHCLKX/GPIO0_29] */
	out32(conf_mdio_data        , MODE(0) | RXACTIVE | PULLUP_EN);	/* MDIO_DATA   [MDIO_CLK    /TIMER5          /UART5_TXD  /UART3_RTSN   /MMC0_SDWP    /MMC1_CLK    /MMC2_CLK     /GPIO0_1 ] */
	out32(conf_mdio_clk         , MODE(0) | PULLUP_EN);				/* MDIO_CLK    [MDIO_DATA   /TIMER6          /UART5_RXD  /UART3_CTSN   /MMC0_SDCD    /MMC1_CMD    /MMC2_CMD     /GPIO0_0 ] */
}

static void init_rmii1_pin_mux(void)
{
																	/*             [GMII1_TXCLK /UART2_RXD       /RGMII1_TCLK/MMC0_DAT7    /MMC1_DAT0    /UART1_DCDN  /MCASP0_ACLKX /GPIO3_9 ] */
	out32(conf_mii1_txd0        , MODE(1));							/* RMII1_TXD0  [GMII1_TXD0  /RMII1_TXD0      /RGMII1_TD0 /MCASP1_AXR2  /MCASP1_ACLKR /EQEP0B_IN   /MMC1_CLK     /GPIO0_28] */
	out32(conf_mii1_txd1        , MODE(1));							/* RMII1_TXD1  [GMII1_TXD1  /RMII1_TXD1      /RGMII1_TD1 /MCASP1_FSR   /MCASP1_AXR1  /EQEP0A_IN   /MMC1_CMD     /GPIO0_2 ] */
																	/*             [GMII1_TXD2  /DCAN0_RX        /RGMII1_TD2 /UART4_TXD    /MCASP1_AXR0  /MMC2_DAT2   /MCASP0_AHCLKX/GPIO0_17] */
																	/*             [GMII1_TXD3  /DCAN0_TX        /RGMII1_TD3 /UART4_RXD    /MCASP1_FSX   /MMC2_DAT1   /MCASP0_FSR   /GPIO0_16] */
	out32(conf_mii1_txen        , MODE(1));							/* RMII1_TXEN  [GMII1_TXEN  /RMII1_TXEN      /RGMII1_TCTL/TIMER4       /MCASP1_AXR0  /EQEP0_INDEX /MMC2_CMD     /GPIO3_3 ] */
	out32(conf_mii1_crs         , MODE(1) | RXACTIVE);				/* RMII1_CRS_DV[GMII1_CRS   /RMII1_CRS_DV    /SPI1_D0    /I2C1_SDA     /MCASP1_ACLKX /UART5_CTSN  /UART2_RXD    /GPIO3_1 ] */
																	/*             [GMII1_COL   /RMII2_REFCLK    /SPI1_SCLK  /UART5_RXD    /MCASP1_AXR2  /MMC2_DAT3   /MCASP0_AXR2  /GPIO3_0 ] */
																	/*             [GMII1_RXCLK /UART2_TXD       /RGMII1_RCLK/MMC0_DAT6    /MMC1_DAT1    /UART1_DSRN  /MCASP0_FSX   /GPIO3_10] */
	out32(conf_mii1_rxd0        , MODE(1) | RXACTIVE);				/* RMII1_RXD0  [GMII1_RXD0  /RMII1_RXD0      /RGMII1_RD0 /MCASP1_AHCLKX/MCASP1_AHCLKR/MCASP1_ACLKR/MCASP0_AXR3  /GPIO2_21] */
	out32(conf_mii1_rxd1        , MODE(1) | RXACTIVE);				/* RMII1_RXD1  [GMII1_RXD1  /RMII1_RXD1      /RGMII1_RD1 /MCASP1_AXR3  /MCASP1_FSR   /EQEP0_STROBE/MMC2_CLK     /GPIO2_20] */
																	/*             [GMII1_RXD2  /UART3_TXD       /RGMII1_RD2 /MMC0_DAT4    /MMC1_DAT3    /UART1_RIN   /MCASP0_AXR1  /GPIO2_19] */
																	/*             [GMII1_RXD3  /UART3_RXD       /RGMII1_RD3 /MMC0_DAT5    /MMC1_DAT2    /UART1_DTRN  /MCASP0_AXR0  /GPIO2_18] */
	out32(conf_mii1_rxerr       , MODE(1) | RXACTIVE);				/* RMII1_RXERR [GMII1_RXERR /RMII1_RXERR     /SPI1_D1    /I2C1_SCL     /MCASP1_FSX   /UART5_RTSN  /UART2_TXD    /GPIO3_2 ] */
																	/*             [GMII1_RXDV  /LCD_MEMORY_CLK  /RGMII1_RCTL/UART5_TXD    /MCASP1_ACLKX /MMC2_DAT0   /MCASP0_ACLKR /GPIO3_4 ] */
	out32(conf_rmii1_refclk     , MODE(0) | RXACTIVE);				/* RMII1_REFCLK[RMII1_REFCLK/XDMA_EVENT_INTR2/SPI1_CS0   /UART5_TXD    /MCASP1_AXR3  /MMC0_POW    /MCASP1_AHCLKX/GPIO0_29] */
	out32(conf_mdio_data        , MODE(0) | RXACTIVE | PULLUP_EN);	/* MDIO_DATA   [MDIO_CLK    /TIMER5          /UART5_TXD  /UART3_RTSN   /MMC0_SDWP    /MMC1_CLK    /MMC2_CLK     /GPIO0_1 ] */
	out32(conf_mdio_clk         , MODE(0) | PULLUP_EN);				/* MDIO_CLK    [MDIO_DATA   /TIMER6          /UART5_RXD  /UART3_CTSN   /MMC0_SDCD    /MMC1_CMD    /MMC2_CMD     /GPIO0_0 ] */
}

void init_pinmux(int ethif)
{
	init_i2c0_pin_mux();
	init_leds_pin_mux();
	switch (ethif)
	{
	case RGMII: init_rgmii1_pin_mux(); break;
	case GMII:  init_gmii1_pin_mux();  break;
	default :   init_rmii1_pin_mux();  break;
	}
	init_mmc0_pin_mux();
	init_uart0_pin_mux();
}


// Pinmuxes for Capes ////////////////////////////////////////////////////////////////////

// Note that pinmuxes for capes depend on what cape is used. Different capes need
// different pinmuxes depending on what devices a cape implements (uart, lcd, etc).

// The functions below are provided as-is only. They demonstrate as an example how
// a pinmux for a specific device on a cape could be configured. The functions provided
// are not a complete set, and you may have to implement your own pinmux for your cape.
// When no cape is present, a default profile of 0 is assumed, and uart 1/3/4, i2c 1/2
// and spi 0/1 are configured.

// Cape ## UART ##

// UART1 - Note: this multiplex choice cannot be used in conjunction with can1 or mmc1.
// Untested - example configuration for cape uart1
static void init_uart1_pin_mux(void)
{																		/*  expansion header (P8 or P9 on schematics)    */
																		/*  |  expansion header pin (1-46 on schematics) */
																		/*  |  |              multiplexing choices       */
	out32(conf_uart1_rxd        , (MODE(0) | PULLUP_EN | RXACTIVE));	/* [P9 26] UART1_RXD [UART1_RXD/MMC1_SDWP/DCAN1_TX/I2C1_SDA//PR1_UART0_RXD/PR1_PRU1_PRU_R31_16/GPIO0_14] */
	out32(conf_uart1_txd        , (MODE(0) | PULLUDEN));				/* [P9 24] UART1_TXD [UART1_TXD/MMC2_SDWP/DCAN1_RX/I2C1_SCL//PR1_UART0_TXD/PR1_PRU0_PRU_R31_16/GPIO0_15] */
}

// UART2 - Note: this multiplex choice cannot be used in conjunction with spi1.
// Untested - example configuration for cape uart2
static void init_uart2_pin_mux(void)
{																		/*  expansion header (P8 or P9 on schematics)    */
																		/*  |  expansion header pin (1-46 on schematics) */
																		/*  |  |              multiplexing choices       */
	out32(conf_spi0_sclk        , (MODE(1) | PULLUP_EN | RXACTIVE));	/* [P9 22] UART2_RXD [SPI0_SCLK/UART2_RXD/I2C2_SDA/EHRPWM0A/PR1_UART0_CTS_N/PR1_EDIO_SOF     /EMU2/GPIO0_2] */
	out32(conf_spi0_d0          , (MODE(1) | PULLUDEN));				/* [P9 21] UART2_TXD [SPI0_D0  /UART2_TXD/I2C2_SCL/EHRPWM0B/PR1_UART0_RTS_N/PR1_EDIO_LATCH_IN/EMU3/GPIO0_3] */
}

// Untested - example configuration for cape uart3
static void init_uart3_pin_mux(void)
{																		/*  expansion header (P8 or P9 on schematics)    */
																		/*  |  expansion header pin (1-46 on schematics) */
																		/*  |  |              multiplexing choices       */
}

// Untested - example configuration for cape uart4
static void init_uart4_pin_mux(void)
{																		/*  expansion header (P8 or P9 on schematics)    */
																		/*  |  expansion header pin (1-46 on schematics) */
																		/*  |  |              multiplexing choices       */
	out32(conf_gpmc_wait0       , (MODE(6) | PULLUP_EN | RXACTIVE));	/* [P9 11] UART4_RXD  [GPMC_WAIT0/GM112_CRS  /GPMC_CSN4/RMII2_CRS_DV/MMC1_SDCD/PR1_MII1_RXDV /UART4_RXD/GPIO0_30] */
	out32(conf_gpmc_wpn         , (MODE(6) | PULLUDEN));				/* [P9 13] UART4_TXD  [GPMC_WPN  /GMII2_RXERR/GPMC_CSN5/RMII2_RXERR /MMC2_SDCD/PR1_MDIO_MDCLK/UART4_TXD/GPIO0_31] */
}

// Untested - example configuration for cape uart5
static void init_uart5_pin_mux(void)
{																		/*  expansion header (P8 or P9 on schematics)    */
																		/*  |  expansion header pin (1-46 on schematics) */
																		/*  |  |              multiplexing choices       */
	out32(conf_lcd_data9        , (MODE(4) | PULLUP_EN | RXACTIVE));	/* [P8 38] UART5_RXD  [LCD_DATA9/GPMC_A13/EHRPWM1_SYNCI_O       /MCASP0_FSX  /UART5_RXD/PR1_MII0_RXD2/UART2_RTSN/GPIO2_15] */
	out32(conf_lcd_data8        , (MODE(4) | PULLUDEN));				/* [P8 37] UART5_TXD  [LCD_DATA8/GPMC_A12/EHRPWM1_TRIPZONE_INPUT/MCASP0_ACLKX/UART5_TXD/PR1_MII0_RXD3/UART2_CTSN/GPIO2_14] */
}

// Cape ## I2C ##

// I2C1 - Note: this multiplex choice cannot be used in conjunction with spi0.
// Untested - example configuration for cape i2c1
static void init_i2c1_pin_mux(void)
{																				/*  expansion header (P8 or P9 on schematics)    */
																				/*  |  expansion header pin (1-46 on schematics) */
																				/*  |  |               multiplexing choices       */
	out32(conf_spi0_d1          , (MODE(2) | RXACTIVE | PULLUDEN | SLEWCTRL));	/* [P9 18] I2C1_DATA  [SPI0_D1 /MMC1_SDWP/I2C1_SDA/EHRPWM0_TRIPZONE_INPUT/PR1_UART0_RXD/PR1_EDIO_DATA_IN0/PR1_EDIO_DATA_OUT0/GPIO0_4] */
	out32(conf_spi0_cs0         , (MODE(2) | RXACTIVE | PULLUDEN | SLEWCTRL));	/* [P9 17] I2C1_SCLK  [SPI0_CS0/MMC2_SDWP/I2C1_SCL/EHRPWM0_SYNCI_O       /PR1_UART0_TXD/PR1_EDIO_DATA_IN1/PR1_EDIO_DATA_OUT1/GPIO0_5] */
}

// Untested - example configuration for cape i2c2
static void init_i2c2_pin_mux(void)
{																				/*  expansion header (P8 or P9 on schematics)    */
																				/*  |  expansion header pin (1-46 on schematics) */
																				/*  |  |               multiplexing choices       */
	out32(conf_uart1_ctsn       , (MODE(3) | RXACTIVE | PULLUDEN | SLEWCTRL));	/* [P9 20] I2C2_DATA  [UART1_CTSN/TIMER6/DCAN0_TX/I2C2_SDA/SPI1_CS0/PR1_UART0_CTS_N/PR1_EDC_LATCH0_IN/GPIO0_12] */
	out32(conf_uart1_rtsn       , (MODE(3) | RXACTIVE | PULLUDEN | SLEWCTRL));	/* [P9 19] I2C2_SCLK  [UART1_RTSN/TIMER5/DCAN0_RX/I2C2_SCL/SPI1_CS1/PR1_UART0_RTS_N/PR1_EDC_LATCH1_IN/GPIO0_13] */
}

// ## Cape SPI ##

// SPI0 - Note: this multiplex choice cannot be used in conjunction with i2c1 and uart2.
// Untested - example configuration for cape spi0
static void init_spi0_pin_mux(void)
{																				/*  expansion header (P8 or P9 on schematics)    */
																				/*  |  expansion header pin (1-46 on schematics) */
																				/*  |  |              multiplexing choices       */
	out32(conf_spi0_sclk        , MODE(0) | PULLUDEN | RXACTIVE);				/* [P9 22] SPI0_SCLK [SPI0_SCLK/UART2_RXD/I2C2_SDA/EHRPWM0A              /PR1_UART0_CTS_N/PR1_EDIO_SOF     /EMU2              /GPIO0_2] */
	out32(conf_spi0_d0          , MODE(0) | PULLUDEN | PULLUP_EN | RXACTIVE);	/* [P9 21] SPI0_D0   [SPI0_D0  /UART2_TXD/I2C2_SCL/EHRPWM0B              /PR1_UART0_RTS_N/PR1_EDIO_LATCH_IN/EMU3              /GPIO0_3] */
	out32(conf_spi0_d1          , MODE(0) | PULLUDEN | RXACTIVE);				/* [P9 18] SPI0_D1   [SPI0_D1  /MMC1_SDWP/I2C1_SDA/EHRPWM0_TRIPZONE_INPUT/PR1_UART0_RXD  /PR1_EDIO_DATA_IN0/PR1_EDIO_DATA_OUT0/GPIO0_4] */
	out32(conf_spi0_cs0         , MODE(0) | PULLUDEN | PULLUP_EN | RXACTIVE);	/* [P9 17] SPI0_CS0  [SPI0_CS0 /MMC2_SDWP/I2C1_SCL/EHRPWM0_SYNCI_O       /PR1_UART0_TXD  /PR1_EDIO_DATA_IN1/PR1_EDIO_DATA_OUT1/GPIO0_5] */
}

// Untested - example configuration for cape spi1
static void init_spi1_pin_mux(void)
{																				/*  expansion header (P8 or P9 on schematics)    */
																				/*  |  expansion header pin (1-46 on schematics) */
																				/*  |  |              multiplexing choices       */
	out32(conf_mcasp0_aclkx     , MODE(3) | PULLUDEN | RXACTIVE);				/* [P9 31] SPI1_SCLK  [MCASP0_ACLKX /EHRPWM0A              /           /SPI1_SCLK/MMC0_SDCD        /PR1_PRU0_PRU_R30_0/PR1_PRU0_PRU_R31_0/GPIO3_14] */
	out32(conf_mcasp0_fsx       , MODE(3) | PULLUDEN | PULLUP_EN | RXACTIVE);	/* [P9 29] SPI1_D0    [MCASP0_FSX   /EHRPWM0B              /           /SPI1_D0  /MMC1_SDCD        /PR1_PRU0_PRU_R30_1/PR1_PRU0_PRU_R31_1/GPIO3_15] */
	out32(conf_mcasp0_axr0      , MODE(3) | PULLUDEN | RXACTIVE);				/* [P9 30] SPI1_D1    [MCASP0_AXR0  /EHRPWM0_TRIPZONE_INPUT/           /SPI1_D1  /MMC2_SDCD        /PR1_PRU0_PRU_R30_2/PR1_PRU0_PRU_R31_2/GPIO3_16] */
	out32(conf_mcasp0_ahclkr    , MODE(3) | PULLUDEN | PULLUP_EN | RXACTIVE);	/* [P9 28] SPI1_CS0   [MCASP0_AHCLKR/EHRPWM0_SYNCI_O       /MCASP0_AXR2/SPI1_CS0 /ECAP2_IN_PWM2_OUT/PR1_PRU0_PRU_R30_3/PR1_PRU0_PRU_R31_3/GPIO3_17] */
}

// Cape ## CAN ##

// CAN0 - Note: this multiplex choice cannot be used in conjunction with i2c2.
// Untested - example configuration for cape can0
static void init_can0_pin_mux(void)
{																		/*  expansion header (P8 or P9 on schematics)    */
																		/*  |  expansion header pin (1-46 on schematics) */
																		/*  |  |              multiplexing choices       */
	out32(conf_uart1_rtsn       , (MODE(2) | PULLUP_EN | RXACTIVE));	/* [P9 19] DCAN0_RX  [UART1_RTSN/TIMER5/DCAN0_RX/I2C2_SCL/SPI1_CS1/PR1_UART0_RTS_N/PR1_EDC_LATCH1_IN/GPIO0_13] */
	out32(conf_uart1_ctsn       , (MODE(2) | PULLUDEN));				/* [P9 20] DCAN0_TX  [UART1_CTSN/TIMER6/DCAN0_TX/I2C2_SDA/SPI1_CS0/PR1_UART0_CTS_N/PR1_EDC_LATCH0_IN/GPIO0_12] */
}

// CAN1 - Note: this multiplex choice cannot be used in conjunction with uart1 or mmc1.
// Untested - example configuration for cape can1
static void init_can1_pin_mux(void)
{																		/*  expansion header (P8 or P9 on schematics)    */
																		/*  |  expansion header pin (1-46 on schematics) */
																		/*  |  |              multiplexing choices       */
	out32(conf_uart1_txd        , (MODE(2) | PULLUP_EN | RXACTIVE));	// [P9 24] DCAN1_RX  [UART1_TXD/MMC2_SDWP/DCAN1_RX/I2C1_SCL//PR1_UART0_TXD/PR1_PRU0_PRU_R31_16/GPIO0_15]
	out32(conf_uart1_rxd        , (MODE(2) | PULLUDEN));				// [P9 26] DCAN1_TX  [UART1_RXD/MMC1_SDWP/DCAN1_TX/I2C1_SDA//PR1_UART0_RXD/PR1_PRU1_PRU_R31_16/GPIO0_14]
}

// Cape ## MMC1 ##

// MMC1 - Note: this multiplex choice cannot be used in conjunction with uart1 or can1 or uart 4.
// Untested - example configuration for cape mmc1
static void init_mmc1_pin_mux(void)
{																		/*  expansion header (P8 or P9 on schematics)    */
																		/*  |  expansion header pin (1-46 on schematics) */
																		/*  |  |              multiplexing choices       */
	out32(conf_gpmc_ad3         , (MODE(1) | RXACTIVE));				/* [P8  6] MMC1_DAT3 [GPMC_AD3  /MMC1_DAT3//////GPIO1_3] */
	out32(conf_gpmc_ad2         , (MODE(1) | RXACTIVE));				/* [P8  5] MMC1_DAT2 [GPMC_AD2  /MMC1_DAT2//////GPIO1_2] */
	out32(conf_gpmc_ad1         , (MODE(1) | RXACTIVE));				/* [P8 24] MMC1_DAT1 [GPMC_AD1  /MMC1_DAT1//////GPIO1_1] */
	out32(conf_gpmc_ad0         , (MODE(1) | RXACTIVE));				/* [P8 25] MMC1_DAT0 [GPMC_AD0  /MMC1_DAT0//////GPIO1_0] */
	out32(conf_gpmc_csn1        , (MODE(2) | RXACTIVE | PULLUP_EN));	/* [P8 21] MMC1_CLK  [GPMC_CSN1 /GPMC_CLK /MMC1_CLK /PRT1EDIO_DATA_IN6/PRT1_EDIO_DATA_OUT6    /PR1_PRU1_PRU_R30_12/PR1_PRU1_PRU_R31_12/GPIO1_30] */
	out32(conf_gpmc_csn2        , (MODE(2) | RXACTIVE | PULLUP_EN));	/* [P8 20] MMC1_CMD  [GPMC_CSN2 /GPMC_BE1N/MMC1_CMD /PR1_EDIO_DATA_IN7/PR1_EDIO_DATA_OUT7     /PR1_PRU1_PRU_R30_13/PR1_PRU1_PRU_R31_13/GPIO1_31] */
	out32(conf_uart1_rxd        , (MODE(1) | RXACTIVE | PULLUP_EN));	/* [P9 26] MMC1_WP   [UART1_RXD /MMC1_SDWP/DCAN1_TX /I2C1_SDA         /                       /PR1_UART0_RXD      /PR1_PRU1_PRU_R31_16/GPIO0_14] */
	out32(conf_gpmc_wait0       , (MODE(4) | RXACTIVE));				/* [P9 11] MMC1_CD   [GPMC_WAIT0/GM112_CRS/GPMC_CSN4/RMII2_CRS_DV     /MMC1_SDCD              /PR1_MII1_RXDV      /UART4_RXD          /GPIO0_30] */
}

// Cape ## LCD ##

// LCD - Note: this multiplex choice cannot be used in conjunction with uart5.
static void init_lcd_pin_mux(void)
{															/*  expansion header (P8 or P9 on schematics)    */
															/*  |  expansion header pin (1-46 on schematics) */
															/*  |  |                   multiplexing choices  */
	out32(conf_lcd_data0        , MODE(0));					/* [P8 45] LCD_DATA0      [LCD_DATA0     /GPMC_A0   /                      /EHRPWM2A              /                      /PR1_PRU1_PRU_R30_0         /PR1_PRU1_PRU_R31_0 /GPIO2_6]  */
	out32(conf_lcd_data1        , MODE(0));					/* [P8 46] LCD_DATA1      [LCD_DATA1     /GPMC_A1   /                      /EHRPWM2B              /                      /PR1_PRU1_PRU_R30_1         /PR1_PRU1_PRU_R31_1 /GPIO2_7]  */
	out32(conf_lcd_data2        , MODE(0));					/* [P8 43] LCD_DATA2      [LCD_DATA2     /GPMC_A2   /                      /EHRPWM2_TRIPZONE_INPUT/                      /PR1_PRU1_PRU_R30_2         /PR1_PRU1_PRU_R31_2 /GPIO2_8]  */
	out32(conf_lcd_data3        , MODE(0));					/* [P8 44] LCD_DATA3      [LCD_DATA3     /GPMC_A3   /                      /EHRPWM2_SYNCI_O       /                      /PR1_PRU1_PRU_R30_3         /PR1_PRU1_PRU_R31_3 /GPIO2_9]  */
	out32(conf_lcd_data4        , MODE(0));					/* [P8 41] LCD_DATA4      [LCD_DATA4     /GPMC_A4   /                      /EQEP2A_IN             /                      /PR1_PRU1_PRU_R30_4         /PR1_PRU1_PRU_R31_4 /GPIO2_10] */
	out32(conf_lcd_data5        , MODE(0));					/* [P8 42] LCD_DATA5      [LCD_DATA5     /GPMC_A5   /                      /EQEP2B_IN             /                      /PR1_PRU1_PRU_R30_5         /PR1_PRU1_PRU_R31_5 /GPIO2_11] */
	out32(conf_lcd_data6        , MODE(0));					/* [P8 39] LCD_DATA6      [LCD_DATA6     /GPMC_A6   /PR1_EDIO_DATA_IN6     /EQEP2_INDEX           /PR1_EDIO_DATA_OUT6    /PR1_PRU1_PRU_R30_6         /PR1_PRU1_PRU_R31_6 /GPIO2_12] */
	out32(conf_lcd_data7        , MODE(0));					/* [P8 40] LCD_DATA7      [LCD_DATA7     /GPMC_A7   /PR1_EDIO_DATA_IN7     /EQEP2_STROBE          /PR1_EDIO_DATA_OUT7    /PR1_PRU1_PRU_R30_7         /PR1_PRU1_PRU_R31_7 /GPIO2_13] */
	out32(conf_lcd_data8        , MODE(0));					/* [P8 37] LCD_DATA8      [LCD_DATA8     /GPMC_A12  /EHRPWM1_TRIPZONE_INPUT/MCASP0_ACLKX          /UART5_TXD             /PR1_MII0_RXD3              /UART2_CTSN         /GPIO2_14] */
	out32(conf_lcd_data9        , MODE(0));					/* [P8 38] LCD_DATA9      [LCD_DATA9     /GPMC_A13  /EHRPWM1_SYNCI_O       /MCASP0_FSX            /UART5_RXD             /PR1_MII0_RXD2              /UART2_RTSN         /GPIO2_15] */
	out32(conf_lcd_data10       , MODE(0));					/* [P8 36] LCD_DATA10     [LCD_DATA10    /GPMC_A14  /EHRPWM1A              /MCASP0_AXR0           /                      /PR1_MII0_RXD1              /UART3_CTSN         /GPIO2_16] */
	out32(conf_lcd_data11       , MODE(0));					/* [P8 34] LCD_DATA11     [LCD_DATA11    /GPMC_A15  /EHRPWM1B              /MCASP0_AHCLKR         /MCASP0_AXR2           /PR1_MII0_RXD0              /UART3_RTSN         /GPIO2_17] */
	out32(conf_lcd_data12       , MODE(0));					/* [P8 35] LCD_DATA12     [LCD_DATA12    /GPMC_A16  /EQEP1A_IN             /MCASP0_ACLKR          /MCASP0_AXR2           /PR1_MII0_RXLINK            /UART4_CTSN         /GPIO0_8]  */
	out32(conf_lcd_data13       , MODE(0));					/* [P8 33] LCD_DATA13     [LCD_DATA13    /GPMC_A17  /EQEP1B_IN             /MCASP0_FSR            /MCASP0_AXR3           /PR1_MII0_RXER              /UART4_RTSN         /GPIO0_9]  */
	out32(conf_lcd_data14       , MODE(0));					/* [P8 31] LCD_DATA14     [LCD_DATA14    /GPMC_A18  /EQEP1_INDEX           /MCASP0_AXR1           /UART5_RXD             /PR1_MII_MR0_CLK            /UART5_CTSN         /GPIO0_10] */
	out32(conf_lcd_data15       , MODE(0));					/* [P8 32] LCD_DATA15     [LCD_DATA15    /GPMC_A19  /EQEP1_STROBE          /MCASP0_AHCLKX         /MCASP0_AXR3           /PR1_MII0_RXDV              /UART5_RTSN         /GPIO0_11] */
	out32(conf_gpmc_ad15        , MODE(1));					/* [P8 15] LCD_DATA16     [GPMC_AD15     /LCD_DATA16/MMC1_DAT7             /MMC2_DAT3             /EQEP2_STROBE          /PR1_ECAP0_ECAP_CAPIN_APWM_O/PR1_PRU0_PRU_R31_15/GPIO1_15] */
	out32(conf_gpmc_ad14        , MODE(1));					/* [P8 16] LCD_DATA17     [GPMC_AD14     /LCD_DATA17/MMC1_DAT6             /MMC2_DAT2             /EQEP2_INDEX           /PR1_MII0_TXD0              /PR1_PRU0_PRU_R31_14/GPIO1_14] */
	out32(conf_gpmc_ad13        , MODE(1));					/* [P8 11] LCD_DATA18     [GPMC_AD13     /LCD_DATA18/MMC1_DAT5             /MMC2_DAT1             /EQEP2B_IN             /PR1_MII0_TXD1              /PR1_PRU0_PRU_R30_15/GPIO1_13] */
	out32(conf_gpmc_ad12        , MODE(1));					/* [P8 12] LCD_DATA19     [GPMC_AD12     /LCD_DATA19/MMC1_DAT4             /MMC2_DAT0             /EQEP2A_IN             /PR1_MII0_TXD2              /PR1_PRU0_PRU_R30_14/GPIO1_12] */
	out32(conf_gpmc_ad11        , MODE(1));					/* [P8 17] LCD_DATA20     [GPMC_AD11     /LCD_DATA20/MMC1_DAT3             /MMC2_DAT7             /EHRPWM2_SYNCI_O       /PR1_MII0_TXD3              /                   /GPIO0_27] */
	out32(conf_gpmc_ad10        , MODE(1));					/* [P8 14] LCD_DATA21     [GPMC_AD10     /LCD_DATA21/MMC1_DAT2             /MMC2_DAT6             /EHRPWM2_TRIPZONE_INPUT/PR1_MII0_TXEN              /                   /GPIO0_26] */
	out32(conf_gpmc_ad9         , MODE(1));					/* [P8 13] LCD_DATA22     [GPMC_AD9      /LCD_DATA22/MMC1_DAT1             /MMC2_DAT5             /EHRPWM2B              /PR1_MII0_CRS               /                   /GPIO0_23] */
	out32(conf_gpmc_ad8         , MODE(1));					/* [P8 19] LCD_DATA23     [GPMC_AD8      /LCD_DATA23/MMC1_DAT0             /MMC2_DAT4             /EHRPWM2A              /PR1_MII_MT0_CLK            /                   /GPIO0_22] */
	out32(conf_lcd_vsync        , MODE(0) | PULLUP_EN);		/* [P8 27] LCD_VSYNC      [LCD_VSYNC     /GPMC_A8   /                      /PR1_EDIO_DATA_IN2     /PR1_EDIO_DATA_OUT2    /PR1_PRU1_PRU_R30_8         /PR1_PRU1_PRU_R31_8 /GPIO2_22] */
	out32(conf_lcd_hsync        , MODE(0) | PULLUP_EN);		/* [P8 29] LCD_HSYNC      [LCD_HSYNC     /GPMC_A9   /                      /PR1_EDIO_DATA_IN3     /PR1_EDIO_DATA_OUT3    /PR1_PRU1_PRU_R30_9         /PR1_PRU1_PRU_R31_9 /GPIO2_23] */
	out32(conf_lcd_pclk         , MODE(0) | PULLUP_EN);		/* [P8 28] LCD_PCLK       [LCD_PCLK      /GPMC_A10  /                      /PR1_EDIO_DATA_IN4     /PR1_EDIO_DATA_OUT4    /PR1_PRU1_PRU_R30_10        /PR1_PRU1_PRU_R31_10/GPIO2_24] */
	out32(conf_lcd_ac_bias_en   , MODE(0) | PULLUDDIS);		/* [P8 30] LCD_AC_BIAS_EN [LCD_AC_BIAS_EN/GPMC_A11  /                      /PR1_EDIO_DATA_IN5     /PR1_EDIO_DATA_OUT5    /PR1_PRU1_PRU_R30_11        /PR1_PRU1_PRU_R31_11/GPIO2_25] */
	// backlight and/or power control
	out32(conf_ecap0_in_pwm0_out, MODE(7) | PULLUDDIS);		/* [P9 42] LCD_AC_BIAS_EN [ECAP0_IN_PWM0_OUT/UART3_TXD/SPI1_CS1/PR1_ECAP0_ECAP_CAPIN_APWM_O/SPI1_SCLK/MMC0_SDWP/XDMA_EVENT_INTR2/GPIO0_7] */
}

typedef struct cape_profile
{
	int uart1;
	int uart2;
	int uart3;
	int uart4;
	int uart5;
	int i2c1;
	int i2c2;
	int spi0;
	int spi1;
	int mmc1;
	int can0;
	int can1;
	int lcd;
} CAPE_PROFILE;

CAPE_PROFILE cape_profiles[] = {
//        uart1
//        | uart2
//        | | uart3
//        | | | uart4
//        | | | | uart5
//        | | | | | i2c1
//        | | | | | | i2c2
//        | | | | | | | spi0
//        | | | | | | | | spi1
//        | | | | | | | | | mmc1
//        | | | | | | | | | | can0
//        | | | | | | | | | | | can1
//        | | | | | | | | | | | | lcd
//        | | | | | | | | | | | | | gpmc
		{ 1,0,0,1,1,1,1,0,1,0,0,0,1 },	/* 00 - this is the default profile where no cape is present */
		{ 0,0,0,0,0,0,0,0,0,0,0,0,0 },	/* 01 - add your own profile here */
};

void init_pinmux_capes(int profile, int showchoices)
{
	// Any UARTS?
	if (cape_profiles[profile].uart1)
	{
		if (showchoices) kprintf("uart1\n");
		init_uart1_pin_mux();
	}
	if (cape_profiles[profile].uart2)
	{
		if (showchoices) kprintf("uart2\n");
		init_uart2_pin_mux();
	}
	if (cape_profiles[profile].uart3)
	{
		if (showchoices) kprintf("uart3\n");
		init_uart3_pin_mux();
	}
	if (cape_profiles[profile].uart4)
	{
		if (showchoices) kprintf("uart4\n");
		init_uart4_pin_mux();
	}
	if (cape_profiles[profile].uart5)
	{
		if (showchoices) kprintf("uart5\n");
		init_uart5_pin_mux();
	}

	// Any i2c?
	if (cape_profiles[profile].i2c1)
	{
		if (showchoices) kprintf("i2c1\n");
		init_i2c1_pin_mux();
	}
	if (cape_profiles[profile].i2c2)
	{
		if (showchoices) kprintf("i2c2\n");
		init_i2c2_pin_mux();
	}

	// Any SPI?
	if (cape_profiles[profile].spi0)
	{
		if (showchoices) kprintf("spi0\n");
		init_spi0_pin_mux();
	}
	if (cape_profiles[profile].spi1)
	{
		if (showchoices) kprintf("spi1\n");
		init_spi1_pin_mux();
	}

	// MMC1?
	if (cape_profiles[profile].mmc1)
	{
		if (showchoices) kprintf("mmc1\n");
		init_mmc1_pin_mux();
	}

	// Any CAN?
	if (cape_profiles[profile].can0)
	{
		if (showchoices) kprintf("can0\n");
		init_can0_pin_mux();
	}
	if (cape_profiles[profile].can1)
	{
		if (showchoices) kprintf("can1\n");
		init_can1_pin_mux();
	}

	// LCD?
	if (cape_profiles[profile].lcd)
	{
		if (showchoices) kprintf("lcd\n");
		init_lcd_pin_mux();
	}
}
