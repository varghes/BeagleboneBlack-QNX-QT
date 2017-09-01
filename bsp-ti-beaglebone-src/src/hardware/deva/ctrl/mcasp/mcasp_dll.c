/*
 * $QNXLicenseC:
 * Copyright 2007-2008, QNX Software Systems.
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
 *
 *    mcasp_dll.c
 */

#include <mcasp.h>

int32_t mcasp_capabilities(HW_CONTEXT_T * mcasp_card, snd_pcm_channel_info_t * info)
{
	int chn_avail = 1;
	int tmp_rx_max_voices = 0;
	int i;
	
	if (info->channel == SND_PCM_CHANNEL_PLAYBACK && mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn)
		chn_avail = 0;
	else if (info->channel == SND_PCM_CHANNEL_CAPTURE)
	{
		if (mcasp_card->pcm_chn[CAP_CHN].pcm_subchn)
			chn_avail = 0;
		else
		{
			// count the number of active rx channels
			for(i=0; i<TDM_NSLOTS; i++)
			{
				if((mcasp_card->mcasp->RTDM>>i) & 1)
					tmp_rx_max_voices++;
			}
			
			info->min_voices = info->max_voices = tmp_rx_max_voices;
		}
	}
	
	if (chn_avail == 0)
	{
		info->formats = 0;
		info->rates = 0;
		info->min_rate = 0;
		info->max_rate = 0;
		info->min_voices = 0;
		info->max_voices = 0;
		info->min_fragment_size = 0;
		info->max_fragment_size = 0;
	}
	
	return (0);
}


int32_t
mcasp_playback_aquire(HW_CONTEXT_T * mcasp_card, PCM_SUBCHN_CONTEXT_T ** pc,
					  ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	
	if (mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return (EAGAIN);
	}
	
	ado_mutex_lock(&mcasp_card->hw_lock);
	
	/* Ensure ASP and DMA are disable */
	mcasp_card->edma3->EECR = (1 << mcasp_card->dma_play_idx);
	mcasp_card->mcasp->XGBLCTL &= ~(XGBLCTL_XSMRST);
	
	if ((config->dmabuf.addr = ado_shm_alloc(config->dmabuf.size, config->dmabuf.name,
											 ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock(&mcasp_card->hw_lock);
		return (errno);
	}
	
	
	memset(&mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx], 0x00, sizeof (mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx]));
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].OPT = OPT_TCINTEN | OPT_TCC(mcasp_card->dma_play_idx);
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].ACNT = (SAMPLE_SIZE);	/* Sample size in bytes */
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].BCNT = ado_pcm_dma_int_size(config) / mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].ACNT;	/* Number of samples per frag */
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].DST = mcasp_card->mcasp_dmax;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].SRCBIDX = mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].ACNT;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].DSTBIDX = 0;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].LINK = 0x4000 + 0x20 * (DMA_RELOAD + mcasp_card->dma_play_idx);
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].BCNTRLD = mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].BCNT;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].SRCCIDX = 0;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].DSTCIDX = 0;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].CCNT = 1;	/* Number of frags per param set */
	
	/* Reload Param set */
	memcpy(&mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx + DMA_RELOAD], &mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx],
		   sizeof (mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx]));
	
	mcasp_card->pcm_chn[PLAY_CHN].pcm_config = config;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn = *pc = subchn;
	ado_mutex_unlock(&mcasp_card->hw_lock);
	return (EOK);
}


int32_t mcasp_playback_release(HW_CONTEXT_T * mcasp_card, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	
	ado_mutex_lock(&mcasp_card->hw_lock);
	mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn = NULL;
	ado_shm_free(config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
	ado_mutex_unlock(&mcasp_card->hw_lock);
	return (0);
}


int32_t
mcasp_capture_aquire(HW_CONTEXT_T * mcasp_card, PCM_SUBCHN_CONTEXT_T ** pc,
					 ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	off_t offset;
	if (mcasp_card->pcm_chn[CAP_CHN].pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return (EAGAIN);
	}
	
	ado_mutex_lock(&mcasp_card->hw_lock);
	
	/* Ensure ASP and DMA are disable */
	mcasp_card->edma3->EECR = (1 << mcasp_card->dma_cap_idx);
	mcasp_card->mcasp->RGBLCTL &= ~(RGBLCTL_RSMRST);
	
	if ((config->dmabuf.addr = ado_shm_alloc(config->dmabuf.size, config->dmabuf.name,
											 ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock(&mcasp_card->hw_lock);
		return (errno);
	}
	mem_offset(&mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx], NOFD, 1, &offset, 0);
	
	memset(&mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx], 0x00, sizeof (mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx]));
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].OPT = OPT_TCINTEN | OPT_TCC(mcasp_card->dma_cap_idx);
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].SRC = mcasp_card->mcasp_dmax;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].ACNT = (SAMPLE_SIZE);	/* Sample size in bytes */
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].BCNT = ado_pcm_dma_int_size(config) / mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].ACNT;	/* Number of samples per frag */
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].SRCBIDX = 0;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].DSTBIDX = mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].ACNT;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].LINK = 0x4000 + 0x20 * (DMA_RELOAD + mcasp_card->dma_cap_idx);
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].BCNTRLD = mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].BCNT;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].SRCCIDX = 0;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].DSTCIDX = 0;
	mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].CCNT = 1;	/* Number of frags per param set */
	
	/* Reload Param set */
	memcpy(&mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx + DMA_RELOAD], &mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx],
		   sizeof (mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx]));
	
	mcasp_card->pcm_chn[CAP_CHN].pcm_config = config;
	mcasp_card->pcm_chn[CAP_CHN].pcm_subchn = *pc = subchn;
	ado_mutex_unlock(&mcasp_card->hw_lock);
	return (EOK);
}

int32_t mcasp_capture_release(HW_CONTEXT_T * mcasp_card, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	ado_mutex_lock(&mcasp_card->hw_lock);
	mcasp_card->pcm_chn[CAP_CHN].pcm_subchn = NULL;
	ado_shm_free(config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
	ado_mutex_unlock(&mcasp_card->hw_lock);
	return (0);
}


int32_t mcasp_prepare(HW_CONTEXT_T * mcasp_card, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	if (pc == mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn)
	{
		/* Set the phys DMA adder in the two param sets */
		mcasp_card->pcm_chn[PLAY_CHN].pcm_cur_frag = 0;
		mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx].SRC =
		config->dmabuf.phys_addr + mcasp_card->pcm_chn[PLAY_CHN].pcm_cur_frag++ * ado_pcm_dma_int_size(config);
		mcasp_card->edma3->PaRAM[mcasp_card->dma_play_idx + DMA_RELOAD].SRC =
		config->dmabuf.phys_addr + mcasp_card->pcm_chn[PLAY_CHN].pcm_cur_frag++ * ado_pcm_dma_int_size(config);
	}
	else
	{
		/* Set the phys DMA adder in the two param sets */
		mcasp_card->pcm_chn[CAP_CHN].pcm_cur_frag = 0;
		mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx].DST =
		config->dmabuf.phys_addr + mcasp_card->pcm_chn[CAP_CHN].pcm_cur_frag++ * ado_pcm_dma_int_size(config);
		mcasp_card->edma3->PaRAM[mcasp_card->dma_cap_idx + DMA_RELOAD].DST =
		config->dmabuf.phys_addr + mcasp_card->pcm_chn[CAP_CHN].pcm_cur_frag++ * ado_pcm_dma_int_size(config);
	}
	return (0);
}


int32_t mcasp_trigger(HW_CONTEXT_T * mcasp_card, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int timeout;
	ado_mutex_lock(&mcasp_card->hw_lock);
	
	
	if (cmd == ADO_PCM_TRIGGER_GO)
	{
				
		/* Enable ASP and DMA Channel */
		if (pc == mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn)
		{
#if !defined(VARIANT_j2_pcm3168a)
			/* AFIFO ENABLE*/
			mcasp_card->mcasp->WFIFOCTL = WFIFOCTL_WNUMEVT(1) | WFIFOCTL_WNUMDMA(1);
			mcasp_card->mcasp->WFIFOCTL |= WFIFOCTL_WENA;
#endif
			mcasp_card->mcasp->SRCTL[TX_SERIALIZER] = SRCTL_XMIT;			
			/* Enable serializer */
			timeout = 1000;
			mcasp_card->mcasp->XGBLCTL |= XGBLCTL_XSRCLR;
			while ( ((mcasp_card->mcasp->XGBLCTL & XGBLCTL_XSRCLR) != XGBLCTL_XSRCLR) && timeout--)
				nanospin_ns(100);
			if(timeout <= 0)
				ado_error("mcasp_trigger: tx serializer not enabled");
			
			/* Enable DMA */
			mcasp_card->edma3->EESR = (1 << mcasp_card->dma_play_idx);
			
			/* Wait for TX FIFO to fill to ensure the hardware does not underrun when we turn it on */
			timeout = 1000;
			while ( ((mcasp_card->mcasp->XSTAT & XSTAT_XDATA)) && timeout--)
				nanospin_ns(100);
			if(timeout <= 0)
				ado_error("mcasp_trigger: TX FIFO is never filled by EDMA");
			
			/* Clear underrun flag*/
			mcasp_card->mcasp->XSTAT |= XSTAT_XUNDRN;
			
			/* Enable underrun interrupt*/
			mcasp_card->mcasp->XINTCTL = XINTCTL_XUNDRN;
			
			/* Release TX State machine from reset */
			timeout = 1000;
			mcasp_card->mcasp->XGBLCTL |= XGBLCTL_XSMRST;
			while ( ((mcasp_card->mcasp->XGBLCTL & XGBLCTL_XSMRST) != XGBLCTL_XSMRST) && timeout--)
				nanospin_ns(100);
			if(timeout <= 0)
				ado_error("mcasp_trigger: unable to release TX State Machine from reset");
			
			
		}
		else
		{
#if !defined(VARIANT_j2_pcm3168a)
			/* AFIFO ENABLE*/
			mcasp_card->mcasp->RFIFOCTL = RFIFOCTL_RNUMEVT(1) | RFIFOCTL_RNUMDMA(1);
			mcasp_card->mcasp->RFIFOCTL |= RFIFOCTL_RENA;
#endif
			mcasp_card->mcasp->SRCTL[RX_SERIALIZER] = SRCTL_RECV;
			
			/* Enable serializer */
			timeout = 1000;
			mcasp_card->mcasp->RGBLCTL |= RGBLCTL_RSRCLR;
			while ( ((mcasp_card->mcasp->RGBLCTL & RGBLCTL_RSRCLR) != RGBLCTL_RSRCLR) && timeout--)
				nanospin_ns(100);
			if(timeout <= 0)
				ado_error("mcasp_trigger: rx serializer not enabled");
			
			/* Enable DMA */
			mcasp_card->edma3->EESR = (1 << mcasp_card->dma_cap_idx);
			
			/* Clear overrun flag */
			mcasp_card->mcasp->RSTAT |=  RSTAT_ROVRN;
			
			/* Enable overrun interrupt */
			mcasp_card->mcasp->RINTCTL = RINTCTL_ROVRN;
			
			/* Release RX State machine from reset */
			timeout = 1000;
			mcasp_card->mcasp->RGBLCTL |= RGBLCTL_RSMRST;
			while (((mcasp_card->mcasp->RGBLCTL & RGBLCTL_RSMRST) != RGBLCTL_RSMRST) && timeout--)
				nanospin_ns(100);
			if(timeout <= 0)
				ado_error("mcasp_trigger: unable to release RX State Machine from reset");
		}
	}
	else
	{
		/* Disable ASP and DMA Channel */
		if (pc == mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn)
		{
			/* Disable underrun interrupt*/
			mcasp_card->mcasp->XINTCTL &= ~XINTCTL_XUNDRN;
			
			/* Disable DMA */
			mcasp_card->edma3->EECR = (1 << mcasp_card->dma_play_idx);
			
#if !defined(VARIANT_j2_pcm3168a)			
			/* Ensure fifo is empty */
			timeout = 5000;
			while( mcasp_card->mcasp->WFIFOSTS && timeout--)				
				nanospin_ns(10);
			if(timeout <= 0)
				ado_debug(DB_LVL_DRIVER, "TX AFIFO was not emptied!");
#endif
			/* Ensure TXBUF is empty */
			timeout = 1000;
			while ( (!(mcasp_card->mcasp->XSTAT & XSTAT_XDATA)) && timeout--)
				nanospin_ns(100);
			
			if(timeout <= 0)
				ado_error("mcasp_trigger: TXBUF was not emptied");
			
			/* Disable TX State Machine */
			mcasp_card->mcasp->XGBLCTL &= ~(XGBLCTL_XSMRST);
			/* Reset TX serializer */
			mcasp_card->mcasp->XGBLCTL &= ~XGBLCTL_XSRCLR;
			mcasp_card->mcasp->SRCTL[TX_SERIALIZER] = 0;
#if !defined(VARIANT_j2_pcm3168a)
			//AFIFO disable
			mcasp_card->mcasp->WFIFOCTL = 0;
#endif					
			
		}
		else
		{			
			/* Disable overrun interrupt */
			mcasp_card->mcasp->RINTCTL &= ~RINTCTL_ROVRN;
			/* Reset RX serializer */
			mcasp_card->mcasp->RGBLCTL &= ~RGBLCTL_RSRCLR;
			mcasp_card->mcasp->SRCTL[RX_SERIALIZER] = 0;
			/* Disable RX State Machine */
			mcasp_card->mcasp->RGBLCTL &= ~(RGBLCTL_RSMRST);
			/* Disable DMA */
			mcasp_card->edma3->EECR = (1 << mcasp_card->dma_cap_idx);
#if !defined(VARIANT_j2_pcm3168a)
			//AFIFO disable
			mcasp_card->mcasp->RFIFOCTL = 0;
#endif
			
		}
		
	}
	ado_mutex_unlock(&mcasp_card->hw_lock);
	return (0);
}


uint32_t mcasp_position(HW_CONTEXT_T * mcasp_card, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	uint32_t pos;
	int idx = mcasp_card->dma_play_idx;
	
	/* Position relative to current fragment */
	if (pc == mcasp_card->pcm_chn[CAP_CHN].pcm_subchn)
		idx = mcasp_card->dma_cap_idx;
	
	ado_mutex_lock(&mcasp_card->hw_lock);
	/* position = Frag size - (samples left * sample size) */
	pos = ado_pcm_dma_int_size(config) - (mcasp_card->edma3->PaRAM[idx].BCNT * (SAMPLE_SIZE));
	ado_mutex_unlock(&mcasp_card->hw_lock);
	return (pos);
}

void mcasp_rx_interrupt(mcasp_card_t* mcasp_card, int32_t irq)
{
	ado_error("mcasp_card: RX Overrun occured!");
	mcasp_card->mcasp->RSTAT |= RSTAT_ROVRN;
}

void mcasp_tx_interrupt(mcasp_card_t* mcasp_card, int32_t irq)
{
	ado_error("mcasp_card: TX Underrun occured!");
	mcasp_card->mcasp->XSTAT |= XSTAT_XUNDRN;
}


void mcasp_play_interrupt(mcasp_card_t * mcasp_card, int32_t irq)
{
	ado_pcm_config_t *config;
	int status = 0;
	
	ado_mutex_lock(&mcasp_card->hw_lock);
	/* Setup next param set */
	if (mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn)
	{
		config = mcasp_card->pcm_chn[PLAY_CHN].pcm_config;
		if (mcasp_card->pcm_chn[PLAY_CHN].pcm_cur_frag * ado_pcm_dma_int_size(config) >= config->dmabuf.size)
			mcasp_card->pcm_chn[PLAY_CHN].pcm_cur_frag = 0;
		
		mcasp_card->edma3->PaRAM[DMA_RELOAD + mcasp_card->dma_play_idx].SRC =
		config->dmabuf.phys_addr + mcasp_card->pcm_chn[PLAY_CHN].pcm_cur_frag++ * ado_pcm_dma_int_size(config);
		status = mcasp_card->dma_play_idx;
	}
	
	ado_mutex_unlock(&mcasp_card->hw_lock);
	
	if (status == mcasp_card->dma_play_idx)
		dma_interrupt(mcasp_card->pcm_chn[PLAY_CHN].pcm_subchn);
}

void mcasp_cap_interrupt(mcasp_card_t * mcasp_card, int32_t irq)
{
	ado_pcm_config_t *config;
	int status = 0;
	
	ado_mutex_lock(&mcasp_card->hw_lock);
	/* Setup next param set */
	if (mcasp_card->pcm_chn[CAP_CHN].pcm_subchn)
	{
		config = mcasp_card->pcm_chn[CAP_CHN].pcm_config;
		if (mcasp_card->pcm_chn[CAP_CHN].pcm_cur_frag * ado_pcm_dma_int_size(config) >= config->dmabuf.size)
			mcasp_card->pcm_chn[CAP_CHN].pcm_cur_frag = 0;
		
		mcasp_card->edma3->PaRAM[DMA_RELOAD + mcasp_card->dma_cap_idx].DST =
		config->dmabuf.phys_addr + mcasp_card->pcm_chn[CAP_CHN].pcm_cur_frag++ * ado_pcm_dma_int_size(config);
		status = mcasp_card->dma_cap_idx;
	}
	
	ado_mutex_unlock(&mcasp_card->hw_lock);
	
	if (status == mcasp_card->dma_cap_idx)
		dma_interrupt(mcasp_card->pcm_chn[CAP_CHN].pcm_subchn);
}


void mcasp_parse_commandline(HW_CONTEXT_T * mcasp_card, char *args)
{
	int opt = 0;
	char *value;
	char *opts[] = {
		"i2c_addr",			// 0
		"clk",				// 1
		"mcasp",			// 2
		"i2c_dev",			// 3
		"tx_voices",		// 4
		"rx_voices",		// 5
		"rate",				// 6
		"protocol",			// 7
		"sample_size",		// 8
		"hxclk_io",			// 9
		"mclk",				// 10
		NULL
	};
	
	mcasp_card->clk_master = CLOCK_MODE;
	mcasp_card->mcasp_baseaddr = MCASP0_BASEADDR;
	mcasp_card->mcasp_dmax = MCASP0_dMAX;
	mcasp_card->dma_play_idx = MCASP0_DMA_PLAY;
	mcasp_card->dma_cap_idx = MCASP0_DMA_CAP;
	mcasp_card->codec_i2c_addr = CODEC_SLAVE_ADDR1;
	mcasp_card->i2c_dev = 0xff; // board-codec layer will overwrite this value
	mcasp_card->tx_voices = TX_VOICES;
	mcasp_card->rx_voices = RX_VOICES;
	mcasp_card->clk_rate = OSC0_CLK_FREQ;
	mcasp_card->protocol = PROTOCOL;
	mcasp_card->sample_size = SAMPLE_SIZE;
	mcasp_card->hxclk_io = 0;	
	mcasp_card->mclk_freq = OSC0_CLK_FREQ;
	while (args != NULL && args[0] != 0)
	{
		switch ((opt = getsubopt(&args, opts, &value)))
		{
			case 0:
				if (value != NULL)
				{
					mcasp_card->codec_i2c_addr = strtoul(value, NULL, 0);
					ado_debug(DB_LVL_DRIVER, "mcasp_card: i2c_addr %d", mcasp_card->codec_i2c_addr);
				}
				break;
			case 1:
				if (value != NULL)
				{
					if (strcmp(value, "master") == 0)
						mcasp_card->clk_master = MASTER;
					else if (strcmp(value, "slave") == 0)
						mcasp_card->clk_master = SLAVE;
					ado_debug(DB_LVL_DRIVER, "mcasp_card: clk master = %s", (mcasp_card->clk_master == 0) ? "Master" : "Slave");
				}
				break;
			case 2:
				if (value != NULL)
				{
					mcasp_card->mcasp_idx = atoi(value);
					if (mcasp_card->mcasp_idx < 0 || mcasp_card->mcasp_idx > 5)
					{
						mcasp_card->mcasp_idx = 2;
						ado_debug(DB_LVL_DRIVER, "mcasp_card: Invalid MCASP index, using default index (MCASP2)");
					}
					else
						ado_debug(DB_LVL_DRIVER, "mcasp_card: Using MCASP%d", mcasp_card->mcasp_idx);
					switch (mcasp_card->mcasp_idx)
					{
						case 0:
							mcasp_card->mcasp_baseaddr = MCASP0_BASEADDR;
							mcasp_card->mcasp_dmax = MCASP0_dMAX;
							mcasp_card->dma_play_idx = MCASP0_DMA_PLAY;
							mcasp_card->dma_cap_idx = MCASP0_DMA_CAP;
							mcasp_card->tx_irq = MCASP0_TX_IRQ;
							mcasp_card->rx_irq = MCASP0_RX_IRQ;
							break;
						case 1:
							mcasp_card->mcasp_baseaddr = MCASP1_BASEADDR;
							mcasp_card->mcasp_dmax = MCASP1_dMAX;
							mcasp_card->dma_play_idx = MCASP1_DMA_PLAY;
							mcasp_card->dma_cap_idx = MCASP1_DMA_CAP;
							mcasp_card->tx_irq = MCASP1_TX_IRQ;
							mcasp_card->rx_irq = MCASP1_RX_IRQ;
							break;
						case 2:
							mcasp_card->mcasp_baseaddr = MCASP2_BASEADDR;
							mcasp_card->mcasp_dmax = MCASP2_dMAX;
							mcasp_card->dma_play_idx = MCASP2_DMA_PLAY;
							mcasp_card->dma_cap_idx = MCASP2_DMA_CAP;
							mcasp_card->tx_irq = MCASP2_TX_IRQ;
							mcasp_card->rx_irq = MCASP2_RX_IRQ;
							break;
						case 3:
							mcasp_card->mcasp_baseaddr = MCASP3_BASEADDR;
							mcasp_card->mcasp_dmax = MCASP3_dMAX;
							mcasp_card->dma_play_idx = MCASP3_DMA_PLAY;
							mcasp_card->dma_cap_idx = MCASP3_DMA_CAP;
							mcasp_card->tx_irq = MCASP3_TX_IRQ;
							mcasp_card->rx_irq = MCASP3_RX_IRQ;
							break;
						case 4:
							mcasp_card->mcasp_baseaddr = MCASP4_BASEADDR;
							mcasp_card->mcasp_dmax = MCASP4_dMAX;
							mcasp_card->dma_play_idx = MCASP4_DMA_PLAY;
							mcasp_card->dma_cap_idx = MCASP4_DMA_CAP;
							mcasp_card->tx_irq = MCASP4_TX_IRQ;
							mcasp_card->rx_irq = MCASP4_RX_IRQ;
							break;
						case 5:
							mcasp_card->mcasp_baseaddr = MCASP5_BASEADDR;
							mcasp_card->mcasp_dmax = MCASP5_dMAX;
							mcasp_card->dma_play_idx = MCASP5_DMA_PLAY;
							mcasp_card->dma_cap_idx = MCASP5_DMA_CAP;
							mcasp_card->tx_irq = MCASP5_TX_IRQ;
							mcasp_card->rx_irq = MCASP5_RX_IRQ;
							break;
						default:
							break;
					}
				}
				break;
			case 3:
				if (value != NULL)
				{
					mcasp_card->i2c_dev = atoi(value);
					if(mcasp_card->i2c_dev < 0 || mcasp_card->i2c_dev > 2)
					{
						mcasp_card->i2c_dev = 0xff; // let board-codec layer determine the default value
						ado_debug(DB_LVL_DRIVER, "mcasp: Invalid i2c_dev value (card will decide what to connect to)");
					}
				}
				break;
			case 4:
				if (value != NULL)
				{
					mcasp_card->tx_voices = atoi(value);
					if(mcasp_card->tx_voices < 1 || mcasp_card->tx_voices > 8)
					{
						mcasp_card->tx_voices = TX_VOICES;
						ado_debug(DB_LVL_DRIVER, "mcasp: Invalid tx channels, using tx_voices=%d",mcasp_card->tx_voices);
					}
				}
				break;
			case 5:
				if (value != NULL)
				{
					mcasp_card->rx_voices = atoi(value);
					if(mcasp_card->rx_voices < 1 || mcasp_card->rx_voices > 8)
					{
						mcasp_card->rx_voices = RX_VOICES;
						ado_debug(DB_LVL_DRIVER, "mcasp: Invalid rx channels, using rx_voices=%d", mcasp_card->rx_voices);
					}
				}
				break;
			case 6:
				if (value != NULL)
				{
					mcasp_card->clk_rate = atoi(value);
					ado_debug(DB_LVL_DRIVER, "mcasp: clock rate set to %d", mcasp_card->clk_rate);
				}
				break;
			case 7:
				if (value != NULL && *value != NULL)
				{
					if(strcmp(value, "tdm_i2s") == 0)
					{
						mcasp_card->protocol = PROTOCOL_TDM_I2S;
						ado_debug(DB_LVL_DRIVER, "mcasp: protocol set to tdm_i2s");
					}
					else if(strcmp(value, "tdm_lj") == 0)
					{
						mcasp_card->protocol = PROTOCOL_TDM_LJ;
						ado_debug(DB_LVL_DRIVER, "mcasp: protocol set to tdm_lj");
					}
					
				}
				break;
			case 8:
				if (value != NULL)
				{
					mcasp_card->sample_size = atoi(value);
				}
				break;
			case 9:
				if (value != NULL)
				{
					mcasp_card->hxclk_io = atoi(value);
				}
				break;
			case 10:
				if (value != NULL)
				{
					mcasp_card->mclk_freq = atoi(value);					
				}
				break;
			default:
				break;
		}
	}
}

void mcasp_init(mcasp_card_t * mcasp_card)
{
	uint16_t divider;
	int i;
	uint32_t regVal;
	int timeout;
	
	/* Setup MCASP for Playback */
	mcasp_card->mcasp->GBLCTL = 0;			 	  // Reset McASP
	mcasp_card->mcasp->PWRDEMU = PWRDEMU_FREE;		 // Free-running mode
	
#if !defined(VARIANT_j2_pcm3168a)	
	/* AFIFO ENABLE*/
	ado_debug(DB_LVL_DRIVER, "mcasp_card: Enabling AFIFO");
	mcasp_card->mcasp->WFIFOCTL = WFIFOCTL_WNUMEVT(1) | WFIFOCTL_WNUMDMA(1);
	mcasp_card->mcasp->RFIFOCTL = RFIFOCTL_RNUMEVT(1) | RFIFOCTL_RNUMDMA(1);
	mcasp_card->mcasp->WFIFOCTL |= WFIFOCTL_WENA;
	mcasp_card->mcasp->RFIFOCTL |= RFIFOCTL_RENA;
#endif	
	
	/* Configure all pins to function as a MCASP pin */
	mcasp_card->mcasp->PFUNC = PFUNC_MCASP;
	mcasp_card->mcasp->PDIR = PDIR_AXR_OUT(TX_SERIALIZER);	/* Set TX pin direction */
	
	/* 32-bit TDM-I2S Mode */
	/* 16/32-bit TDM-I2S / TDM_LJ Mode */
	if(2 == mcasp_card->sample_size)
	{
		mcasp_card->mcasp->XFMT = XFMT_XRVRS_MSB | XFMT_XSSZ_16BIT | (0x4); // rotate by 16 bits
		mcasp_card->mcasp->RFMT = RFMT_RRVRS_MSB | RFMT_RSSZ_16BIT | (0x4); // rotate by 16 bits
	}
	else
	{
		mcasp_card->mcasp->XFMT = XFMT_XRVRS_MSB | XFMT_XSSZ_32BIT;
		mcasp_card->mcasp->RFMT = RFMT_RRVRS_MSB | RFMT_RSSZ_32BIT;
	}
	
	if(PROTOCOL_TDM_I2S == mcasp_card->protocol)
	{
		mcasp_card->mcasp->XFMT |= XFMT_XDATDLY_1BIT;
		mcasp_card->mcasp->RFMT |= RFMT_RDATDLY_1BIT;
		
		//Transmit and receive clocks operate synchronously, transmit on falling edge, receive on rising
		mcasp_card->mcasp->ACLKXCTL = ACLKXCTL_CLKXP;
		mcasp_card->mcasp->ACLKRCTL = ACLKRCTL_CLKRP;
		
		if(TDM_NSLOTS == 4)
		{
			mcasp_card->mcasp->AFSXCTL = AFSXCTL_XMOD_4CH;	//FS period = 4 words
			mcasp_card->mcasp->AFSRCTL = AFSRCTL_RMOD_4CH;	
		}
		else if(TDM_NSLOTS == 8)
		{
			mcasp_card->mcasp->AFSXCTL = AFSXCTL_XMOD_8CH;	// FS period = 4 words
			mcasp_card->mcasp->AFSRCTL = AFSRCTL_RMOD_8CH;	
			
		}
		else 
		{
			//Transmit on falling edge, 2-slot (I2S mode)
			mcasp_card->mcasp->AFSXCTL = AFSXCTL_XMOD_I2S;
			//Receive on raising edge, 2-slot (I2S mode)
			mcasp_card->mcasp->AFSRCTL &= ~(AFSRCTL_FSRP);
			mcasp_card->mcasp->AFSRCTL |= AFSRCTL_RMOD_I2S;
		}
		
		
	}
	mcasp_card->mcasp->XMASK = 0xffffffff;			 // Don't Mask any bits
	mcasp_card->mcasp->RMASK = 0xffffffff;			 // Don't Mask any bits
	
	if (mcasp_card->clk_master == SLAVE)
	{
		//External frame sync, Single word, transmit on falling edge
		mcasp_card->mcasp->AFSXCTL |= AFSXCTL_FXWID |AFSXCTL_FSXP; 
		mcasp_card->mcasp->AFSXCTL &= ~(AFSXCTL_FSXM);
		
		//External frame sync, 1 single word,
		mcasp_card->mcasp->AFSRCTL &= ~(AFSRCTL_FSRM);
		mcasp_card->mcasp->AFSRCTL |= AFSRCTL_FRWID;
		
		//Transmit and receive clocks operate synchronously, External clock
		mcasp_card->mcasp->ACLKXCTL &= ~(ACLKXCTL_CLKXM | ACLKXCTL_ASYNC);
		
		//External clock source
		mcasp_card->mcasp->ACLKRCTL &= ~(ACLKRCTL_CLKRM);
		
		
		mcasp_card->mcasp->AHCLKXCTL = 0;
		mcasp_card->mcasp->AHCLKRCTL = 0;

		if(mcasp_card->hxclk_io != 0)
		{
			// set AHCLK as output
			mcasp_card->mcasp->PDIR |= (PDIR_AHCLKX_OUT);
			mcasp_card->mcasp->AHCLKXCTL = 0;
		}
	}
	else
	{
		/* Must program 1 less then desired value for clock dividers */
		divider = (OSC0_CLK_FREQ / AUXCLK_FREQ) - 1;
		mcasp_card->mcasp->AHCLKXCTL = AHCLKXCTL_HCLKXM | divider;	// Internal High Frequency Clock
		divider = (AUXCLK_FREQ / (SAMPLE_RATE * TDM_SLOT_SIZE * TDM_NSLOTS)) - 1;
		mcasp_card->mcasp->ACLKXCTL |= ACLKXCTL_CLKXM | divider;	// Internal BCLK
		
		/* Clock lines are output */
		mcasp_card->mcasp->PDIR |= (PDIR_AHCLKX_OUT | PDIR_ACLKX_OUT | PDIR_AFSX_OUT);
		
		mcasp_card->mcasp->AHCLKRCTL = 0;			 // Slave off of TX Clocks (External clock)
		mcasp_card->mcasp->ACLKRCTL = 0;			 // Slave off of TX Clocks (External clock)
		
		
		mcasp_card->mcasp->AFSXCTL |= AFSXCTL_FSXM | AFSXCTL_FXWID;	// Internal Frame Sync, FS width = 1 word
		mcasp_card->mcasp->AFSRCTL |= AFSRCTL_FRWID;	// Slave off of TX clocks (External clock)
		
			
	}
		
		/* Enable TX slots */
	regVal = 0;
	for(i=TX_TDM_SLOT_START;i<(TX_TDM_SLOT_START+mcasp_card->tx_voices);i++)
	{
		regVal |= TDMS(i);
	}
	mcasp_card->mcasp->XTDM = regVal;
	mcasp_card->mcasp->XINTCTL = 0;
	
	/* Enable RX slots */
	regVal = 0;
	for(i=RX_TDM_SLOT_START; i<(RX_TDM_SLOT_START+mcasp_card->rx_voices);i++)
	{
		regVal |= TDMS(i);
	}
	
	mcasp_card->mcasp->RTDM = regVal;
	mcasp_card->mcasp->RINTCTL = 0;
	
	/* Release TX High Frequency clk divider from reset*/
	timeout = 1000;
	mcasp_card->mcasp->XGBLCTL |= XGBLCTL_XHCLKRST;
	while (((mcasp_card->mcasp->XGBLCTL & XGBLCTL_XHCLKRST) != XGBLCTL_XHCLKRST) && timeout--)
		nanospin_ns(100);
	if(timeout <= 0)
		ado_error("mcasp_init: unable to release TX clk divider from reset");
	
	/* Reset TX BCLK divider */
	timeout = 1000;
	mcasp_card->mcasp->XGBLCTL |= XGBLCTL_XCLKRST;
	while (((mcasp_card->mcasp->XGBLCTL & XGBLCTL_XCLKRST) != XGBLCTL_XCLKRST) && timeout--)
		nanospin_ns(100);
	if(timeout <= 0)
		ado_error("mcasp_init: unable to release TX BCLK divider from reset");
	
	/* Release Frame Sync Generator from reset (even with external supplied clock ) */
	timeout = 1000;
	mcasp_card->mcasp->XGBLCTL |= XGBLCTL_XFRST;
	while (((mcasp_card->mcasp->XGBLCTL & XGBLCTL_XFRST) != XGBLCTL_XFRST) && timeout--)
		nanospin_ns(100);
	if(timeout <= 0)
		ado_error("mcasp_init: unable to release TX FS from reset");
	
	/* Enable high frequency clock (required even with external clock) */
	timeout = 1000;
	mcasp_card->mcasp->RGBLCTL |= RGBLCTL_RHCLKRST;
	while (((mcasp_card->mcasp->RGBLCTL & RGBLCTL_RHCLKRST) != RGBLCTL_RHCLKRST) && timeout--)
		nanospin_ns(100);
	if(timeout <= 0)
		ado_error("mcasp_init: unable to enable HF clock");
	/* Reset RX BCLK divider */
	timeout = 1000;
	mcasp_card->mcasp->RGBLCTL |= RGBLCTL_RCLKRST;
	while (((mcasp_card->mcasp->RGBLCTL & RGBLCTL_RCLKRST) != RGBLCTL_RCLKRST) && timeout--)
		nanospin_ns(100);
	if(timeout <= 0)
		ado_error("mcasp_init: unable to release RX BCLK divider from reset");
	
	/* Release Frame Sync Generator from reset (even with external supplied clock ) */
	timeout = 1000;
	mcasp_card->mcasp->RGBLCTL |= RGBLCTL_RFRST;
	while (((mcasp_card->mcasp->RGBLCTL & RGBLCTL_RFRST) != RGBLCTL_RFRST) && timeout--)
		nanospin_ns(100);
	if(timeout <= 0)
		ado_error("mcasp_init: unable to release RX FS from reset");
}

ado_dll_version_t ctrl_version;
void ctrl_version(int *major, int *minor, char *date)
{
	*major = ADO_MAJOR_VERSION;
	*minor = 1;
	date = __DATE__;
}

ado_ctrl_dll_init_t ctrl_init;
int ctrl_init(HW_CONTEXT_T ** hw_context, ado_card_t * card, char *args)
{
	mcasp_card_t *mcasp_card;
	
	ado_debug(DB_LVL_DRIVER, "pCTRL_DLL_INIT: mcasp_card");
	
	if ((mcasp_card = ado_calloc(1, sizeof (mcasp_card_t))) == NULL)
	{
		ado_error("Unable to allocate memory for mcasp_card (%s)", strerror(errno));
		return -1;
	}
	*hw_context = mcasp_card;
	mcasp_card->card = card;
	
	ado_card_set_shortname(card, "mcasp_card");
	ado_card_set_longname(card, "mcasp_card", 0x1000);
	
	mcasp_parse_commandline(mcasp_card, args);
	
	if ((mcasp_card->mcasp = mmap_device_memory(0, sizeof (mcasp_t),
												PROT_READ | PROT_WRITE | PROT_NOCACHE, 0,
												mcasp_card->mcasp_baseaddr)) == MAP_FAILED)
	{
		printf("Unable to mmap mcasp (%s) \n", strerror(errno));
		ado_free(mcasp_card);
		return -1;
	}
	
	if ((mcasp_card->edma3 = mmap_device_memory(0, sizeof (edma3_t),
												PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, EDMA3CC_ADDR)) == MAP_FAILED)
	{
		printf("Unable to mmap DMA (%s) \n", strerror(errno));
		ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
		ado_free(mcasp_card);
		return -1;
	}
	
	ado_mutex_init(&mcasp_card->hw_lock);
	
	/* Initialize the card */
	mcasp_card->edma3->DRAE0 = mcasp_card->edma3->DRAE0 | (1 << mcasp_card->dma_play_idx) | (1 << mcasp_card->dma_cap_idx);
	mcasp_card->edma3->EECR = (1 << mcasp_card->dma_play_idx) | (1 << mcasp_card->dma_cap_idx);
	
	mcasp_init(mcasp_card);
	
	if (ado_attach_interrupt(card, (EDMA_BASE_IRQ + mcasp_card->dma_play_idx), mcasp_play_interrupt, mcasp_card) != 0)
	{
		ado_error("Unable to attach playback interrupt (%s)", strerror(errno));
		ado_mutex_destroy(&mcasp_card->hw_lock);
		ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
		ado_device_munmap(mcasp_card->edma3, sizeof (edma3_t));
		ado_free(mcasp_card);
		return (-1);
	}
	
	if (ado_attach_interrupt(card, (EDMA_BASE_IRQ + mcasp_card->dma_cap_idx), mcasp_cap_interrupt, mcasp_card) != 0)
	{
		ado_error("Unable to attach capture interrupt (%s)", strerror(errno));
		ado_mutex_destroy(&mcasp_card->hw_lock);
		ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
		ado_device_munmap(mcasp_card->edma3, sizeof (edma3_t));
		ado_free(mcasp_card);
		return (-1);
	}
	
	if (ado_attach_interrupt(card, (mcasp_card->tx_irq), mcasp_tx_interrupt, mcasp_card) != 0)
	{
		ado_error("Unable to attach tx interrupt (%s)", strerror(errno));
		ado_mutex_destroy(&mcasp_card->hw_lock);
		ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
		ado_device_munmap(mcasp_card->edma3, sizeof (edma3_t));
		ado_free(mcasp_card);
		return (-1);
	}
	
	if (ado_attach_interrupt(card, (mcasp_card->rx_irq), mcasp_rx_interrupt, mcasp_card) != 0)
	{
		ado_error("Unable to attach rx interrupt (%s)", strerror(errno));
		ado_mutex_destroy(&mcasp_card->hw_lock);
		ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
		ado_device_munmap(mcasp_card->edma3, sizeof (edma3_t));
		ado_free(mcasp_card);
		return (-1);
	}
	
	
	mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.chn_flags = SND_PCM_CHNINFO_BLOCK | SND_PCM_CHNINFO_STREAM |
	SND_PCM_CHNINFO_INTERLEAVE | SND_PCM_CHNINFO_BLOCK_TRANSFER | SND_PCM_CHNINFO_MMAP | SND_PCM_CHNINFO_MMAP_VALID;
	if(2 == mcasp_card->sample_size)
		mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.formats = SND_PCM_FMT_S16_LE;
	else
		mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.formats = SND_PCM_FMT_S32_LE;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.rates = ado_pcm_rate2flag(SAMPLE_RATE);
	mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.min_rate = SAMPLE_RATE;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.max_rate = SAMPLE_RATE;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.min_voices = mcasp_card->tx_voices;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.max_voices = mcasp_card->tx_voices;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.min_fragsize = 64;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_caps.max_fragsize = 0xfffe;
	
	memcpy(&mcasp_card->pcm_chn[CAP_CHN].pcm_caps, &mcasp_card->pcm_chn[PLAY_CHN].pcm_caps,
		   sizeof (mcasp_card->pcm_chn[CAP_CHN].pcm_caps));
	mcasp_card->pcm_chn[CAP_CHN].pcm_caps.min_voices = mcasp_card->rx_voices;
	mcasp_card->pcm_chn[CAP_CHN].pcm_caps.max_voices = mcasp_card->rx_voices;
	
	mcasp_card->pcm_chn[PLAY_CHN].pcm_funcs.capabilities = mcasp_capabilities;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_funcs.aquire = mcasp_playback_aquire;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_funcs.release = mcasp_playback_release;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_funcs.prepare = mcasp_prepare;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_funcs.trigger = mcasp_trigger;
	mcasp_card->pcm_chn[PLAY_CHN].pcm_funcs.position = mcasp_position;
	
	mcasp_card->pcm_chn[CAP_CHN].pcm_funcs.capabilities = mcasp_capabilities;
	mcasp_card->pcm_chn[CAP_CHN].pcm_funcs.aquire = mcasp_capture_aquire;
	mcasp_card->pcm_chn[CAP_CHN].pcm_funcs.release = mcasp_capture_release;
	mcasp_card->pcm_chn[CAP_CHN].pcm_funcs.prepare = mcasp_prepare;
	mcasp_card->pcm_chn[CAP_CHN].pcm_funcs.trigger = mcasp_trigger;
	mcasp_card->pcm_chn[CAP_CHN].pcm_funcs.position = mcasp_position;
	
	if (ado_pcm_create(card, "mcasp_card PCM 0", 0, "mcasp_card-ssi", 1,
					   &mcasp_card->pcm_chn[PLAY_CHN].pcm_caps, &mcasp_card->pcm_chn[PLAY_CHN].pcm_funcs, 1,
					   &mcasp_card->pcm_chn[CAP_CHN].pcm_caps, &mcasp_card->pcm_chn[CAP_CHN].pcm_funcs, &mcasp_card->pcm))
	{
		ado_error("Unable to create pcm devices (%s)", strerror(errno));
		ado_mutex_destroy(&mcasp_card->hw_lock);
		ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
		ado_device_munmap(mcasp_card->edma3, sizeof (edma3_t));
		ado_free(mcasp_card);
		return (-1);
	}
	
	if (codec_mixer(card, mcasp_card))
	{
		ado_error("Unable to create a mixer");
		ado_mutex_destroy(&mcasp_card->hw_lock);
		ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
		ado_device_munmap(mcasp_card->edma3, sizeof (edma3_t));
		ado_free(mcasp_card);
		return -1;
	}
	
	
	if (ado_pcm_sw_mix(card, mcasp_card->pcm, mcasp_card->mixer))
	{
		ado_error("Unable to create a pcm sw mixer");
		ado_mutex_destroy(&mcasp_card->hw_lock);
		ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
		ado_device_munmap(mcasp_card->edma3, sizeof (edma3_t));
		ado_free(mcasp_card);
		return (-1);
	}
	
	return 0;
}


ado_ctrl_dll_destroy_t ctrl_destroy;
int ctrl_destroy(HW_CONTEXT_T * mcasp_card)
{
	ado_debug(DB_LVL_DRIVER, "CTRL_DLL_DESTROY: mcasp_card");
	ado_mutex_destroy(&mcasp_card->hw_lock);
	ado_device_munmap(mcasp_card->mcasp, sizeof (mcasp_t));
	ado_device_munmap(mcasp_card->edma3, sizeof (edma3_t));
	ado_free(mcasp_card);
	return (0);
}

__SRCVERSION("$URL:$ $Rev:$");
