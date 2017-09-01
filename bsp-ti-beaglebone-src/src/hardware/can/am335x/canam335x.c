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
#include <stdint.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <devctl.h>
#include <atomic.h>
#include <gulliver.h>
#include <sys/mman.h>
#include <hw/inout.h>

#include "canam335x.h"

// Set default CAN message ID's in a range that is valid for both standard and extended ID's
#define CANMID_DEFAULT              0x00100000

/* Forward declaration */
void dcan_setobj(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id, int domask, int doarb, int doctrl, int dodata, int dodatb, int startxfer, int dodma);
void dcan_getobj(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id, int domask, int doarb, int doctrl, int dodata, int dodatb, int eoi, int dodma);
void dcan_rxack(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id);
void dcan_txack(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id);
void dcan_setmask(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id);

/* Function prototypes */
void can_am335x_tx(CANDEV_AM335X *cdev, canmsg_t *txmsg);
void can_print_mailbox(CANDEV_AM335X_INFO *devinfo);
void can_print_reg(CANDEV_AM335X_INFO *devinfo);

///////////////////////////////////////////////////////////////////////
// DCAN time-stamping. There's no DCAN time-stamp support in the AM335X
// CAN device, so we create our own simple time-stamping function. Note:
// don't use a printf or slog style function, as these functions are
// used in the interrupt (signal) routine, and using a printf or slog
// will crash the interrupt function..
///////////////////////////////////////////////////////////////////////
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
#define TYPE_MAX(t) \
  ((t) (! TYPE_SIGNED (t) \
        ? (t) -1 \
        : ~ (~ (t) 0 << (sizeof (t) * CHAR_BIT - 1))))

#if AMDBG
int sec[64];
int nsec[64];
int yints=0;
unsigned int sa[64]={0,0,0,0,0,0,0,0}, sb[64]={0,0,0,0,0,0,0,0}, sc[64]={0,0,0,0,0,0,0,0}, sd[64]={0,0,0,0,0,0,0,0};
unsigned int zz=0;
am335x_time_t raw[64] = { 0,0,0,0,0,0,0};
am335x_time_t del[64] = { 0,0,0,0,0,0,0};
am335x_time_t res[64] = { 0,0,0,0,0,0,0};
#endif

am335x_time_t get_am335x_time(void)
{
	struct timespec		timsp;		// For nanosecond timestamp
	am335x_time_t		rawtime;

	if (clock_gettime(CLOCK_REALTIME, &timsp) == 0)
	{
    	rawtime = timsp.tv_sec * 1000 + timsp.tv_nsec/10000000;
#if AMDBG
    	sec[zz] = timsp.tv_sec;
    	nsec[zz] = timsp.tv_nsec;
#endif
    }
	else
	{
		rawtime = 0x00badbad;
	}
	return rawtime;
}

am335x_time_t get_timestamp(CANDEV_AM335X_INFO *devinfo)
{
	am335x_time_t	rawtime;                              // For localtime
	am335x_time_t	delta;
	am335x_time_t	result;

	// Get current clock
	rawtime = get_am335x_time();
#if AMDBG
	raw[zz] = rawtime;
#endif
	// Calculate time passed since calibrate point
	if (rawtime >= devinfo->calibrate)
	{
		delta = rawtime - devinfo->calibrate;
#if AMDBG
		del[zz] = delta;
#endif
	}
	else
	{
		delta = rawtime + (TYPE_MAX(time_t) - devinfo->calibrate);
#if AMDBG
		del[zz] = delta;
#endif
	}
	// Calculate time relative to user timestamp
	result = devinfo->userstamp + delta;
#if AMDBG
	res[zz] = result;
#endif

   return result;
}
// End of time-stamp module

/* CAN Interrupt Handler */
const struct sigevent *can_intr(void *area, int id)
{
    struct sigevent         *event = NULL;
    CANDEV_AM335X_INFO      *devinfo = area;
    CANDEV_AM335X           *devlist = devinfo->devlist;
    uint32_t                 errst;
    uint32_t                 err;
    uint32_t				 val;
    canmsg_list_t           *msglist = NULL;
    canmsg_t                *txmsg = NULL;
    canmsg_t                *rxmsg = NULL;
    uint32_t				 i;
    uint8_t					 intid[2];
    uint8_t                  mbxid;      /* mailbox index */
    uint32_t                *val32;

#if AMDBG
    yints++;
#endif

    // Check for System and Error Interrupts - log the error and clear the interrupt source.
	errst = in32(devinfo->base + AM335X_DCAN_ES_EOI);
	if (errst & AM335X_DCAN_ES_EOI_PDA)
	{
		// Application request for setting DCAN to local power-down mode was successful. DCAN is in local power-down mode.
		atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_PDA);
	}
	if (errst & AM335X_DCAN_ES_EOI_WUPND)
	{
		// DCAN has initiated a wake up of the system due to dominant CAN bus while module power down. 
		atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_WUPND);
		// This bit will be reset if error and status register is read.
	}
	if (errst & AM335X_DCAN_ES_EOI_PER)
	{
		// The parity check mechanism has detected a parity error in the Message RAM.
		atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_PER);
		// This bit will be reset if error and status register is read.
	}
	if (errst & AM335X_DCAN_ES_EOI_BOFF)
	{
		// The CAN module is in bus-off state.
		atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_BOFF);
	}
	if (errst & AM335X_DCAN_ES_EOI_EWARN)
	{
		// At least one of the error counters has reached the error warning limit of 96.
		atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_EWARN);
	}
	if (errst & AM335X_DCAN_ES_EOI_EPASS)
	{
		// The CAN core is in the error passive state as defined in the CAN Specification.
		atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_EPASS);
	}
	if (errst & AM335X_DCAN_ES_EOI_RXOK)
	{
		// The CAN core is in the error passive state as defined in the CAN Specification.
		atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_RXOK);
		// This bit will be reset if error and status register is read.
	}
	if (errst & AM335X_DCAN_ES_EOI_TXOK)
	{
		// The CAN core is in the error passive state as defined in the CAN Specification.
		atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_TXOK);
		// This bit will be reset if error and status register is read.
	}
	if (errst & AM335X_DCAN_ES_EOI_LEC_MASK)
	{
		err = errst & AM335X_DCAN_ES_EOI_LEC_MASK;
		switch (err)
		{
		case AM335X_DCAN_ES_EOI_LEC_STUFF:
			atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_STUFF);
			break;
		case AM335X_DCAN_ES_EOI_LEC_FORM:
			atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_FORM);
			break;
		case AM335X_DCAN_ES_EOI_LEC_ACK:
			atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_ACK);
			break;
		case AM335X_DCAN_ES_EOI_LEC_BIT1:
			atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_BIT1);
			break;
		case AM335X_DCAN_ES_EOI_LEC_BIT0:
			atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_BIT0);
			break;
		case AM335X_DCAN_ES_EOI_LEC_CRC:
			atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_CRC);
			break;
		case AM335X_DCAN_ES_EOI_LEC_EVENT:
			atomic_set(&devinfo->errstatus, AM335X_DCAN_ERR_CRC);
			break;
		}
	}

	val      = in32(devinfo->base + AM335X_DCAN_INT);
	intid[0] = (val & AM335X_DCAN_INT1_MASK) >> 16;
	intid[1] = (val & AM335X_DCAN_INT0_MASK) & 0xFF;
#if AMDBG
	sc[zz]   = val;
	sa[zz] = intid[0];
    sb[zz] = intid[1];
	if (intid[0]||intid[1]) zz++;
#endif
	while ((intid[0])||(intid[1]))
	{
		for (i=0; i<2; i++)
		{
			if (intid[i])
			{
				mbxid = intid[i]-1;

				// Get the mailbox's data message list
				msglist = devlist[mbxid].cdev.msg_list;

				if (mbxid < devinfo->numrx)
				{
					// Get the next free receive message
					if ((rxmsg = msg_enqueue_next(msglist)))
					{
						// Access the data as a uint32_t array
						val32 = (uint32_t *)rxmsg->cmsg.dat;

						//dcan_getobj(devinfo, &devinfo->canmsg[mbxid], mbxid, 1);
						//dcan_getobj(devinfo, &devinfo->canmsg[mbxid], mbxid,1,1,1,1,1,1,1);
						dcan_rxack(devinfo, &devinfo->canmsg[mbxid], mbxid);
						val32[0] = in32(devinfo->base + AM335X_DCAN_IF1DATA);
						val32[1] = in32(devinfo->base + AM335X_DCAN_IF1DATB);

						if (devinfo->iflags & INFO_FLAGS_ENDIAN_SWAP)
						{
							// Convert from Big Endian to Little Endian since data is received MSB
							ENDIAN_SWAP32(&val32[0]);
							ENDIAN_SWAP32(&val32[1]);
						}

						// Determine if we should store away extra receive message info
						if (devinfo->iflags & INFO_FLAGS_RX_FULL_MSG)
						{
							// Save the message ID
							rxmsg->cmsg.mid = devinfo->canmsg[mbxid].dcanmsg.ID;
							// Save message timestamp
							rxmsg->cmsg.ext.timestamp = get_timestamp(devinfo);
						}

						// Indicate receive message is ready with data
						msg_enqueue(msglist);
					}
				}
				else
				{
					// Get next transmit message
					if ((txmsg = msg_dequeue_next(msglist)))
					{
						// Acknowledge message was transmitted
						msg_dequeue(msglist);

						// Signal interrupt acknowledge (EOI)
						dcan_txack(devinfo, &devinfo->canmsg[mbxid], mbxid);

						// Deterimine if there is another message to transmit
						if ((txmsg = msg_dequeue_next(msglist)))
						{
							// Transmit next message
							can_am335x_tx(&devlist[mbxid], txmsg);
						}
						else
						{
							// No more transmit messages, end transmission
							atomic_clr(&devlist[mbxid].dflags, CANDEV_AM335X_TX_ENABLED);
						}
					}
				}

				// Check if we have to notify any blocked clients
				event = can_client_check(&devlist[mbxid].cdev);

				if (event)
					return event;
			}
        }

		val      = in32(devinfo->base + AM335X_DCAN_INT);
		intid[0] = (val & AM335X_DCAN_INT1_MASK) >> 16;
		intid[1] = (val & AM335X_DCAN_INT0_MASK) & 0xFF;
#if AMDBG
		sc[zz]   = val;
		sa[zz] = intid[0];
	    sb[zz] = intid[1];
		if (intid[0]||intid[1]) zz++;
#endif
	}

    return event;
}

/* LIBCAN driver transmit function */
void can_drvr_transmit(CANDEV *cdev)
{
    CANDEV_AM335X      *dev = (CANDEV_AM335X *)cdev;
    canmsg_t            *txmsg;

    // Make sure transmit isn't already in progress and there is valid data
    if(!(dev->dflags & CANDEV_AM335X_TX_ENABLED) && (txmsg = msg_dequeue_next(cdev->msg_list)))
    {
        // Indicate transmit is in progress
        atomic_set(&dev->dflags, CANDEV_AM335X_TX_ENABLED);
        // Start transmission
        can_am335x_tx(dev, txmsg);
    }
}

/* LIBCAN driver devctl function */
int can_drvr_devctl(CANDEV *cdev, int dcmd, DCMD_DATA *data)
{
    CANDEV_AM335X          *dev = (CANDEV_AM335X *)cdev;
    CANDEV_AM335X_INFO     *devinfo = dev->devinfo;
    int                     mbxid = dev->mbxid;

    switch(dcmd)
    {
        case CAN_DEVCTL_SET_MID:
            // Set new message ID
        	devinfo->canmsg[mbxid].dcanmsg.ID = data->mid;
    		dcan_setobj(devinfo, &devinfo->canmsg[mbxid], mbxid, 0,1,0,0,0,0,1);
            break;
        case CAN_DEVCTL_GET_MID:
            // Read device's current message ID (removing non-message ID bits)
    		dcan_getobj(devinfo, &devinfo->canmsg[mbxid], mbxid, 0,1,0,0,0,0,1);
        	data->mid = devinfo->canmsg[mbxid].arb & AM335X_DCAN_IFxARB_MIDMASK;
            break;
        case CAN_DEVCTL_SET_MFILTER:
            if (data->mfilter & AM335X_DCAN_IFxMSK_MASK)
            {
            	devinfo->canmsg[mbxid].dcanmsg.msk = data->mfilter;
            	dcan_setmask(devinfo, &devinfo->canmsg[mbxid], mbxid);
            }
            else
            {
            	devinfo->canmsg[mbxid].dcanmsg.msk  = 0;
            	dcan_setmask(devinfo, &devinfo->canmsg[mbxid], mbxid);
            }
            break;
        case CAN_DEVCTL_GET_MFILTER:
        	dcan_getobj(devinfo, &devinfo->canmsg[mbxid], mbxid,1,1,1,1,1,0,1);
        	data->mfilter = devinfo->canmsg[mbxid].mask & AM335X_DCAN_IFxMSK_MASK;
            break;
        case CAN_DEVCTL_SET_PRIO:
        	fprintf(stderr, "Unable to change priority. The receive/transmit priority for the message objects is fixed.\n");
            break;
        case CAN_DEVCTL_GET_PRIO:
        	// The receive/transmit priority for the message objects is attached to the message number, not to the CAN
        	// identifier. Message object 1 has the highest priority, while the last implemented message object has the
        	// lowest priority. If more than one transmission request is pending, they are serviced due to the priority of
        	// the corresponding message object so messages with the highest priority, for example, can be placed in
        	// the message objects with the lowest numbers.
        	data->prio = AM335X_CAN_NUM_MAILBOX - mbxid;
            break;
        case CAN_DEVCTL_SET_TIMESTAMP:
        	devinfo->userstamp = data->timestamp;
        	devinfo->calibrate = get_am335x_time();
            break;
        case CAN_DEVCTL_GET_TIMESTAMP :
            // Read the current Local Network Time
      	    data->timestamp = get_timestamp(devinfo);
            break;
        case CAN_DEVCTL_READ_CANMSG_EXT:
            // This is handled by the interrupt handler
            break;
        case CAN_DEVCTL_ERROR:
            // Read current state of CAN Error and Status register
            data->error.drvr1 = in32(devinfo->base + AM335X_DCAN_ES_EOI);
            // Read and clear previous devctl info
            data->error.drvr2 = atomic_clr_value(&devinfo->errstatus, 0xffffffff);
            break;
        case CAN_DEVCTL_DEBUG_INFO:
            // Print debug information
            can_am335x_debug(devinfo);
            break;
        default:
            // Driver does not support this DEVCTL
            return(ENOTSUP);
    }

    return(EOK);
}

/* Initialize CAN device registers */
void can_init_intr(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit)
{
	uint32_t	val;

    devinfo->iidmsg = InterruptAttach(devinit->irqmsg, can_intr, devinfo, 0, 0);
    if(devinfo->iidmsg == -1) 
    {
        perror("InterruptAttach irqmsg");
        exit(EXIT_FAILURE);
    }

    // Attach interrupt handler for system interrupts 
    devinfo->iidsys = InterruptAttach(devinit->irqsys, can_intr, devinfo, 0, 0);
    if(devinfo->iidsys == -1) 
    {
        perror("InterruptAttach irqsys");
        exit(EXIT_FAILURE);
    }

    // Attach interrupt handler for system interrupts 
    devinfo->iiderr = InterruptAttach(devinit->irqerr, can_intr, devinfo, 0, 0);
    if(devinfo->iidsys == -1) 
    {
        perror("InterruptAttach irqsys");
        exit(EXIT_FAILURE);
    }

    // Enable all system/error interrupts to be generated on DCAN interrupt lines
    // Note - must do this AFTER calling InterruptAttach since mini-driver clears OMAP5905_CANGIM
    // on the MDRIVER_INTR_ATTACH state.
	val  = in32(devinfo->base + AM335X_DCAN_CTL);
	val |= AM335X_DCAN_CTL_IE0;		// Interrupt line 0 enable
	val |= AM335X_DCAN_CTL_IE1;		// Interrupt line 1 enable
	val |= AM335X_DCAN_CTL_SIE;		// Status change interrupt enable
	val |= AM335X_DCAN_CTL_EIE;		// Error interrupt enable
	out32(devinfo->base + AM335X_DCAN_CTL, val);
}

void reset_ram(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit)
{
    uintptr_t	 cfg_base = 0;
	uint32_t	 val;
	unsigned int start_bit;
	unsigned int done_bit;
	unsigned int loops;

	// Initialise RAM
	cfg_base = mmap_device_io(AM335X_CTRLMOD_MOD, AM335X_CTRLMOD);
	if (cfg_base == MAP_DEVICE_FAILED)
	{
		perror("Can't map device I/O");
		exit(EXIT_FAILURE);
	}
	if ((devinit->port = AM335X_CAN0_REG_BASE))
	{
		start_bit = AM335X_DCAN0_RAMINIT_START;
		done_bit  = AM335X_DCAN0_RAMINIT_DONE;
	}
	else
	{
		start_bit = AM335X_DCAN1_RAMINIT_START;
		done_bit  = AM335X_DCAN1_RAMINIT_DONE;
	}

	printf("start-bit=%x\n", start_bit);
	out32(cfg_base + AM335X_DCAN_RAMINIT, start_bit);
	val = in32(cfg_base + AM335X_DCAN_RAMINIT);
	printf("1.raminit=%x\n", val);
	loops=0;
	while ((!(val & done_bit))&&(loops<10))
	{
		val = in32(cfg_base + AM335X_DCAN_RAMINIT);
		printf("2.raminit=%x\n", val);
		loops++;
		delay(500);
	}

    munmap_device_io(cfg_base, AM335X_CTRLMOD_MOD);

    printf("3.raminit=%x\n", val);
    if (loops >= 10)
    {
      	printf("TIMEOUT\n");
    }
}

/* Initialize CAN device registers */
void can_init_hw(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit)
{
	uint32_t	val;
	uint32_t	brp;
	uint32_t	brpe;

	devinfo->userstamp = 0;
	devinfo->calibrate = 0;

	// Configuration of CAN Bit Timing
	// The CAN module must be in initialization mode to configure the CAN bit timing.

	// Step 1: Enter initialization mode by setting the Init (Initialization) bit in the CAN control register.
	val  = in32(devinfo->base + AM335X_DCAN_CTL);
	val |= AM335X_DCAN_CTL_Init;
	out32(devinfo->base + AM335X_DCAN_CTL, val);

	// Step 2: Set the Configure Change Enable (CCE) bit in the CAN control register.
	val |= AM335X_DCAN_CTL_CCE;
	out32(devinfo->base + AM335X_DCAN_CTL, val);

	// Step 3: Wait for the Init bit to get set. This would make sure that the module has entered Initialization mode.
	val  = in32(devinfo->base + AM335X_DCAN_CTL);
	while (!(val & AM335X_DCAN_CTL_Init))
	{
		val  = in32(devinfo->base + AM335X_DCAN_CTL);
	}

	// Step 4: Write the bit timing values into the bit timing register.
    // Program bitrate
	brp  = devinit->br_brp &  AM335X_CANBTC_BRP_MASK;
	brpe = devinit->br_brp >> AM335X_CANBTC_BRP_MASK_SHIFT;
#if AMDBG
	printf("brp %x, brpe %x, sjw %x, tseg1 %x, tseg2 %x\n", brp, brpe, devinit->br_sjw, devinit->br_tseg1, devinit->br_tseg2);
#endif
    out32(devinfo->base + AM335X_DCAN_BTR, (brp               << AM335X_DCAN_BTR_BRP_SHIFT  ) |
    									   (brpe              << AM335X_CANBTC_BRPE_SHIFT   ) |
                                           (devinit->br_sjw   << AM335X_DCAN_BTR_SJW_SHIFT  ) | 
                                           (devinit->br_tseg1 << AM335X_DCAN_BTR_TSeg1_SHIFT) | 
                                           (devinit->br_tseg2 << AM335X_DCAN_BTR_TSeg2_SHIFT)   ); 
#if AMDBG
	printf("BTR %x\n", (brp               << AM335X_DCAN_BTR_BRP_SHIFT  ) |
			           (brpe              << AM335X_CANBTC_BRPE_SHIFT   ) |
                       (devinit->br_sjw   << AM335X_DCAN_BTR_SJW_SHIFT  ) |
                       (devinit->br_tseg1 << AM335X_DCAN_BTR_TSeg1_SHIFT) |
                       (devinit->br_tseg2 << AM335X_DCAN_BTR_TSeg2_SHIFT));
#endif

	// Clear dcan internal msg RAM
//	reset_ram(devinfo, devinit);

	// Step 5: Clear the CCE and Init bit.
	val &= ~AM335X_DCAN_CTL_Init;
	val &= ~AM335X_DCAN_CTL_CCE;
	out32(devinfo->base + AM335X_DCAN_CTL, val);

	// Step 6: Wait for the Init bit to clear. This would ensure that the module has come out of initialization mode.
	val  = in32(devinfo->base + AM335X_DCAN_CTL);
	while (val & AM335X_DCAN_CTL_Init)
	{
		val  = in32(devinfo->base + AM335X_DCAN_CTL);
	}

    // Enable self-test/loop-back for testing
    if (devinit->flags & INIT_FLAGS_LOOPBACK)
    {
    	// For all test modes, the test bit in CAN control register needs
    	// to be set to one. If test bit is set, the RDA, EXL, Tx1, Tx0,
    	// LBack and Silent bits are writable.
		val  = in32(devinfo->base + AM335X_DCAN_CTL);
		val |= AM335X_DCAN_CTL_Test;
        out32(devinfo->base + AM335X_DCAN_CTL, val);
		val  = in32(devinfo->base + AM335X_DCAN_TEST);
		val |= AM335X_DCAN_TEST_LBACK;
        out32(devinfo->base + AM335X_DCAN_TEST, val);
    }
    else
    if (devinit->flags & INIT_FLAGS_EXTLOOPBACK)
    {
    	// For all test modes, the test bit in CAN control register needs
    	// to be set to one. If test bit is set, the RDA, EXL, Tx1, Tx0,
    	// LBack and Silent bits are writable.
		val  = in32(devinfo->base + AM335X_DCAN_CTL);
		val |= AM335X_DCAN_CTL_Test;
        out32(devinfo->base + AM335X_DCAN_CTL, val);
		val  = in32(devinfo->base + AM335X_DCAN_TEST);
		val |= AM335X_DCAN_TEST_EXL;
        out32(devinfo->base + AM335X_DCAN_TEST, val);
    }

    // Enable auto bus on
    if(devinit->flags & INIT_FLAGS_AUTOBUS)
    {
		val  = in32(devinfo->base + AM335X_DCAN_CTL);
		val |= AM335X_DCAN_CTL_ABO;
        out32(devinfo->base + AM335X_DCAN_CTL, val);
    }

    // Set initial value for Local Network Time
    if(devinit->flags & INIT_FLAGS_TIMESTAMP)
    {
    	devinfo->userstamp = devinit->timestamp;
    	devinfo->calibrate = get_am335x_time();
    }

    // Clear interrupt registers
	in32(devinfo->base + AM335X_DCAN_ES_EOI);
}

void init_msg_tx(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit, int id, int mxid)
{
	CAN_MSG_OBJ *msg = &devinfo->canmsg[id];

	/* IF1/IF2 Mask Registers */
	msg->dcanmsg.mxtd   = 0;
	msg->dcanmsg.mdir   = 0;
	msg->dcanmsg.msk    = 0;

	/* IF1/IF2 Arbitration Registers */
	msg->dcanmsg.MsgVal = 1;
	if (devinit->flags & INIT_FLAGS_EXTENDED_MID)
	{
		msg->dcanmsg.Xtd  = 1;	/* The 29-bit (“extended”) Identifier is used for this message object. */
		msg->dcanmsg.mxtd = 1;	/* The extended identifier bit (IDE) is used for acceptance filtering. */
	}
	else
	{
		msg->dcanmsg.Xtd  = 0;	/* The 11-bit (“standard”) Identifier is used for this message object. */
		msg->dcanmsg.mxtd = 0;	/* The extended identifier bit (IDE) has no effect on the acceptance filtering. */
	}
	msg->dcanmsg.Dir    = 1;	/* Direction = transmit: */
	msg->dcanmsg.ID     = devinit->midrx + CANMID_DEFAULT * mxid;
#if AMDBG
	printf("TX: [%d] %x\n", id, msg->dcanmsg.ID);
#endif

	/* IF1/IF2 Message Control Registers */
	msg->dcanmsg.NewDat = 0;
	msg->dcanmsg.MsgLst = 0;
	msg->dcanmsg.IntPnd = 0;
	msg->dcanmsg.UMask  = 0;
	msg->dcanmsg.TxIE   = 1;
	msg->dcanmsg.RxIE   = 0;
	msg->dcanmsg.RmtEn  = 1;
	msg->dcanmsg.TxRqst = 0;
	msg->dcanmsg.EoB    = 1;
	msg->dcanmsg.DLC    = devinit->cinit.datalen;
}

void init_msg_rx(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit, int id, int mxid)
{
	CAN_MSG_OBJ *msg = &devinfo->canmsg[id];

	/* IF1/IF2 Mask Registers */
	msg->dcanmsg.mxtd   = 0;
	msg->dcanmsg.mdir   = 0;
	msg->dcanmsg.msk    = 0;

	/* IF1/IF2 Arbitration Registers */
	msg->dcanmsg.MsgVal = 1;
	if (devinit->flags & INIT_FLAGS_EXTENDED_MID)
	{
		msg->dcanmsg.Xtd  = 1;	/* The 29-bit (“extended”) Identifier is used for this message object. */
		msg->dcanmsg.mxtd = 1;	/* The extended identifier bit (IDE) is used for acceptance filtering. */
	}
	else
	{
		msg->dcanmsg.Xtd  = 0;	/* The 11-bit (“standard”) Identifier is used for this message object. */
		msg->dcanmsg.mxtd = 0;	/* The extended identifier bit (IDE) has no effect on the acceptance filtering. */
	}
	msg->dcanmsg.Dir    = 0;	/* Direction = receive */
	msg->dcanmsg.ID     = devinit->midrx + CANMID_DEFAULT * mxid;
#if AMDBG
	printf("RX: [%d] %x\n", id, msg->dcanmsg.ID);
#endif

	/* IF1/IF2 Message Control Registers */
	msg->dcanmsg.NewDat = 0;
	msg->dcanmsg.MsgLst = 0;
	msg->dcanmsg.IntPnd = 0;
	msg->dcanmsg.UMask  = 0;
	msg->dcanmsg.TxIE   = 0;
	msg->dcanmsg.RxIE   = 1;
	msg->dcanmsg.RmtEn  = 1;
	msg->dcanmsg.TxRqst = 0;
	msg->dcanmsg.EoB    = 1;
	msg->dcanmsg.DLC    = devinit->cinit.datalen;
}

void dcan_setobj(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id, int domask, int doarb, int doctrl, int dodata, int dodatb, int startxfer, int dodma)
{
	volatile uint32_t	cmd;

	cmd = AM335X_DCAN_IFxCMD_WR;

	if (domask)
	{
		uint32_t	mask;

		/* IF1/IF2 Mask Registers */
		mask = (msg->dcanmsg.mxtd << AM335X_DCAN_IFxMSK_MXTD_SHIFT) |
			   (msg->dcanmsg.mdir << AM335X_DCAN_IFxMSK_MDIR_SHIFT) |
			   (msg->dcanmsg.msk  << AM335X_DCAN_IFxMSK_MASK_SHIFT);
		out32(devinfo->base + AM335X_DCAN_IF1MSK, mask);

		cmd |= AM335X_DCAN_IFxCMD_MASK;

#if AMDBG
		printf("*mask %x\n", mask);
#endif
	}

	if (doarb)
	{
		uint32_t	arb;

#if AMDBG
		printf("msgval %d, xtd %d, dir %d\n", msg->dcanmsg.MsgVal, msg->dcanmsg.Xtd, msg->dcanmsg.Dir);
#endif
		/* IF1/IF2 Arbitration Registers */
		arb  = (msg->dcanmsg.MsgVal << AM335X_DCAN_IFxARB_MSGVAL_SHIFT ) |
			   (msg->dcanmsg.Xtd    << AM335X_DCAN_IFxARB_XTD_SHIFT    ) |
			   (msg->dcanmsg.Dir    << AM335X_DCAN_IFxARB_DIR_SHIFT    ) |
		       (msg->dcanmsg.ID     << AM335X_DCAN_IFxARB_MSK_SHIFT_STD) ;
		out32(devinfo->base + AM335X_DCAN_IF1ARB, arb);

		cmd |= AM335X_DCAN_IFxCMD_ARB;
#if AMDBG
		printf("*arb  %x\n", arb);
#endif
	}

	if (doctrl)
	{
		uint32_t	ctrl;

		/* IF1/IF2 Message Control Registers */
		ctrl = (msg->dcanmsg.NewDat << AM335X_DCAN_IFxMCTL_NEWDAT_SHIFT) |
			   (msg->dcanmsg.MsgLst << AM335X_DCAN_IFxMCTL_MSGLST_SHIFT) |
			   (msg->dcanmsg.IntPnd << AM335X_DCAN_IFxMCTL_INTPND_SHIFT) |
			   (msg->dcanmsg.UMask  << AM335X_DCAN_IFxMCTL_UMASK_SHIFT ) |
			   (msg->dcanmsg.TxIE   << AM335X_DCAN_IFxMCTL_TXIE_SHIFT  ) |
			   (msg->dcanmsg.RxIE   << AM335X_DCAN_IFxMCTL_RXIE_SHIFT  ) |
			   (msg->dcanmsg.RmtEn  << AM335X_DCAN_IFxMCTL_RMTEN_SHIFT ) |
			   (msg->dcanmsg.TxRqst << AM335X_DCAN_IFxMCTL_TXRQST_SHIFT) |
			   (msg->dcanmsg.EoB    << AM335X_DCAN_IFxMCTL_EOB_SHIFT   ) |
			   (msg->dcanmsg.DLC    << AM335X_DCAN_IFxMCTL_DLC_SHIFT   );
		out32(devinfo->base + AM335X_DCAN_IF1MCTL, ctrl);

		cmd |= AM335X_DCAN_IFxCMD_CTRL;
#if AMDBG
		printf("*ctrl %x\n", ctrl);
#endif
	}

	if (dodata)
	{
		out32(devinfo->base + AM335X_DCAN_IF1DATA, msg->canmdl);
		cmd |= AM335X_DCAN_IFxCMD_DATAA;
#if AMDBG
		printf("*data %x\n", msg->canmdl);
#endif
	}

	if (dodatb)
	{
		out32(devinfo->base + AM335X_DCAN_IF1DATB, msg->canmdh);
		cmd |= AM335X_DCAN_IFxCMD_DATAB;
#if AMDBG
		printf("*data %x\n", msg->canmdh);
#endif
	}

	if (startxfer)
	{
		cmd |= AM335X_DCAN_IFxCMD_TXRNDT;
	}

	if (dodma)
	{
		cmd |= AM335X_DCAN_IFxCMD_DMAACT;
	}

	/* IF1/IF2 Message Control Command */
	cmd |= ((id+1) << AM335X_DCAN_IFxCMD_MSGNO_SHIFT);
	out32(devinfo->base + AM335X_DCAN_IF1CMD, cmd);
#if AMDBG
	printf("*cmd %x\n", cmd);
#endif

	cmd = in32(devinfo->base + AM335X_DCAN_IF1CMD);
	while (cmd & AM335X_DCAN_IFxCMD_BUSY)
	{
		cmd = in32(devinfo->base + AM335X_DCAN_IF1CMD);
	}
}

void dcan_clrobj(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id)
{
	volatile uint32_t	cmd;

	out32(devinfo->base + AM335X_DCAN_IF1MSK , 0);
	out32(devinfo->base + AM335X_DCAN_IF1ARB , 0);
	out32(devinfo->base + AM335X_DCAN_IF1MCTL, 0);
	out32(devinfo->base + AM335X_DCAN_IF1DATA, 0);
	out32(devinfo->base + AM335X_DCAN_IF1DATB, 0);

	/* IF1/IF2 Message Control Command */
	cmd  = AM335X_DCAN_IFxCMD_WR;
	cmd |= AM335X_DCAN_IFxCMD_MASK;
	cmd |= AM335X_DCAN_IFxCMD_ARB;
	cmd |= AM335X_DCAN_IFxCMD_CTRL;
	cmd |= AM335X_DCAN_IFxCMD_DATAA;
	cmd |= AM335X_DCAN_IFxCMD_DATAB;
	cmd |= AM335X_DCAN_IFxCMD_DMAACT;
	cmd |= ((id+1) << AM335X_DCAN_IFxCMD_MSGNO_SHIFT);
	out32(devinfo->base + AM335X_DCAN_IF1CMD, cmd);

	cmd = in32(devinfo->base + AM335X_DCAN_IF1CMD);
	while (cmd & AM335X_DCAN_IFxCMD_BUSY)
	{
		cmd = in32(devinfo->base + AM335X_DCAN_IF1CMD);
	}
}

void dcan_start_tx(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id)
{
	dcan_setobj(devinfo, msg, id, 0,0,0,1,1,1,1);

#if AMDBG
	// TODO: remove once completed debug
	can_am335x_debug(devinfo);
#endif
}

void dcan_getobj(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id, int domask, int doarb, int doctrl, int dodata, int dodatb, int eoi, int dodma)
{
	uint32_t cmd = 0;

	/* IF1/IF2 Message Control Command */
	if (domask)	cmd |= AM335X_DCAN_IFxCMD_MASK;
	if (doarb)  cmd |= AM335X_DCAN_IFxCMD_ARB;
	if (doctrl) cmd |= AM335X_DCAN_IFxCMD_CTRL;
	if (dodata) cmd |= AM335X_DCAN_IFxCMD_DATAA;
	if (dodatb) cmd |= AM335X_DCAN_IFxCMD_DATAB;
	if (dodma)  cmd |= AM335X_DCAN_IFxCMD_DMAACT;
	if (eoi)    cmd |= AM335X_DCAN_IFxCMD_CTRLINTPND;
	cmd |= ((id+1) << AM335X_DCAN_IFxCMD_MSGNO_SHIFT);

	out32(devinfo->base + AM335X_DCAN_IF1CMD, cmd);

	if (domask) msg->mask   = in32(devinfo->base + AM335X_DCAN_IF1MSK);
	if (doarb)  msg->arb    = in32(devinfo->base + AM335X_DCAN_IF1ARB);
	if (doctrl) msg->mctrl  = in32(devinfo->base + AM335X_DCAN_IF1MCTL);
	if (dodata) msg->canmdl = in32(devinfo->base + AM335X_DCAN_IF1DATA);
	if (dodatb) msg->canmdh = in32(devinfo->base + AM335X_DCAN_IF1DATB);
}

void dcan_rxack(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id)
{
	dcan_getobj(devinfo, msg, id, 1, 1, 1, 1, 1, 1, 1);
}

void dcan_txack(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id)
{
	dcan_getobj(devinfo, msg, id, 0, 0, 0, 0, 0, 1, 1);
}

void dcan_setmask(CANDEV_AM335X_INFO *devinfo, CAN_MSG_OBJ *msg, int id)
{
	// If the UMask bit is set to one, the message object's mask bits have to be
	// programmed during initialization of the message object before MsgVal is
	// set to one.

	// Disable message object
	dcan_clrobj(devinfo, msg, id);
#if AMDBG
	can_am335x_debug(devinfo);
#endif

	// Write mask
	msg->dcanmsg.MsgVal = 0;
	msg->dcanmsg.UMask  = 1;
	dcan_setobj(devinfo, msg, id, 1,1,1,0,0,0,1);
#if AMDBG
	can_am335x_debug(devinfo);
#endif

	// Enable message object
	msg->dcanmsg.MsgVal = 1;
	dcan_setobj(devinfo, msg, id, 0,1,1,0,0,0,1);
#if AMDBG
	can_am335x_debug(devinfo);
#endif
}

/* Initialize CAN mailboxes in device memory */
void can_init_mailbox(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit)
{
    int         i;
    int         counter = 0;

    // Configure Receive Mailboxes
    counter = 0;
    for(i = 0; i < devinfo->numrx; i++)
    {
		init_msg_rx(devinfo, devinit, i, counter++);
		dcan_setobj(devinfo, &devinfo->canmsg[i], i, 1,1,1,0,0,0,1);
    }

    // Configure Transmit Mailboxes
    counter = 0;
    for(i = devinfo->numrx; i < AM335X_CAN_NUM_MAILBOX; i++)
    {
		init_msg_tx(devinfo, devinit, i, counter++);
		dcan_setobj(devinfo, &devinfo->canmsg[i], i, 1,1,1,0,0,0,1);
    }
}

/* Transmit a CAN message from the specified mailbox */
void can_am335x_tx(CANDEV_AM335X *dev, canmsg_t *txmsg)
{
    CANDEV_AM335X_INFO     *devinfo = dev->devinfo;
    int                     mbxid = dev->mbxid;
    // Access the data as a uint32_t array
    uint32_t                *val32 = (uint32_t *)txmsg->cmsg.dat;

    // Copy message data into transmit mailbox
    devinfo->canmsg[mbxid].canmdl = val32[0];
    devinfo->canmsg[mbxid].canmdh = val32[1];

    if(devinfo->iflags & INFO_FLAGS_ENDIAN_SWAP)
    {
        // Convert from Little Endian to Big Endian since data is transmitted MSB
        ENDIAN_SWAP32(&devinfo->canmsg[mbxid].canmdl);
        ENDIAN_SWAP32(&devinfo->canmsg[mbxid].canmdh);
    }

    // Transmit message
	out32(devinfo->base + AM335X_DCAN_IF1DATA, devinfo->canmsg[mbxid].canmdl);
	out32(devinfo->base + AM335X_DCAN_IF1DATB, devinfo->canmsg[mbxid].canmdh);
	dcan_start_tx(devinfo, &devinfo->canmsg[mbxid], mbxid);
}

/* Print out debug information */
void can_am335x_debug(CANDEV_AM335X_INFO *devinfo)
{
    fprintf(stderr, "\nCAN REG\n");
    can_print_reg(devinfo);
    fprintf(stderr, "\nMailboxes\n");
    can_print_mailbox(devinfo);
}

/* Print CAN device registers */
void can_print_reg(CANDEV_AM335X_INFO *devinfo)
{
    fprintf(stderr, "\n************************************************************************************\n");
    fprintf(stderr, "AM335X_DCAN_CTL      = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_CTL));
    fprintf(stderr, "AM335X_DCAN_ES_EOI   = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_ES_EOI));
    fprintf(stderr, "AM335X_DCAN_ERRC     = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_ERRC));
    fprintf(stderr, "AM335X_DCAN_BTR      = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_BTR));
    fprintf(stderr, "AM335X_DCAN_INT      = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_INT));
    fprintf(stderr, "AM335X_DCAN_TEST     = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_TEST));
    fprintf(stderr, "AM335X_DCAN_PERR     = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_PERR));
    fprintf(stderr, "AM335X_DCAN_ABOTR    = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_ABOTR));
    fprintf(stderr, "AM335X_DCAN_TXRQ_X   = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_TXRQ_X));
    fprintf(stderr, "AM335X_DCAN_TXRQ12   = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_TXRQ12));
    fprintf(stderr, "AM335X_DCAN_TXRQ34   = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_TXRQ34));
    fprintf(stderr, "AM335X_DCAN_TXRQ56   = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_TXRQ56));
    fprintf(stderr, "AM335X_DCAN_TXRQ78   = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_TXRQ78));
    fprintf(stderr, "AM335X_DCAN_NWDAT_X  = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_NWDAT_X));
    fprintf(stderr, "AM335X_DCAN_NWDAT12  = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_NWDAT12));
    fprintf(stderr, "AM335X_DCAN_NWDAT34  = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_NWDAT34));
    fprintf(stderr, "AM335X_DCAN_NWDAT56  = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_NWDAT56));
    fprintf(stderr, "AM335X_DCAN_NWDAT78  = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_NWDAT78));
    fprintf(stderr, "AM335X_DCAN_INTPND_X = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_INTPND_X));
    fprintf(stderr, "AM335X_DCAN_INTPND12 = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_INTPND12));
    fprintf(stderr, "AM335X_DCAN_INTPND34 = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_INTPND34));
    fprintf(stderr, "AM335X_DCAN_INTPND56 = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_INTPND56));
    fprintf(stderr, "AM335X_DCAN_INTPND78 = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_INTPND78));
    fprintf(stderr, "AM335X_DCAN_MSGVAL_X = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_MSGVAL_X));
    fprintf(stderr, "AM335X_DCAN_MSGVAL12 = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_MSGVAL12));
    fprintf(stderr, "AM335X_DCAN_MSGVAL34 = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_MSGVAL34));
    fprintf(stderr, "AM335X_DCAN_MSGVAL56 = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_MSGVAL56));
    fprintf(stderr, "AM335X_DCAN_MSGVAL78 = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_MSGVAL78));
    fprintf(stderr, "AM335X_DCAN_INTMUX12 = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_INTMUX12));
    fprintf(stderr, "AM335X_DCAN_INTMUX34 = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_INTMUX34));
    fprintf(stderr, "AM335X_DCAN_INTMUX56 = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_INTMUX56));
    fprintf(stderr, "AM335X_DCAN_INTMUX78 = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_INTMUX78));
    fprintf(stderr, "AM335X_DCAN_IF1CMD   = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF1CMD));
    fprintf(stderr, "AM335X_DCAN_IF2CMD   = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF2CMD));
    fprintf(stderr, "AM335X_DCAN_IF1MSK   = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF1MSK));
    fprintf(stderr, "AM335X_DCAN_IF2MSK   = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF2MSK));
    fprintf(stderr, "AM335X_DCAN_IF1ARB   = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF1ARB));
    fprintf(stderr, "AM335X_DCAN_IF2ARB   = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF2ARB));
    fprintf(stderr, "AM335X_DCAN_IF1MCTL  = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF1MCTL));
    fprintf(stderr, "AM335X_DCAN_IF2MCTL  = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF2MCTL));
    fprintf(stderr, "AM335X_DCAN_IF1DATA  = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF1DATA));
    fprintf(stderr, "AM335X_DCAN_IF1DATB  = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF1DATB));
    fprintf(stderr, "AM335X_DCAN_IF2DATA  = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF2DATA));
    fprintf(stderr, "AM335X_DCAN_IF2DATB  = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF2DATB));
    fprintf(stderr, "AM335X_DCAN_IF3OBS   = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF3OBS));
    fprintf(stderr, "AM335X_DCAN_IF3MSK   = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF3MSK));
    fprintf(stderr, "AM335X_DCAN_IF3ARB   = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF3ARB));
    fprintf(stderr, "AM335X_DCAN_IF3MCTL  = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF3MCTL));
    fprintf(stderr, "AM335X_DCAN_IF3DATA  = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF3DATA));
    fprintf(stderr, "AM335X_DCAN_IF3DATB  = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF3DATB));
    fprintf(stderr, "AM335X_DCAN_IF3UPD12 = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF3UPD12));
    fprintf(stderr, "AM335X_DCAN_IF3UPD56 = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF3UPD56));
    fprintf(stderr, "AM335X_DCAN_IF3UPD78 = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_IF3UPD78));
    fprintf(stderr, "AM335X_DCAN_IF3UPD78 = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_IF3UPD78));
    fprintf(stderr, "AM335X_DCAN_TIOC     = 0x%8X  ", in32(devinfo->base + AM335X_DCAN_TIOC));
    fprintf(stderr, "AM335X_DCAN_RIOC     = 0x%8X\n", in32(devinfo->base + AM335X_DCAN_RIOC));
#if AMDBG
    {
   	int i;
    fprintf(stderr, "int                  = 0x%8X\n", yints);
    fprintf(stderr, "sa %d %d %d %d %d %d %d %d\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5],sa[6],sa[7]);
    fprintf(stderr, "sb %d %d %d %d %d %d %d %d\n", sb[0],sb[1],sb[2],sb[3],sb[4],sb[5],sb[6],sb[7]);
    fprintf(stderr, "sc %d %d %d %d %d %d %d %d\n", sc[0],sc[1],sc[2],sc[3],sc[4],sc[5],sc[6],sc[7]);
    fprintf(stderr, "sd %d %d %d %d %d %d %d %d\n", sd[0],sd[1],sd[2],sd[3],sd[4],sd[5],sd[6],sd[7]);
    for (i=0; i<8; i++)
	fprintf(stderr, "rawtime %x, calibrate %x, delta %x, result %x\n", raw[i], devinfo->calibrate, del[i], res[i]);
    for (i=0; i<8; i++)
    fprintf(stderr, "sec %x, nsec %x\n", sec[i], nsec[i]);
    fprintf(stderr, "************************************************************************************\n");
    }
#endif
}

/* Print CAN device mailbox memory */
void can_print_mailbox(CANDEV_AM335X_INFO *devinfo)
{
    int         i = 0;

    fprintf(stderr, "RX Mailboxes\n");
    fprintf(stderr, "MB\tMASK\t\tARB\t\tMCTRL\t\tMDL\t\tMDH\n");
    fprintf(stderr, "==================================================================================================\n");
    for(i = 0; i < devinfo->numrx; i++)
    {
    	dcan_getobj(devinfo,&devinfo->canmsg[i],i,1,1,1,1,1,0,1);
    	fprintf(stderr, "RX%d\t0x%8X\t0x%8X\t0x%8X\t0x%8X\t0x%8X\n",i,
                devinfo->canmsg[i].mask, devinfo->canmsg[i].arb,
                devinfo->canmsg[i].mctrl, devinfo->canmsg[i].canmdl,
                devinfo->canmsg[i].canmdh);
    }
    fprintf(stderr, "\nTX Mailboxes\n");
    fprintf(stderr, "MB\tMASK\t\tARB\t\tMCTRL\t\tMDL\t\tMDH\n");
    fprintf(stderr, "==================================================================================================\n");
    for(i = devinfo->numrx; i < AM335X_CAN_NUM_MAILBOX; i++)
    {
    	dcan_getobj(devinfo,&devinfo->canmsg[i],i,1,1,1,1,1,0,1);
        fprintf(stderr, "TX%d\t0x%8X\t0x%8X\t0x%8X\t0x%8X\t0x%8X\n",i,
                devinfo->canmsg[i].mask, devinfo->canmsg[i].arb,
                devinfo->canmsg[i].mctrl, devinfo->canmsg[i].canmdl,
                devinfo->canmsg[i].canmdh);
    }
}


__SRCVERSION( "$URL$ $Rev$" );
