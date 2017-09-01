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

#include <stdio.h>
#include "edma.h"
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/rsrcdbmgr.h>
#include <sys/rsrcdbmsg.h>

/* function called only once. (in the first use of library)
 * & must be sync between process (call this function after getting process_lock)
 */

void init_once(edma_t *edma)
{
	uint16_t i;
	uint32_t j;
	/* do for all channels */
	for(j = 0; j < edma->cc_nos; j++) {
		/* enable every channel to be accessed by shadow region 0*/
		out32(edma->cc[j].vbase + AM335X_EDMA_DRAE(0), ~0);
		out32(edma->cc[j].vbase + AM335X_EDMA_QRAE(0), ~0);
		/* Disable interrupts for all channels */
		out32(edma->cc[j].vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_IECR, ~0UL);
		
		/* disable event for all channels */
		out32(edma->cc[j].vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_ECR, ~0UL);
		out32(edma->cc[j].vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_EECR, ~0UL);
		out32(edma->cc[j].vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_QEECR, ~0UL);
		
		/* clear up all previous pending interrupts */
		out32(edma->cc[j].vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_SECR, ~0UL);
		out32(edma->cc[j].vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_QSECR, ~0UL);
		out32(edma->cc[j].vbase + AM335X_EDMA_REGION0 + AM335X_EDMA_ICR, ~0UL);
		out32(edma->cc[j].vbase + AM335X_EDMA_EMCR, ~0UL);
		out32(edma->cc[j].vbase + AM335X_EDMA_QEMCR, ~0UL);
		out32(edma->cc[j].vbase + AM335X_EDMA_CCERRCLR, ~0UL);

		/* map all QDMA channels to param, from the last param used for edma */
		for(i = 0; i < edma->cc[j].qdma_chnos; i++)
			out32(edma->cc[j].vbase + AM335X_EDMA_QCHMAP(i), PAENTRY( edma->cc[j].edma_chnos + i));
	}
}

/*
 * map all device memory
 * must be called once per process
 */
int init_mmapregs(edma_t *edma)
{
	int i;
	for(i = 0; i < edma->cc_nos; i++) {
		edma->cc[i].vbase = mmap_device_io(edma->cc[i].size, edma->cc[i].pbase);
		if(edma->cc[i].vbase == MAP_DEVICE_FAILED) {
			perror("mmap_device_io");
			goto fail;
		}
	}
	return 0;
	
fail:
	while(i--)
		munmap_device_io(edma->cc[i].vbase, edma->cc[i].size);
	return -1;
}

/*
 * destroys mapping of all device memory
 * called before exiting process
 */
 
void fini_mmapregs(edma_t *edma)
{
	int i;
	for(i = 0; i < edma->cc_nos; i++) {
		munmap_device_io(edma->cc[i].vbase, edma->cc[i].size);
		edma->cc[i].vbase = NULL;
	}
};

static char rsrc_chparam[] = {RSRC_PARAM}, rsrc_chedma[] = {RSRC_EDMA};
static char rsrc_chqdma[] = {RSRC_QDMA};

/*
 * Destroy all the allocated resources for edma from in reversed order starting
 * from the edma number given in count
 */
static void destroy_edma_rsrc_cnt(edma_t *edma, int last_cnt)
{
	rsrc_alloc_t rsrcdb[MAX_RSRC_PER_CC];
	for(;last_cnt >= 0; last_cnt--) {
		memset(rsrcdb, 0, sizeof(rsrcdb));
		/* create the resource name by appending the channel controller number */
		rsrc_chqdma[strlen(RSRC_QDMA)] = rsrc_chparam[strlen(RSRC_PARAM)] = last_cnt + '0';
		rsrc_chedma[strlen(RSRC_EDMA)] = rsrc_chqdma[strlen(RSRC_QDMA)];

		slog("deallocating resources %s %s %s", rsrc_chparam, rsrc_chedma, rsrc_chqdma);
		rsrcdb[0].start = 0;
		rsrcdb[0].end = edma->cc[last_cnt].edma_chnos - 1;
		rsrcdb[0].name = rsrc_chedma;
		rsrcdb[0].flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_NOREMOVE;

		rsrcdb[1].start = 0;
		rsrcdb[1].end = edma->cc[last_cnt].qdma_chnos - 1;
		rsrcdb[1].name = rsrc_chqdma;
		rsrcdb[1].flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_NOREMOVE;

		rsrcdb[2].start = 0;
		rsrcdb[2].end = 127;/* change this to macro */
		rsrcdb[2].name = rsrc_chparam;
		rsrcdb[2].flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_NOREMOVE;
		if(rsrcdbmgr_destroy(rsrcdb, MAX_RSRC_PER_CC) != EOK) {
			dlog("fatal error : rsrcdbmgr_destroy : %s", strerror(errno));
		}
	}
}

/*
 * Destroy all the allocated edma channel, qdma channels, params
 */
void destroy_edma_rsrc(edma_t *edma)
{
	destroy_edma_rsrc_cnt(edma, edma->cc_nos - 1);
}
/*
 * Create resources for edma
 * create db for dma channels , param set used for scatter gather transactions
 */
int create_edma_rsrc(edma_t *edma)
{
	rsrc_alloc_t rsrcdb[MAX_RSRC_PER_CC];
	int i;
	for(i = 0; i < edma->cc_nos; i++) {
		memset(rsrcdb, 0, sizeof(rsrcdb));
		/* create the resource name by appending the channel controller number */
		rsrc_chqdma[strlen(RSRC_QDMA)] = rsrc_chparam[strlen(RSRC_PARAM)] = i + '0';
		rsrc_chedma[strlen(RSRC_EDMA)] = rsrc_chqdma[strlen(RSRC_QDMA)];
		slog("allocating resources %s, %s, %s", rsrc_chparam, rsrc_chedma, rsrc_chqdma);
		rsrcdb[0].start = 0;
		rsrcdb[0].end = edma->cc[i].edma_chnos - 1;
		rsrcdb[0].name = rsrc_chedma;
		rsrcdb[0].flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_NOREMOVE;

		rsrcdb[1].start = 0;
		rsrcdb[1].end = edma->cc[i].qdma_chnos - 1;;
		rsrcdb[1].name = rsrc_chqdma;
		rsrcdb[1].flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_NOREMOVE;

		rsrcdb[2].start = 0;
		rsrcdb[2].end = 127;
		rsrcdb[2].name = rsrc_chparam;
		rsrcdb[2].flags = RSRCDBMGR_FLAG_NAME | RSRCDBMGR_FLAG_NOREMOVE;

		if(rsrcdbmgr_create(rsrcdb, MAX_RSRC_PER_CC) != EOK) {
			slog("fatal error : rsrcdbmgr_create : %s", strerror(errno));
			destroy_edma_rsrc_cnt(edma, --i);
			return -1;
		}
	}
	return 0;
}
