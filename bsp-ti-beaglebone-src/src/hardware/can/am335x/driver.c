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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/syspage.h>

#include "canam335x.h"
#include "proto.h"
#include "externs.h"

// Define default bitrate settings for TI OSK5905 board
// based on CAN Bit Rate Calculation and rules from OMAP 5905
// HECC Reference Guide.  These defaults assume a 12 Mhz clock.  
// The TSEG1, TSEG2, and SJW values are held constant and the
// BRP value is changed for the different default bitrates.
#define OSK5905_CAN_BITRATE_TSEG1_DEFAULT       9
#define OSK5905_CAN_BITRATE_TSEG2_DEFAULT       2
#define OSK5905_CAN_BITRATE_SJW_DEFAULT         1
#define OSK5905_CAN_BITRATE_BRP_5K_DEFAULT      200 
#define OSK5905_CAN_BITRATE_BRP_10K_DEFAULT     100 
#define OSK5905_CAN_BITRATE_BRP_20K_DEFAULT     50  
#define OSK5905_CAN_BITRATE_BRP_25K_DEFAULT     40  

// Function prototypes
void device_init(int argc, char *argv[]);
void create_device(CANDEV_AM335X_INIT *devinit);

int main(int argc, char *argv[])
{
    // Driver implemented functions called by CAN library
    can_drvr_funcs_t    drvr_funcs = {can_drvr_transmit, can_drvr_devctl};

    // Get I/O privity
    ThreadCtl( _NTO_TCTL_IO, 0 );

    // Initialize Resource Manager
    can_resmgr_init(&drvr_funcs);

    // Process options and create devices
    device_init(argc, argv);

    // Start Handling Clients
    can_resmgr_start();

    return EXIT_SUCCESS;
}

void device_init(int argc, char *argv[])
{
    int                     opt;
    int                     numcan = 0;
    char                    *cp;
    // Set default options
    CANDEV_AM335X_INIT     devinit =
    {
        {   CANDEV_TYPE_RX,             /* devtype */   
            0,                          /* can_unit - set this later */
            0,                          /* dev_unit - set this later*/
            100,                        /* msgnum */
            AM335X_DCANMCTL_DLC_BYTE8,	/* datalen */
        },
//        AM335X_CAN1_REG_BASE,			/* DCAN port registers base address for AM335x */
        AM335X_CAN_CLK_12MHZ,			/* clk */
        AM335X_CAN_CLK_24MHZ,			/* clk */
        0,                              /* bitrate */
        AM335X_CAN_BITRATE_BRP_10K_DEFAULT - AM335X_CANBTC_DECREMENT, /* br_brp */
        AM335X_CAN_BITRATE_SJW_DEFAULT     - AM335X_CANBTC_DECREMENT, /* br_sjw */
        AM335X_CAN_BITRATE_TSEG1_DEFAULT   - AM335X_CANBTC_DECREMENT, /* br_tseg1 */
        AM335X_CAN_BITRATE_TSEG2_DEFAULT   - AM335X_CANBTC_DECREMENT, /* br_tseg2 */
        AM335X_DCAN0_INT0,				/* irqmsg for AM3517 board */
        AM335X_DCAN0_INT1,				/* irqsys for AM3517 board */
        AM335X_DCAN0_PARITY,			/* irqerr for AM3517 board */
        (INIT_FLAGS_MDRIVER_INIT),      /* flags */
        AM335X_CAN_NUM_MAILBOX/2,		/* numrx */
        0x100C0000,                     /* midrx */
        0x100C0000,                     /* midtx */
        0x0,                            /* timestamp */
    };

    // Process command line options and create associated devices
    while(optind < argc)
    {
        // Process dash options
        while((opt = getopt(argc, argv, "ab:B:c:Di:l:m:Mn:or:StTu:x")) != -1)
        {
            switch(opt){
            case 'a':
                devinit.flags |= INIT_FLAGS_AUTOBUS;
                break;
            case 'b':
                // Set CANBTC params to default
                devinit.br_tseg1 = AM335X_CAN_BITRATE_TSEG1_DEFAULT - AM335X_CANBTC_DECREMENT;
                devinit.br_tseg2 = AM335X_CAN_BITRATE_TSEG2_DEFAULT - AM335X_CANBTC_DECREMENT;
                devinit.br_sjw   = AM335X_CAN_BITRATE_SJW_DEFAULT   - AM335X_CANBTC_DECREMENT;

                // Determine BRP value for desired bitrate
                if(strncmp(optarg, "5K", 2) == 0)
                    devinit.br_brp = AM335X_CAN_BITRATE_BRP_5K_DEFAULT - AM335X_CANBTC_DECREMENT;
                else if(strncmp(optarg, "10K", 3) == 0)
                    devinit.br_brp = AM335X_CAN_BITRATE_BRP_10K_DEFAULT - AM335X_CANBTC_DECREMENT;
                else if(strncmp(optarg, "20K", 3) == 0)
                    devinit.br_brp = AM335X_CAN_BITRATE_BRP_20K_DEFAULT - AM335X_CANBTC_DECREMENT;
                else if(strncmp(optarg, "25K", 3) == 0)
                    devinit.br_brp = AM335X_CAN_BITRATE_BRP_25K_DEFAULT - AM335X_CANBTC_DECREMENT;
                else if(strncmp(optarg, "100K", 3) == 0)
                    devinit.br_brp = AM335X_CAN_BITRATE_BRP_100K_DEFAULT - AM335X_CANBTC_DECREMENT;
                else
                    // Set default to 10K
                    devinit.br_brp = AM335X_CAN_BITRATE_BRP_10K_DEFAULT - AM335X_CANBTC_DECREMENT;
                break;
            case 'B':
                // Values to program bitrate manually
                devinit.br_brp = strtoul(optarg, &optarg, 0) - AM335X_CANBTC_DECREMENT;

                if((cp = strchr(optarg, ',')))
                {
                    cp += 1;    // Skip over the ','
                    devinit.br_tseg1 = strtoul(cp, &cp, 0) - AM335X_CANBTC_DECREMENT;
                }
                if((cp = strchr(cp, ',')))
                {
                    cp += 1;    // Skip over the ','
                    devinit.br_tseg2 = strtoul(cp, &cp, 0) - AM335X_CANBTC_DECREMENT;
                }
                if((cp = strchr(cp, ',')))
                {
                    cp += 1;    // Skip over the ','
                    devinit.br_sjw = strtoul(cp, &cp, 0) - AM335X_CANBTC_DECREMENT;
                }

                // Check for valid bitrate settings
                if(devinit.br_brp   > AM335X_CANBTC_BRP_MAXVAL   ||
                   devinit.br_sjw   > AM335X_CANBTC_SJW_MAXVAL   ||
                   devinit.br_tseg1 > AM335X_CANBTC_TSEG1_MAXVAL ||
                   devinit.br_tseg2 > AM335X_CANBTC_TSEG2_MAXVAL ||
                   devinit.br_tseg1 == 0 || devinit.br_tseg2 == 0) 
                {
                    fprintf(stderr, "Invalid manual bitrate settings\n");
                    exit(EXIT_FAILURE);
                }

                break;
            case 'c':
                if(strncmp(optarg, "12M", 3) == 0)
                    devinit.clk = AM335X_CAN_CLK_12MHZ;
                else if(strncmp(optarg, "24M", 3) == 0)
                    devinit.clk = AM335X_CAN_CLK_24MHZ;
                else if(strncmp(optarg, "48M", 3) == 0)
                    devinit.clk = AM335X_CAN_CLK_48MHZ;
                else
                    devinit.clk = strtoul(optarg, NULL, 0);
                break;
            case 'D':
                devinit.flags &= ~INIT_FLAGS_MDRIVER_INIT;
                break;
            case 'i':
                devinit.midrx = strtoul(optarg, &optarg, 16);
                if((cp = strchr(optarg, ',')))
                {
                    devinit.midtx = strtoul(cp + 1, NULL, 0);
                }
                break;
            case 'l':
                devinit.cinit.datalen = strtoul(optarg, NULL, 0);
                if(devinit.cinit.datalen > AM335X_DCANMCTL_DLC_BYTE8)
                {
                    fprintf(stderr, "Invalid CAN message data length, setting to %d\n", AM335X_DCANMCTL_DLC_BYTE8);
                    devinit.cinit.datalen = AM335X_DCANMCTL_DLC_BYTE8;
                }
                break;
            case 'm':
                devinit.flags |= INIT_FLAGS_TIMESTAMP;
                devinit.timestamp = strtoul(optarg, NULL, 16);
                break;
            case 'M':
                devinit.flags |= INIT_FLAGS_RX_FULL_MSG;
                break;
            case 'n':
                devinit.cinit.msgnum = strtoul(optarg, NULL, 0);
                break;
            case 'o':
                devinit.flags |= INIT_FLAGS_MSGDATA_LSB;
                break;
            case 'r':
                devinit.numrx = strtoul(optarg, NULL, 0);
                if(devinit.numrx > AM335X_CAN_NUM_MAILBOX)
                {
                    fprintf(stderr, "Invalid number or RX CAN mailboxes, setting to %d\n", AM335X_CAN_NUM_MAILBOX);
                    devinit.numrx = AM335X_CAN_NUM_MAILBOX;
                }
                break;
            case 'S':
                devinit.flags |= INIT_FLAGS_MDRIVER_SORT;
                break;
            case 't':
                devinit.flags |= INIT_FLAGS_LOOPBACK;
                break;
            case 'T':
                devinit.flags |= INIT_FLAGS_EXTLOOPBACK;
                break;
            case 'u':
                devinit.cinit.can_unit = strtoul(optarg, NULL, 0);
                break;
            case 'x':
                devinit.flags |= INIT_FLAGS_EXTENDED_MID;
                break;
            default:
                break;
            }
        }

        // Process ports and interrupt
        while(optind < argc && *(optarg = argv[optind]) != '-')
        {
            /* Set defaults for AM335x Board */ 
            if(strncmp(optarg, "am335xcan1", 10) == 0)
            {
                devinit.port   = AM335X_CAN0_REG_BASE;
                devinit.irqmsg = AM335X_DCAN0_INT0;		// DCAN0 dcan_intr0_intr_pend
                devinit.irqsys = AM335X_DCAN0_INT1;		// DCAN0 dcan_intr1_intr_pend
				devinit.irqerr = AM335X_DCAN0_PARITY;	// DCAN0 dcan_uerr_intr_pend
                // Set default can unit number
                if(!devinit.cinit.can_unit)
                    devinit.cinit.can_unit = 1;
            }
            else
            if(strncmp(optarg, "am335xcan2", 10) == 0)
            {
                devinit.port   = AM335X_CAN1_REG_BASE;
                devinit.irqmsg = AM335X_DCAN1_INT0;		// DCAN1 dcan_intr0_intr_pend
                devinit.irqsys = AM335X_DCAN1_INT1;		// DCAN1 dcan_intr1_intr_pend
				devinit.irqerr = AM335X_DCAN1_PARITY;	// DCAN1 dcan_uerr_intr_pend
                // Set default can unit number
                if(!devinit.cinit.can_unit)
                    devinit.cinit.can_unit = 2;
            }
            /* Set command line option for HECC */
            else
            {
                // Set default port for CAN 1
                if(strncmp(optarg, "can1", 4) == 0)
                {
                    // Set defaults even though user may override them
				    devinit.port   = AM335X_CAN0_REG_BASE;
				    devinit.irqmsg = AM335X_DCAN0_INT0;		// DCAN0 dcan_intr0_intr_pend
				    devinit.irqsys = AM335X_DCAN0_INT1;		// DCAN0 dcan_intr1_intr_pend
					devinit.irqerr = AM335X_DCAN0_PARITY;	// DCAN0 dcan_uerr_intr_pend
                    // Set default can unit number
                    if(!devinit.cinit.can_unit)
                        devinit.cinit.can_unit = 1;
                    // Increment optarg
                    optarg += 4;
                }
                else
                // Set default port for CAN 2
                if(strncmp(optarg, "can2", 4) == 0)
                {
                    // Set defaults even though user may override them
	                devinit.port   = AM335X_CAN1_REG_BASE;
	                devinit.irqmsg = AM335X_DCAN1_INT0;		// DCAN1 dcan_intr0_intr_pend
	                devinit.irqsys = AM335X_DCAN1_INT1;		// DCAN1 dcan_intr1_intr_pend
					devinit.irqerr = AM335X_DCAN1_PARITY;	// DCAN1 dcan_uerr_intr_pend
                    // Set default can unit number
                    if(!devinit.cinit.can_unit)
                        devinit.cinit.can_unit = 2;
                    // Increment optarg
                    optarg += 4;
                }
                else
                {
                    fprintf(stderr, "Invalid options\n");
                    exit(EXIT_FAILURE);
                }
                // Set DCAN port registers address
                if(*optarg == ',') devinit.port   = strtoul(optarg + 1, &optarg, 0);
                // Set message interrupt vector
                if(*optarg == ',') devinit.irqmsg = strtoul(optarg + 1, &optarg, 0);
                // Set system interrupt vector
                if(*optarg == ',') devinit.irqsys = strtoul(optarg + 1, NULL, 0);
                // Set system interrupt vector
                if(*optarg == ',') devinit.irqerr = strtoul(optarg + 1, NULL, 0);
            }
            ++optind;

            // Create the CAN device
            create_device(&devinit);
            // Reset unit number for next device
            devinit.cinit.can_unit = 0;
            numcan++;
        }   
    }
    // If no devices have been created yet, create the default device 
    if(numcan == 0)
    {
        // Create the default CAN device
        devinit.cinit.can_unit = 1;
        create_device(&devinit);
    }
}

void create_device(CANDEV_AM335X_INIT *devinit)
{
    CANDEV_AM335X_INFO     *devinfo;
    CANDEV_AM335X          *devlist;
    int                     mdriver_intr = -1;
    int                     i;

#ifdef DEBUG_DRVR
    fprintf(stderr, "port = 0x%X\n", devinit->port);
    fprintf(stderr, "mem = 0x%X\n", devinit->mem);
    fprintf(stderr, "clk = %d\n", devinit->clk);
    fprintf(stderr, "bitrate = %d\n", devinit->bitrate);
    fprintf(stderr, "brp = %d\n", devinit->br_brp);
    fprintf(stderr, "sjw = %d\n", devinit->br_sjw);
    fprintf(stderr, "tseg1 = %d\n", devinit->br_tseg1);
    fprintf(stderr, "tseg2 = %d\n", devinit->br_tseg2);
    fprintf(stderr, "irqmsg = %d\n", devinit->irqmsg);
    fprintf(stderr, "irqsys = %d\n", devinit->irqsys);
    fprintf(stderr, "msgnum = %u\n", devinit->cinit.msgnum);
    fprintf(stderr, "datalen = %u\n", devinit->cinit.datalen);
    fprintf(stderr, "unit = %u\n", devinit->cinit.can_unit);
    fprintf(stderr, "flags = %u\n", devinit->flags);
    fprintf(stderr, "numrx = %u\n", devinit->numrx);
    fprintf(stderr, "midrx = 0x%X\n", devinit->midrx);
    fprintf(stderr, "midtx = 0x%X\n", devinit->midtx);
#endif

    // Allocate device info
    devinfo = (void *) _smalloc(sizeof(*devinfo));
    if(!devinfo)
    {
        fprintf(stderr, "_smalloc failed\n");
        exit(EXIT_FAILURE);
    }
    memset(devinfo, 0, sizeof(*devinfo));

    // Allocate an array of devices - one for each mailbox
    devlist = (void *) _smalloc(sizeof(*devlist) * AM335X_CAN_NUM_MAILBOX);
    if(!devlist)
    {
        fprintf(stderr, "_smalloc failed\n");
        exit(EXIT_FAILURE);
    }
    memset(devlist, 0, sizeof(*devlist) * AM335X_CAN_NUM_MAILBOX);

    // Map device registers
    devinfo->base = mmap_device_io(AM335X_CAN_REG_SIZE, devinit->port);
    if(devinfo->base == MAP_DEVICE_FAILED)
    {
        perror("Can't map device I/O");
        exit(EXIT_FAILURE);
    }

    // Determine if there is an active mini-driver and initialize driver to support it
    if(devinit->flags & INIT_FLAGS_MDRIVER_INIT)
    {
        mdriver_intr = mdriver_init(devinfo, devinit);
    }

#if AMDBG
    printf("INIT_FLAGS_MDRIVER_INIT %x mdriver_intr %x\n", (devinit->flags & INIT_FLAGS_MDRIVER_INIT), mdriver_intr);
#endif

    // Map device message memory
    devinfo->canmsg = (void *) _smalloc(AM335X_CAN_NUM_MAILBOX*sizeof(CAN_MSG_OBJ));
    if(devinfo->canmsg == NULL)
    {
        perror("Can't map device memory");
        exit(EXIT_FAILURE);
    }
	memset(devinfo->canmsg, 0, AM335X_CAN_NUM_MAILBOX*sizeof(CAN_MSG_OBJ));

    // Setup device info
    devinfo->devlist = devlist;
    // Setup the RX and TX mailbox sizes
    devinfo->numrx = devinit->numrx;
    devinfo->numtx = AM335X_CAN_NUM_MAILBOX - devinit->numrx;

    // Initialize flags
    if(devinit->flags & INIT_FLAGS_RX_FULL_MSG)
        devinfo->iflags |= INFO_FLAGS_RX_FULL_MSG;
    if(!(devinit->flags & INIT_FLAGS_MSGDATA_LSB))
        devinfo->iflags |= INFO_FLAGS_ENDIAN_SWAP;

    // Initialize all device mailboxes
    for(i = 0; i < AM335X_CAN_NUM_MAILBOX; i++)
    {
        // Set index into device mailbox memory
        devlist[i].mbxid = i;
        // Store a pointer to the device info
        devlist[i].devinfo = devinfo;

        // Set device mailbox unit number 
        devinit->cinit.dev_unit = i;
        // Set device mailbox as transmit or receive
        if(i < devinfo->numrx)
            devinit->cinit.devtype = CANDEV_TYPE_RX;
        else
            devinit->cinit.devtype = CANDEV_TYPE_TX;

        // Initialize the CAN device
        can_resmgr_init_device(&devlist[i].cdev, &devinit->cinit);

        // Create the resmgr device
        can_resmgr_create_device(&devlist[i].cdev);
    }

    // Initialize device hardware if there is no mini-driver
    if(!(devinit->flags & INIT_FLAGS_MDRIVER_INIT) || mdriver_intr == -1)
        can_init_hw(devinfo, devinit);

#ifdef DEBUG_DRVR
    can_print_reg(devinfo);
#endif

    // Initialize device mailboxes if there is no mini-driver
    if(!(devinit->flags & INIT_FLAGS_MDRIVER_INIT) || mdriver_intr == -1)
        can_init_mailbox(devinfo, devinit);
#ifdef DEBUG_DRVR
    can_print_mailbox(devinfo);
#endif

    // Initialize interrupts and attach interrupt handler
    can_init_intr(devinfo, devinit);

    // Add mini-driver's bufferred CAN messages if mini-driver is active.
    // Note1: This must be done BEFORE we start handling client requests and AFTER calling 
    // InterruptAttach() to ensure we don't miss any data - mini-driver has not ended until 
    // InterruptAttach is called.
    // Note2: We may receive new CAN messages (via the interrupt handler) while we add the
    // mini-driver's buffered messages to the driver's message queue.  The interrupt handler
    // will had new messages to the "head" of the message queue while the mini-driver messages
    // are added to the "tail" of the message queue - this allows us to add both old and new
    // messages to the queue in parrallel and in the correct order.
    // Note3: It is important that enough CAN messages are pre-allocated on the message 
    // queue to ensure that there is room to add both mini-driver buffered messages and
    // new message.  The number required will be be dependent on the CAN data rate and the
    // time it takes for the system to boot and the full driver to take over. 
    if(devinit->flags & INIT_FLAGS_MDRIVER_INIT && mdriver_intr != -1)
    {
        mdriver_init_data(devinfo, devinit);
    }

#if AMDBG
	can_am335x_debug(devinfo);
#endif
}

__SRCVERSION( "$URL$ $Rev$" );
