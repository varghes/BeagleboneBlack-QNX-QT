/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include "proto.h"

int
omap_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed)
{
    omap_dev_t      *dev = hdl;
    unsigned long   iclk;
    unsigned        scll;

  /* This driver support bus speed range from 8KHz to 400KHz
     * limit the low bus speed to 8KHz to protect SCLL/SCLH from overflow(large than 0xff)
     * if speed=8KHz, iclk=4MHz, then SCLL=0xf3, SCLH=0xf5
     */
    if (speed > 400000 || speed < 8000) {
        fprintf(stderr, "i2c-omap35xx:  Invalid bus speed(%d)\n", speed);
        errno = EINVAL;
        return -1;
    }
    
    /* Set the I2C prescaler register to obtain the maximum I2C bit rates
     * and the maximum period of the filtered spikes in F/S mode:
     * Stander Mode: I2Ci_INTERNAL_CLK = 4 MHz
     * Fast Mode:    I2Ci_INTERNAL_CLK = 9.6 MHz
     */
	
#if defined(VARIANT_j5)
    if (speed <= 100000) {
		out16(dev->regbase + OMAP_I2C_PSC, 11);     // I2Ci_INTERNAL_CLK = 4 MHz
		iclk = OMAP_I2C_ICLK;
	} else {
		out16(dev->regbase + OMAP_I2C_PSC, 4);      // I2Ci_INTERNAL_CLK = 9.6 MHz
		iclk = OMAP_I2C_ICLK_9600K;
	}
#else
    if (speed <= 100000) {
        out16(dev->regbase + OMAP_I2C_PSC, 23);     // I2Ci_INTERNAL_CLK = 4 MHz
        iclk = OMAP_I2C_ICLK;
    } else {
        out16(dev->regbase + OMAP_I2C_PSC, 9);      // I2Ci_INTERNAL_CLK = 9.6 MHz
        iclk = OMAP_I2C_ICLK_9600K;
    }
#endif

	/* OMAP_I2C_PSC is already set */
    /* Set clock for "speed" bps */
	scll = iclk/(speed << 1) - 7;

    out16(dev->regbase + OMAP_I2C_SCLL, scll);
    out16(dev->regbase + OMAP_I2C_SCLH, scll + 2);

	 dev->speed = iclk / ((scll + 7) << 1);
	 if (ospeed)
        *ospeed = dev->speed;

    return 0;
}

__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-j5-evm/1.0.0/latest/hardware/i2c/omap35xx/bus_speed.c $ $Rev: 514806 $" );
