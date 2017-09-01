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

#ifndef __EDMA_SHMEM_H__
#define __EDMA_SHMEM_H__

#include <stdint.h>
#include <pthread.h>
typedef struct {
	uint32_t			process_cnt;
	pthread_mutex_t		process_lock;
}edma_sharedmem_t;

#define SHAREDMEM	"/edma_sharedmem"

int init_perprocess(void); 
void fini_perprocess(void);
pthread_mutex_t* getprocess_mutex(void);
int incr_process_cnt(void);
int process_cnt(void);
int dec_process_cnt(void);
int get_process_cnt(void);
#endif	/* #ifndef __EDMA_SHMEM_H__ */
