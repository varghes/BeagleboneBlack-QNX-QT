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
//#include <arm/omap.h>
#include <hw/inout.h>
#include <sys/syspage.h>

#include "canam335x.h"
#include "externs.h"

// Function prototypes
void mdriver_print_data(CANDEV_AM335X_INFO *devinfo);

/* 
 * Function to search through the list of mini-drivers for one that matches
 * our interrupt vector.
 *
 * Returns: Matching interrupt vector or -1
 */
int mdriver_init(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit)
{
    struct mdriver_entry    *mdriver = (struct mdriver_entry *)SYSPAGE_ENTRY(mdriver);
    int                     num_mdrivers = 0;
    int                     mdriver_intr = -1;
    int                     mindex;

    // Determine how many mini-drivers are installed
    num_mdrivers = _syspage_ptr->mdriver.entry_size/sizeof(*mdriver);
    #ifdef DEBUG_MDRVR
    fprintf(stderr, "Number of installed mini-drivers = %d\n", num_mdrivers);
    #endif
    // Search through list of mini-drivers for one using our interrupt
    for(mindex = 0; mindex < num_mdrivers; mindex++)
    {
        // Look for a mini-driver matching either the system or message interrupt
        if((mdriver[mindex].intr == devinit->irqsys) || (mdriver[mindex].intr == devinit->irqmsg))
        {
            #ifdef DEBUG_MDRVR
            fprintf(stderr, "Found mdriver %s attached to interrupt %d\n", SYSPAGE_ENTRY(strings)->data + mdriver[mindex].name, mdriver[mindex].intr);
            #endif
            // Found a matching interrupt, stop searching
            mdriver_intr = mdriver[mindex].intr;
            break;
        }
    }

    if(mdriver_intr != -1)
    {
        // Map mini-driver buffer 
        if((devinfo->mdata = (MINICAN_DATA *)mmap_device_memory(0, mdriver[mindex].data_size, 
                                PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, mdriver[mindex].data_paddr)) == NULL)
        {
            perror("Can't map mini-driver data");
            exit(EXIT_FAILURE);
        }
        // Buffered messages of type CAN_MSG_OBJ follow the MINICAN_DATA data structure in memory
        devinfo->mcanmsg = (CAN_MSG_OBJ *)((uint8_t *)devinfo->mdata + sizeof(*devinfo->mdata));
        #ifdef DEBUG_MDRVR
        mdriver_print_data(devinfo);
        #endif
    }
    #ifdef DEBUG_MDRVR
    else
    {
        fprintf(stderr, "No active mini-drivers found on interrupts %d or %d\n", devinit->irqsys, devinit->irqmsg);
    }
    #endif

    return(mdriver_intr);
}

/* 
 * Function to search though all devices to find a matching message ID 
 *
 * Returns: Matching mailbox ID or -1. 
 */
int mdriver_find_mbxid(CANDEV_AM335X_INFO *devinfo, uint32_t canmid)
{
    int                 i;

    // Search through all receive mailboxes to find a matching message ID
    for(i = 0; i < devinfo->numrx; i++)
    {
        if(canmid == devinfo->canmsg[i].dcanmsg.ID)
        {
            return(i);
        }
    }
    
    // Didn't find a match
    return(-1);
}

/* 
 * Function to add buffered mdriver CAN messages to the driver's message buffer.
 * This function also sorts the messages into the appropriate device according to
 * the message ID.
 *
 * Returns: void 
 */
void mdriver_init_data(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit)
{
    MINICAN_DATA        *mdata = devinfo->mdata;
    CAN_MSG_OBJ         *mcanmsg = devinfo->mcanmsg;
    int                 i;
    CANDEV_AM335X      *devlist = devinfo->devlist;
    canmsg_list_t       *msglist = NULL;
    canmsg_t            *rxmsg = NULL;
    int                 mbxid = 0;      /* mailbox index defaults to first rx mailbox */
    uint32_t            *val32;
    short               sort = 0;

    if(!mdata || !mcanmsg)
        return;

    // Determine if data should be sorted based on MID
    if(devinit->flags & INIT_FLAGS_MDRIVER_SORT)
        sort = 1;

    // Loop through all buffered mini-driver messages backwards to put them into
    // the buffer in the correct order since we are adding the data to the tail 
    // instead of the head of the message list.
    for(i = mdata->nrx - 1; i > -1; i--)
    {
        if(sort)
            // Determine which mailbox this message belongs in
            mbxid = mdriver_find_mbxid(devinfo, mcanmsg[i].dcanmsg.ID);

        if(mbxid != -1) 
        {
            // Get the mailbox's data message list
            msglist = devlist[mbxid].cdev.msg_list;

            // Get the next free receive message
            if ((rxmsg = msg_enqueue_mdriver_next(msglist)))
            {
                // Access the data as a uint32_t array
                val32 = (uint32_t *)rxmsg->cmsg.dat;

                // Copy data from mdriver buffer to receive message
                val32[0] = mcanmsg[i].canmdl;
                val32[1] = mcanmsg[i].canmdh;
    
                // Mini-driver stores raw receive data, determine if we should swap the Endian
                if(devinfo->iflags & INFO_FLAGS_ENDIAN_SWAP)
                {
                    // Convert from Big Endian to Little Endian since data is received MSB
                    ENDIAN_SWAP32(&val32[0]);
                    ENDIAN_SWAP32(&val32[1]);
                }
    
                // Determine if we should store away extra receive message info
                if(devinfo->iflags & INFO_FLAGS_RX_FULL_MSG)
                {
                    // Save the message ID
                    rxmsg->cmsg.mid = mcanmsg[i].dcanmsg.ID;
                    // Save message timestamp (mini-driver doesn't store this info) 
                    rxmsg->cmsg.ext.timestamp = 0;
                }

                // Indicate receive message is ready with data
                msg_enqueue_mdriver(msglist);
            }
        }
    }
}

/* 
 * Function to print out the mdriver's stats and buffered data
 *
 * Returns: void 
 */
void mdriver_print_data(CANDEV_AM335X_INFO *devinfo)
{
    MINICAN_DATA        *mdata = devinfo->mdata;
    CAN_MSG_OBJ         *mcanmsg = devinfo->mcanmsg;
    int                 i;

    fprintf(stderr, "---------------- MDRIVER DATA -------------------\n");
    fprintf(stderr, "\tMDRIVER_STARTUP         calls = %d\n", mdata->nstartup);
    fprintf(stderr, "\tMDRIVER_STARTUP_PREPARE calls = %d\n", mdata->nstartupp);
    fprintf(stderr, "\tMDRIVER_STARTUP_FINI    calls = %d\n", mdata->nstartupf);
    fprintf(stderr, "\tMDRIVER_KERNEL          calls = %d\n", mdata->nkernel);
    fprintf(stderr, "\tMDRIVER_PROCESS         calls = %d\n", mdata->nprocess);
    fprintf(stderr, "\tNum RX                  calls = %d\n", mdata->nrx);
    fprintf(stderr, "\tTX Enabled              calls = 0x%X\n", mdata->tx_enabled);
    fprintf(stderr, "----------------------------------------------\n");

    fprintf(stderr, "\nBuffered %d CAN messages\n\n", mdata->nrx);
    if(mdata->nrx)
    {
        fprintf(stderr, "MID\tMDH\t\tMDL\n");
        fprintf(stderr, "==========================================================\n");
        for(i = 0; i < mdata->nrx; i++)
        {
            fprintf(stderr, "0x%8X\t0x%8X\t0x%8X\n",
                    mcanmsg[i].dcanmsg.ID,
                    mcanmsg[i].canmdh, mcanmsg[i].canmdl);
        }
    }
}


__SRCVERSION( "$URL$ $Rev$" );
