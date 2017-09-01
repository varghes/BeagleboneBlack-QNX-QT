/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems. 
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


#include	<sim_mmc.h>

#ifdef MMCSD_VENDOR_STA2X11

#include	<sim_sta2x11.h>

static void _sta2x11_dumpregs(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	sta2x11_ext_t	*sta2x11;
	uintptr_t		base;
	
	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;
	
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": ========== REGISTER DUMP ========");
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2,": power: 0x%08x",SDI_PWR(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2,": clock: 0x%08x",SDI_CLKCR(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2,": Argument: 0x%08x", SDI_ARG(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": Command: 0x%08x", SDI_CMD(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": respondcmd: 0x%08x", SDI_RESPCMD(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": response0: 0x%08x", SDI_RESP0(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": response1: 0x%08x", SDI_RESP1(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": response2: 0x%08x",SDI_RESP2(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": response3: 0x%08x",SDI_RESP3(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": data timer: 0x%08x",SDI_DTIMER(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": data length: 0x%08x",SDI_DLEN(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": data control: 0x%08x",SDI_DCTRL(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": data count: 0x%08x",SDI_DCOUNT(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2,": status: 0x%08x",SDI_STA(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": clear : 0x%08x",SDI_ICR(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": mask0: 0x%08x",SDI_MASK0(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": mask1: 0x%08x",SDI_MASK1(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2,": cesl: 0x%08x",SDI_CSEL(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2,": fifo count: 0x%08x",SDI_FIFOCNT(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": fifo: 0x%08x",SDI_FIFO(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": db timer: 0x%08x",SDI_DBTIMER(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": trsca: 0x%08x",SDI_TRSCA(base));
	slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, ": ============================\n");
}

#ifdef POLL_MODE
#define TIMER_START sta2x11->itime.it_value.tv_nsec = 10000000 * 1; \
	timer_settime(sta2x11->timer_id, 0, &sta2x11->itime, NULL);
#define TIMER_STOP timer_settime(sta2x11->timer_id, 0, &sta2x11->zerotime, NULL);
#else
#define TIMER_START
#define TIMER_STOP
#endif
static int _sta2x11_detect(SIM_HBA *hba)
{
	// The controller has no ability to detect the card
	return (MMC_SUCCESS);
}
static void _sta2x11_finish_io(sta2x11_ext_t *sta2x11)
{
	uintptr_t	base = sta2x11->base;

	if(sta2x11->state & (SDI_STATE_PIO|SDI_STATE_DMA))
		SDI_DCTRL(base)=0; 
	if(sta2x11->state & SDI_STATE_DMA_STARTED){
		sta2x11->dmafuncs.xfer_abort (sta2x11->dma_chn);
	}
	SDI_DLEN(base) = 0;
	SDI_CMD(base) = 0;
	SDI_MASK0(base)=0;
	sta2x11->state =0;
	sta2x11->translen= 0;
	sta2x11->currlen = 0;
	nanospin_ns(1000);
}

static int _sta2x11_interrupt(SIM_HBA *hba, int irq, int resp_type, uint32_t *resp)
{
	SIM_MMC_EXT		*ext;
	sta2x11_ext_t	*sta2x11;
	uintptr_t		base;
	uint32_t		sts, mask, imask=0;
	int			intr = 0;
	int 			dcount;
	static int 		pollcounter=0;
	
	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;
	
	sts   = SDI_STA(base);
	mask  = SDI_MASK0(base);
	dcount =SDI_DCOUNT(base);
	
	TIMER_STOP
	pollcounter++;
	
	if(!sts && !mask){ //not our interrupt
		TIMER_START;
		if(hba->verbosity)
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): polling no success status 0x%x, mask %x, state %x", __func__, __LINE__, sts, mask, sta2x11->state);
		return 0;
	}
	
	if(irq==-1 && hba->verbosity){
		slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d):poll timer: command %d, sts 0x%x, msk %x, dcount %x, dctrl %x tlen %d, crrlen %d, state %x, fcnt %x", __func__, __LINE__, sta2x11->command, sts, mask, dcount, SDI_DCTRL(base), sta2x11->translen, sta2x11->currlen, sta2x11->state, SDI_FIFOCNT(base));
	}
	
	if(hba->verbosity>3 || (sts & ERR_MASK) 
		||((sts & STA_CCRCFAIL) && (resp_type  & MMC_RSP_CRC))){
		slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d):command %d, sts 0x%x, msk %x, dcount %x, dctrl %x tlen %d, crrlen %d, state %x, fcnt %x resp %x", __func__, __LINE__, sta2x11->command, sts, mask, dcount, SDI_DCTRL(base), sta2x11->translen, sta2x11->currlen, sta2x11->state, SDI_FIFOCNT(base), SDI_RESP0(base));
	}
	
	if (mask & (STA_CMDREND|STA_CMDSENT)) {		// Command complete interrupt enabled ?
		//controller will emit CCRCFAIL if MMC_RSP_CRC not set
		if ((sts & (STA_CTIMEOUT|ICR_TXUNDERRC | ICR_RXOVERRC | STA_STBITERR))
			||((sts & STA_CCRCFAIL) && (resp_type  & MMC_RSP_CRC))){
			if (sts & STA_CTIMEOUT){	// Command Timeout ?
				intr |= MMC_ERR_CMD_TO | MMC_INTR_ERROR | MMC_INTR_COMMAND; 
			}else  if(sts &  STA_CCRCFAIL){
				intr |= MMC_ERR_CMD_CRC | MMC_INTR_ERROR | MMC_INTR_COMMAND;
			}else{ //we shouldn't get here
				intr |= MMC_INTR_ERROR | MMC_INTR_COMMAND; 
				slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): command intr unknown error 0x%x, mask %x", __func__, __LINE__, sts, mask);
			}
			_sta2x11_finish_io(sta2x11);
			if(hba->verbosity>3)
				slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "%s(%d): command intr error 0x%x, mask %x cmd %dm resp %x", __func__, __LINE__, sts, mask, sta2x11->command, SDI_RESP0(base));
			pollcounter =0;
			SDI_ICR(base) = ICR_CLR_ALL;
			goto finish;
		}else if ((sts & (STA_CMDREND|STA_CMDSENT)) 
			||((sts & STA_CCRCFAIL) && !(resp_type  & MMC_RSP_CRC))) {
			/*controller fire an spurious CRC Fail interrupt even no CRC check required in some case*/

			/* Are we getting any data errors at this stage??? */
			if(sts & (STA_DTIMEOUT | STA_DCRCFAIL )){
				intr |= MMC_INTR_COMMAND |MMC_INTR_ERROR;
				_sta2x11_finish_io(sta2x11);
				slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): DATA ERROR at command phase: status 0x%x, mask %x, dcount %x, dctrl %x tlen %d, crrlen %d, state %x, fcnt %x, resp %x", __func__, __LINE__, sts, mask, dcount, SDI_DCTRL(base), sta2x11->translen, sta2x11->currlen, sta2x11->state, SDI_FIFOCNT(base), SDI_RESP0(base));
				SDI_ICR(base) = ICR_CLR_ALL;
				goto finish;
			}

			if(resp_type & MMC_RSP_136) { 
				resp[0] = SDI_RESP3(base);
				resp[1] = SDI_RESP2(base);
				resp[2] = SDI_RESP1(base);
				resp[3] = SDI_RESP0(base);
			}else if (resp_type & MMC_RSP_PRESENT) {
				resp[0] = SDI_RESP0(base);
			}

			if(sta2x11->state & (SDI_STATE_PIO|SDI_STATE_DMA)){ 
				uint32_t dctrl = SDI_DCTRL(base) & (DCTRL_DBLOCKSIZE_MSK<<DCTRL_DBLOCKSIZE_SHIFT);
				// all successful, if we are doing read/write, 
				// enable appropriate interrupt and start DMA  or start pio read/write
				SDI_ICR(base) = sts & ICR_CLR_ALL;
				if(sta2x11->state & SDI_STATE_PIO_IN){
					intr |= MMC_INTR_DATA;
					SDI_MASK0(base)=0;
					if(hba->verbosity>3)
						slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "%s(%d): SDI_STATE_PIO_IN sts 0x%x, mask %x, dcount %x, fcnt %x", __func__, __LINE__, SDI_STA(base), mask, SDI_DCOUNT(base), SDI_FIFOCNT(base));
				}else if(sta2x11->state & SDI_STATE_PIO_OUT){
					intr |= MMC_INTR_DATA;
					dctrl |= DCTRL_RWMOD | DCTRL_DTEN ;
					SDI_MASK0(base)=0;
					SDI_DCTRL(base) = dctrl;
				}else if(sta2x11->state & SDI_STATE_DMA_OUT){
					imask = STA_DATAEND;
					intr |= MMC_INTR_COMMAND;
					dctrl |= DCTRL_DMAEN|DCTRL_DMAREQCTL|DCTRL_DTEN;
					sta2x11->dmafuncs.xfer_start (sta2x11->dma_chn);
					sta2x11->state |= SDI_STATE_DMA_STARTED;
					SDI_MASK0(base) = imask | STA_DTIMEOUT | STA_RXOVERR | STA_TXUNDERR | STA_STBITERR | STA_DCRCFAIL ;
					SDI_DCTRL(base) = dctrl;
					TIMER_START
					if(hba->verbosity>3)
						slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "%s(%d): SDI_STATE_DMA_OUT sts 0x%x, mask %x, dcount %x, fcnt %x", __func__, __LINE__, SDI_STA(base), mask, SDI_DCOUNT(base), SDI_FIFOCNT(base));
				}else{
					if(sts & STA_DATAEND){ 
						//in case we already finished
						intr |= MMC_INTR_DATA;
						SDI_MASK0(base) = 0;
					}else{
						intr |= MMC_INTR_COMMAND;
						imask = STA_DATAEND;
						SDI_MASK0(base) = imask | STA_DTIMEOUT | STA_RXOVERR | STA_TXUNDERR | STA_STBITERR | STA_DCRCFAIL ;
						TIMER_START
					}
					if(hba->verbosity>3)
						slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "%s(%d): SDI_STATE_DMA_IN sts 0x%x, mask %x, dcount %x, fcnt %x", __func__, __LINE__, SDI_STA(base), mask, SDI_DCOUNT(base), SDI_FIFOCNT(base));
					
				}
				sta2x11->state &= ~SDI_STATE_CMD;
				pollcounter =0;

				goto finish;
			}else{ //pure command transfer, return command complete
				intr |= MMC_INTR_COMMAND;
				if(hba->verbosity>5)
					slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG1, "%s(%d): command success status 0x%x, mask %x, dcount %x, dctrl %x tlen %d, crrlen %d, state %x, fcnt %x", __func__, __LINE__, sts, mask, dcount, SDI_DCTRL(base), sta2x11->translen, sta2x11->currlen, sta2x11->state, SDI_FIFOCNT(base));
				_sta2x11_finish_io(sta2x11);
				pollcounter=0;
				SDI_ICR(base) = ICR_CLR_ALL;
				goto finish;
			}
		}else{
			if(hba->verbosity)
				slogf(_SLOGC_SIM_MMC, _SLOG_WARNING, "%s(%d): interrupt (poll %d) status 0x%x, mask %x, dcount %x, dctrl %x tlen %d, crrlen %d, state %x, fcnt %x", __func__, __LINE__, pollcounter,sts, mask, dcount, SDI_DCTRL(base), sta2x11->translen, sta2x11->currlen, sta2x11->state, SDI_FIFOCNT(base));
			if(pollcounter>50){
				intr |= MMC_ERR_CMD_TO | MMC_INTR_ERROR | MMC_INTR_COMMAND; 
				_sta2x11_finish_io(sta2x11);
				slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): command intr error 0x%x, mask %x cmd %d", __func__, __LINE__, sts, mask, sta2x11->command);

				pollcounter =0;
				SDI_ICR(base) = ICR_CLR_ALL;
				goto finish;
			}
			TIMER_START
			goto finish;
		}
	}

	//Data phase, either polling or we are getting interrupt (erro, pio write end, or dma)
	if(sts & (STA_DTIMEOUT | STA_RXOVERR | STA_TXUNDERR | STA_STBITERR | STA_DCRCFAIL )){
		intr |= MMC_INTR_DATA |MMC_INTR_ERROR;
		if(sts & STA_DTIMEOUT)
			intr |= MMC_ERR_DATA_TO;
		if(sts & STA_DCRCFAIL)
			intr |= MMC_ERR_DATA_CRC;
		slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): DATA ERROR: status 0x%x, mask %x, dcount %x, dctrl %x tlen %d, crrlen %d, state %x, fcnt %x, resp %x", __func__, __LINE__, sts, mask, dcount, SDI_DCTRL(base), sta2x11->translen, sta2x11->currlen, sta2x11->state, SDI_FIFOCNT(base), SDI_RESP0(base));
		_sta2x11_finish_io(sta2x11);
		SDI_ICR(base) = ICR_CLR_ALL;
		goto finish;
	}else if((mask & (STA_DATAEND)) && (sta2x11->state & SDI_STATE_PIO_OUT)){
		intr |= MMC_INTR_DATA;
		_sta2x11_finish_io(sta2x11);
		if(hba->verbosity>3)
			slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "%s(%d): PIO OUT END: status 0x%x, mask %x, dcount %x, dctrl %x tlen %d, crrlen %d, state %x, fcnt %x", __func__, __LINE__, sts, mask, dcount, SDI_DCTRL(base), sta2x11->translen, sta2x11->currlen, sta2x11->state, SDI_FIFOCNT(base));
		SDI_ICR(base) = ICR_CLR_ALL;
		goto finish;
	}else if((sts & STA_DATAEND) && (sta2x11->state & SDI_STATE_DMA)){
		intr |= MMC_INTR_DATA;
		if(hba->verbosity>3)
			slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "%s(%d): DMA Interrupt END: sts 0x%x, mask %x, dcount %x, fcnt %x", __func__, __LINE__, SDI_STA(base), mask, SDI_DCOUNT(base), SDI_FIFOCNT(base));
		SDI_MASK0(base) = 0;
		SDI_ICR(base) = sts & ICR_CLR_ALL;
		goto finish;
	}

	if(pollcounter>50){
		intr |= MMC_ERR_DATA_TO | MMC_INTR_ERROR | MMC_INTR_DATA; 
		_sta2x11_finish_io(sta2x11);
		slogf(_SLOGC_SIM_MMC, _SLOG_WARNING, "%s(%d): data phase poll timed out, status 0x%x, mask %x, dcount %x, fcnt %x", __func__, __LINE__, sts, mask, SDI_DCOUNT(base), SDI_FIFOCNT(base));
		pollcounter =0;
		SDI_ICR(base) = ICR_CLR_ALL;
		goto finish;
	}else{
		slogf(_SLOGC_SIM_MMC, _SLOG_WARNING, "%s(%d): interrupt (poll %d) status 0x%x, mask %x, dcount %x, fcnt %x", __func__, __LINE__, pollcounter,sts, mask, SDI_DCOUNT(base), SDI_FIFOCNT(base));
	}
	TIMER_START

finish:
#if 0
{
#define PAB_PEXISR_MISC 0xBA8
	unsigned val = 1<<((sta2x11->DevFunc & 0x7)+2);
	/* We need to manually clear PAB_PEXISR_MISC */
	pci_write_config32( sta2x11->BusNumber, sta2x11->DevFunc,
					PAB_PEXISR_MISC, 1, &val);
}
#endif
	
	return intr;
}

/*
 * setup DMA transfer
 */
static int _sta2x11_setup_dma(SIM_HBA *hba, paddr_t paddr, int len, int dir)
{
	SIM_MMC_EXT *ext;
	sta2x11_ext_t	*sta2x11;
	dma_transfer_t *tinfo;
	uint32_t 		dctrl;
	int			i, blkcnt, blksz;
	uintptr_t		base;

	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;
	tinfo = &sta2x11->tinfo;
	blksz = sta2x11->blksz;
	blkcnt = len / blksz;

	if (hba->verbosity>5)
		slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, "%s(%d): paddr %x, len %d, dir %x, blksize %d %d", __FUNCTION__, __LINE__, (uint32_t)paddr, len, dir, ext->mmc_blksize, sta2x11->blksz);

	len = blkcnt * sta2x11->blksz;
	dctrl = (sta2x11->blksz_bits<<DCTRL_DBLOCKSIZE_SHIFT);
	tinfo->src_addrs = sta2x11->src_addr;
	tinfo->dst_addrs = sta2x11->dst_addr;
	tinfo->dst_fragments = blkcnt;
	tinfo->src_fragments = blkcnt;
	if (dir == MMC_DIR_IN){
		dctrl |= DCTRL_DTDIR|DCTRL_DMAEN|DCTRL_DTEN;
		tinfo->dst_flags = DMA_ADDR_FLAG_MEMORY;
		tinfo->src_flags = DMA_ADDR_FLAG_NO_INCREMENT|DMA_ADDR_FLAG_DEVICE;
		for(i=0; i<blkcnt; i++){
			tinfo->src_addrs[i].paddr = sta2x11->fifo_reg;
			tinfo->src_addrs[i].len = blksz;
			tinfo->dst_addrs[i].paddr = paddr;
			tinfo->dst_addrs[i].len = blksz;
			paddr +=blksz;
		}
		sta2x11->state = SDI_STATE_DMA_IN;
		tinfo->xfer_bytes= len;
		if(sta2x11->dmafuncs.setup_xfer (sta2x11->dma_chn, tinfo)==-1){
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): setup_xfer failed DMA paddr %x, len %d, dir %x, blksize %d %d", __FUNCTION__, __LINE__, (uint32_t)paddr, len, dir, ext->mmc_blksize, sta2x11->blksz);
			sta2x11->dmafuncs.xfer_abort (sta2x11->dma_chn);
			sta2x11->dmafuncs.setup_xfer (sta2x11->dma_chn, tinfo);
		}		
		sta2x11->dmafuncs.xfer_start (sta2x11->dma_chn);
		sta2x11->state |= SDI_STATE_DMA_STARTED;
	}else{
		tinfo->dst_flags = DMA_ADDR_FLAG_NO_INCREMENT|DMA_ADDR_FLAG_DEVICE;
		tinfo->src_flags = DMA_ADDR_FLAG_MEMORY;
		for(i=0; i<blkcnt; i++){
			tinfo->src_addrs[i].paddr = paddr;
			tinfo->src_addrs[i].len = blksz;
			tinfo->dst_addrs[i].paddr = sta2x11->fifo_reg;
			tinfo->dst_addrs[i].len = blksz;
			paddr +=512;
		}
		tinfo->xfer_bytes= len;
		sta2x11->state = SDI_STATE_DMA_OUT;
		if(sta2x11->dmafuncs.setup_xfer (sta2x11->dma_chn, tinfo)==-1){
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): setup_xfer failed DMA paddr %x, len %d, dir %x, blksize %d %d", __FUNCTION__, __LINE__, (uint32_t)paddr, len, dir, ext->mmc_blksize, sta2x11->blksz);
			sta2x11->dmafuncs.xfer_abort (sta2x11->dma_chn);
			sta2x11->dmafuncs.setup_xfer (sta2x11->dma_chn, tinfo);
		}		
	}
	sta2x11->translen = len;
	sta2x11->currlen = 0;
	SDI_DTIMER(base)=sta2x11->clock;
	SDI_DLEN(base) = len;	
	SDI_DCTRL(base) = dctrl;
	
	return (sta2x11->translen);
}

/*
 * setup DMA transfer
 */
static int _sta2x11_setup_sgdma(SIM_HBA *hba, int dir)
{
	SIM_MMC_EXT *ext;
	sta2x11_ext_t	*sta2x11;
	CCB_SCSIIO	*ccb;
	SG_ELEM		*sge;
	dma_transfer_t *tinfo;
	uintptr_t		base;
	uint32_t 		dctrl, paddr;
	uint32_t		sgcnt, sgi, tlen=0;
	int				len, blkcnt=0, blksz;

	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;
	blksz = sta2x11->blksz;
	ccb = ext->nexus;
	tinfo = &sta2x11->tinfo;
	tinfo->src_addrs = sta2x11->src_addr;
	tinfo->dst_addrs = sta2x11->dst_addr;
	sge = ccb->cam_data.cam_sg_ptr;
	sgcnt= ccb->cam_sglist_cnt;

	if (hba->verbosity>5)
		slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, "%s(%d): sgcnt %d, dir %x", __FUNCTION__, __LINE__, sgcnt, dir);

	dctrl = (sta2x11->blksz_bits<<DCTRL_DBLOCKSIZE_SHIFT);
	if (dir == MMC_DIR_IN){
		tinfo->dst_flags = DMA_ADDR_FLAG_MEMORY;
		tinfo->src_flags = DMA_ADDR_FLAG_NO_INCREMENT|DMA_ADDR_FLAG_DEVICE;
		for(sgi=0; sgi<sgcnt; sgi++){
			len= sge[sgi].cam_sg_count;
			paddr = sge[sgi].cam_sg_address;
			
			if (hba->verbosity>5)
				slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): sgcnt %d, addr %x, len %d dir %x", __FUNCTION__, __LINE__, sgcnt, paddr, len, dir);
			do{
				tinfo->src_addrs[blkcnt].paddr = sta2x11->fifo_reg;
				tinfo->src_addrs[blkcnt].len = blksz;
				tinfo->dst_addrs[blkcnt].paddr = paddr;
				tinfo->dst_addrs[blkcnt].len = blksz;
				blkcnt++;
				len -= blksz;
				paddr +=blksz;
			}while(len>0);
			tlen += sge[sgi].cam_sg_count;;
		}
		dctrl |= DCTRL_DTDIR|DCTRL_DMAEN|DCTRL_DTEN;
		sta2x11->state = SDI_STATE_DMA_IN;
		tinfo->dst_fragments = blkcnt;
		tinfo->src_fragments = blkcnt;
		tinfo->xfer_bytes= tlen;
		if(sta2x11->dmafuncs.setup_xfer (sta2x11->dma_chn, tinfo)==-1){
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): setup_xfer failed DMA sgcnt %d, dir %x", __FUNCTION__, __LINE__, sgcnt, dir);
			sta2x11->dmafuncs.xfer_abort (sta2x11->dma_chn);
			sta2x11->dmafuncs.setup_xfer (sta2x11->dma_chn, tinfo);
		}		
		sta2x11->dmafuncs.xfer_start (sta2x11->dma_chn);
	}else{
		tinfo->dst_flags = DMA_ADDR_FLAG_NO_INCREMENT|DMA_ADDR_FLAG_DEVICE;
		tinfo->src_flags = DMA_ADDR_FLAG_MEMORY;
		for(sgi=0; sgi<sgcnt; sgi++){
			len= sge[sgi].cam_sg_count;
			paddr = sge[sgi].cam_sg_address;
			if (hba->verbosity>5)
				slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, "%s(%d): sgcnt %d, addr %x, len %d dir %x", __FUNCTION__, __LINE__, sgcnt, paddr, len, dir);
			do{
				tinfo->dst_addrs[blkcnt].paddr = sta2x11->fifo_reg;
				tinfo->dst_addrs[blkcnt].len = blksz;
				tinfo->src_addrs[blkcnt].paddr = paddr;
				tinfo->src_addrs[blkcnt].len = blksz;
				blkcnt++;
				len -= blksz;
				paddr +=blksz;
			}while(len>0);
			tlen += sge[sgi].cam_sg_count;;
		}
		sta2x11->state = SDI_STATE_DMA_OUT;
		tinfo->dst_fragments = blkcnt;
		tinfo->src_fragments = blkcnt;
		tinfo->xfer_bytes= tlen;
		if(sta2x11->dmafuncs.setup_xfer (sta2x11->dma_chn, tinfo)==-1){
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): setup_xfer failed DMA sgcnt %d, dir %x", __FUNCTION__, __LINE__, sgcnt, dir);
			sta2x11->dmafuncs.xfer_abort (sta2x11->dma_chn);
			sta2x11->dmafuncs.setup_xfer (sta2x11->dma_chn, tinfo);
		}		
	}
	sta2x11->currlen = 0;
	sta2x11->translen =tlen;
	SDI_DTIMER(base)=sta2x11->clock;
	SDI_DLEN(base) = tlen;	
	SDI_DCTRL(base) = dctrl;
	return (tlen);
}

/*
 * setup PIO transfer
 */
static int _sta2x11_setup_pio(SIM_HBA *hba, int len, int dir)
{
	SIM_MMC_EXT 	*ext;
	sta2x11_ext_t 	*sta2x11;
	uintptr_t		base;
	uint32_t		blkcnt;
	uint32_t 		dctrl;

	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;

	if (hba->verbosity >5)
		slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, "%s(%d): len %d, dir %x, blksize %d %d", __FUNCTION__, __LINE__, len, dir, ext->mmc_blksize, sta2x11->blksz);
	blkcnt = len / sta2x11->blksz;
	if (blkcnt == 0)
		return 0;
	
	sta2x11->translen = blkcnt * sta2x11->blksz;
	sta2x11->currlen = 0;
	dctrl = (sta2x11->blksz_bits<<DCTRL_DBLOCKSIZE_SHIFT);
	if (dir == MMC_DIR_IN){
		dctrl |= DCTRL_DTDIR| DCTRL_DTEN;
		sta2x11->state = SDI_STATE_PIO_IN;
	}else{
		sta2x11->state = SDI_STATE_PIO_OUT;
	}
	SDI_DTIMER(base)=sta2x11->clock;
	SDI_DLEN(base) = sta2x11->translen;
	SDI_DCTRL(base) = dctrl /*| DCTRL_BUSYMODE*/;
	nanospin_ns(10000);
	
	return (sta2x11->translen);
}

static int _sta2x11_dma_done(SIM_HBA *hba, int dir)
{
	SIM_MMC_EXT		*ext;
	sta2x11_ext_t	*sta2x11;
	uintptr_t		base, reg;
	uint32_t		imask, mask;
	uint32_t		sts, dcount, fcnt;
	int 			len; 
	int 			i=0;
	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;
	len = sta2x11->translen;
	if(hba->verbosity>5)
		slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, "%s(%d):DMA DONE: status 0x%x, mask %x, dcnt %x fcnt %x", __FUNCTION__, __LINE__, SDI_STA(base),SDI_MASK0(base),SDI_DCOUNT(base), SDI_FIFOCNT(base));
	sts = SDI_STA(base);
	dcount = SDI_DCOUNT(base);
	fcnt = SDI_FIFOCNT(base);
	imask = (dir == MMC_DIR_IN)?(STA_RXACT|STA_RXDAVL):(/*STA_TXACT|*/STA_TXDAVL);
	reg = (dir == MMC_DIR_IN)?0x48:0x30;
	while(i++<POLL_TIMEOUT){
		mask = SDI_STA(base) & imask;
		if(!mask && __REG32(base+reg)==0)
			break;
		nanospin_ns(1000);
	}
	if(i>500 && hba->verbosity>2){
		slogf(_SLOGC_SIM_MMC, _SLOG_WARNING, "%s(%d):DMA %d done timed out: i %d status 0x%x, mask %x, dcnt %x fcnt %x (%x %x %x)", __FUNCTION__, __LINE__,dir, i, SDI_STA(base),SDI_MASK0(base),SDI_DCOUNT(base), SDI_FIFOCNT(base), sts, dcount, fcnt);
	}
	sta2x11->dmafuncs.xfer_complete(sta2x11->dma_chn);
	sta2x11->translen= 0;
	sta2x11->currlen = 0;
	sta2x11->state = 0;
	SDI_DCTRL(base)= 0; 
	SDI_DLEN(base) = 0;
	SDI_CMD(base) = 0;
	nanospin_ns(1000);

	if(i>=POLL_TIMEOUT){
		return MMC_FAILURE;
	}
	return MMC_SUCCESS;
}

static int _sta2x11_pio_done(SIM_HBA *hba, char *buf, int len, int dir)
{
	SIM_MMC_EXT		*ext;
	sta2x11_ext_t	*sta2x11;
	uintptr_t		base;
	uint32_t		status=0, sstatus=0;
	int				loop, cnt=0, fifocnt;
	uint32_t		*pbuf = (uint32_t *)buf;

	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;
	
	if (dir == MMC_DIR_IN) {
		sstatus = SDI_STA(base);
		//read the data in
		for(loop = 0; cnt<len && loop<POLL_TIMEOUT; ){
			status = SDI_STA(base);
			fifocnt=(sta2x11->translen-sta2x11->currlen)-(SDI_FIFOCNT(base)<<2);
			if(/*(status & STA_RXDAVL) ||*/ fifocnt>0){
				while(fifocnt){
					*pbuf++ = SDI_FIFO(base);
					sta2x11->currlen +=4;
					cnt+=4;
					fifocnt -=4;
					nanospin_ns(100);
				}
				loop = 0;
			}else{
				nanospin_ns(1000);
				loop++;
			}
		}
		if(loop>=POLL_TIMEOUT ){
			slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): pio failed command %d sstatus %x, sts 0x%x, dcount %x, fcnt %x dctrl %x", 
				__func__, __LINE__, sta2x11->command, sstatus, SDI_STA(base), SDI_DCOUNT(base), SDI_FIFOCNT(base), SDI_DCTRL(base));
		}
		nanospin_ns(10000);
		sta2x11->translen= 0;
		sta2x11->currlen = 0;
		sta2x11->state = 0;
		SDI_DCTRL(base)= 0; //DCTRL_BUSYMODE;
		SDI_DLEN(base) = 0;
		SDI_CMD(base) = 0;
		//FIXME: because of busy wait bug, we have to delay sometime
		delay(5);
	} else {
		//write the data out
		for(loop = 0; cnt<len && loop<POLL_TIMEOUT; ){
			status = SDI_STA(base);
			if(!(status & STA_TXFIFOF)){
				SDI_FIFO(base) = *pbuf++;
				sta2x11->currlen +=4;
				cnt+=4;
				loop = 0;
			}else{
				nanospin_ns(1000);
				loop++;
			}
		}
		//enable dataend interrupt which will return INTR_DATA to higher layer
		SDI_MASK0(base) = STA_DTIMEOUT | STA_RXOVERR | STA_TXUNDERR | STA_STBITERR | STA_DCRCFAIL | STA_DATAEND;
		TIMER_START
	}
	if(loop>=POLL_TIMEOUT ){
		slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "%s(%d): pio failed command %d sstatus %x, sts 0x%x, dcount %x, fcnt %x dctrl %x", 
			__func__, __LINE__, sta2x11->command, sstatus, SDI_STA(base), SDI_DCOUNT(base), SDI_FIFOCNT(base), SDI_DCTRL(base));
	}

	return cnt;
}

static int _sta2x11_command(SIM_HBA *hba, mmc_cmd_t *cmd)
{
	SIM_MMC_EXT		*ext;
	sta2x11_ext_t	*sta2x11;
	uintptr_t		base;
	uint32_t		command, imask;

	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;

	/* save the command */
	sta2x11->command = cmd->opcode & CMD_INDEX_MSK;

	if(hba->verbosity>5)
		slogf(_SLOGC_SIM_MMC, _SLOG_DEBUG2, "%s(%d): cmd %d", __FUNCTION__, __LINE__, (cmd->opcode & CMD_INDEX_MSK));
	if (SDI_CMD(base) & CMD_CPSMEN) {
		if(hba->verbosity)
			slogf(_SLOGC_SIM_MMC, _SLOG_WARNING, "_sta2x11_command: previous command not finisehd %x\n", SDI_CMD(base));
		SDI_CMD(base) = 0;
		nanospin_ns(10000);
	}

	command = (cmd->opcode & CMD_INDEX_MSK) | CMD_CPSMEN;
	if (cmd->resp & MMC_RSP_PRESENT) {
		if (cmd->resp & MMC_RSP_136) 	// must be R2
			command |= CMD_LONGRESP;
		command |= CMD_WAITRESP;
		imask = STA_CMDREND;
	}else imask = STA_CMDSENT;

	SDI_MASK0(base) = ERR_MASK | STA_CCRCFAIL | imask;
	SDI_ARG(base) = cmd->argument;
	SDI_CMD(base)  = command;
	sta2x11->state |=SDI_STATE_CMD;

	TIMER_START
		
	return (MMC_SUCCESS);
}

static int _sta2x11_cfg_bus(SIM_HBA *hba, int width, int mmc)
{
	SIM_MMC_EXT		*ext;
	sta2x11_ext_t	*sta2x11;
	uintptr_t		base;
	uint32_t		clkcr;

	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;

	clkcr = SDI_CLKCR(base) & ~CLKCR_WIDBUS_MSK;
	switch(width){
		case 8: clkcr |= 0x2<<CLKCR_WIDBUS_SHIFT; break;
		case 4: clkcr |= 0x1<<CLKCR_WIDBUS_SHIFT; break;
		default: 		break;
	}
	SDI_CLKCR(base) = clkcr;
	nanospin_ns(10000);
	if (hba->verbosity>4)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "_sta2x11_clock: SDI_CLKCR %x", SDI_CLKCR(base));

	return (MMC_SUCCESS);
}

static int _sta2x11_clock(SIM_HBA *hba, int *clock, int high_speed)
{
	SIM_MMC_EXT	*ext;
	sta2x11_ext_t		*sta2x11;
	uintptr_t			base;
	uint32_t			clkctl=0, clkcr;
	int				clk, sclock;
	
	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;

	clk = sta2x11->clock;
	
	// Disable SD clock first
	clkcr = SDI_CLKCR(base) & ~(CLKCR_CLKEN|CLKCR_CLKDIV_MSK);
	SDI_CLKCR(base) = clkcr;
	nanospin_ns(10000);
	
	sclock = *clock;
	if(*clock>=clk){
		*clock = clk;
		clkcr |=CLKCR_BYPASS;
	}else{
		for(clkctl=0; clkctl<255; clkctl++){
			if(( clk / (clkctl + 2))<= *clock){
				break;
			}
		}
		*clock = clk / (clkctl + 2);
		clkcr &= ~CLKCR_BYPASS;
		clkcr |=clkctl;
	}
	
	SDI_CLKCR(base) = clkcr|CLKCR_CLKEN;
	nanospin_ns(20000);
	if (hba->verbosity>4)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "_sta2x11_clock: SDI_CLKCR %x (request %d set %d)", SDI_CLKCR(base),sclock, *clock);

	return (MMC_SUCCESS);
}

static int _sta2x11_block_size(SIM_HBA *hba, int blksz)
{
	SIM_MMC_EXT		*ext;
	sta2x11_ext_t	*sta2x11;
	uintptr_t		base;
	uint32_t 		val;
	int i, p;
	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;

	if (blksz > 16384) //max 16K
		return (MMC_FAILURE);

	sta2x11->blksz = blksz;

	for(i=14, p=16384; i>0; i--){
		if(p==blksz){
			break;
		}
		p >>=1;
	}
	
	val = SDI_DCTRL(base) & ~(DCTRL_DBLOCKSIZE_MSK<<DCTRL_DBLOCKSIZE_SHIFT);
	SDI_DCTRL(base) = val | (i<<DCTRL_DBLOCKSIZE_SHIFT);
	sta2x11->blksz_bits = i;
	nanospin_ns(10000);
	
	return (MMC_SUCCESS);
}

/*
 * Reset host controller and card
 * The clock should be enabled and set to minimum (<400KHz)
 */
static int _sta2x11_powerup(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	sta2x11_ext_t	*sta2x11;
	uintptr_t		base;
	uint32_t		clk;
	int		clkctl;
	uint32_t		clkcr;

	ext   = (SIM_MMC_EXT *)hba->ext;
	sta2x11 = (sta2x11_ext_t *)ext->handle;
	base  = sta2x11->base;

	sta2x11->translen= 0;
	sta2x11->currlen = 0;
	sta2x11->state = 0;

#ifdef POLL_MODE
	if(!sta2x11->timer_initialized){
		SIGEV_PULSE_INIT (&sta2x11->timer, hba->coid, getprio(0)+1, SIM_DMA_INTERRUPT, NULL);
		if(timer_create(CLOCK_REALTIME, &sta2x11->timer, &sta2x11->timer_id)==-1){
			fprintf(stderr, "sta2x11: Timer create failed");
			return (MMC_FAILURE);
		}
		sta2x11->timer_initialized = 1;
	}
#endif
	/*
	 * Card is in
	 */
	ext->eflags &= ~MMC_EFLAG_WP;

	/*
	 * Apply a software reset
	 * nothing need to be done for this controller
	 */

	/*
	 * Enable internal clock
	 * Set clock to < 400KHz for ident
	 */
	clk = sta2x11->clock;
	if (hba->verbosity>4)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "_sta2x11_clock: %d", clk);
	for(clkctl=0; clkctl<=255; clkctl++){
		if(( clk / (clkctl + 2))< 400000){
			break;
		}
	}

	/*
	 * Enable internal clock first as well as HW flow control
	 */
	clkcr =clkctl | CLKCR_HWFC_EN ;
	if (hba->verbosity>4)
		slogf(_SLOGC_SIM_MMC, _SLOG_INFO, "_sta2x11_clock: %d clkcr %x", clk, clkcr);
	SDI_CLKCR(base) = clkcr|CLKCR_CLKEN; 
	nanospin_ns(10000);

	if(sta2x11->port){
		//set low to enable power
		__REG32(sta2x11->gpio_base+GPIO_DATC) = (1<<sta2x11->gpio_pwr);
		delay(1);
	}

	/*
	 * Powerup the bus
	 */
	SDI_PWR(sta2x11->base) = 0x03;	
	nanospin_ns(20000);

	/*
	 * Currently set the timeout to proximately 1sec
	 */
	SDI_DTIMER(base)=sta2x11->clock;
	SDI_CSEL(base)=0;
	SDI_DCTRL(base) = 0; //DCTRL_BUSYMODE;
	/*
	 * disable all interrupts
	 */
	SDI_MASK0(base) = 0;
	SDI_ICR(base) = ICR_CLR_ALL;
	nanospin_ns(10000);
	if(hba->verbosity>5)
		_sta2x11_dumpregs(hba);
	
	return (MMC_SUCCESS);
}

static int _sta2x11_powerdown(SIM_HBA *hba)
{
	CONFIG_INFO		*cfg;
	SIM_MMC_EXT	*ext;
	sta2x11_ext_t		*sta2x11;

	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	sta2x11 = (sta2x11_ext_t *)ext->handle;

#ifdef POLL_MODE
	if(sta2x11->timer_initialized)
		TIMER_STOP
#endif

	SDI_CMD(sta2x11->base) = 0;
	SDI_DCTRL(sta2x11->base) = 0;
	nanospin_ns(10000);
	/* Power off the bus */
	SDI_PWR(sta2x11->base) = 0x00;
	nanospin_ns(20000);

	if(sta2x11->port){
		//set high
		__REG32(sta2x11->gpio_base+GPIO_DATS) = (1<<sta2x11->gpio_pwr);
		delay(1);
	}

	/* Disable clocks */
	SDI_CLKCR(sta2x11->base) = 0x00;
	nanospin_ns(20000);
		
	return (MMC_SUCCESS);
}

static int _sta2x11_shutdown(SIM_HBA *hba)
{
	CONFIG_INFO			*cfg;
	SIM_MMC_EXT			*ext;
	sta2x11_ext_t		*sta2x11;

	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	sta2x11 = (sta2x11_ext_t *)ext->handle;

	/* disable all interrupts and clear all status*/
	SDI_MASK0(sta2x11->base) = 0;
	SDI_ICR(sta2x11->base) = ICR_CLR_ALL;

	_sta2x11_powerdown(hba);

	munmap_device_memory((void *)sta2x11->base, cfg->MemLength[0]);
	pci_detach_device(sta2x11->pci_dev_hdl);
	pci_detach(sta2x11->pci_hdl);
	free(sta2x11);

	return (MMC_SUCCESS);
}

int	sta2x11_init(SIM_HBA *hba)
{
	CONFIG_INFO			*cfg;
	SIM_MMC_EXT			*ext;
	sta2x11_ext_t		*sta2x11;
	struct pci_dev_info	pci_info;
	int 				pindex=0;
	uintptr_t			base;
	
	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;


	if ((sta2x11 = calloc(1, sizeof(sta2x11_ext_t))) == NULL)
		return (MMC_FAILURE);

	memset(&sta2x11->spinlock, 0, sizeof( intrspin_t ) );

	if (cfg->Device_ID.DevID != 0){
		if ((sta2x11->pci_hdl = pci_attach(0)) == -1) {
			slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "sta2x11_init: pci_attach failed");
			goto fail1;
		}

		pci_info.VendorId = cfg->Device_ID.DevID & 0xFFFF;
		pci_info.DeviceId = cfg->Device_ID.DevID >> 16;
		pindex = cfg->Device_ID.SerialNum >>16;
		sta2x11->pci_dev_hdl = pci_attach_device(NULL, PCI_SEARCH_VENDEV | PCI_MASTER_ENABLE 
			| PCI_INIT_ALL|PCI_SHARE, pindex, &pci_info);
		if (sta2x11->pci_dev_hdl == NULL){
			fprintf(stderr, "%s(%d): pci_attach_device failed\n", __FUNCTION__, __LINE__);
			goto fail2;
		}
		sta2x11->BusNumber = pci_info.BusNumber;
		sta2x11->DevFunc = pci_info.DevFunc;
		if (cfg->NumIRQs == 0) {
			cfg->IRQRegisters[0] = pci_info.Irq;
			cfg->NumIRQs = 1;
			cfg->IRQRegisters[1] = -1;
		}

		if (cfg->NumMemWindows == 0) {
			cfg->MemBase[0] = PCI_MEM_ADDR(pci_info.CpuBaseAddress[0]);
			cfg->MemLength[0] = pci_info.BaseAddressSize[0];
			cfg->NumMemWindows = 1;
		}
	} else if (cfg->NumMemWindows == 0) {
		slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "sta2x11_init: invalid commandline args\n");
		fprintf(stderr, "%s(%d): \n", __FUNCTION__, __LINE__);
		return (MMC_FAILURE);
	}

	base = (uintptr_t)mmap_device_memory(NULL, cfg->MemLength[0],
				PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, cfg->MemBase[0]);
	if (base == (uintptr_t)MAP_FAILED) {
		slogf (_SLOGC_SIM_MMC, _SLOG_ERROR, "sta2x11_init:memory map failed\n");
		if(cfg->Device_ID.DevID != 0)
			goto fail3;
		free(sta2x11);
		return (MMC_FAILURE);
	}

#ifdef POLL_MODE
	sta2x11->itime.it_value.tv_sec = 0;
	/* million nsecs */
	sta2x11->itime.it_value.tv_nsec    =  1000000 * 100;
	memset(&sta2x11->zerotime, 0, sizeof(sta2x11->zerotime));
#endif

	ext->hccap |= MMC_HCCAP_BW1 | MMC_HCCAP_BW4 | MMC_HCCAP_NOCD_BUSY;
	ext->hccap |= MMC_HCCAP_18V | MMC_HCCAP_30V | MMC_HCCAP_33V | MMC_HCCAP_DMA | MMC_HCCAP_HS;
	ext->maxio	= NR_DESCS_PER_CHANNEL;
	ext->pwr_retry = 0;
	ext->pwr_delay = 1;
	sta2x11->clock = SDI_MAX_CLK;  //this could be override by board specific code
	sta2x11->base  = base;
	sta2x11->pbase = cfg->MemBase[0];
	sta2x11->hba   = hba;
	
	ext->handle    = sta2x11;
	ext->clock     = sta2x11->clock;
	ext->detect    = _sta2x11_detect;
	ext->powerup   = _sta2x11_powerup;
	ext->powerdown = _sta2x11_powerdown;
	ext->cfg_bus   = _sta2x11_cfg_bus;
	ext->set_clock = _sta2x11_clock;
	ext->set_blksz = _sta2x11_block_size;
	ext->interrupt = _sta2x11_interrupt;
	ext->command   = _sta2x11_command;
	ext->setup_dma = _sta2x11_setup_dma;
	ext->setup_sgdma = _sta2x11_setup_sgdma;
	ext->dma_done  = _sta2x11_dma_done;
	ext->setup_pio = _sta2x11_setup_pio;
	ext->pio_done  = _sta2x11_pio_done;
	ext->shutdown  = _sta2x11_shutdown;
	/* disable all interrupts and clear all status*/
	SDI_MASK0(base) = 0;
	SDI_ICR(base) = ICR_CLR_ALL;
	
	return (MMC_SUCCESS);

fail3:
	pci_detach_device(sta2x11->pci_dev_hdl);
fail2:
	pci_detach(sta2x11->pci_hdl);
fail1:
	free(sta2x11);
	return (MMC_FAILURE);
}

#endif

__SRCVERSION( "$URL$ $Rev$" );

