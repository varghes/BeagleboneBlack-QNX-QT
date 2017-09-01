/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
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



 

#include "externs.h"


#if 0
static void seromap_setsleep(uintptr_t *port, uint8_t mode, uint8_t wakeup)
{
	set_port(port[OMAP_UART_EFR], OMAP_EFR_ENHANCED, OMAP_EFR_ENHANCED);	// Set enhanced bit
	set_port(port[OMAP_UART_IER], OMAP_IER_SLEEP, mode);					// Set sleep mode
	set_port(port[OMAP_UART_EFR], OMAP_EFR_ENHANCED, 0);					// clear enhanced bit
	set_port(port[OMAP_UART_SCR], OMAP_SCR_WAKEUPEN, wakeup);				// Set wakeup interrupt
}

static void seromap_enable(DEV_OMAP *dev, int enable)
{
	uintptr_t		*port = dev->port;

	write_omap(port[OMAP_UART_LCR], 0x80);

	if (!enable) {
		atomic_set(&dev->pwm_flag, SEROMAP_PWM_PAGED);

		// If HW flow control is ON, assert the RTS line
		if (dev->tty.c_cflag & IHFLOW)
			set_port(port[OMAP_UART_MCR], OMAP_MCR_DTR|OMAP_MCR_RTS, 0);

		while (!(read_omap(port[OMAP_UART_LSR]) & OMAP_LSR_TSRE))
			;
		nanospin_ns(1000000);			// pause for 1ms
		write_omap(port[OMAP_UART_MDR1], 0x07);
		write_omap(port[OMAP_UART_DLL], 0xFF);
		write_omap(port[OMAP_UART_DLH], 0xFF);
	}
	else {
		write_omap(port[OMAP_UART_DLL], dev->brd);
		write_omap(port[OMAP_UART_DLH], (dev->brd >> 8) & 0xff);
		write_omap(port[OMAP_UART_MDR1], 0x00);

		// If HW flow control is ON, de-assert the RTS line
		if(dev->tty.c_cflag & IHFLOW)
			set_port(port[OMAP_UART_MCR], OMAP_MCR_DTR|OMAP_MCR_RTS, OMAP_MCR_DTR|OMAP_MCR_RTS);

		// Allow data transmission to resume
		atomic_clr(&dev->pwm_flag, SEROMAP_PWM_PAGED);
	}

	write_omap(port[OMAP_UART_LCR], dev->lcr);
}

/*
 * Active	: Normal operation
 * Idle		: Normal operation but 1) enable sleep, 2) enable wakeup interrupt
 * Standby	: UART disabled, wakeup interrupt enabled
 * Off		: Standby but wakeup interrupt disabled
 */

int seromap_setpower(pmd_attr_t *pmd)
{
	int				ret = EOK;
	pm_power_mode_t	mode = PM_POWER_MODE(pmd->new_mode);
	DEV_OMAP		*dev = (DEV_OMAP *)pmd->data;
	uintptr_t		*port = dev->port;

	if (pmd->cur_mode == pmd->new_mode && pmd->cur_flags == pmd->new_flags)
		return (EOK);

	InterruptMask(dev->intr, dev->iid);
	switch (mode) {
		case PM_MODE_ACTIVE:	// Power UP 
			seromap_setsleep(port, 0, 0);
			if (pmd->cur_mode != PM_MODE_IDLE)
				seromap_enable(dev, 1);
			break;

		case PM_MODE_IDLE:		// Idle mode : active + enable the sleep mode
			if (pmd->cur_mode != PM_MODE_ACTIVE) {
				seromap_setsleep(port, 0, 0);		// Disable sleep mode first
				seromap_enable(dev, 1);				// Re-enable UART
			}
			seromap_setsleep(port, OMAP_IER_SLEEP, OMAP_SCR_WAKEUPEN);
			break;

		case PM_MODE_OFF:		// Off
			if (pmd->cur_mode != PM_MODE_STANDBY) {
				if (pmd->cur_mode == PM_MODE_IDLE)
					seromap_setsleep(port, 0, 0);	// Disable sleep mode first
				seromap_enable(dev, 0);				// Disable UART
			}
			seromap_setsleep(port, OMAP_IER_SLEEP, 0);	// Enable sleep mode, disable wakeup
			break;

		case PM_MODE_STANDBY:	
			if (pmd->cur_mode != PM_MODE_OFF) {
				if (pmd->cur_mode == PM_MODE_IDLE)
					seromap_setsleep(port, 0, 0);	// Disable sleep mode first
				seromap_enable(dev, 0);				// Disable UART
			}
			seromap_setsleep(port, OMAP_IER_SLEEP, OMAP_SCR_WAKEUPEN);
			break;

		default:
			// Unsupported Power Mode
   			ret = EINVAL;
			break;
	}
	InterruptUnmask(dev->intr, dev->iid);

	// Kick the driver to restart data transmission 
	if (mode == PM_MODE_IDLE || mode == PM_MODE_ACTIVE)
		kick((TTYDEV *)dev);

//	pmd_confirm(pmd, EOK);

    return ret;
}


// Set power modes supported by driver
pmd_mode_attr_t	drv_modes[SEROMAP_NUM_POWER_MODES] = {
	{ PM_MODE_OFF, 0, },
	{ PM_MODE_STANDBY, 0, },
	{ PM_MODE_IDLE, 0, },
	{ PM_MODE_ACTIVE, 0, }
};
#endif

void seromap_power_init(DEV_OMAP *dev, TTYINIT_OMAP *dip)
{
#if 0
	// Initialize power management structures
	pmd_attr_init(&dev->driver_pmd);
	pmd_attr_setmodes(&dev->driver_pmd, PM_MODE_ACTIVE, drv_modes, SEROMAP_NUM_POWER_MODES);
	pmd_attr_setpower(&dev->driver_pmd, seromap_setpower, dev);

	// Register Power Management and provide necessary data to IO-Char
   	dev->tty.power.pwm_attr = &dev->driver_pmd;
   	dev->tty.power.pwm_event = &dev->pwm_event;
	ttc(TTC_INIT_POWER, &dev->tty, dip->pwm_init);
#endif
}

__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-j5-evm/1.0.0/latest/hardware/devc/seromap/power.c $ $Rev: 217582 $" );
