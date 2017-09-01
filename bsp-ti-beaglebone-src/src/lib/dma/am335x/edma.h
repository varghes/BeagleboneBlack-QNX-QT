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

#ifndef __EDMA_H__
#define __EDMA_H__
#include <stdio.h>
#include <sys/neutrino.h>
#include <stdint.h>
#include <hw/inout.h>
#include <hw/dma.h>
#include <sys/slog.h>
#include <errno.h>
#include "edma_am335x.h"
#include <arm/am335x.h>

/* EDMA RESOURCE NAMES */
#define RSRC_EDMA	"EDMA"
#define RSRC_QDMA	"QDMA"
#define RSRC_PARAM	"PARAM"

#define MAX_RSRC_PER_CC		3	/* maximum no:of resources that must be allocated per channel controller (edma + qdma + param)*/

#define MnS_BITS(V, S, M)		( ( (V) & ((1UL << M) - 1UL) ) << (S) )	/* mask & shift bits */
#define MONE_BIT(V, S)			MnS_BITS(V, S, 1UL)
#define MTWO_BIT(V, S)			MnS_BITS(V, S, 2UL)
#define MTHREE_BIT(V, S)		MnS_BITS(V, S, 3UL)
#define MFOUR_BIT(V, S)			MnS_BITS(V, S, 4UL)
#define MSIX_BIT(V, S)			MnS_BITS(V, S, 6UL)
#define MNINE_BIT(V, S)			MnS_BITS(V, S, 9UL)

/* bits for opt field in PARAM */
#define PARAM_OPT_SAM(V)		MONE_BIT(V,0)
#define PARAM_OPT_DAM(V)		MONE_BIT(V,1)

#define PARAM_OPT_SYNCDIM(V)	MONE_BIT(V,2)
#define SYNCDIM_A_SYNC			0x0
#define SYNCDIM_AB_SYNC			0x1

#define PARAM_OPT_STATIC(V)		MONE_BIT(V,3)
#define PARAM_OPT_FWID(V)		MTHREE_BIT(V,8)
#define PARAM_OPT_TCCMOD(V)		MONE_BIT(V,11)
#define PARAM_OPT_TCC(V)		MSIX_BIT(V,12)
#define PARAM_OPT_TCINTEN		MONE_BIT(1,20)
#define PARAM_OPT_ITCINTEN		MONE_BIT(1,21)
#define PARAM_OPT_TCCHEN(V)		MONE_BIT(V,22)
#define PARAM_OPT_ITCCHEN(V)	MONE_BIT(V,23)

#define G_BITS(V,S,M)		( ((V) >> (S)) & ( (1 << (M)) - 1) )
#define G_TCC(V)			G_BITS(V, 12, 4)

#define PAENTRY(V)		MNINE_BIT(V, 5)

static inline uint32_t setbit(uint32_t setme)__attribute__((always_inline));
static inline uint32_t setbit(uint32_t setme)
{
	return (1UL << setme);
}

typedef struct {
	int i;
} edmainfo_t;

#define PARAM_LINK(V)		MnS_BITS(V, 0, 16)
#define PARAM_DST_BIDX(V)	MnS_BITS(V, 16, 16)
#define PARAM_SRC_BIDX(V)	MnS_BITS(V, 0, 16)

#define PARAM_DST_CIDX(V)	MnS_BITS(V, 16, 16)
#define PARAM_SRC_CIDX(V)	MnS_BITS(V, 0, 16)

#define PARAM_BCNT(V)	MnS_BITS(V, 16, 16)
#define PARAM_ACNT(V)	MnS_BITS(V, 0, 16)

#define PARAM_LINK_NULL	0xFFFF

typedef struct {
	uint32_t	opt;
	uint32_t	srcaddr;
	uint32_t	a_b_cnt;
	uint32_t	dstaddr;
	uint32_t	src_dst_bidx;
	uint32_t	link_bcntreload;
	uint32_t	src_dst_cidx;
	uint32_t	ccnt;
} param_t;

/* edma channel controller structure 
 * for every edma channel controller there is a structure like this
 */
typedef struct {
	uintptr_t vbase;			/* virtual address of channel controller register base */
	uintptr_t pbase;			/* virtual address of channel controller register base */
	uint32_t irq_no;			/* irq base number of channel controller */
	uint32_t size;				/* size of region from pbase to be mapped */
	uint16_t edma_chnos;		/* no:of edma channels */
	uint16_t qdma_chnos;		/* no:of qdma channels */
	uint8_t tcno;				/* no:of transfer controller for this channel controllers */
} edmacc_t;

/* this contains the details of entire edma
 */
typedef struct {
	edmacc_t *cc;				/* array of channel controller structures */
	uint16_t cc_nos;			/* no:of channel controllers */
} edma_t;

/********** CHFL FIELDS ********/
#define CH_QDMA		setbit(0)
#define CH_EDMA		setbit(1)
#define CH_FRAG		setbit(2)
#define CH_SEG		setbit(3)
#define CH_CLR		(~MnS_BITS(0Xf, 0, 4))

#define CHS_ATTACH			setbit(31)
#define CHS_XFERSETUP		setbit(30)
#define CHS_XFERSTART		setbit(29)
#define CHS_XFERABORT		setbit(28)
#define CHS_XFERCOMPLETE	setbit(27)
#define CHS_CLR				(~MnS_BITS(0X1f, 27, 5))

#define CH_TRIG_MANUAL		setbit(26)
#define CH_TRIG_AUTO		setbit(25)
#define CH_TRIG_DEVICE		setbit(24)
#define CH_TRIG_CLR			(~MnS_BITS(0X7, 24, 3))

#define CH_ASYNC		setbit(23)
#define CH_ABSYNC		setbit(22)
#define CH_SYNC_CLR		(~MnS_BITS(0X3, 22, 2))
/*******************************/

#define DMA_CHANNEL_NAME_LEN	16	/* max len of the buffer that stores the 
									 * name to be passed to rsrcdbmgr_attach
									 */
/* Channel structure
 * for every used channel there is a structure like this
 * all channel related are here
 */
typedef struct {
	edmacc_t *cc;				/* pointer to the channel controller structure to which this channel belongs */
	struct sigevent *event;		/* event to be delivered when an interrupt happens */
	uint16_t ccno;				/* channel controller no, to which channel controller this belong */
	dma_attach_flags flags;		/* flags given during attaching time */
	uint16_t chno;				/* channel number */
	uint16_t prio;				/* priority of the channel */
	uint32_t chfl;				/* */
	/* used only for scatter-gather & linked operations */
	param_t *sg_param;			/* PARAM start address */
	uint16_t sg_param_nos;		/* no:of additional param s allocated */
	uint16_t sg_param_start;	/* PARAM no from where additional param is allocated */
	uint32_t irq_id;			/* interrupt id of irq_no */
} channel_t;

static inline int is_edma(channel_t *chan) __attribute__((always_inline));
static inline int is_edma(channel_t *chan)
{
	return(chan->chfl & CH_EDMA);
}

static inline int is_qdma(channel_t *chan) __attribute__((always_inline));
static inline int is_qdma(channel_t *chan)
{
	return(chan->chfl & CH_QDMA);
}

static inline int edma_irqno(channel_t *ch)__attribute__((always_inline));
int edma_irqno(channel_t *ch)
{
	return(ch->cc->irq_no + ch->chno);
}

static inline int __attribute__((always_inline)) am335x_edma_param(int paramno)
{
	return (AM335X_EDMA_PARAM_BASE + ((paramno) * 32));
}

static inline param_t* __attribute__((always_inline)) get_param(channel_t *ch)
{
	int paramno = (is_edma(ch) ? ch->chno : (ch->chno + ch->cc->edma_chnos));
	return ((void*)ch->cc->vbase + am335x_edma_param(paramno));
}

/*
 * DEBUGGING MACROS
 * To enable debugging macros define EDMA_DEBUG
 */


#define DBG_MARKER()
#define DBG_PERR(X)
#define dlog(...)

#define slog(...)	slogf(_SLOG_SETCODE(_SLOG_DEBUG1, 0), _SLOG_DEBUG1, __VA_ARGS__)

extern paddr_t mphys(void*);

#endif /* #ifndef __EDMA_H__ */
