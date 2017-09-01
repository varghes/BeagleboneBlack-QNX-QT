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

#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include "edma.h"
#include "edma_shmem.h"
#include <errno.h>
#include <stdio.h>

static edma_sharedmem_t *pshmem;
static int shmemfd;

/*	initializes the shared memory for synchronization between process
 *	must be called only once per process
 */

int init_perprocess(void)
{
    pthread_mutexattr_t mutex_attr;
    int newshmem = 0;
    /* if a sharedmem already exists attach to it else create new &
       do necessary initialization
    */
    shmemfd = shm_open(SHAREDMEM, O_RDWR, 0666);
	if(shmemfd < 0) {
		slog("opening of shared mem failed");
    	shmemfd = shm_open(SHAREDMEM, O_RDWR | O_CREAT | O_EXCL, 0666);
		if(shmemfd < 0) {
			slog("creation of new shared mem failed");	
			goto fail1;
		} else
			slog("new shared mem created");
		if(ftruncate(shmemfd, sizeof(edma_sharedmem_t)) < 0) {
			perror("ftruncate");
			goto fail2;
		}
		newshmem = 1;
	}
	pshmem = mmap(0, sizeof(edma_sharedmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmemfd, 0);
	if(pshmem == MAP_FAILED)
		goto fail2;
	if(newshmem) {
		if(pthread_mutexattr_init(&mutex_attr)) {
			perror("pthread_mutexattr_init");
			goto fail3;
		}
		if(pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED)) {
			perror("pthread_mutexattr_setpshared");
			goto fail3;
		}

		if(pthread_mutex_init(&pshmem->process_lock, &mutex_attr)) {
			goto fail3;
		}
	}
	return 0;
fail3:
	munmap(pshmem,sizeof(edma_sharedmem_t));
fail2:
	close(shmemfd);
fail1:
	return -1;
}
/*	closes shared memory mapping
 *	must be called only after last user from within a process exit
 */

void fini_perprocess(void)
{
	munmap(pshmem, sizeof(*pshmem));
	close(shmemfd);
}

pthread_mutex_t* getprocess_mutex(void)
{
	if(pshmem)
		return &pshmem->process_lock;
	return NULL;
}

int incr_process_cnt(void)
{
	if(pshmem)
		return ++pshmem->process_cnt;
	return -1;
}

int dec_process_cnt(void)
{
	if(pshmem)
		return --pshmem->process_cnt;
	return -1;
}

int get_process_cnt(void)
{
	return pshmem->process_cnt;
}
