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

/*
 The driver can support 7-bit and 10-bit slave addressing. 
 However the driver is not tested for 10-bit slave addressing 
 due to unavailabilty 10-bit slave. 
 */
int
omap_set_slave_addr(void *hdl, unsigned int addr, i2c_addrfmt_t fmt)
{
    omap_dev_t      *dev = hdl;

	if(fmt != I2C_ADDRFMT_7BIT && fmt != I2C_ADDRFMT_10BIT)
		return -1;

	dev->slave_addr = addr;
    
	return 0;
}

__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-j5-evm/1.0.0/latest/hardware/i2c/omap35xx/slave_addr.c $ $Rev: 222214 $" );
