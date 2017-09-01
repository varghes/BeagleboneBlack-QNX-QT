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
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <errno.h>
#include <sys/procmgr.h>
#include <drvr/hwinfo.h>
#include <arm/am335x.h>
#include <fcntl.h>
#include <hw/i2c.h>

/*
 * AM335x Watchdog driver
 *
 *  NOTE:  This driver assumes UNSECURE mode!
 */

#define AM335X_WDT_MAX_TIMEOUT		131000 /* mseconds */
#define AM335X_WDT_DFLT_KCKTIME		15000

//-----------------------------------------------------
int main(int argc, char *argv[])
{
    int opt, curr;
    int exit = 0;
    int priority = 10;
    uint32_t kicktime = AM335X_WDT_DFLT_KCKTIME;
    uint32_t timeout = 0;
    uint32_t counter_reset_val;
    uint32_t  base;

    /* Process dash options.*/
    while ((opt = getopt( argc, argv, "p:t:k:e" )) != -1)
    {
        switch (opt) {
            case 'p': // priority
                priority = strtoul( optarg, NULL, 0 );
                break;
            case 't': // watchdog timeout in milliseconds
                timeout = strtoul( optarg, NULL, 0 );
                break;
            case 'k': // kick time period in milliseconds
                kicktime = strtoul( optarg, NULL, 0 );
                break;
			case 'e':
				exit = 1;
				break;
        }
    }
    fprintf( stdout, "changing thread parameters\n" );

    // Enable IO capability.
    if (ThreadCtl( _NTO_TCTL_IO, NULL ) == -1)
    {
        slogf( _SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "netio:  ThreadCtl" );
        return EXIT_FAILURE;
    }

    // Run in the background
    if (procmgr_daemon( EXIT_SUCCESS, PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL ) == -1)
    {
        slogf( _SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "%s:  procmgr_daemon", argv[0] );
        return EXIT_FAILURE;
    }

    // If requested: Change priority.
    curr = getprio( 0 );
    if (priority != curr && setprio( 0, priority ) == -1)
        slogf( _SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO, "WDT:  can't change priority" );

    // No timeout specified; use default
    if (timeout == 0)
    {
        fprintf( stdout, "AM335X Watchdog: No timeout specified, using 2x kicktime = %u ms\n",
        		kicktime * 2 );
        timeout = kicktime * 2;
    }
    else if (timeout > AM335X_WDT_MAX_TIMEOUT)
    {
        // AM335x max timeout value is 131 seconds
        fprintf( stdout, "AM335X Watchdog:  Timeout requested exceeds max of %dms.  Setting to %dms. \n",
        		AM335X_WDT_MAX_TIMEOUT, AM335X_WDT_MAX_TIMEOUT);
                timeout = AM335X_WDT_MAX_TIMEOUT;
    }

    fprintf( stdout, "phy_base=0x%08x size=0x%08x\n", WDT_BASE, WDT_SIZE );
	base = (uint32_t)mmap_device_memory( 	NULL,
											WDT_SIZE,
											PROT_READ | PROT_WRITE | PROT_NOCACHE,
											0,
											WDT_BASE );
	if (base == MAP_DEVICE_FAILED) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"Failed to map WDT registers");
		return EXIT_FAILURE;
	}
	fprintf( stdout, "stop timer\n" );

	// Stop the timer
	out32(base + WDT_WSPR, WDT_WSPR_STOPVAL1);
	delay(1);
	out32(base + WDT_WSPR, WDT_WSPR_STOPVAL2);
	delay(1);

	slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"Watchdog Timer is disabled now.");
	if(exit)
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_INFO,"Terminating Watchdog Timer Module.");
		munmap_device_memory( (void*)base, WDT_SIZE );
		return EXIT_SUCCESS;
	}

	// [BA 2011/11/10]
	// The WDT state machine is clocked from the 32KHz WDT clock which means
	// that we must leave enough time between register accesses for previous
	// writes to take effect.  A 1 millisecond delay is sufficient.

	// Disable prescaler, no event delay
	out32(base + WDT_WCLR, 0);
	delay(1);
	out32(base + WDT_WDLY, 0);
	delay(1);

	// Calculate counter value to provide required ms duration until overflow
	counter_reset_val = 0xFFFFFFFF - ((timeout * WDTI_FCLK) / 1000);

	// Load the actual counter, and set the load register value for the periodic resets
	out32(base + WDT_WCRR, counter_reset_val);
	delay(1);
	out32(base + WDT_WLDR, counter_reset_val);
	delay(1);

	// Start the timer
	out32(base + WDT_WSPR, WDT_WSPR_STARTVAL1);
	delay(1);
	out32(base + WDT_WSPR, WDT_WSPR_STARTVAL2);
	delay(1);

    //kick the watchdog timer with kicktime interval
    while (1)
    {
    	// reset the overflow counter for the watchdog
    	out32(base + WDT_WTGR, ~in32(base + WDT_WTGR));

        // Delay is in ms
        delay( kicktime );
    }

    return EXIT_SUCCESS;
}
