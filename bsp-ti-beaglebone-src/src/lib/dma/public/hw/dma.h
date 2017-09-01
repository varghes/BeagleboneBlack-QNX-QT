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


#ifndef _HW_DMA_DRIVER_H_INCLUDED
#define _HW_DMA_DRIVER_H_INCLUDED
#ifndef __PLATFORM_H_INCLUDED
#include <sys/platform.h>
#endif
#ifndef __NEUTRINO_H_INCLUDED
#include <sys/neutrino.h>
#endif

/* Constants for filling in the dma_module_info structure */
#define DMALIB_VERSION_MAJOR			1
#define	DMALIB_VERSION_MINOR			0
#define	DMALIB_REVISION				0

typedef enum {
	DMA_CAP_BURST_MODE =		0x00000001,
	DMA_CAP_REPEAT =		0x00000002,
	DMA_CAP_EVENT_ON_COMPLETE =	0x00000004,
	DMA_CAP_EVENT_PER_SEGMENT =	0x00000008,
	DMA_CAP_SRC_SEGMENTED =		0x00000010,
	DMA_CAP_SRC_SCATTER_GATHER =	0x00000020,
	DMA_CAP_SRC_INCREMENT =		0x00000040,	/* Source address can increment */
	DMA_CAP_SRC_DECREMENT =		0x00000080,	/* Source address can decrement */
	DMA_CAP_SRC_NO_INCREMENT =	0x00000100,	/* Source address remain unchanged */
	DMA_CAP_SRC_UNALIGNED =		0x00000200,	/* Source address need not be aligned on transfer unit size boundarys */
	DMA_CAP_DST_SEGMENTED =		0x00000400,
	DMA_CAP_DST_SCATTER_GATHER =	0x00000800,
	DMA_CAP_DST_INCREMENT =		0x00001000,	/* Dest address can increment */
	DMA_CAP_DST_DECREMENT =		0x00002000,	/* Dest address can decrement */
	DMA_CAP_DST_NO_INCREMENT =	0x00004000,	/* Dest address can remain unchanged */
	DMA_CAP_DST_UNALIGNED =		0x00008000,	/* Dest address need not be aligned on transfer unit size boundarys */
	DMA_CAP_IO_TO_MEMORY =		0x00010000,	/* IO to memory transfers are supported */
	DMA_CAP_IO_TO_IO =		0x00020000,	/* IO to IO transfers are supported */
	DMA_CAP_IO_TO_DEVICE =		0x00040000,	/* IO to device transfers are supported */
	DMA_CAP_MEMORY_TO_MEMORY =	0x00080000,	/* Memory to memory transfers are supported */
	DMA_CAP_MEMORY_TO_IO =		0x00100000,	/* Memory to IO transfers are supported */
	DMA_CAP_MEMORY_TO_DEVICE =	0x00200000,	/* Memory to device transfers are supported */
	DMA_CAP_DEVICE_TO_MEMORY =	0x00400000,	/* Device to memory transfers are supported */
	DMA_CAP_DEVICE_TO_IO =		0x00800000,	/* Device to IO transfers are supported */
	DMA_CAP_DEVICE_TO_DEVICE =	0x01000000	/* Device to device transfers are supported */
} dma_channel_caps;

/* flags to channel_attach */
typedef enum {
	DMA_ATTACH_ANY_CHANNEL =	0x00000001,	/* Attach to any free channel */
	DMA_ATTACH_PRIORITY_STRICT =	0x00000002,	/* "priority" parameter exactly specifies required priority */
	DMA_ATTACH_PRIORITY_ATLEAST =	0x00000004,	/* Priority must be at least as high as "priority" parameter */
	DMA_ATTACH_PRIORITY_HIGHEST =	0x00000008,	/* Want the highest-priority available */
	DMA_ATTACH_EVENT_ON_COMPLETE =	0x00000010,	/* Want an event on transfer completion */
	DMA_ATTACH_EVENT_PER_SEGMENT =	0x00000020,	/* Want an event per fragment transfer completion */
	DMA_ATTACH_CASCADE =		0x00000040,	/* Put channel into cascaded mode */
	DMA_ATTACH_FRIENDLY =		0x00000080	/* Prepared to share DMA resources with other users of the library */
} dma_attach_flags;

typedef enum {
	DMA_MODE_FLAG_REPEAT =		0x00000001,	/* Continuously repeat transfer */
	DMA_MODE_FLAG_BURST =		0x00000002	/* Use burst mode */
} dma_mode_flags;

typedef enum {
	DMA_ADDR_FLAG_IO =		0x00000001,	/* Target I/O space */
	DMA_ADDR_FLAG_MEMORY =		0x00000002,	/* Target memory space */
	DMA_ADDR_FLAG_DEVICE =		0x00000004,	/* Target an external device that can generate DMA requests */
	DMA_ADDR_FLAG_NO_INCREMENT =	0x00000008,	/* Don't increment address */
	DMA_ADDR_FLAG_DECREMENT =	0x00000010,	/* Decrement address */
	DMA_ADDR_FLAG_SEGMENTED =	0x00000020,	/* Buffer is divided into segments */
	DMA_ADDR_FLAG_SCATTER_GATHER =	0x00000040	/* Buffer is scatter-gather */
} dma_xfer_flags;

#define DMA_BUF_FLAG_NOCACHE 0x00000001
#define DMA_BUF_FLAG_SHARED  0x00000002

typedef struct _dma_driver_info {
	_Uint8t		dma_version_major;	/* Major version of DMA lib interface */
	_Uint8t		dma_version_minor;	/* Minor version of DMA lib interface */
	_Uint8t		dma_rev;		/* Rev. of DMA lib interface */
	_Uint8t		lib_rev;		/* Library revision number */
	char		*description;
	_Uint32t	num_channels;
	_Uint32t	max_priority;
	_Uint32t	reserved[16];
} dma_driver_info_t;

typedef struct {
	_Uint32t		max_xfer_size;
	_Uint32t		max_src_fragments;
	_Uint32t		max_dst_fragments;
	_Uint32t		max_src_segments;
	_Uint32t		max_dst_segments;
	_Uint32t		xfer_unit_sizes;
	dma_channel_caps	caps;
	_Uint64t		mem_upper_limit;
	_Uint64t		mem_lower_limit;
	_Uint64t		mem_nocross_boundary;
	_Uint32t		reserved[16];
} dma_channel_info_t;

typedef struct dma_channel_query {
	_Uint32t		chan_idx;
	_Uint32t		irq;
	_Uint32t		irq_he;
	_Uint32t		reserved[15];	
} dma_channel_query_t;

typedef struct {
	void		*vaddr;			/* Virtual address */
	off64_t		paddr;			/* Physical address */
	_Uint32t	len;			/* Length in bytes */
	_Uint32t	reserved;
} dma_addr_t;

typedef struct _dma_transfer {
	dma_addr_t	*src_addrs;
	dma_addr_t	*dst_addrs;

	unsigned	src_fragments;	/* Size of src_addrs array, or number of segments */
	unsigned	dst_fragments;	/* Size of dst_addrs array, or number of segments */

	dma_xfer_flags	src_flags;
	dma_xfer_flags	dst_flags;

	unsigned	xfer_unit_size;	/* Size of each data unit transfer */
	unsigned	xfer_bytes;	/* Total bytes to be transferred */

	dma_mode_flags	mode_flags;

	unsigned	reserved[8];
} dma_transfer_t;

typedef struct _dma_functions {
	int		(*init)(const char *options);
	void		(*fini)(void);
	int		(*driver_info)(dma_driver_info_t *devinfo);
	int		(*channel_info)(unsigned channel, dma_channel_info_t *chinfo);
	void *		(*channel_attach)(const char *options,
			    const struct sigevent *event, unsigned *channel,
			    int priority, dma_attach_flags flags);
	void		(*channel_release)(void *handle);
	int		(*alloc_buffer)(void *handle,
			    dma_addr_t *addr, unsigned size, unsigned flags);
	void		(*free_buffer)(void *handle, dma_addr_t *addr);
	int		(*setup_xfer)(void *handle, const dma_transfer_t *tinfo);
	int		(*xfer_start)(void *handle);
	int		(*xfer_abort)(void *handle);
	int		(*xfer_complete)(void *handle);
	unsigned	(*bytes_left)(void *handle);
	void	(*query_channel)(void *handle, dma_channel_query_t *chinfo);
} dma_functions_t;

/* Macro used by H/W driver when populating dma_functions table */
#define DMA_ADD_FUNC(table, entry, func, limit) \
	if (((size_t)&(((dma_functions_t *)0)->entry)) + \
	    sizeof (void (*)()) <= (limit)) \
		(table)->entry = (func);

typedef int (*get_dmafuncs_t)(dma_functions_t *funcs, int tabsize);

int get_dmafuncs(dma_functions_t *funcs, int tabsize);

#endif /* _HW_DMA_H_INCLUDED */
