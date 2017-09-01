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


/*
 * init_raminfo.c
 * Tell syspage about our RAM configuration
 */
#include "startup.h"

void init_raminfo()
{
	// EMIF0 SDRAM 0x8000_0000 0xBFFF_FFFF 1GB DDR (1 GB Space)
	// BeagleBone AM335X 15mmx15mm: DDR2  SDRAM 256MB

	add_ram(0x80000000, MEG(256));
}

__SRCVERSION( "$URL$ $Rev$" );
