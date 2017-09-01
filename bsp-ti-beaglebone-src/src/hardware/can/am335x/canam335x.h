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
#ifndef __CANOSK_H_INCLUDED
#define __CANOSK_H_INCLUDED

#include <hw/can.h>

#define AM335X_CAN_NUM_MAILBOX			32

#define AM335X_CANBTC_BRPE_SHIFT		16
#define AM335X_CANBTC_BRP_MASK			0x3F
#define AM335X_CANBTC_BRP_MASK_SHIFT	6
#define AM335X_CANBTC_BRP_MAXVAL		0x3FF
#define AM335X_CANBTC_BRP_SHIFT			0
#define AM335X_CANBTC_SJW_MAXVAL		0x3
#define AM335X_CANBTC_SJW_SHIFT			6
#define AM335X_CANBTC_TSEG2_MAXVAL		0x7
#define AM335X_CANBTC_TSEG2_SHIFT		12
#define AM335X_CANBTC_TSEG1_MAXVAL		0xF
#define AM335X_CANBTC_TSEG1_SHIFT		8
#define AM335X_CANBTC_DECREMENT			1			/* All values stored in CANBTC must be adjusted */
#define AM335X_CANBTC_SAM_BRP_MIN		5			/* Minimum BRP value before SAM can be enabled */
#define AM335X_CAN_BITRATE_MAX			100000000 	/* 1Mbps max bitrate, depending on bus and transceiver */

#define AM335X_CAN_BITRATE_TSEG1_DEFAULT       9
#define AM335X_CAN_BITRATE_TSEG2_DEFAULT       2
#define AM335X_CAN_BITRATE_SJW_DEFAULT         1

#define AM335X_CAN_BITRATE_BRP_5K_DEFAULT		400
#define AM335X_CAN_BITRATE_BRP_10K_DEFAULT		200
#define AM335X_CAN_BITRATE_BRP_20K_DEFAULT		100
#define AM335X_CAN_BITRATE_BRP_25K_DEFAULT		 80
#define AM335X_CAN_BITRATE_BRP_50K_DEFAULT		 40
#define AM335X_CAN_BITRATE_BRP_100K_DEFAULT		 20
#define AM335X_CAN_BITRATE_BRP_500K_DEFAULT		  4
#define AM335X_CAN_BITRATE_BRP_1M_DEFAULT		  2

#define AM335X_CAN_CLK_12MHZ 			12000000
#define AM335X_CAN_CLK_24MHZ 			24000000
#define AM335X_CAN_CLK_48MHZ 			48000000

/* Bit definitions for DCAN Message Control Register */
#define AM335X_DCANMCTL_DLC_BYTE0			0x0
#define AM335X_DCANMCTL_DLC_BYTE1			0x1
#define AM335X_DCANMCTL_DLC_BYTE2			0x2
#define AM335X_DCANMCTL_DLC_BYTE3			0x3
#define AM335X_DCANMCTL_DLC_BYTE4			0x4
#define AM335X_DCANMCTL_DLC_BYTE5			0x5
#define AM335X_DCANMCTL_DLC_BYTE6			0x6
#define AM335X_DCANMCTL_DLC_BYTE7			0x7
#define AM335X_DCANMCTL_DLC_BYTE8			0x8


#define AM335X_CAN0_REG_BASE 		0x481CC000
#define AM335X_CAN0_MEM_BASE 		0x481CC000
#define AM335X_CAN1_REG_BASE 		0x481D0000
#define AM335X_CAN1_MEM_BASE 		0x481D0000
#define AM335X_CAN_REG_SIZE			0x00002000
#define AM335X_CAN_MEM_SIZE			0x00002000

#define AM335X_DCAN0_INT0			52		// DCAN0 dcan_intr0_intr_pend
#define AM335X_DCAN0_INT1			53		// DCAN0 dcan_intr1_intr_pend
#define AM335X_DCAN0_PARITY			54		// DCAN0 dcan_uerr_intr_pend
#define AM335X_DCAN1_INT0			55		// DCAN1 dcan_intr0_intr_pend
#define AM335X_DCAN1_INT1			56		// DCAN1 dcan_intr1_intr_pend
#define AM335X_DCAN1_PARITY			57		// DCAN1 dcan_uerr_intr_pend


#define AM335X_CTRLMOD				0x44E10000
#define AM335X_CTRLMOD_MOD			0x00010000

#define AM335X_DCAN_RAMINIT			0x664
	#define AM335X_DCAN1_RAMINIT_DONE	(1<<9)	// 0: DCAN1 RAM Initialization NOT complete 1: DCAN1 RAM Initialization complete
	#define AM335X_DCAN0_RAMINIT_DONE	(1<<8)	// 0: DCAN0 RAM Initialization NOT complete 1: DCAN0 RAM Initialization complete
	#define AM335X_DCAN1_RAMINIT_START	(1<<1)	// A transition from 0 to 1 will start DCAN1 RAM initialization sequence.
	#define AM335X_DCAN0_RAMINIT_START	(1<<0)	// A transition from 0 to 1 will start DCAN1 RAM initialization sequence

#define AM335X_DCAN_CTL				0x00	// CAN Control Register
	#define AM335X_DCAN_CTL_Init		(1<<0)
	#define AM335X_DCAN_CTL_IE0			(1<<1)
	#define AM335X_DCAN_CTL_SIE			(1<<2)
	#define AM335X_DCAN_CTL_EIE			(1<<3)
	#define AM335X_DCAN_CTL_DAR			(1<<5)
	#define AM335X_DCAN_CTL_CCE			(1<<6)
	#define AM335X_DCAN_CTL_Test		(1<<7)
	#define AM335X_DCAN_CTL_IDS			(1<<8)
	#define AM335X_DCAN_CTL_ABO			(1<<9)
	#define AM335X_DCAN_CTL_PMD_MASK	(0xF<<10)
	#define AM335X_DCAN_CTL_PMD_V(v)	((v&0xF)<<10)
	#define AM335X_DCAN_CTL_SWR			(1<<15)
	#define AM335X_DCAN_CTL_InitDbg		(1<<16)
	#define AM335X_DCAN_CTL_IE1			(1<<17)
	#define AM335X_DCAN_CTL_DE1			(1<<18)
	#define AM335X_DCAN_CTL_DE2			(1<<19)
	#define AM335X_DCAN_CTL_DE3			(1<<20)
	#define AM335X_DCAN_CTL_PDR			(1<<24)
	#define AM335X_DCAN_CTL_WUBA		(1<<25)
#define AM335X_DCAN_ES_EOI			0x04	// Error and Status Register
	#define AM335X_DCAN_ES_EOI_PDA			(1<<10)
	#define AM335X_DCAN_ES_EOI_WUPND		(1<<9)
	#define AM335X_DCAN_ES_EOI_PER			(1<<8)
	#define AM335X_DCAN_ES_EOI_BOFF			(1<<7)
	#define AM335X_DCAN_ES_EOI_EWARN		(1<<6)
	#define AM335X_DCAN_ES_EOI_EPASS		(1<<5)
	#define AM335X_DCAN_ES_EOI_RXOK			(1<<4)
	#define AM335X_DCAN_ES_EOI_TXOK			(1<<3)
	#define AM335X_DCAN_ES_EOI_LEC_MASK		(~7)
	#define AM335X_DCAN_ES_EOI_LEC_STUFF	(1)
	#define AM335X_DCAN_ES_EOI_LEC_FORM		(2)
	#define AM335X_DCAN_ES_EOI_LEC_ACK		(3)
	#define AM335X_DCAN_ES_EOI_LEC_BIT1		(4)
	#define AM335X_DCAN_ES_EOI_LEC_BIT0		(5)
	#define AM335X_DCAN_ES_EOI_LEC_CRC		(6)
	#define AM335X_DCAN_ES_EOI_LEC_EVENT	(7)
	// devinfo->errstatus register. Bits 0..15 error, bits 16..31 status
	#define AM335X_DCAN_ERR_PER				(1<<0)		// Parity error detected
	#define AM335X_DCAN_ERR_BOFF			(1<<1)		// Bus-Off state
	#define AM335X_DCAN_ERR_EWARN			(1<<2)		// Warning state
	#define AM335X_DCAN_ERR_EPASS			(1<<3)		// Error passive state
	#define AM335X_DCAN_ERR_STUFF			(1<<4)		// Stuff error: More than five equal bits in a row have been detected in a part of a received message where this is not allowed.
	#define AM335X_DCAN_ERR_FORM			(1<<5)		// Form error: A fixed format part of a received frame has the wrong format.
	#define AM335X_DCAN_ERR_ACK				(1<<6)		// Ack error: The message this CAN core transmitted was not acknowledged by another node.
	#define AM335X_DCAN_ERR_BIT1			(1<<7)		// Bit1 error: During the transmission of a message (with the exception of the arbitration field), the device wanted to send a recessive level (bit of logical value '1'), but the monitored bus value was dominant.
	#define AM335X_DCAN_ERR_BIT0			(1<<8)		// Bit0 error: During the transmission of a message (or acknowledge bit, or active error flag, or overload flag), the device wanted to send a dominant level (logical value '0'), but the monitored bus level was recessive.
	#define AM335X_DCAN_ERR_CRC				(1<<9)		// CRC error: In a received message, the CRC check sum was incorrect.
	#define AM335X_DCAN_ERR_PDA				(1<<16)		// Local power-down mode acknowledge
	#define AM335X_DCAN_ERR_WUPND			(1<<17)		// Wake up pending.
	#define AM335X_DCAN_ERR_RXOK			(1<<18)		// Received a message successfully
	#define AM335X_DCAN_ERR_TXOK			(1<<19)		// Transmitted a message successfully
	#define AM335X_DCAN_ERR_EVENT			(1<<20)		// No CAN bus event was detected since the last time the CPU read the error and status register.

#define AM335X_DCAN_ERRC			0x08	// Error Counter Register
#define AM335X_DCAN_BTR				0x0C	// Bit Timing Register
	#define AM335X_DCAN_BTR_BRP_SHIFT		0
	#define AM335X_DCAN_BTR_SJW_SHIFT		6
	#define AM335X_DCAN_BTR_TSeg1_SHIFT		8
	#define AM335X_DCAN_BTR_TSeg2_SHIFT		12
	#define AM335X_DCAN_BTR_BRPE_SHIFT		16
#define AM335X_DCAN_INT				0x10	// Interrupt Register
	#define AM335X_DCAN_INT1_MASK		(0x00FF0000)
	#define AM335X_DCAN_INT0_MASK		(0x0000FFFF)
#define AM335X_DCAN_TEST			0x14	// Test Register
	#define AM335X_DCAN_TEST_RDA		(1<<9)		// direct access enable
	#define AM335X_DCAN_TEST_EXL		(1<<8)		// External loopback mode
	#define AM335X_DCAN_TEST_RX			(1<<7)		// Receive pin. Monitors the actual value of the CAN_RX pin
	#define AM335X_DCAN_TESTCAN_TX_V(v)	((v&3)<<5)	// Tx[1:0] Control of pin
													// 00 Normal operation, CAN_TX is controlled by the CAN core.
													// 01 Sample point can be monitored at CAN_TX pin.
													// 10 CAN_TX pin drives a dominant value.
													// 11 CAN_TX pin drives a recessive value.
	#define AM335X_DCAN_TEST_LBACK		(1<<4)		// Loopback mode
	#define AM335X_DCAN_TEST_SILENT		(1<<3)		//Silent mode
#define AM335X_DCAN_PERR			0x1C	// Parity Error Code Register
#define AM335X_DCAN_ABOTR			0x80	// Auto-Bus-On Time Register
#define AM335X_DCAN_TXRQ_X			0x84	// Transmission Request X Register
#define AM335X_DCAN_TXRQ12			0x88	// Transmission Request Registers
#define AM335X_DCAN_TXRQ34			0x8C 
#define AM335X_DCAN_TXRQ56			0x90 
#define AM335X_DCAN_TXRQ78			0x94 
#define AM335X_DCAN_NWDAT_X			0x98	// New Data X Register
#define AM335X_DCAN_NWDAT12			0x9C	// New Data Registers
#define AM335X_DCAN_NWDAT34			0xA0 
#define AM335X_DCAN_NWDAT56			0xA4 
#define AM335X_DCAN_NWDAT78			0xA8 
#define AM335X_DCAN_INTPND_X		0xAC	// Interrupt Pending X Register
#define AM335X_DCAN_INTPND12		0xB0	// Interrupt Pending Registers
#define AM335X_DCAN_INTPND34		0xB4
#define AM335X_DCAN_INTPND56		0xB8
#define AM335X_DCAN_INTPND78		0xBC
#define AM335X_DCAN_MSGVAL_X		0xC0	// Message Valid X Register
#define AM335X_DCAN_MSGVAL12		0xC4	// Message Valid Registers
#define AM335X_DCAN_MSGVAL34		0xC8 
#define AM335X_DCAN_MSGVAL56		0xCC 
#define AM335X_DCAN_MSGVAL78		0xD0 
#define AM335X_DCAN_INTMUX12		0xD8	// Interrupt Multiplexer Registers
#define AM335X_DCAN_INTMUX34		0xDC 
#define AM335X_DCAN_INTMUX56		0xE0 
#define AM335X_DCAN_INTMUX78		0xE4 
#define AM335X_DCAN_IF1CMD			0x100	// IF1 Command Registers
#define AM335X_DCAN_IF2CMD			0x120	// IF2 Command Registers
	#define AM335X_DCAN_IFxCMD_WR				(1<<23)
	#define AM335X_DCAN_IFxCMD_MASK				(1<<22)
	#define AM335X_DCAN_IFxCMD_ARB				(1<<21)
	#define AM335X_DCAN_IFxCMD_CTRL				(1<<20)
	#define AM335X_DCAN_IFxCMD_CTRLINTPND		(1<<19)
	#define AM335X_DCAN_IFxCMD_TXRNDT			(1<<18)
	#define AM335X_DCAN_IFxCMD_DATAA			(1<<17)
	#define AM335X_DCAN_IFxCMD_DATAB			(1<<16)
	#define AM335X_DCAN_IFxCMD_BUSY				(1<<15)
	#define AM335X_DCAN_IFxCMD_DMAACT			(1<<14)
	#define AM335X_DCAN_IFxCMD_MSGNO_SHIFT		0
#define AM335X_DCAN_IF1MSK			0x104	// IF1 Mask Register
#define AM335X_DCAN_IF2MSK			0x124	// IF2 Mask Register
	#define AM335X_DCAN_IFxMSK_MXTD_SHIFT		31
	#define AM335X_DCAN_IFxMSK_MDIR_SHIFT		30
	#define AM335X_DCAN_IFxMSK_MASK_SHIFT		0
	#define AM335X_DCAN_IFxMSK_MASK				0x1FFFFFFF
#define AM335X_DCAN_IF1ARB			0x108	// IF1 Arbitration Register
#define AM335X_DCAN_IF2ARB			0x128	// IF2 Arbitration Register
	#define AM335X_DCAN_IFxARB_MSGVAL_SHIFT		31
	#define AM335X_DCAN_IFxARB_XTD_SHIFT		30
	#define AM335X_DCAN_IFxARB_DIR_SHIFT		29
	#define AM335X_DCAN_IFxARB_MSK_SHIFT_EXT	18
	#define AM335X_DCAN_IFxARB_MSK_SHIFT_STD	 0
	#define AM335X_DCAN_IFxARB_MIDMASK			0x1FFFFFFF
#define AM335X_DCAN_IF1MCTL			0x10C	// IF1 Message Control Register
#define AM335X_DCAN_IF2MCTL			0x12C	// IF2 Message Control Register
	#define AM335X_DCAN_IFxMCTL_NEWDAT_SHIFT	15
	#define AM335X_DCAN_IFxMCTL_MSGLST_SHIFT	14
	#define AM335X_DCAN_IFxMCTL_INTPND_SHIFT	13
	#define AM335X_DCAN_IFxMCTL_UMASK_SHIFT		12
	#define AM335X_DCAN_IFxMCTL_TXIE_SHIFT		11
	#define AM335X_DCAN_IFxMCTL_RXIE_SHIFT		10
	#define AM335X_DCAN_IFxMCTL_RMTEN_SHIFT		 9
	#define AM335X_DCAN_IFxMCTL_TXRQST_SHIFT	 8
	#define AM335X_DCAN_IFxMCTL_EOB_SHIFT		 7
	#define AM335X_DCAN_IFxMCTL_DLC_SHIFT		 0
#define AM335X_DCAN_IF1DATA			0x110	// IF1 Data A Register
#define AM335X_DCAN_IF1DATB			0x114	// IF1 Data B Register
#define AM335X_DCAN_IF2DATA			0x130	// IF2 Data A Register
#define AM335X_DCAN_IF2DATB			0x134	// IF2 Data B Register
#define AM335X_DCAN_IF3OBS			0x140	// IF3 Observation Register
#define AM335X_DCAN_IF3MSK			0x144	// IF3 Mask Register
#define AM335X_DCAN_IF3ARB			0x148	// IF3 Arbitration Register
#define AM335X_DCAN_IF3MCTL			0x14C	// IF3 Message Control Register
#define AM335X_DCAN_IF3DATA			0x150	// IF3 Data A Register
#define AM335X_DCAN_IF3DATB			0x154	// IF3 Data B Register
#define AM335X_DCAN_IF3UPD12		0x160	// IF3 Update Enable Registers
#define AM335X_DCAN_IF3UPD34		0x164
#define AM335X_DCAN_IF3UPD56		0x168
#define AM335X_DCAN_IF3UPD78		0x16C 
#define AM335X_DCAN_TIOC			0x1E0	// CAN TX IO Control Register
#define AM335X_DCAN_RIOC			0x1E4	// CAN RX IO Control Register


#define CANDEV_AM335X_TX_ENABLED        0x00000001

#define INIT_FLAGS_LOOPBACK             0x00000001  /* Enable self-test/loopback mode */
#define INIT_FLAGS_EXTENDED_MID         0x00000002  /* Enable 29 bit extended message ID */
#define INIT_FLAGS_MSGDATA_LSB          0x00000004  /* Transmit message data LSB */
#define INIT_FLAGS_AUTOBUS              0x00000008  /* Enable auto bus on */
#define INIT_FLAGS_LNTC                 0x00000010  /* Enable Local Network Time Clear Bit on mailbox 16 */
#define INIT_FLAGS_TIMESTAMP            0x00000020  /* Set initial local network time */
#define INIT_FLAGS_RX_FULL_MSG          0x00000040  /* Receiver should store message ID, timestamp, etc. */
#define INIT_FLAGS_BITRATE_ERM          0x00000080  /* Enable Bitrate Edge Resynchronization on rising and falling edges */
#define INIT_FLAGS_BITRATE_SAM          0x00000100  /* Enable Bitrate Triple Sample Mode */
#define INIT_FLAGS_MDRIVER_INIT         0x00000200  /* Initialize from mini-driver (if it exists and is running) */
#define INIT_FLAGS_MDRIVER_SORT         0x00000400  /* Sort buffered mdriver message based on MID */
#define INIT_FLAGS_EXTLOOPBACK          0x00000800  /* Enable self-test/loopback mode (external loopback) */

#define INFO_FLAGS_RX_FULL_MSG          0x00000001  /* Receiver should store message ID, timestamp, etc. */
#define INFO_FLAGS_ENDIAN_SWAP          0x00000002  /* Data is TX/RX'd MSB, need to perform ENDIAN conversions */


#define am335x_time_t		int

struct candev_am335x_entry;

/* Structure of a AM335X message object */
typedef struct am335x_dcanmsg
{
	/* IF1/IF2 Mask Registers */
	uint32_t		mxtd;					/* Mask Extended Identifier */
	uint32_t		mdir;					/* Mask Message Direction */
	uint32_t		msk;					/* Bits 28-0 Identifier Mask */
	/* IF1/IF2 Arbitration Registers */
	uint32_t		MsgVal;					/* Message valid */
	uint32_t		Xtd;					/* Extended identifier */
	uint32_t		Dir;					/* Message direction */
	uint32_t		ID;						/* Message identifier 28-0 (29-bit identifier (extended frame)) or 28-18 (11-bit identifier (standard frame)) */
	/* IF1/IF2 Message Control Registers */
	uint32_t		NewDat;					/* New data */
	uint32_t		MsgLst;					/* Message lost (only valid for message */
	uint32_t		IntPnd;					/* Interrupt pending */
	uint32_t		UMask;					/* Use acceptance mask */
	uint32_t		TxIE;					/* Transmit interrupt enable */
	uint32_t		RxIE;					/* Receive interrupt enable */
	uint32_t		RmtEn;					/* Remote enable */
	uint32_t		TxRqst;					/* Transmit request */
	uint32_t		EoB;					/* Data frame has 0-8 data bits. */
	uint32_t		DLC;					/* Data length code */

} AM335X_DCANMSG;

/* CAN Message Object Data Structure */
typedef struct can_msg_obj
{
	AM335X_DCANMSG	dcanmsg;

	uint32_t		mask;
	uint32_t		arb;
	uint32_t		mctrl;

	/* IF1/IF2 Data A and Data B Registers */
    uint32_t        canmdl;                 /* Message Data Low Word */
    uint32_t        canmdh;                 /* Message Data High Word */
} CAN_MSG_OBJ;

/* Mini-driver Data in memory - this must exactly match the definition used by the mdriver in Startup */
typedef struct minican_data
{
    uintptr_t       canport;        /* CAN base register mapping */
    uintptr_t       canport_k;      /* CAN base register mapping */
    CAN_MSG_OBJ     *canmem;        /* CAN base memory mapping */
    CAN_MSG_OBJ     *canmem_k;      /* CAN base memory mapping */
    uint16_t        nstartup;       /* Stats for number of calls to MDRIVER_STARTUP */
    uint16_t        nstartupp;      /* Stats for number of calls to MDRIVER_STARTUP_PREPARE */
    uint16_t        nstartupf;      /* Stats for number of calls to MDRIVER_STARTUP_FINI */
    uint16_t        nkernel;        /* Stats for number of calls to MDRIVER_KERNEL */
    uint16_t        nprocess;       /* Stats for number of calls to MDRIVER_PROCESS */
    uint16_t        nrx;            /* Number of received messages */
    uint32_t        tx_enabled;     /* Flags to keep track of which mailboxes have a tx in progress */
    /* Buffered messages of type CAN_MSG_OBJ follow this data structure in memory */
} MINICAN_DATA;

/* Initialization and Options Structure */
typedef struct candev_am335x_init_entry
{
    CANDEV_INIT                 cinit;      /* Generic CAN Device Init Params */
    _Paddr32t                   port;       /* Device Physical Register Address */
    uint32_t                    clk;        /* CAN Clock */
    /* Bitrate related parameters */
    uint32_t                    bitrate;    /* Bitrate */
    uint16_t                    br_brp;     /* Bitrate Prescaler */
    uint8_t                     br_sjw;     /* Bitrate Synch Jump Width */
    uint8_t                     br_tseg1;   /* Bitrate Time Segment 1 */
    uint8_t                     br_tseg2;   /* Bitrate Time Segment 2 */
    int                         irqmsg;     /* Device Message Interrupt Vector */
    int                         irqsys;     /* Device Message System Vector */
    int                         irqerr;     /* Device Message parity error  */
    uint32_t                    flags;      /* Initialization flags */
    uint32_t                    numrx;      /* Number of RX Mailboxes (TX = Total Mailboxes - RX) */
    uint32_t                    midrx;      /* RX Message ID */
    uint32_t                    midtx;      /* TX Message ID */
    uint32_t                    timestamp;  /* Initial value for local network time */
} CANDEV_AM335X_INIT;

/* General Device Information Structure */
typedef struct candev_am335x_info_entry
{
    uintptr_t                   base;       /* Device Virtual Register Mapping */
    CAN_MSG_OBJ                 *canmsg;    /* Array of CAN message objects */ 
    volatile uint32_t           errstatus;  /* Keep track of CANFIG register status for devctl */
    struct candev_am335x_entry *devlist;   /* Array of all device mailboxes */
    int                         iidmsg;     /* Return iid from InterruptAttach */
    int                         iidsys;     /* Return iid from InterruptAttach */
    int                         iiderr;     /* Return iid from InterruptAttach */
    uint32_t                    numtx;      /* Number of TX Mailboxes */
    uint32_t                    numrx;      /* Number of RX Mailboxes */
    uint32_t                    iflags;     /* Info flags */
    MINICAN_DATA                *mdata;     /* Mini-driver data */
    CAN_MSG_OBJ                 *mcanmsg;   /* Mini-driver buffered CAN messages */
    am335x_time_t				userstamp;	/* Time value set by user */
    am335x_time_t				calibrate;	/* Actual time_t()/clock() value for userstamp */
} CANDEV_AM335X_INFO;

/* Device specific extension of CANDEV struct */
typedef struct candev_am335x_entry
{
    CANDEV                      cdev;       /* CAN Device - MUST be first entry */
    int                         mbxid;      /* Index into mailbox memory */
    volatile uint32_t           dflags;     /* Device specific flags */
    CANDEV_AM335X_INFO         *devinfo;   /* Common device information */
} CANDEV_AM335X;

/* Driver implemented CAN library function prototypes */
void can_drvr_transmit(CANDEV *cdev);
int can_drvr_devctl(CANDEV *cdev, int dcmd, DCMD_DATA *data);

/* Function prototypes */
void can_options(CANDEV_AM335X_INIT *devinit, int argc, char *argv[]);
void can_init_mailbox(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit);
void can_init_hw(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit);
void can_init_intr(CANDEV_AM335X_INFO *devinfo, CANDEV_AM335X_INIT *devinit);
void can_print_reg(CANDEV_AM335X_INFO *devinfo);
void can_print_mailbox(CANDEV_AM335X_INFO *devinfo);
void set_port32(unsigned port, uint32_t mask, uint32_t data);
void can_am335x_debug(CANDEV_AM335X_INFO *devinfo);

#endif


__SRCVERSION( "$URL$ $Rev$" );
