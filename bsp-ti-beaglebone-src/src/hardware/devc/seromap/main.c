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








#include "externs.h"
#include <stdio.h>
#include <pwd.h>

int
main(int argc, char *argv[]) {

	struct	passwd*  pw;
	uid_t            uid = 0;
	gid_t            gid = 0;

	ttyctrl.max_devs = 16;
	ttc(TTC_INIT_PROC, &ttyctrl, 24);

    if(options(argc, argv) == 0) {
		fprintf(stderr, "%s: No serial ports found\n", argv[0]);
		exit(0);
    }

	if (user_parm != NULL) {
		if(*user_parm >= '0' && *user_parm <= '9') {
			uid = strtol(user_parm, (char**)&user_parm, 0);
			if(*user_parm++ == ':') {
				gid = strtol(user_parm, (char**)&user_parm, 0);
			}
		}
		else if (( pw = getpwnam( user_parm ) ) != NULL ) {
			uid = pw->pw_uid;
			gid = pw->pw_gid;
		}

		if(setgid(gid) == -1 ||	setuid(uid) == -1 ) {
			fprintf(stderr, "%s: unable to set uid/gid - %s", "devc-seromap", strerror(errno));
			return -1;
		}

	}

	ttc(TTC_INIT_START, &ttyctrl, 0);
	return 0;
}

__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-j5-evm/1.0.0/latest/hardware/devc/seromap/main.c $ $Rev: 488283 $" );
