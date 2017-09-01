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


#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <gulliver.h>
#include <sys/slog.h>
#include <sys/neutrino.h>
#include <fs/etfs.h>
#include "devio.h"

int internal_eraseblk(struct etfs_devio *dev, unsigned blk);
int internal_markBlkBad(struct etfs_devio *dev, unsigned blk);
int internal_markBlkGood(struct etfs_devio *dev, unsigned blk);
//
// Initialize the part and stuff physical paramaters for the part.
//
int devio_init(struct etfs_devio *dev) {
	uint8_t			id[5];
	CHIPIO			*cio = dev->cio;

	// Allow IO operations
	if(ThreadCtl(_NTO_TCTL_IO, 0) != EOK) {
		dev->log(_SLOG_CRITICAL, "You must be root.");
		// We will not return
	}
	// Do anything special to get the part ready to use
	if(nand_init(dev) != 0) {
		dev->log(_SLOG_CRITICAL, "nand_init failed : %s", strerror(errno));
		// We will not return
	}

	// Reset the part
	nand_write_cmd(cio, NANDCMD_RESET_2048);
	if(nand_wait_busy(cio, MAX_RESET_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on RESET");

	// Read id info from the part
	nand_write_cmd(cio, NANDCMD_IDREAD_2048);
	nand_write_pageaddr(cio, 0, 1, 0);				// Address, 1 cycle
	if (nand_wait_busy(cio, MAX_READ_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on ReadID");
	nand_read_status(cio, id, 5);				// Get Device ID and Configuration Codes

	//dev->log(_SLOG_WARNING, "NAND ID: %2.2x %2.2x %2.2x %2.2x %2.2x",
	//		id[0], id[1], id[2], id[3], id[4]);

	cio->page_2048 = 0;  // Default to 512 byte page.

	switch(id[1]) {
	// A Fake 32M part for testing
	case 0x00:
		dev->numblks = 256;
		cio->addrcycles = 4;
		break;
    // 16M
    case 0x73:
        dev->numblks = 1024;
        cio->addrcycles = 3;
        break;

    // 32M
    case 0x75:
        dev->numblks = 2048;
        cio->addrcycles = 3;
        break;

    // 64M
    case 0x76:
        dev->numblks = 4096;
        cio->addrcycles = 4;
        break;
    // 128M
    case 0x72: case 0x74: case 0x78: case 0x79:
        dev->numblks = 8192;
        cio->addrcycles = 4;
        break;

	// 128M
	case 0xa1: case 0xf1: case 0xb1: case 0xc1:
		dev->numblks = 1024;
		cio->addrcycles = 5;
		cio->page_2048 = 1;
		break;

	// 256M
	case 0xaa: case 0xda: case 0xba: case 0xca:
		dev->numblks = 2048;
		cio->addrcycles = 5;
		cio->page_2048 = 1;
		break;

	// 512M
	case 0xac: case 0xdc: case 0xbc: case 0xcc:
		dev->numblks = 4096;
		cio->addrcycles = 5;
		cio->page_2048 = 1;
		break;

	// 1G
	case 0xd3:
		dev->numblks = 8192 - cio->resvblks;
		cio->addrcycles = 5;
		cio->page_2048 = 1;
		break;

	default:
		dev->log(_SLOG_CRITICAL, "Unsupported NAND device (%2.2x %2.2x)", id[0], id[1]);
		// We will not return
	}

	// These must be initialized here. All other numbers are built from them.
	dev->memtype       = ETFS_MEMTYPE_NAND;

	if ( cio->page_2048 ){
		dev->clustersize   = DATASIZE_2048;
		dev->sparesize     = SPARESIZE_2048;
		dev->clusters2blk  = PAGES2BLK_2048;
		dev->blksize       = (dev->clustersize + SPARESIZE_2048) * dev->clusters2blk;
	}
	else{
		// We glue to physical pages at the driver and report back their combined size
		dev->clustersize   = DATASIZE_512 * PAGES2CLUSTER_512;
		dev->sparesize     = SPARESIZE_512 * PAGES2CLUSTER_512;
		dev->clusters2blk  = PAGES2BLK_512 / PAGES2CLUSTER_512;
		dev->blksize       = (dev->clustersize + SPARESIZE_512) * dev->clusters2blk;
	}
	cio->lastpage = ~0;

	return(EOK);
}

unsigned nand_unlock_blocks(struct etfs_devio *dev, unsigned start, unsigned end)
{
	 CHIPIO	*cio = dev->cio;
	 nand_write_cmd(cio, NANDCMD_UNLOCK_LOW);
	 nand_write_blkaddr(cio, start, 3);
	 nand_write_cmd(cio, NANDCMD_UNLOCK_HIGH);
	 nand_write_blkaddr(cio, end, 3);
	 if(nand_wait_busy(cio, MAX_ERASE_USEC) != 0)
		 dev->log(_SLOG_ERROR,"Timeout on Block unlock\n");

	 return 0;
}

//
// Read a cluster of data.
//
// Verify crc for both the spare area and the entire page (data + spare).
// The passed buffer "buf" is larger than the cluster size. It can hold
// (PAGESIZE bytes. This is for convienence when reading
// data from the device ands calculating the crc. The work area after clustersize
// bytes is ignored by the caller.
//
int devio_readcluster(struct etfs_devio *dev, unsigned cluster, uint8_t *buf, struct etfs_trans *trp) {
	struct spare		*sp;
	struct spare1       *sp1;
	struct spare2       *sp2;
	unsigned            crcdata;
	int					err, err2;
	unsigned			page;
	CHIPIO				*cio = dev->cio;
	uint8_t				ecc[12]; // lower 128MB ecc code
	int					bLowNAND = 0;	// flag used to indicate whether we are writing to the lower part of NAND

	//
	// Read page from the device
	//
	if ( cio->page_2048 ){
		bLowNAND = ((cluster < (NAND_UPPER_BOUND_MIN*PAGES2BLK_2048)) && isTEBPlatform(cio))? 1:0;
		page = cluster;
		nand_write_cmd(cio, NANDCMD_READ_2048);
		nand_write_pageaddr(cio, page, cio->addrcycles, 0);
		nand_write_cmd(cio, NANDCMD_READCONFIRM_2048);

		if(nand_wait_busy(cio,  MAX_READ_USEC) != 0)
			dev->log(_SLOG_CRITICAL, "Timeout on READ");

		// hw ecc for lower nand clusters
		if(bLowNAND)
			nand_reset_hw_ecc(cio);

		nand_read_data(cio, buf, PAGESIZE_2048);

		if(bLowNAND)
			nand_get_hw_ecc(cio, ecc, 0);

		cio->lastpage = page;

		sp = (struct spare *)(buf + DATASIZE_2048);

		if(sp->status != 0xff || sp->status2 != 0xff) {
			dev->log(_SLOG_ERROR, "readtrans BADBLK on cluster %d", cluster);
			trp->tacode = ETFS_TRANS_BADBLK;
		}
		else if(((uint64_t*)sp)[0] == ~0ll && ((uint64_t*)sp)[1] == ~0ll &&
				((uint64_t*)sp)[2] == ~0ll && ((uint64_t*)sp)[3] == ~0ll &&
				((uint64_t*)sp)[5] == ~0ll && ((uint64_t*)sp)[6] == ~0ll &&
				((uint64_t*)sp)[7] == ~0ll
		   )
		{
			if(sp->erasesig[0] == ERASESIG1 && sp->erasesig[1] == ERASESIG2)
				trp->tacode = ETFS_TRANS_ERASED;
			else
				trp->tacode = ETFS_TRANS_FOXES;
		}

		else if(dev->crc32((uint8_t *) sp, sizeof(*sp) - sizeof(sp->crctrans)) != sp->crctrans)
		{
			dev->log(_SLOG_ERROR, "readcluster trans DATAERR on cluster %d", cluster);
			trp->tacode = ETFS_TRANS_DATAERR;
		}

		else
			trp->tacode = ETFS_TRANS_OK;

		// Build transaction data from data in the spare area.
		trp->sequence    = ENDIAN_LE32(sp->sequence);
		trp->fid         = ENDIAN_LE16(sp->fid);
		trp->cluster     = ENDIAN_LE16(sp->clusterlo) + (sp->clusterhi << 16);
		trp->nclusters   = sp->nclusters;

		trp->dacode = ETFS_TRANS_OK;
		if(trp->tacode == ETFS_TRANS_OK)
		{
			if(dev->crc32(buf, DATASIZE_2048) != sp->crcdata)
			{
				if(!bLowNAND)
				{
					// perform ecc correction on upper 128MB for now
					err = dev->ecc(buf, &sp->eccv[0], 8, 1);
					if(err >= ETFS_ECC_ERROR || dev->crc32(buf, DATASIZE_2048) != sp->crcdata)
					{
						dev->log(_SLOG_ERROR, "readcluster DATAERR on cluster %d", cluster);
						return(trp->dacode = ETFS_TRANS_DATAERR);
					}

					dev->log(_SLOG_ERROR, "readcluster ECCERR on cluster %d", cluster);
					return(trp->dacode = ETFS_TRANS_ECC);
				}
				else
				{
					// no ecc correction
					dev->log(_SLOG_ERROR, "readcluster DATAERR on cluster %d", cluster);
					return(trp->dacode = ETFS_TRANS_DATAERR);
				}
			}
		}
		return(trp->tacode);
	}
	else {
		page = cluster * PAGES2CLUSTER_512;
		//
		// Read first page from the device
		//
	    if(cio->lastcmd != NANDCMD_READ_512 || (page & (PAGES2BLK_512-1)) == 0 || cio->lastpage + 1 != page) {
    	    nand_write_cmd(cio, NANDCMD_READ_512);
       	 	cio->inspare = 0;
			nand_write_pageaddr(cio, page, cio->addrcycles, 0);
        }

	    if(nand_wait_busy(cio,  MAX_READ_USEC) != 0)
   		     dev->log(_SLOG_CRITICAL, "Timeout on READ");
    	nand_read_data(cio, buf, PAGESIZE_512);

  		sp1 = (struct spare1 *)(buf + DATASIZE_512);
    	if(sp1->status != 0xff) {
        	dev->log(_SLOG_ERROR, "readtrans BADBLK on cluster %d", cluster);
        	trp->tacode = ETFS_TRANS_BADBLK;

    	} else if(((uint64_t *)sp1)[0] == ~0ll && ((uint32_t *)sp1)[2] == ~0 && sp1->nclusters == 0xff && sp1->crctrans == 0xffff)
        	if(sp1->erasesig == ERASESIG)
            	trp->tacode = ETFS_TRANS_ERASED;
        	else
            	trp->tacode = ETFS_TRANS_FOXES;

	    else if(dev->crc16((uint8_t *) sp1, sizeof(*sp1) - sizeof(sp1->crctrans)) != sp1->crctrans) {
    	    dev->log(_SLOG_ERROR, "readcluster trans DATAERR on cluster %d", cluster);
       		trp->tacode = ETFS_TRANS_DATAERR;

	    } else
    	    trp->tacode = ETFS_TRANS_OK;
    		crcdata = sp1->crcdatalo;

	    // Build transaction data from data in the spare area.
	    trp->sequence    = ENDIAN_LE32(sp1->sequence);
   		trp->fid         = ENDIAN_LE16(sp1->fid);
	    trp->cluster     = ENDIAN_LE16(sp1->clusterlo) + (sp1->clusterhi << 16);
  		trp->nclusters   = sp1->nclusters;

    	//
    	// Second page read from device
    	//
    	if(nand_wait_busy(cio,  MAX_READ_USEC) != 0)
			dev->log(_SLOG_CRITICAL, "Timeout on READ");
		nand_read_data(cio, buf + DATASIZE_512, PAGESIZE_512);
    	cio->lastpage = page + 1;
    	sp2 = (struct spare2 *) (buf + 2*DATASIZE_512);
    	crcdata |= sp2->crcdatahi << 16;

    	trp->dacode = ETFS_TRANS_OK;
    	if(trp->tacode == ETFS_TRANS_OK) {
        	if(dev->crc32(buf, 2*DATASIZE_512) != crcdata) {
	            err = dev->ecc(buf, &sp2->ecc1[0], 1, 1);
    	        err2 = dev->ecc(buf + 256, &sp2->ecc234[0], 3, 1);
        	    if(err >= ETFS_ECC_ERROR || err2 >= ETFS_ECC_ERROR || dev->crc32(buf, 2*DATASIZE_512) != crcdata) {
           	    	dev->log(_SLOG_ERROR, "readcluster DATAERR on cluster %d", cluster);
            	    return(trp->dacode = ETFS_TRANS_DATAERR);
	            }

    	        dev->log(_SLOG_ERROR, "readcluster ECCERR on cluster %d", cluster);
        	    return(trp->dacode = ETFS_TRANS_ECC);
        	}
    	}

    	return(trp->tacode);
	}
}


//
// Read the spare area of a page (not the data) to return transaction information.
//
// This called is used heavily on startup to process the transactions. It is
// a cheaper call than readcluster() since it reads less data and has a smaller
// checksum to calculate.
//
int devio_readtrans(struct etfs_devio *dev, unsigned cluster, struct etfs_trans *trp) {
	struct spare		spare;
	struct spare1		spare1;
	unsigned			page;
	CHIPIO				*cio = dev->cio;

	if ( cio->page_2048 ){
		page = cluster;
		nand_write_cmd(cio, NANDCMD_READ_2048);
		nand_write_pageaddr(cio, page, cio->addrcycles, 1);
		nand_write_cmd(cio, NANDCMD_READCONFIRM_2048);

		if(nand_wait_busy(cio, MAX_READ_USEC) != 0)
			dev->log(_SLOG_CRITICAL, "Timeout on READ");
		nand_read_data(cio, (uint8_t *)&spare, sizeof spare);

		cio->lastpage = page;

		if(spare.status != 0xff || spare.status2 != 0xff) {
			dev->log(_SLOG_ERROR, "readtrans BADBLK on cluster %d", cluster);
			return(ETFS_TRANS_BADBLK);
		}
		else
		{
			// spare[4] contains the erase signature and thus should not be 0xFF
			if(	((uint64_t*)&spare)[0] == ~0ll && ((uint64_t*)&spare)[1] == ~0ll &&
				((uint64_t*)&spare)[2] == ~0ll && ((uint64_t*)&spare)[3] == ~0ll &&
				((uint64_t*)&spare)[5] == ~0ll && ((uint64_t*)&spare)[6] == ~0ll &&
				((uint64_t*)&spare)[7] == ~0ll
			   )
			{
				if(spare.erasesig[0] == ERASESIG1 && spare.erasesig[1] == ERASESIG2)
				{
					return(ETFS_TRANS_ERASED);
				}

				else if(spare.erasesig[0] != 0xFF || spare.erasesig[1] != 0xFF)
				{
					return(ETFS_TRANS_FOXES);
				}
			}

			else if(dev->crc32((uint8_t *) &spare, sizeof(spare) - sizeof(spare.crctrans)) != spare.crctrans)
			{
				dev->log(_SLOG_ERROR, "readtrans DATAERR on cluster %d", cluster);
				return(ETFS_TRANS_DATAERR);
			}

			trp->sequence    = ENDIAN_LE32(spare.sequence);
			trp->fid         = ENDIAN_LE16(spare.fid);
			trp->cluster     = ENDIAN_LE16(spare.clusterlo) + (spare.clusterhi << 16);
			trp->nclusters   = spare.nclusters;

			return(ETFS_TRANS_OK);
		}
	}
	else{
		page = cluster * PAGES2CLUSTER_512;
		// We pipeline reads to the spare area since we pretty much
	    // read it sequentially as a scan on startup.
	    if(cio->lastcmd != NANDCMD_SPAREREAD_512 || (page & (PAGES2BLK_512-1)) == 0 || cio->lastpage + 1 != page) {
    	    nand_write_cmd(cio, NANDCMD_SPAREREAD_512);
       		cio->inspare = 1;
        	nand_write_pageaddr(cio, page, cio->addrcycles, 0);
   		}

    	if(nand_wait_busy(cio, MAX_READ_USEC) != 0)
        	dev->log(_SLOG_CRITICAL, "Timeout on READ");

	    nand_read_data(cio, (uint8_t *)&spare1, sizeof(spare1));
    	cio->lastpage = page;

	    if(spare1.status != 0xff) {
    	    dev->log(_SLOG_ERROR, "readtrans BADBLK on cluster %d", cluster);
       		return(ETFS_TRANS_BADBLK);
    	}

	    if(((uint64_t *)&spare1)[0] == ~0ll && ((uint32_t *)&spare1)[2] == ~0 && spare1.nclusters == 0xff && spare1.crctrans == 0xffff) {
    	    if(spare1.erasesig == ERASESIG) {
        	    return(ETFS_TRANS_ERASED);
    	    } else {
    	        return(ETFS_TRANS_FOXES);
    	    }
	    }

	    if(dev->crc16((uint8_t *) &spare1, sizeof(spare1) - sizeof(spare1.crctrans)) != spare1.crctrans) {
    	    dev->log(_SLOG_ERROR, "readtrans DATAERR on cluster %d", cluster);
	        return(ETFS_TRANS_DATAERR);
    	}

	    trp->sequence    = ENDIAN_LE32(spare1.sequence);
   	 	trp->fid         = ENDIAN_LE16(spare1.fid);
    	trp->cluster     = ENDIAN_LE16(spare1.clusterlo) + (spare1.clusterhi << 16);
	    trp->nclusters   = spare1.nclusters;

		return(ETFS_TRANS_OK);
	}
}


//
// Post a cluster of data.
//
// Set crc for both the spare area and the entire page (data + spare).
// The passed buffer "buf" is larger than the cluster size. It can hold
// PAGESIZE bytes. This is for convienence writing
// data to the device ands calculating the crc. The work area after clustersize
// bytes is ignored by the caller.
//
//	NOTE: 	Both erase and write for the lower 128MB of NAND SLC need to go through this function.
//			If a post/erase is being performed on the lower 128MB of NAND (where IPL and IFS are located)
//			you need to unlock the bottom first. To do that, etfs_trans (trp) needs to contain the action
//			(MAGIC_OP_ERASE or MAGIC_OP_WRITE) in trp->fid followed by the unlock key in trp->cluster. The
//			optional force_ecc debug parameter is also added to the last 4 bits of trp->fid if needed.
//
//			If operation is erase, the function internally calls internal_devio_eraseblk and erases
//			the ONE block located in cluster. If operation is write, the function unlocks the block
//			which cluster belongs to
//
int devio_postcluster(struct etfs_devio *dev, unsigned cluster, uint8_t *buf, struct etfs_trans *trp) {
	struct spare		*sp;
	struct spare1		*sp1;
	struct spare2		*sp2;
	uint8_t				status;
	unsigned			page;
	unsigned			crcdata;
	int					bLowNAND;	// flag used to indicate whether we are writing to the lower part of NAND
	int 				force_ecc;
	CHIPIO				*cio = dev->cio;
	int					ret;

	bLowNAND = 0;	// set to false
	force_ecc = 0;
	if( cio->page_2048 )
	{
		if(trp)
		{
			// Determine whether it's a write/erase to the lower portion of NAND SLC
			if(MAGIC_NUMBER == trp->cluster)
			{
				bLowNAND = 1;	// set flag to true

				// unlock the block
				nand_unlock_blocks(dev, cluster>>6, cluster>>6);

				// Determine whether it is a write or erase
				if( MAGIC_OP_ERASE == (trp->fid & 0xfffffff0) )
				{
					// erase the block
					if ( (ret = internal_eraseblk(dev, cluster>>6)) != ETFS_TRANS_OK)
					{
						dev->log(_SLOG_ERROR,"Could not erase blk = %d, which is in the lower part of NAND", cluster>>6);
						nand_unlock_blocks(dev, NAND_UPPER_BOUND_MIN, NAND_UPPER_BOUND_MAX);
						return ret;
					}
					memset(buf, 0xff, PAGESIZE_2048);
					trp = NULL;
				}
				else if(MAGIC_OP_MRK_BLK_BAD == (trp->fid & 0xfffffff0))
				{
				    // mark block as bad
				    if( (ret = internal_markBlkBad(dev, cluster>>6)) != 0)
				    {
				    	dev->log(_SLOG_ERROR, "Could not mark blk = %d as bad", cluster>>6);
				    	nand_unlock_blocks(dev, NAND_UPPER_BOUND_MIN, NAND_UPPER_BOUND_MAX);
				    	return (ETFS_TRANS_DEVERR);
				    }
				    return ETFS_TRANS_OK;
				}
				else if(MAGIC_OP_MRK_BLK_GOOD == (trp->fid & 0xfffffff0))
				{
				    // mark block as bad
				    if( (ret = internal_markBlkGood(dev, cluster>>6)) != 0)
				    {
				    	dev->log(_SLOG_ERROR, "Could not mark blk = %d as good", cluster>>6);
				    	nand_unlock_blocks(dev, NAND_UPPER_BOUND_MIN, NAND_UPPER_BOUND_MAX);
				    	return (ETFS_TRANS_DEVERR);
				    }
				    return ETFS_TRANS_OK;
				}
				else if( MAGIC_OP_WRITE != (trp->fid & 0xfffffff0) )
				{
					// invalid code sent. Lock the lower portions
					dev->log(_SLOG_ERROR, "Invalid code set. Lower NAND was NOT unlocked");
					nand_unlock_blocks(dev, NAND_UPPER_BOUND_MIN, NAND_UPPER_BOUND_MAX);
					bLowNAND = 0;
				}
				else
					force_ecc = trp->fid & 1;
			}
		}

		page = cluster;

		// Build spare area
		sp = (struct spare *) (buf + DATASIZE_2048);
		memset((void *)sp, 0xff, sizeof(*sp));
		sp->erasesig[0] = ERASESIG1;
		sp->erasesig[1] = ERASESIG2;

		if(trp)
		{
			sp->sequence   = ENDIAN_LE32(trp->sequence);
			sp->fid        = ENDIAN_LE16((uint16_t) trp->fid);
			sp->clusterlo  = ENDIAN_LE16((uint16_t) trp->cluster);
			sp->clusterhi  = trp->cluster >> 16;
			sp->nclusters  = trp->nclusters;
			sp->status     = 0xff;
			sp->status2    = 0xff;
			sp->crcdata    = dev->crc32(buf, DATASIZE_2048);

			if(!bLowNAND)
				// Perform software ECC on upper NAND only
				dev->ecc(buf, &sp->eccv[0], 8, 0);
		}

		nand_write_cmd(cio, NANDCMD_PROGRAM_2048);
		nand_write_pageaddr(cio, page, cio->addrcycles, 0);

		if(bLowNAND && trp)	// perform hardware ECC on lower NAND only
			nand_reset_hw_ecc(cio);

		// write the data portion first
		nand_write_data(cio, buf, DATASIZE_2048);

		if(bLowNAND && trp)
			nand_get_hw_ecc(cio, &sp->eccv[0][0], force_ecc);	//write ECC for lower nand

		if(trp)
		{
			// now that all ECC values and other spare parts are populated, calculate crc of the spare section
			sp->crctrans   = dev->crc32((uint8_t *) sp, sizeof(*sp) - sizeof(sp->crctrans));

			if(cluster % dev->clusters2blk == 0)
			{
				sp->erasesig[0] = ~0;	// Can only punch bits down once and we did it
				sp->erasesig[1] = ~0;	// on the erase
			}
		}

		// write the buffer portion
		nand_write_data(cio, &buf[DATASIZE_2048], SPARESIZE_2048);
		nand_write_cmd(cio, NANDCMD_PROGRAMCONFIRM_2048);

		if(nand_wait_busy(cio, MAX_POST_USEC) != 0)
			dev->log(_SLOG_CRITICAL, "Timeout on POST");

		nand_write_cmd(cio, NANDCMD_STATUSREAD_2048);		// Read status
		nand_read_status(cio, &status, 1);


		if((status & 0xC3) != 0xC0) {
			// Error on write! Mark the block containing this page as BAD
			dev->log(_SLOG_ERROR, "Post error on page %d", page);

			if(bLowNAND) {
				internal_markBlkBad(dev, cluster>>6);
				nand_unlock_blocks(dev, NAND_UPPER_BOUND_MIN, NAND_UPPER_BOUND_MAX);
			}

			return(ETFS_TRANS_DEVERR);
		}

		if(bLowNAND) 			//unlock the top 128MB again
			nand_unlock_blocks(dev, NAND_UPPER_BOUND_MIN, NAND_UPPER_BOUND_MAX);

		return(ETFS_TRANS_OK);
	}
	else{
		page = cluster * PAGES2CLUSTER_512;
	    // Build second spare area
	    sp2 = (struct spare2 *) (buf + 2*DATASIZE_512);
	    memset((void *)sp2, 0xff, sizeof(*sp2));
   		dev->ecc(buf, &sp2->ecc1[0], 1, 0);
  		dev->ecc(buf + 256, &sp2->ecc234[0], 3, 0);

	    // Build first spare area
   		sp1 = (struct spare1 *) (buf + 2*DATASIZE_512 + SPARESIZE_512);
	    memset((void *)sp1, 0xff, sizeof(*sp1));
	    if(trp) {
    	    sp1->erasesig   = 0;
       		sp1->sequence   = ENDIAN_LE32(trp->sequence);
       		sp1->fid        = ENDIAN_LE16((uint16_t) trp->fid);
	        sp1->clusterlo  = ENDIAN_LE16((uint16_t) trp->cluster);
   		    sp1->clusterhi  = trp->cluster >> 16;
	        sp1->nclusters  = trp->nclusters;
    	    crcdata         = dev->crc32(buf, 2*DATASIZE_512);
	        sp1->crcdatalo  = crcdata;
	        sp2->crcdatahi  = crcdata >> 16;
    	    sp1->crctrans   = dev->crc16((uint8_t *) sp1, sizeof(*sp1) - sizeof(sp1->crctrans));
    	}
    	else
	        sp1->erasesig = ERASESIG;

	    // Write first page of data
   		// This forces us out of the spare area.
	    if(cio->inspare) {
	        nand_write_cmd(cio, NANDCMD_READ_512);
	        cio->inspare = 0;
	    }
	    nand_write_cmd(cio, NANDCMD_PROGRAM_512);
   		nand_write_pageaddr(cio, page, cio->addrcycles, 0);
	    nand_write_data(cio, buf, DATASIZE_512);
	    nand_write_data(cio, (uint8_t *)sp1, SPARESIZE_512);
	    nand_write_cmd(cio, NANDCMD_PROGRAMCONFIRM_512);
	    if(nand_wait_busy(cio, MAX_POST_USEC) != 0)
    	    dev->log(_SLOG_CRITICAL, "Timeout on POST");
	    nand_write_cmd(cio, NANDCMD_STATUSREAD_512);
   	 	nand_read_data(cio, &status, 1);
	    if((status & 0x01) != 0) {
    	    dev->log(_SLOG_ERROR, "Post error on page %d", page);
        	return(ETFS_TRANS_DEVERR);
    	}

	    // Our work is done if called from devio_eraseblk()
    	if(trp == NULL)
        	return(ETFS_TRANS_OK);

	    // Write second page of data
	    ++page;
	    nand_write_cmd(cio, NANDCMD_PROGRAM_512);
	    nand_write_pageaddr(cio, page, cio->addrcycles, 0);
	    nand_write_data(cio, buf + DATASIZE_512, PAGESIZE_512);
   		nand_write_cmd(cio, NANDCMD_PROGRAMCONFIRM_512);
	    if(nand_wait_busy(cio, MAX_POST_USEC) != 0)
    	    dev->log(_SLOG_CRITICAL, "Timeout on POST");
    	nand_write_cmd(cio, NANDCMD_STATUSREAD_512);
    	nand_read_data(cio, &status, 1);

    	if((status & 0x01) != 0) {
       		dev->log(_SLOG_ERROR, "Post error on page %d", page);
  		    return(ETFS_TRANS_DEVERR);
    	}
    	return(ETFS_TRANS_OK);
	}
}


//
// Erase a block.
//
int devio_eraseblk(struct etfs_devio *dev, unsigned blk) {
	uint8_t			status;
	CHIPIO			*cio = dev->cio;
	uint8_t			*buf ;

	if ( cio->page_2048 ){
		if(blk < NAND_UPPER_BOUND_MIN && isTEBPlatform(cio))
		{
			// Cannot erase lower NAND by calling this function. Refer to devio_postcluster for more information
			return ETFS_TRANS_OK;
		}
		buf = alloca(PAGESIZE_2048);
		// Erase the blk
		nand_write_cmd(cio, NANDCMD_ERASE_2048);
		nand_write_blkaddr(cio, blk, cio->addrcycles);		// Actually 3 Address Cycles for NAND2048 device
		nand_write_cmd(cio, NANDCMD_ERASECONFIRM_2048);
		if(nand_wait_busy(cio, MAX_ERASE_USEC) != 0)
			dev->log(_SLOG_CRITICAL, "Timeout on ERASE");
		nand_write_cmd(cio, NANDCMD_STATUSREAD_2048);
		nand_read_status(cio, &status, 1);
	}
	else{
		buf = alloca(PAGESIZE_512);
	    nand_write_cmd(cio, NANDCMD_ERASE_512);
	    nand_write_blkaddr(cio, blk, cio->addrcycles);
   		nand_write_cmd(cio, NANDCMD_ERASECONFIRM_512);
    	if(nand_wait_busy(cio, MAX_ERASE_USEC) != 0)
       		dev->log(_SLOG_CRITICAL, "Timeout on ERASE");
	    nand_write_cmd(cio, NANDCMD_STATUSREAD_512);
    	nand_read_data(cio, &status, 1);

    }
	if((status & 0xC3) != 0xC0) {
		dev->log(_SLOG_ERROR, "erase error on blk %d (%2.2x)", blk, status);
		return(ETFS_TRANS_DEVERR);
	}
	// Write the erase signature. We only write non FFs in the first 16 bytes of the
	// spare area and put FFs everywhere else. This is required for
	// multilevel NAND devices.
	if ( cio->page_2048 )
		memset(buf, 0xff, PAGESIZE_2048);
	else
		memset(buf, 0xff, PAGESIZE_512);
	status = devio_postcluster(dev, blk * dev->clusters2blk, buf, NULL);
	return(status);
}

/**
 * This is the internal eraseblk function which is not accessible to
 * the outside world. This function is called if an erase is requested
 * to the lower 128MB of the SLC NAND. The request goes through devio_postcluster
 * and if the magic key is provided correctly, this function is called.
 *
 * The locking and unlocking is done in postcluster and not here. This way, if one tries to
 * call this function by mistake, they cannot erase the lower portion because it is still locked.
 */
int internal_eraseblk(struct etfs_devio *dev, unsigned blk)
{
	uint8_t			status;
	CHIPIO			*cio = dev->cio;

	// Erase the blk
	nand_write_cmd(cio, NANDCMD_ERASE_2048);
	nand_write_blkaddr(cio, blk, cio->addrcycles);		// Actually 3 Address Cycles for NAND2048 device
	nand_write_cmd(cio, NANDCMD_ERASECONFIRM_2048);
	if(nand_wait_busy(cio, MAX_ERASE_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on ERASE");
	nand_write_cmd(cio, NANDCMD_STATUSREAD_2048);
	nand_read_status(cio, &status, 1);

	if((status & 0xC3) != 0xC0) {
		dev->log(_SLOG_ERROR, "erase error on blk %d (%2.2x)", blk, status);
		return(ETFS_TRANS_DEVERR);
	}

	return ETFS_TRANS_OK;
}

/**
 * This function marks a block as BAD by writing to the first
 * 2 bytes of the spare area of the first page of the input block.
 *
 * This function does not lock/unlock the block and therefore the calling
 * function must make sure the block is unlocked.
 *
 * returns 0 if marking was successful and -1 otherwise.
 */
int internal_markBlkBad(struct etfs_devio *dev, unsigned blk)
{
	uint8_t			status;
	CHIPIO			*cio = dev->cio;
	uint8_t*		blkStatus;

	blkStatus = alloca(2);

	/* 	Mark Block as bad. Find the first page of the block
	 *  and write zero to the status bytes
	*/

	nand_write_cmd(cio, NANDCMD_PROGRAM_2048);
	nand_write_pageaddr(cio, blk*PAGES2BLK_2048, cio->addrcycles, 1);

	blkStatus[0] = 0x00;
	blkStatus[1] = 0x00;

	// write only to the status bytes
	nand_write_data(cio, blkStatus, 2);
	nand_write_cmd(cio, NANDCMD_PROGRAMCONFIRM_2048);

	if(nand_wait_busy(cio, MAX_POST_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on POST");

	nand_write_cmd(cio, NANDCMD_STATUSREAD_2048);		// Read status
	nand_read_status(cio, &status, 1);

	if((status & 0xC3) != 0xC0) {
		dev->log(_SLOG_ERROR, "Unable to mark blk %d as bad (%x)",blk, status);
		return -1;
	}

	return 0;
}

/**
 * FOR TESTING ONLY!!!!!
 *
 * This function marks a block as GOOD by writing 0xFF to the first
 * 2 bytes of the spare area of the first page of the input block.
 *
 * This function does not lock/unlock the block and therefore the calling
 * function must make sure the block is unlocked.
 *
 * returns 0 if marking was successful and -1 otherwise.
 *
 */
int internal_markBlkGood(struct etfs_devio *dev, unsigned blk)
{
	uint8_t			status;
	CHIPIO			*cio = dev->cio;
	uint8_t*		blkStatus;

	blkStatus = alloca(2);

	/* 	Mark Block as bad. Find the first page of the block
	 *  and write zero to the status bytes
	*/

	nand_write_cmd(cio, NANDCMD_PROGRAM_2048);
	nand_write_pageaddr(cio, blk*PAGES2BLK_2048, cio->addrcycles, 1);

	blkStatus[0] = 0xff;
	blkStatus[1] = 0xff;

	// write only to the status bytes
	nand_write_data(cio, blkStatus, 2);
	nand_write_cmd(cio, NANDCMD_PROGRAMCONFIRM_2048);

	if(nand_wait_busy(cio, MAX_POST_USEC) != 0)
		dev->log(_SLOG_CRITICAL, "Timeout on POST");

	nand_write_cmd(cio, NANDCMD_STATUSREAD_2048);		// Read status
	nand_read_status(cio, &status, 1);

	if((status & 0xC3) != 0xC0) {
		dev->log(_SLOG_ERROR, "Unable to mark blk %d as good (%x)",blk, status);
		return -1;
	}

	return 0;
}

//
// Called to allow the driver to flush any cached data that
// has not be written to the device. The NAND class driver does
// not need it.
//
int devio_sync(struct etfs_devio *dev) {
	return(-1);
}

