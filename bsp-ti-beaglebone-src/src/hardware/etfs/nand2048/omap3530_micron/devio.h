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


#include <stdint.h>

//
// The spare area used for the NAND. It is 64 bytes in size.
// 2K page parts
//
struct spare {
	uint8_t		status;				// Factory marking for bad blks (0xff == GOOD)
	uint8_t		status2;			// For 16 bit wide parts
	uint8_t		eccv[8][3];			// ecc used in the custom etfs driver
	uint8_t		align[6];
	uint32_t	erasesig[2];		// The erase signature created by devio_eraseblk

	// This must start on a 16 byte boundry because you can only write to a 16
	// byte region once for the new multilevel NAND parts.
	uint8_t		unused[6];
	uint8_t		nclusters;			// Number of clusters
	uint8_t		clusterhi;			// High order 8 bits of cluster
	uint32_t	sequence;			// Sequence number
	uint16_t	fid;				// File id
	uint16_t	clusterlo;			// Low order 16 bits of logical cluster
	//uint8_t	eccv[8][3];			// ECCs for page data
	uint32_t	crcdata;			// Crc for readcluster()
	uint32_t	crctrans;			// Crc for readtrans().
};
#define ERASESIG1	0x756c7769
#define ERASESIG2	0x70626d66

/**
 * The lower 128MB of NAND SLC are protected and hence in order
 * to write to it, one must provide the action along with the key
 * to unlocking the lower portion of NAND SLC
 *
 * etfs_trans.fid contains the operation that is requested by the user
 * and etfs_trans.cluster contains the four byte unlock key and finally
 *
 * The variable force_ecc which is used for testing is in the lower nibble
 * of the last byte of etfs_trans.fid.
 */

#define MAGIC_NUMBER				0x3E45AF9B
#define MAGIC_OP_WRITE				0x68AB7140
#define	MAGIC_OP_ERASE				0x91FF5720
#define MAGIC_OP_MRK_BLK_BAD		0xAD4E39C0
#define MAGIC_OP_MRK_BLK_GOOD		0x74F53DC0
#define MAGIC_FORCE_ECC				0x1

#define NAND_UPPER_BOUND_MIN		1028
#define NAND_UPPER_BOUND_MAX		2047

//
// The spare area used for the NAND. It is 16 bytes in size. We combine
// two pages to give us a 1K cluster and a 32 byte effective spare area.
// 512 byte page parts.
//
struct spare1 {
    uint32_t    sequence;           // Sequence number
    uint8_t     clusterhi;          // High order 8 bits of cluster
    uint8_t     status;             // Factory set - FF means good. Must be offset 6!!!
    uint16_t    fid;                // File id
    uint16_t    clusterlo;          // Low order 16 bits of logical cluster
    uint16_t    crcdatalo;          // Low order 16 bits of crc for readcluster()
    uint8_t     nclusters;          // Number of clusters
    uint8_t     erasesig;
    uint16_t    crctrans;           // Crc for readtrans()
};
#define ERASESIG    0xA0

struct spare2 {
    uint16_t    crcdatahi;          // High order 16 bits of crc
    uint8_t     ecc1[1][3];         // ECC for 1st 256 bytes
    uint8_t     status;             // Factory set - FF means good
    uint8_t     ecc234[3][3];       // ECC for 2nd,3rd,4th 256 bytes
    uint8_t     unused1;
};


// This struct is included in the chipio structure defined in the low level board driver
struct _chipio {
	unsigned	addrcycles;
	unsigned	lastcmd;
	unsigned	lastpage;
	unsigned	inspare;
	unsigned	page_2048;
	unsigned	resvblks;			// reserved last n blocks from the top memory end of NAND device
 };

#ifndef CHIPIO
#define CHIPIO struct _chipio
#endif

//
// Nand device specific data structures
// 2K page size parts
//
#define NANDCMD_SPAREREAD_2048			0x50
#define NANDCMD_READ_2048				0x00
#define NANDCMD_READCONFIRM_2048     	0x30
#define NANDCMD_PROGRAM_2048			0x80
#define NANDCMD_PROGRAMCONFIRM_2048		0x10
#define NANDCMD_ERASE_2048				0x60
#define NANDCMD_ERASECONFIRM_2048		0xD0
#define NANDCMD_IDREAD_2048				0x90
#define NANDCMD_STATUSREAD_2048			0x70
#define NANDCMD_RESET_2048				0xFF
#define NANDCMD_UNLOCK_LOW				0x23
#define NANDCMD_UNLOCK_HIGH				0x24


#define DATASIZE_2048					2048
#define SPARESIZE_2048					64
#define PAGESIZE_2048					(DATASIZE_2048 + SPARESIZE_2048)
#define PAGES2BLK_2048					64

#define NAND_BLK_PER_IFS				256
//
// Nand device specific data structures
// 512 byte pagesize
//
#define NANDCMD_SPAREREAD_512       0x50
#define NANDCMD_READ_512            0x00
#define NANDCMD_PROGRAM_512         0x80
#define NANDCMD_PROGRAMCONFIRM_512  0x10
#define NANDCMD_ERASE_512           0x60
#define NANDCMD_ERASECONFIRM_512    0xD0
#define NANDCMD_IDREAD_512          0x90
#define NANDCMD_STATUSREAD_512      0x70
#define NANDCMD_RESET_512           0xFF

#define DATASIZE_512        		512
#define SPARESIZE_512       		16
#define PAGESIZE_512    			(DATASIZE_512 + SPARESIZE_512)
#define PAGES2BLK_512       		32
#define PAGES2CLUSTER_512   		2

// These timeouts are very generous.
#define MAX_RESET_USEC				600			// 600us
#define MAX_READ_USEC				150			// 150us
#define MAX_POST_USEC				2000		//   2ms
#define MAX_ERASE_USEC				10000		//  10ms


// Prototypes for chip interface
int  nand_init(struct etfs_devio *dev);
int  nand_wait_busy(CHIPIO *cio, uint32_t usec);
void nand_write_pageaddr(CHIPIO *cio, unsigned page, int addr_cycles, int spare);
void nand_write_blkaddr(CHIPIO *cio, unsigned blk, int addr_cycles);
void nand_write_cmd(CHIPIO *cio, int command);
void nand_write_data(CHIPIO *cio, uint8_t *databuffer, int data_cycles);
void nand_read_data(CHIPIO *cio, uint8_t *databuffer, int data_cycles);
void nand_read_status(CHIPIO *cio, uint8_t *databuffer, int data_cycles);
void nand_reset_hw_ecc(CHIPIO *cio);
void nand_get_hw_ecc(CHIPIO *cio, uint8_t* ecc_code, int force_ecc);
uint8_t isTEBPlatform(CHIPIO *cio);

