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

// Module Description:  board specific interface

#include <sim_mmc.h>
#include <sim_omap3.h>
#include <arm/omap3530.h>


static int netra_detect(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	omap3_ext_t		*omap;
	uintptr_t		base;
	ext    = (SIM_MMC_EXT *)hba->ext;
	omap = (omap3_ext_t *)ext->handle;
	base = omap->mmc_base;

	if (in32(base + OMAP3_MMCHS_PSTATE) & PSTATE_CINS) {
		//FIXME write protection not working yet
#if 0
		in32(_base + OMAP3_MMCHS_PSTATE) & PSTATE_WP)		// Write protected
			ext->eflags |= MMC_EFLAG_WP;
		else
			ext->eflags &= ~MMC_EFLAG_WP;
#endif
//		printf("Card detected %x pstate %x, test %x, status %x\n", in32(omap->mmc_base + OMAP3_MMCHS_HCTL), in32(omap->mmc_base + OMAP3_MMCHS_PSTATE), in32(base + 0x128), in32(base + OMAP3_MMCHS_SYSSTATUS));
//		printf("%s(%d): SYSCONFIG %x CON %x, HCTL %x\n", __func__, __LINE__, in32(base + OMAP3_MMCHS_SYSCONFIG), in32(base + OMAP3_MMCHS_CON), in32(base + OMAP3_MMCHS_HCTL));
		return MMC_SUCCESS;
	} 
//		printf("%s(%d): SYSCONFIG %x CON %x, HCTL %x\n", __func__, __LINE__, in32(base + OMAP3_MMCHS_SYSCONFIG), in32(base + OMAP3_MMCHS_CON), in32(base + OMAP3_MMCHS_HCTL));
//		printf("Card detection loop %x pstate %x, test %x, status %x\n", in32(omap->mmc_base + OMAP3_MMCHS_HCTL), in32(omap->mmc_base + OMAP3_MMCHS_PSTATE), in32(base + 0x128), in32(base + OMAP3_MMCHS_SYSSTATUS));
	return MMC_FAILURE;
}

int bs_init(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	omap3_ext_t		*omap;
	CONFIG_INFO		*cfg;

	cfg = (CONFIG_INFO *)&hba->cfg;

	if (!cfg->NumIOPorts) {
		cfg->IOPort_Base[0]   = 0x48060100;
		cfg->IOPort_Length[0] = OMAP3_MMCHS_SIZE;
		cfg->IOPort_Base[1]   = DRA446_EDMA_BASE;
		cfg->IOPort_Length[1] = DRA446_EDMA_SIZE;
		cfg->NumIOPorts = 2;
	} else if (cfg->NumIOPorts < 2) {
		slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "OMAP3 MMCSD: DMA4 base address must be specified");
		return MMC_FAILURE;
	}
	
	if (!cfg->NumIRQs) {
		cfg->IRQRegisters[0] = 64;
		cfg->NumIRQs = 1;
	}

	if (!cfg->NumDMAs) {
		cfg->DMALst[0] = 24;	// DMA request line for MMC1 TX
		cfg->DMALst[1] = 25;	// DMA request line for MMC1 RX
		cfg->NumDMAs = 2;
	} else if (cfg->NumDMAs == 1) {
		cfg->DMALst[1] = cfg->DMALst[1]+1;	// DMA request line for MMC1 RX
		cfg->NumDMAs = 2;
	} else if (cfg->NumDMAs < 2) {
		slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "OMAP3 MMCSD: DMA channel and Tx/Rx request event must be specified");
		return MMC_FAILURE;
	}

	if (omap3_attach(hba) != MMC_SUCCESS)
		return MMC_FAILURE;


	ext  = (SIM_MMC_EXT *)hba->ext;
	omap = (omap3_ext_t *)ext->handle;
	ext->detect = netra_detect;
	ext->hccap &= ~MMC_HCCAP_BW8; //the hardware does not support 8data pin
	omap->mmc_clock = 192000000;
	ext->clock     = omap->mmc_clock;

	return MMC_SUCCESS;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

int bs_dinit(SIM_HBA *hba)
{
	return (CAM_SUCCESS);
}

__SRCVERSION( "$URL$ $Rev$" );

