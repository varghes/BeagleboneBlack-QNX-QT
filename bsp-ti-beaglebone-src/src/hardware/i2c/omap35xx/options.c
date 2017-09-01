/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include "proto.h"

int
omap_options(omap_dev_t *dev, int argc, char *argv[])
{
    int     c;
    int     prev_optind;
    int     done = 0;

    /* defaults */
    dev->intr = OMAP3530_I2C_1_IRQ;
    dev->iid = -1;
    dev->physbase = OMAP35XX_I2C_BASE1;
    dev->reglen = OMAP_I2C_SIZE;
    dev->own_addr = OMAP_I2C_ADDRESS;
    dev->slave_addr = TWL4030_AUDIO_SLAVE_ADDRESS; /* audio codec */
    dev->options = 0;
	dev->re_start = 0;

    while (!done) {
        prev_optind = optind;
		
#if defined(VARIANT_j5)
        c = getopt(argc, argv, "a:i:p:s:v");
#else
        c = getopt(argc, argv, "a:i:p:s:c:v");
#endif

        switch (c) {

		case 'a':
            dev->own_addr = strtoul(optarg, &optarg, NULL);
            break;

        case 'i':
            dev->intr = strtol(optarg, &optarg, NULL);
            break;

        case 'p':
            dev->physbase = strtoul(optarg, &optarg, NULL);
            break;

        case 's':
            dev->slave_addr = strtoul(optarg, &optarg, NULL);
            break;

        case 'v':
            dev->options |= OMAP_OPT_VERBOSE;

			break;

#if !defined(VARIANT_j5)
		case 'c':
			dev->speed = 1000*strtoul(optarg, &optarg, NULL);
			break;
#endif				
		case '?':
            if (optopt == '-') {
                ++optind;
                break;
            }
            return -1;

        case -1:
            if (prev_optind < optind) /* -- */
                return -1;

            if (argv[optind] == NULL) {
                done = 1;
                break;
            } 
            if (*argv[optind] != '-') {
                ++optind;
                break;
            }
            return -1;

        case ':':
        default:
            return -1;
        }
    }

    return 0;
}

__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-j5-evm/1.0.0/latest/hardware/i2c/omap35xx/options.c $ $Rev: 535139 $" );
