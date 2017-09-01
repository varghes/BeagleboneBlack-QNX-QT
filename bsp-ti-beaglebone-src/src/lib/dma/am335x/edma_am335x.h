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

#ifndef __EDMA_AM335X_H__
#define __EDMA_AM335X_H__

#define AM335X_MAX_CC_NO			1

#define AM335X_EDMA_QCHMAP_TRWORD_CCNT	(0X7 << 2)
#define AM335X_CC_NOS				1
#define AM335X_EDMA0_TCNOS		3
#define AM335X_MAX_PARAM_NO		127

	/* for channel controller 0 */
#define AM335X_EDMA0_IRQ_NO		0x200

#define AM335X_EDMA0_NUM_CHAN		64
#define AM335X_QDMA0_NUM_CHAN		8

#define AM335X_EDMA0_FIRST_CHAN	0
#define AM335X_EDMA0_LAST_CHAN	63
#define AM335X_QDMA0_FIRST_CHAN	0
#define AM335X_QDMA0_LAST_CHAN	7

#define AM335X_EDMA0_MAX_CHAN		(AM335X_EDMA0_NUM_CHAN + AM335X_QDMA0_NUM_CHAN)

	/* for total edma */
#define AM335X_EDMA_NUM_CHAN		AM335X_EDMA0_NUM_CHAN
#define AM335X_QDMA_NUM_CHAN		AM335X_QDMA0_NUM_CHAN

#define AM335X_MAX_NUM_CHANNELS	AM335X_EDMA_NUM_CHAN


#define AM335X_DMA_NAME		"DMA Engine For AM335x"
#define AM335X_LIB_REVISION	1
#define AM335X_DMA_NUM_CHAN	AM335X_MAX_NUM_CHANNELS
#define AM335X_MAXPRIO		0

#define AM335X_MEM_NOCROSS		0
#define AM335X_MEM_LOWER_LIM	0x00700000	/* actually this can be 0, but l137 doesnot have any addressable memory lower than this */
#define AM335X_MEM_UPPER_LIM	0xffffffff
#define AM335X_MAX_ACNT		((1 << 16) - 1UL)
#define AM335X_MAX_BCNT		((1 << 16) - 1UL)
#define AM335X_MAX_XFER_SIZ	(AM335X_MAX_BCNT * AM335X_MAX_ACNT)
#define AM335X_MAX_SRC_SEG		AM335X_MAX_BCNT
#define AM335X_MAX_DST_SEG		AM335X_MAX_BCNT
#define AM335X_MAX_SRC_FRAG	5
#define AM335X_MAX_DST_FRAG	5
#define AM335X_XFER_UNIT_SIZ	(16 | 8 | 4 | 2 | 1)		/* DBS set in sysconfig */

/****************************************************************************/

#endif
