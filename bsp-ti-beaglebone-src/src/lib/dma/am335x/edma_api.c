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

#include <sys/rsrcdbmgr.h>
#include <sys/rsrcdbmsg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "edma_shmem.h"
#include "edma.h"
#include "edma_init.h"
#include "edma_irq.h"

#define FIRST_USER	1

/* these two  variables (user_*) used to track & sync the users within same process */
static int user_cnt;
static pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;
static int qdma_ipr_no;
static channel_t *ch_arr[80];


/* This struct has configuration for AM335x
 */
static edmacc_t edmacc_s[AM335X_MAX_CC_NO] = {
		{
			.pbase = AM335X_EDMA0_CC_BASE,
			.edma_chnos = AM335X_EDMA0_NUM_CHAN,
			.qdma_chnos = AM335X_QDMA0_NUM_CHAN,
			.irq_no	= AM335X_EDMA0_IRQ_NO,
			.size = AM335X_EDMA_CC_SIZE,
			.tcno = AM335X_EDMA0_TCNOS
		}
};

static edma_t edma = {
	.cc = edmacc_s,
	.cc_nos = AM335X_CC_NOS
};


/* Returns the maximum number of channel present in the edma */
static inline int __attribute__((always_inline))max_ch(edma_t edma)
{
	int i = edma.cc_nos, rval = 0;
	while(i--)
		rval += edma.cc[i].edma_chnos + edma.cc[i].qdma_chnos;
	return rval;
}

/* Only this function directly access the global variable
 * edma 
 */
static inline edma_t* __attribute__((always_inline))get_edma(void)
{
	return &edma;
}

/* check whether the channel is a edma channel from the channel number given */
static inline int __attribute__((always_inline))is_edma_ch(edma_t edma, int channel)
{
	int i = edma.cc_nos, edmachnos = 0;
	while(i--)
		edmachnos += edma.cc[i].edma_chnos;
	return (channel >= 0 && channel < edmachnos);
}
/*
 * command line options for init
 */
//enum init_option_index {VARIANT, EDMA0_IRQ, EDMA1_IRQ, EDMA0_BASE, EDMA1_BASE};
enum init_option_index {VARIANT, EDMA0_IRQ, EDMA0_BASE};
static char *const init_options[] = {
	[VARIANT]					= "variant",	/* variant=am335x */
	[EDMA0_IRQ]					= "edma0irq",
	[EDMA0_BASE]				= "edma0base",
	NULL
};

static int parse_init_opt(char *options, edma_t *edmap)
{
	int opt;
	char *valuep;
	if(options)
		while((opt = getsubopt(&options, init_options, &valuep)) != -1) {
			switch(opt) {
				case VARIANT:
						if(!strcmp(valuep, "am335x")) {
							edmap->cc_nos = 1;
							slog("setting to variant %s", valuep);
						}
					break;
				case EDMA0_IRQ:
					edmap->cc[0].irq_no = strtoul(valuep, NULL, 0);
					break;
				case EDMA0_BASE:
					edmap->cc[0].pbase = strtoul(valuep, NULL, 0);
					break;
				default:
					slog("unknown option given to init function\n");
					break;
			}
		}
	return 0;
}

static void print_edma_info(edma_t edma)
{

	int i;
	edmacc_t *cc = NULL;
	/* read all the configuration not exported by driver info & device info */
	slog("compiled on %s @ %s ", __DATE__, __TIME__);
	slog("%d channel controllers", edma.cc_nos);
	/* some hardware details of EDMA */
	for(i = 0; i < edma.cc_nos; i++) {
			cc = &edma.cc[i];
			slog("Channel Controller%d revision = %x", i, in32(cc->vbase + AM335X_EDMA_PID));
			slog("Channel Controller%d Configuration = %x", i, in32(cc->vbase + AM335X_EDMA_CCCFG));
	}
}

/* EDMA library initialization function */
static int edma_init(const char *options)
{
	char *tmp = (char*) options;
	parse_init_opt(tmp, &edma);
	pthread_mutex_lock(&user_mutex);
	if(++user_cnt == FIRST_USER) {					/* do the following only for 1st user of a process */
		if(init_perprocess())
			goto fail0;
		if(init_mmapregs(&edma))
			goto fail1;
    	/**************************/
		pthread_mutex_lock(getprocess_mutex());		/* do only once to use dma in entire system ,ie sync aross all process */
		/* ok this is first process */
		if(incr_process_cnt() == 1) {
			slog("this is the first process");
			init_once(&edma);
			/* create dma channel resources for the entire system */
			if(create_edma_rsrc(&edma) == -1) {
	        	slog("error resource creation failed\n");
	        	dec_process_cnt();
				pthread_mutex_unlock(getprocess_mutex());
				goto fail2;
	    	}
	    }
		pthread_mutex_unlock(getprocess_mutex());
	   	/**************************/
	}
	pthread_mutex_unlock(&user_mutex);
	print_edma_info(edma);
	return 0;

fail2:
		fini_mmapregs(&edma);
fail1:
		fini_perprocess();
fail0:
	--user_cnt;
	pthread_mutex_unlock(&user_mutex);
    return -1;
}

/* Channel arrangement for AM335x
 *  	CHANNEL CONTROLLER 0 EDMA CHANNELS => 0 - 63
 *		CHANNEL CONTROLLER 0 QDMA CHANNELS => 64 - 71
 */
static int edma_channel_info(unsigned int channel, dma_channel_info_t *channelinfo)
{
	if(channel >= max_ch(edma))
		return -1;

	channelinfo->max_xfer_size = (unsigned long)AM335X_MAX_XFER_SIZ; /* ACNT * BCNT */
	channelinfo->max_src_segments = AM335X_MAX_SRC_SEG; /* BCNT */
	channelinfo->max_dst_segments = AM335X_MAX_DST_SEG; /* BCNT */
	/* no:of dst & src fragments is not constant, depends upon the no:of free params */
	channelinfo->max_src_fragments = AM335X_MAX_SRC_FRAG;
	channelinfo->max_dst_fragments = AM335X_MAX_DST_FRAG;
	channelinfo->xfer_unit_sizes = AM335X_XFER_UNIT_SIZ; /* DBS */;
	channelinfo->mem_upper_limit = AM335X_MEM_UPPER_LIM;
	channelinfo->mem_lower_limit = AM335X_MEM_LOWER_LIM;
	channelinfo->mem_nocross_boundary = AM335X_MEM_NOCROSS;
	channelinfo->caps = DMA_CAP_REPEAT| DMA_CAP_SRC_SEGMENTED | DMA_CAP_SRC_SCATTER_GATHER | DMA_CAP_SRC_SEGMENTED |
						DMA_CAP_SRC_SCATTER_GATHER | DMA_CAP_SRC_INCREMENT | DMA_CAP_SRC_DECREMENT |
						DMA_CAP_SRC_INCREMENT | DMA_CAP_SRC_NO_INCREMENT | DMA_CAP_SRC_UNALIGNED |
						DMA_CAP_DST_SEGMENTED | DMA_CAP_DST_INCREMENT | DMA_CAP_DST_DECREMENT |
						DMA_CAP_DST_NO_INCREMENT | DMA_CAP_DST_UNALIGNED | DMA_CAP_IO_TO_MEMORY |
						DMA_CAP_IO_TO_IO | DMA_CAP_IO_TO_DEVICE | DMA_CAP_MEMORY_TO_MEMORY |
						DMA_CAP_MEMORY_TO_IO | DMA_CAP_MEMORY_TO_DEVICE | DMA_CAP_DEVICE_TO_MEMORY |
						DMA_CAP_DEVICE_TO_IO | DMA_CAP_DEVICE_TO_DEVICE;
	/* only edma channel are mapped to ipr bits */
	if(is_edma_ch(edma, channel))
		channelinfo->caps |= DMA_CAP_EVENT_ON_COMPLETE | DMA_CAP_EVENT_PER_SEGMENT;
	return 0;
}

static int edma_driver_info(dma_driver_info_t *driverinfo)
{
	driverinfo->dma_version_major = DMALIB_VERSION_MAJOR;
	driverinfo->dma_version_minor = DMALIB_VERSION_MINOR;
	driverinfo->dma_rev = DMALIB_REVISION;
	driverinfo->lib_rev = AM335X_LIB_REVISION;
	driverinfo->description = AM335X_DMA_NAME;
	driverinfo->num_channels = AM335X_MAX_NUM_CHANNELS;
	driverinfo->max_priority = AM335X_MAXPRIO;
	return 0;
}

/*
 * Deallocates the given dma memory buffer
 */
static void edma_free_buffer(void *handle, dma_addr_t *addr)
{
	munmap(addr->vaddr, addr->len);
	addr->vaddr = NULL;
	addr->paddr = NULL;
}

/*
 * Allocates a physically contigious dma memory buffer
 */
static int edma_alloc_buffer(void *handle, dma_addr_t *addr, unsigned size, unsigned flags)
{
	if(size <= 0)
		return -1;
	addr->len = size;
	addr->vaddr = mmap(0, addr->len, PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_PRIVATE | MAP_ANON | MAP_PHYS, NOFD, 0);
	if(addr->vaddr == MAP_FAILED)
		goto fail0;
	addr->paddr = mphys(addr->vaddr);
	if(addr->paddr == -1)
		goto fail1;
	return 0;

fail1:
	edma_free_buffer(handle, addr);
fail0:
	return -1;
}

/*
 * command line options for channel_attach
 */
enum channel_option_index {EDMA, QDMA, CHANNEL_CONTROLLER, ASYNC, ABSYNC, MANUAL_TRIGGER, DEVICE_TRIGGER, AUTO_TRIGGER};
static char *const channel_options[] = {
	[EDMA]					= "edma",					/* edma channel is needed & given as channel argument */
	[QDMA]					= "qdma",					/* qdma channel is needed & given as channel argument */
	[CHANNEL_CONTROLLER]	= "channel_controller",		/* which channel controller to use,eg channel_controller=0 */
	[ASYNC]					= "async",					/* A synchronized transfer (1 dimension) */
	[ABSYNC]				= "absync",					/* AB synchronized transfer (2 dimension) */
	[MANUAL_TRIGGER]		= "manual_trigger",			/* manually triggered */
	[DEVICE_TRIGGER]		= "device_trigger",			/* device triggered */
	[AUTO_TRIGGER]			= "auto_trigger",			/* auto triggered (only for qdma)*/
	NULL
};

/*
 * parse command line options given to channel_attach to find out
 * 		channel to use (EDMA/QDMA)
 *		channel controller to use
 */

static int parse_channel_opt(char *options, channel_t *chan)
{
	int opt, tmp;
	char *valuep;
	edma_t *edma = get_edma();
	if(options == NULL)
		return -1;
	while((opt = getsubopt(&options, channel_options,&valuep)) != -1) {
		switch(opt) {
			case EDMA:
				chan->chfl = chan->chfl & CH_CLR | CH_EDMA;
				break;
			case QDMA:
				chan->chfl = chan->chfl & CH_CLR | CH_QDMA;
				slog("setting channel to qdma");
				break;
			case CHANNEL_CONTROLLER:
				tmp = strtoll(valuep, NULL, 0);
				if(tmp < 0 || tmp > edma->cc_nos - 1) {
					chan->ccno = 0;
					slog("invalid channel controller number setting to channel controller number 0");
				} else
					slog("setting channel controller to %d", tmp);
				chan->ccno = tmp;
				break;
			case ASYNC:
				chan->chfl = chan->chfl & CH_SYNC_CLR | CH_ASYNC;
				break;
			case ABSYNC:
				chan->chfl = chan->chfl & CH_SYNC_CLR | CH_ABSYNC;
				break;
			case MANUAL_TRIGGER:
				chan->chfl = chan->chfl & CH_TRIG_CLR | CH_TRIG_MANUAL;
				break;
			case DEVICE_TRIGGER:
				chan->chfl = chan->chfl & CH_TRIG_CLR | CH_TRIG_DEVICE;
				break;
			case AUTO_TRIGGER:
				chan->chfl = chan->chfl & CH_TRIG_CLR | CH_TRIG_AUTO;
				break;
			default:
				slog("unknown option given to channel_attach\n");
				break;
		}
	}
	return 0;
}

static void set_channel(channel_t *ch, int channel)
{
	edma_t *edma = get_edma();
	if(edma->cc_nos == 1){	/* l137 */
		ch->ccno = 0;
		if(channel < edma->cc[0].edma_chnos)
			ch->chno = channel;
		else
			ch->chno = channel - edma->cc[0].edma_chnos;
	}
	else if(edma->cc_nos == 2){	/* l138 */
		if(channel < edma->cc[0].edma_chnos + edma->cc[0].qdma_chnos) {	/* in channel controller 1 */
			ch->ccno = 0;
			if(channel < edma->cc[0].edma_chnos)
				ch->chno = channel;
			else
				ch->chno = channel - edma->cc[0].edma_chnos;
		}
		else{	/* channel controller 2 */
			ch->ccno = 1;
			channel -= edma->cc[0].edma_chnos + edma->cc[0].qdma_chnos;
			if(channel < edma->cc[1].edma_chnos)
				ch->chno = channel;
			else
				ch->chno = channel - edma->cc[1].edma_chnos;
		}
	}
}

/*
 * Channel number can be as given below
 *  	CHANNEL CONTROLLER 0 EDMA CHANNELS => 0 - 63
 *		CHANNEL CONTROLLER 0 QDMA CHANNELS => 64 - 71
 * Or you can give a channel number from 0-31 after setting
 * channel_controller & edma/qdma option to select the desired channel
 * CHANNEL NUMBER GIVEN TAKE THE HIGHER PRIORITY THAN THE COMMAND LINE OPTION
 * (INFORMATION REGARDING A CHANNEL IS COLLECTED USING CHANNEL NUMBER SO
 * IT IS GIVEN HIGHER PRIORITY THAN COMMAND LINE OPTION)
 * For l137 edma channels are from 0 -31 & qdma from 32-39
 */
static void* edma_channel_attach(const char *options, const struct sigevent *event, unsigned *channel,
			    int priority, dma_attach_flags flags)
{
	if(user_cnt <= 0) {
		slog("calling channel_attach before calling init");
		goto fail0;
	}
	channel_t *ch;
	edma_t *edma = get_edma();
	edmacc_t *cc;
	char rsrc_ch_name[16] = {0}, *tmp_opt = (char*)options;
	rsrc_request_t req;

	if(flags & DMA_ATTACH_EVENT_ON_COMPLETE && flags & DMA_ATTACH_EVENT_PER_SEGMENT)
		goto fail0;

	ch = calloc(1, sizeof(*ch));
	if(ch == NULL)
		goto fail0;
	/* Default configuration */
	ch->chfl |= CH_EDMA;
	ch->ccno = 0;
	set_channel(ch, *channel);
	slog("options = %s", options);
	if(parse_channel_opt(tmp_opt, ch) == -1)							/* typecasted to silence warnings */
		slog("no options given taking the default");
	if(is_qdma(ch) && (flags & DMA_ATTACH_EVENT_ON_COMPLETE || flags & DMA_ATTACH_EVENT_PER_SEGMENT))
		goto fail1;
	cc = &edma->cc[ch->ccno];
	ch->cc = cc;
	sprintf(rsrc_ch_name, "%s%d", is_edma(ch) ? RSRC_EDMA : RSRC_QDMA, ch->ccno);
	slog("requesting for %s channel %d", rsrc_ch_name, *channel);
	req.length = 1;
	req.end = req.start = *channel;
	req.flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_RANGE;
	req.name = rsrc_ch_name;
	if(flags & DMA_ATTACH_ANY_CHANNEL) {
		req.end = (is_edma(ch)? cc->edma_chnos : cc->qdma_chnos) - 1;
		req.start = 0;
	}
	if(rsrcdbmgr_attach(&req, 1) == -1)
		goto fail1;
	ch->flags = flags;
	*channel = ch->chno = req.start;
	ch->prio = priority;
	ch_arr[ch->chno] = ch;
	uint32_t tmp = in32(cc->vbase + AM335X_EDMA_DRAE(0));
	slog("attached channel %d", ch->chno);

	/* setup DRAE0 for the channel */
	if(is_edma(ch)) {
		tmp = in32(cc->vbase + AM335X_EDMA_DRAE(0));
		out32(cc->vbase + AM335X_EDMA_DRAE(0), tmp | setbit(ch->chno));
	} else if(is_qdma(ch)) {
		tmp = in32(cc->vbase + AM335X_EDMA_QRAE(0));
		out32(cc->vbase +  AM335X_EDMA_QRAE(0), tmp | setbit(ch->chno));
	}

	/* setup events to be delivered on completion */
	if(flags & (DMA_ATTACH_EVENT_ON_COMPLETE | DMA_ATTACH_EVENT_PER_SEGMENT))
		ch->event = (struct sigevent*)event;

	/* priority is set by assigning channel to a higher priority TC 
	 * Channel controller with out more than 1 Transfer controller does not
	 * set priority
	 * 0 is the highest priority
	 * all channels are default at higher priority
	 */
	if((priority || !(flags & DMA_ATTACH_PRIORITY_HIGHEST)) && cc->tcno > 1) {
		slog("setting channel%d of cc%dto lower priority, cc->tcno = %x", ch->chno, ch->ccno, cc->tcno);
		if(is_edma(ch)) {
			tmp = in32(cc->vbase +  AM335X_EDMA_DMAQNUM(ch->chno >> 3));
			out32(cc->vbase +  AM335X_EDMA_DMAQNUM(ch->chno >> 3), tmp | setbit((ch->chno & 7) << 2));
		} else if(is_qdma(ch)) {
			tmp = in32(cc->vbase + AM335X_EDMA_QDMAQNUM);
			out32(cc->vbase +  AM335X_EDMA_QDMAQNUM, tmp | setbit((ch->chno << 2)));
		}
	}
	ch->chfl = ch->chfl & CHS_CLR | CHS_ATTACH;
	return ch;

fail1:
	free(ch);
fail0:
	return NULL;
}


static int cleanup_params(channel_t *ch)
{
	edmacc_t *cc = ch->cc;
	rsrc_request_t req[2] = {{0}};
	char rsrc_chparam[16] = {0};
	uint32_t tmp;
	param_t *param = (param_t *) (cc->vbase + (is_edma(ch) ? am335x_edma_param(ch->chno) : am335x_edma_param(ch->chno + cc->edma_chnos)));
		/* fill up param with 0x0 , as we dont need it any more */
	memset(param, 0x0, sizeof(*param));
	if(ch->sg_param)
		memset(ch->sg_param, 0, ch->sg_param_nos * sizeof(*ch->sg_param));

	param->link_bcntreload = PARAM_LINK_NULL;
	for(tmp = 0; tmp < ch->sg_param_nos; tmp++)
		ch->sg_param[tmp].link_bcntreload = PARAM_LINK_NULL;					/* no linking */
	sprintf(rsrc_chparam, "%s%d", RSRC_PARAM, ch->ccno);

	tmp = 1;
	req[0].length = 1;
	req[0].end = req[0].start = (is_edma(ch) ? ch->chno : (ch->chno + cc->edma_chnos));
	req[0].flags = RSRCDBMGR_FLAG_NAME;
	req[0].name = rsrc_chparam;

	if(ch->sg_param) {
		req[1].length = ch->sg_param_nos;
		req[1].start = ch->sg_param_start;
		req[1].end = ch->sg_param_start + ch->sg_param_nos;
		req[1].flags = RSRCDBMGR_FLAG_NAME;
		req[1].name = rsrc_chparam;
		tmp = 2;
	}
	if(rsrcdbmgr_detach(req, tmp) < 0) {
		slog("rsrcdbmgr_detach %s", strerror(errno));
		slog("%s  from %lld to %lld, tmp = %d", req[0].name, req[0].start, req[0].end, tmp);
		return -1;
	}
	ch->sg_param = NULL;
	ch->sg_param_nos = 0;
	ch->sg_param_start = 0;
	return 0;	
}

/*
 *	combination of dma transfer this supports
 *	SRC					-			DST
 *	segmented			-			contiguous
 *	contigious			-			segmented
 *	scatter-gather		-			contiguous
 *	contiguous			-			scatter-gather
 *	scatter-gather		-			scatter-gather	(dst frag no == src frag no)
 */
static int	edma_setup_xfer(void *handle, const dma_transfer_t *tinfo)
{
	channel_t *ch = (channel_t*) handle;
	if(ch == NULL || !(ch->chfl & (CHS_ATTACH | CHS_XFERCOMPLETE | CHS_XFERABORT))) {
		slog("setup xfer in not called at the proper time chfl = %x", ch->chfl);
		goto fail0;
	}
	edmacc_t *cc = ch->cc;
	param_t *param;
	uint16_t acnt = tinfo->xfer_unit_size, bcnt = 0, srcbidx = 0, dstbidx = 0;
	uint32_t tmp, i, opt = 0, paramno = 0;
	uintptr_t srcaddr = 0, dstaddr = 0;	
	char chparam[16] = {0};
	sprintf(chparam, "%s%d", RSRC_PARAM, ch->ccno);
	rsrc_request_t req[2] = {{0}};

	/* if param for channel has been setup previously clean it up */
	if(!(ch->chfl & CHS_ATTACH)) {
		cleanup_params(ch);
	}
	/* check out error combination of flags */
	if( (!(tinfo->src_flags & DMA_ADDR_FLAG_SCATTER_GATHER) && tinfo->src_fragments > 1) ||
		(!(tinfo->dst_flags & DMA_ADDR_FLAG_SCATTER_GATHER) && tinfo->dst_fragments > 1) ) {
		slog("only for scatter-gather the fragments can be greater than one\n");
		goto fail0;
	}
	if( ((tinfo->src_flags & DMA_ADDR_FLAG_SCATTER_GATHER) && 
		(tinfo->src_flags & (DMA_ADDR_FLAG_DECREMENT | DMA_ADDR_FLAG_NO_INCREMENT | DMA_ADDR_FLAG_SEGMENTED ))) ||
		((tinfo->dst_flags & DMA_ADDR_FLAG_SCATTER_GATHER) &&
		(tinfo->dst_flags & (DMA_ADDR_FLAG_DECREMENT | DMA_ADDR_FLAG_NO_INCREMENT | DMA_ADDR_FLAG_SEGMENTED ))) ) {
		 slog("cannot specify flag_decrement & flag_no_increment with scatter-gather transaction\n");
		 goto fail0;
	}
	if(tinfo->src_flags & DMA_ADDR_FLAG_SEGMENTED && tinfo->dst_flags & DMA_ADDR_FLAG_SEGMENTED) {
		slog("source & destination cannot be segmented\n");
		goto fail0;
	}
	if( (tinfo->src_flags & DMA_ADDR_FLAG_NO_INCREMENT && tinfo->src_flags & DMA_ADDR_FLAG_DECREMENT) ||
		(tinfo->dst_flags & DMA_ADDR_FLAG_NO_INCREMENT && tinfo->dst_flags & DMA_ADDR_FLAG_DECREMENT) ) {
		slog("no-increment & addr-decrement flags are mutually exclusive\n");
		goto fail0;
	}
	if( (tinfo->src_flags & DMA_ADDR_FLAG_SEGMENTED && !tinfo->src_fragments) ||
		(tinfo->dst_flags & DMA_ADDR_FLAG_SEGMENTED && !tinfo->dst_fragments) ) {
		slog("segmented buffer require no:of segments to be passed\n");
		goto fail0;
	}

	if(tinfo->src_flags & DMA_ADDR_FLAG_SCATTER_GATHER && tinfo->dst_flags & DMA_ADDR_FLAG_SCATTER_GATHER) {
		slog("scatter-gather for source & destination simultaneously not suppported\n");
		goto fail0;
	}

	if( (tinfo->src_flags & DMA_ADDR_FLAG_SCATTER_GATHER || tinfo->dst_flags & DMA_ADDR_FLAG_SCATTER_GATHER) &&
		(tinfo->src_fragments > AM335X_MAX_SRC_FRAG || tinfo->dst_fragments > AM335X_MAX_DST_FRAG) ) {
		slog("scatter-gather fragment exceeded the available limit\n");
		goto fail0;
	}

	/* these are for normal operation */
	srcaddr = (uint32_t)tinfo->src_addrs->paddr;
	dstaddr = (uint32_t)tinfo->dst_addrs->paddr;
	acnt = tinfo->xfer_unit_size;
	bcnt = tinfo->xfer_bytes / tinfo->xfer_unit_size;
	paramno = (is_edma(ch) ? ch->chno : (ch->chno + cc->edma_chnos));

	req[0].end = req[0].start = paramno;
	req[0].length = 1;
	req[0].name = chparam;
	req[0].flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_RANGE;
	if(rsrcdbmgr_attach(req, 1) != EOK) {
		perror("rsrcdbmgr_attach");
		goto fail0;
	}
	paramno = req[0].start;
	param = (param_t*) ((void*)cc->vbase + am335x_edma_param(paramno));
	/* setup for segmented txr */
	if(tinfo->src_flags & DMA_ADDR_FLAG_SEGMENTED) {
		ch->chfl |= CH_SEG;
		bcnt = tinfo->src_fragments;
		acnt = tinfo->xfer_bytes / tinfo->src_fragments;
	} else if(tinfo->dst_flags & DMA_ADDR_FLAG_SEGMENTED) {
		ch->chfl |= CH_SEG;
		bcnt = tinfo->dst_fragments;
		acnt = tinfo->xfer_bytes / tinfo->dst_fragments;
	}

	/* SRC BIDX SETTINGS */
	if(tinfo->src_flags & DMA_ADDR_FLAG_NO_INCREMENT)	/* can a buffer be segmented & addr no increment ? , think no*/
		srcbidx = 0;
	else if(tinfo->src_flags & DMA_ADDR_FLAG_DECREMENT)
		srcbidx = -acnt;
	else
		srcbidx = acnt;

	/* DST BIDX SETTINGS */
	if(tinfo->dst_flags & DMA_ADDR_FLAG_NO_INCREMENT)	/* can a buffer be segmented & addr no increment ? , think no */
		dstbidx = 0;
	else if(tinfo->dst_flags & DMA_ADDR_FLAG_DECREMENT)
		dstbidx = -acnt;
	else
		dstbidx = acnt;

	/* OPT SETTINGS */
	if(ch->flags & DMA_ATTACH_EVENT_ON_COMPLETE)
		opt = PARAM_OPT_TCINTEN;
	if(ch->flags & DMA_ATTACH_EVENT_PER_SEGMENT)
		opt |= PARAM_OPT_ITCINTEN;
	if(is_edma(ch))
		opt |= PARAM_OPT_TCC(ch->chno);
	else if(is_qdma(ch)) {
		opt |= PARAM_OPT_TCC(qdma_ipr_no);
		tmp = in32(cc->vbase + AM335X_EDMA_QCHMAP(ch->chno));	/* all edma are mapped to 1 ipr bit */
		out32(cc->vbase + AM335X_EDMA_QCHMAP(ch->chno),
							tmp | AM335X_EDMA_QCHMAP_TRWORD_CCNT);	/* setup the trigger zone */
	}

	/* Peripheral devices trigger event itself so ASYNC is enough
	 * DMA_ADDR_FLAG_NO_INCREMENT IS USUALLY USED FOR THAT
	 * by default we use ABSYNC
	 */
	opt |= PARAM_OPT_SYNCDIM(SYNCDIM_AB_SYNC);
	if(tinfo->dst_flags & DMA_ADDR_FLAG_NO_INCREMENT || tinfo->src_flags & DMA_ADDR_FLAG_NO_INCREMENT || ch->chfl & CH_ASYNC)
		opt &= ~PARAM_OPT_SYNCDIM(SYNCDIM_AB_SYNC);

	/* fill up all param values computed earlier */
	param->opt = opt;
	param->srcaddr = srcaddr;
	param->a_b_cnt = PARAM_ACNT(acnt) | PARAM_BCNT(bcnt);
	param->dstaddr = dstaddr;
	param->src_dst_bidx = PARAM_SRC_BIDX(srcbidx) | PARAM_DST_BIDX(dstbidx);
	param->src_dst_cidx = PARAM_DST_CIDX(1) | PARAM_SRC_CIDX(1);
	param->link_bcntreload = PARAM_LINK_NULL;
	param->ccnt = 1;

	/* scatter-gather transfer */

	if(tinfo->src_flags & DMA_ADDR_FLAG_SCATTER_GATHER || tinfo->dst_flags & DMA_ADDR_FLAG_SCATTER_GATHER) {
		req[1].start = cc->edma_chnos + cc->qdma_chnos - 1;
		req[1].end = AM335X_MAX_PARAM_NO;
		req[1].name = chparam;
		req[1].flags = RSRCDBMGR_FLAG_NAME  | RSRCDBMGR_FLAG_RANGE;

		if(tinfo->src_flags & DMA_ADDR_FLAG_SCATTER_GATHER) {
			req[1].length = tinfo->src_fragments - 1;
		} else if(tinfo->dst_flags & DMA_ADDR_FLAG_SCATTER_GATHER) {
			req[1].length = tinfo->dst_fragments - 1;
		}
		if(req[1].length && (rsrcdbmgr_attach(&req[1], 1) != EOK)) {
			slog("failure in getting %lld resource for scatter-gather : %s\n", req[0].length, strerror(errno));
			goto fail0;
		}
		ch->sg_param_nos = req[1].length;
		ch->sg_param_start = req[1].start;
		ch->sg_param = (param_t*) ((void*)cc->vbase + am335x_edma_param(req[1].start));

		/* setup first param */
		param->srcaddr = tinfo->src_addrs->paddr;
		param->dstaddr = tinfo->dst_addrs->paddr;
		tmp = (tinfo->src_flags & DMA_ADDR_FLAG_SCATTER_GATHER) ? tinfo->src_addrs[0].len : tinfo->dst_addrs[0].len;
		bcnt = tmp / acnt;
		param->a_b_cnt = PARAM_ACNT(acnt) | PARAM_BCNT(bcnt);
		param->link_bcntreload = req[1].length ? PARAM_LINK(am335x_edma_param(req[1].start)) : PARAM_LINK_NULL;
		param_t *tmp_param = NULL;

		/* setup other params */
		for(i = 0; i < req[1].length && req[1].length; i++ ) {
			tmp_param = &ch->sg_param[i];
			memcpy(tmp_param, param, sizeof(*param));
			if(tinfo->src_flags & DMA_ADDR_FLAG_SCATTER_GATHER) {
				tmp_param->srcaddr = tinfo->src_addrs[i + 1].paddr;
				tmp_param->dstaddr = tinfo->dst_addrs->paddr + tmp;
				tmp += tinfo->src_addrs[i + 1].len;
			} else {
				tmp_param->srcaddr = tinfo->src_addrs->paddr + tmp;
				tmp_param->dstaddr = tinfo->dst_addrs[i + 1].paddr;
				tmp += tinfo->dst_addrs[i + 1].len;
			}
			/* for the last param make ith a null link */
			tmp_param->link_bcntreload = (i == req[1].length - 1) ? PARAM_LINK_NULL : PARAM_LINK(am335x_edma_param(req[1].start + i + 1));
		}
	}

	/* repeated transfer can be setup using self-linked transfer */
	if(tinfo->mode_flags & DMA_MODE_FLAG_REPEAT) {
		memset(req, 0, 1);
		req[0].start = cc->edma_chnos + cc->qdma_chnos - 1;
		req[0].end = AM335X_MAX_PARAM_NO;
		req[0].length = 1;
		req[0].name = chparam;
		req[0].flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_RANGE;
		if(rsrcdbmgr_attach(req, 1)) {
			slog("rsrcdbmgr_attach : %s\n", strerror(errno));
			goto fail1;
		}
		ch->sg_param = (param_t*)((void*)cc->vbase + am335x_edma_param(req[1].start));
		ch->sg_param_nos = 1;
		ch->sg_param_start = req[0].start;
		memcpy(ch->sg_param, param, sizeof(*param));
		ch->sg_param->link_bcntreload |= am335x_edma_param(req[1].start);	/* link to itsef */
	}
	if(edma_init_irq(ch))
		goto fail2;
	ch->chfl = ch->chfl & CHS_CLR | CHS_XFERSETUP;
	return 0;
fail2:
	cleanup_params(ch);
fail1:
	rsrcdbmgr_detach(&req[1], 1);
fail0:
	slog("function failure: ");
	return -1; 
}

static void edma_channel_release(void *handle)
{
	if(handle == NULL) {
		slog("function called with null handle");
		return;
	}
	uint32_t tmp, chno;
	channel_t *ch = (channel_t*) handle;
	edmacc_t *cc = ch->cc;
	rsrc_request_t req = {0};
	char rsrc_chname[16] = {0};
	chno = ch->chno;
	edma_fini_irq(ch);
	if(is_edma(ch))	{
		//out32(cc->vbase + AM335X_EDMA_IECR, setbit(ch->chno));				/* disable interrupt */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_ECR, setbit(ch->chno));					/* disable event */
		out32(cc->vbase + AM335X_EDMA_EECR, setbit(ch->chno));					/* disable event */
		tmp = in32(cc->vbase +  AM335X_EDMA_DMAQNUM(ch->chno >> 3));
		out32(cc->vbase +  AM335X_EDMA_DMAQNUM(ch->chno >> 3), tmp & ~setbit((ch->chno & 7) << 2));	/* reset to use TC0 (high prio) */
		tmp = in32(cc->vbase + AM335X_EDMA_DRAE(0));
		out32(cc->vbase + AM335X_EDMA_DRAE(0), tmp & ~setbit(ch->chno));		/* disable access to shadow region 0 */
	} else if(is_qdma(ch)) {
		/* we dont disable qdma interrupt, for that is shared between all qdma channels */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_QEECR, setbit(ch->chno));				/* disable event */
		tmp = in32(cc->vbase + AM335X_EDMA_QDMAQNUM);
		out32(cc->vbase +  AM335X_EDMA_DMAQNUM(ch->chno >> 3), tmp & ~setbit(ch->chno<< 2));	/* reset to use TC0 (high prio) */
		tmp = in32(cc->vbase + AM335X_EDMA_QRAE(0));
		out32(cc->vbase +  AM335X_EDMA_QRAE(0), tmp & ~setbit(ch->chno));		/* disable access to shadow region 0 */
		chno += cc->edma_chnos;
	}

	ch->event = NULL;	/* clean up irq event */
	cleanup_params(ch);
	/* give back the channel dbsrcmgr */
	sprintf(rsrc_chname, "%s%d", is_edma(ch) ? RSRC_EDMA : RSRC_QDMA, ch->ccno);

	req.length = 1;
	req.start = req.end = ch->chno;
	req.flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_RANGE;
	req.name = rsrc_chname;

	/* debuging output */
	slog("deallocating resource %s from %lld to %lld\n", req.name, req.start, req.end);

	if(rsrcdbmgr_detach(&req, 1) == -1)
		slog(" resource deallocation failed : %s", strerror(errno));
	ch_arr[chno] = NULL;
	free(ch);
	return ;
}

static int edma_xfer_start(void *handle)
{
	channel_t *ch = handle;
	if(ch == NULL || !(ch->chfl & (CHS_XFERSETUP | CHS_XFERCOMPLETE | CHS_XFERABORT))) {
		slog("xfer_start not called at the proper time");
		return -1;
	}
	edmacc_t *cc = ch->cc;
	param_t *param;

	if(is_edma(ch)) {
		/* disabling event & interrupt before cleaning up previously enabled interrupts */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_EECR, setbit(ch->chno));
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_IECR, setbit(ch->chno));

		/* clear up all pending events from previous events */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_ECR, setbit(ch->chno));
		out32(cc->vbase + 0x1000 + AM335X_EDMA_SECR, setbit(ch->chno));
		out32(cc->vbase + AM335X_EDMA_EMCR, setbit(ch->chno));
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_ICR, setbit(ch->chno));
		
		/* enable interrupts */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_IESR, setbit(ch->chno));
		/* events must be enbled always */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_EESR, setbit(ch->chno));
		/* if channel is not cascaded, no device will give sync event 
		 * so set this as manually triggered & give a trigger event
		 */
		if(!(ch->flags & DMA_ATTACH_CASCADE)) {
			ch->chfl = ch->chfl & CH_TRIG_CLR | CH_TRIG_MANUAL;
			out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_ESR, setbit(ch->chno));
		}
	} else if(is_qdma(ch)) {
		/* disabling event & interrupt before cleaning up previously enabled interrupts */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_QEECR, setbit(ch->chno));
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_IECR, qdma_ipr_no);

		/* clear up all pending events from previous events */
		out32(cc->vbase + AM335X_EDMA_QEMCR, setbit(ch->chno));
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_QSECR, setbit(ch->chno));
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_ICR, qdma_ipr_no);
		
		/* enable interrupts & events */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_IESR, qdma_ipr_no);
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_QEESR, setbit(ch->chno));
		
		/* trigger the qdma channel txr by writing to the trigger field
		 * we set up a constant trigger field here ,ie ccnt
		 */
		param = (param_t *) (cc->vbase + am335x_edma_param(ch->chno + cc->edma_chnos));
		param->ccnt = 1;
	}
	ch->chfl = ch->chfl & CHS_CLR | CHS_XFERSTART; // check here
	return 0;
}

static int edma_xfer_abort(void *handle)
{
	channel_t *ch = handle;
	if(ch == NULL || !(ch->chfl & CHS_XFERSTART))
		return -1;
	edmacc_t *cc = ch->cc;
	if(is_edma(ch)) {
		/* disabling event & interrupt */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_EECR, setbit(ch->chno));
		InterruptMask(edma_irqno(ch), ch->irq_id);
	} else if(is_qdma(ch)) {
		/* disabling event & interrupt */
		out32(cc->vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_QEECR, setbit(ch->chno));
	}
	ch->chfl = ch->chfl & CHS_CLR | CHS_XFERABORT;
	return 0;
}

/* this does not return the exact amount of data transferred
 * Even if we read param , value will keep on changing before we
 * return the value
 * so if transfer is not complete we return 1 else return 0
 */
static unsigned int edma_bytes_left(void *handle)
{
	if(handle == NULL)
		return 1;
	channel_t *ch = handle;
	param_t *param = get_param(ch);
	return (param->a_b_cnt || param->ccnt);
}

static int edma_xfer_complete(void *handle)
{
	channel_t *ch = handle;
	if(ch == NULL || !(ch->chfl & CHS_XFERSTART)) {
		slog("channel not started chfl ");
		return -1;
	}

	int rval = -1, tmp = edma_bytes_left(handle);
	if(tmp == 0) {
		edma_xfer_abort(handle);
		rval = 0;
		ch->chfl = ch->chfl & CHS_CLR | CHS_XFERCOMPLETE;
	}	
	return rval;
}

static void edma_query_channel(void *handle, dma_channel_query_t *query)
{
	if(handle == NULL)
		return ;
	channel_t *ch = handle;
	query->chan_idx = ch->chno;
	query->irq = ch->cc->irq_no;
}

static void edma_fini(void)
{
	int i;
	pthread_mutex_lock(&user_mutex);
	if(--user_cnt == 0) {	/* last user in the process */
		for(i = 0; i < 64; i++) {
			if(ch_arr[i]) {
				edma_xfer_abort(ch_arr[i]);
				edma_channel_release(ch_arr[i]);
			}
		}
		pthread_mutex_lock(getprocess_mutex());		/* sync aross all process */
		if(dec_process_cnt() == 0) {				/* this is the last process in system using edma */
			slog("this is the last process so deleting all resources process count = %d", get_process_cnt());
			destroy_edma_rsrc(&edma);
			init_once(&edma);
		}
		pthread_mutex_unlock(getprocess_mutex());

		fini_mmapregs(&edma);
		fini_perprocess();
	}
	pthread_mutex_unlock(&user_mutex);
}

int get_dmafuncs(dma_functions_t *func_tbl, int tabsize)
{
    DMA_ADD_FUNC(func_tbl, init, edma_init, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, fini, edma_fini, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, driver_info, edma_driver_info, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, channel_info, edma_channel_info, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, channel_attach, edma_channel_attach, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, channel_release, edma_channel_release, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, alloc_buffer, edma_alloc_buffer, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, free_buffer, edma_free_buffer, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, setup_xfer, edma_setup_xfer, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, xfer_start, edma_xfer_start, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, xfer_abort, edma_xfer_abort, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, xfer_complete, edma_xfer_complete, sizeof(*func_tbl));
    DMA_ADD_FUNC(func_tbl, bytes_left, edma_bytes_left, sizeof(*func_tbl));    
    DMA_ADD_FUNC(func_tbl, query_channel, edma_query_channel, sizeof(*func_tbl));    
	return 0;
}

