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


#include <sys/devi.h>

extern input_module_t am335xTSC_dev;
extern input_module_t am335xTSC_proto;

input_module_t *modules[] = {

	&am335xTSC_dev,
	&am335xTSC_proto,
	NULL
};


/* Our main routine, just call the library function begin() and
 * you are done.  The only thing you need to do is declare your
 * module(s) and put them in a list called modules terminated 
 * by NULL
 *
 */


int
main(int argc, char *argv[])
{
	return begin(argc, argv);
}
