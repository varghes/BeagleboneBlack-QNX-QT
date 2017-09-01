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
#include <arm/dm6446.h>
#include <arm/am335x.h>

void
init_edma(void)
{
	unsigned	chnl;
	uintptr_t	base = 0x49000000;	// EDMA base

	out32(AM335X_CM_PER_TPCC_CLKCTRL , 2);	// Enable TPCC clock
	out32(AM335X_CM_PER_TPTC0_CLKCTRL, 2);	// Enable TPTC0 clock
	out32(AM335X_CM_PER_TPTC1_CLKCTRL, 2);	// Enable TPTC1 clock
	out32(AM335X_CM_PER_TPTC2_CLKCTRL, 2);	// Enable TPTC2 clock
	in32(AM335X_CM_PER_TPCC_CLKCTRL);

	// DMA Channel mapping
	for (chnl = 0; chnl < 64; chnl++)
		out32(base + 0x100 + chnl * 4, chnl << 5);

	// Enable region 0 access for all channels, except the McASP1 channel, which is used by DSP to receive radio data from the tuner. 
	out32(base + DM6446_EDMA_DRAE(0), 0xFFFFFFF);
	out32(base + DM6446_EDMA_DRAEH(0), 0xFFFFFFFF);

	// Disable all events
	base += DM6446_EDMA_GLOBAL;
	out32(base + DM6446_EDMA_EECR, 0xFFFFFFFF);
	out32(base + DM6446_EDMA_EECRH, 0xFFFFFFFF);
	out32(base + DM6446_EDMA_SECR, 0xFFFFFFFF);
	out32(base + DM6446_EDMA_SECRH, 0xFFFFFFFF);

	// Clear events just in case
	out32(base + DM6446_EDMA_ECR, 0xFFFFFFFF);
	out32(base + DM6446_EDMA_ECRH, 0xFFFFFFFF);
	// Clear missed events just in case
	//out32(base + DM6446_EDMA_EMCR, 0xFFFFFFFF);
	//out32(base + DM6446_EDMA_EMCRH, 0xFFFFFFFF);

	// Disable all interrupts
	out32(base + DM6446_EDMA_IECR, 0xFFFFFFFF);
	out32(base + DM6446_EDMA_IECRH, 0xFFFFFFFF);
	// Clear all interrupts just in case
	out32(base + DM6446_EDMA_ICR, 0xFFFFFFFF);
	out32(base + DM6446_EDMA_ICRH, 0xFFFFFFFF);
}

__SRCVERSION( "$URL$ $Rev$" );
