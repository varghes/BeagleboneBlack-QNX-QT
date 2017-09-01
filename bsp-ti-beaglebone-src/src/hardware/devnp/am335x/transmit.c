/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems.
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

#include    "bpfilter.h"
#include    "ti814x.h"
#include    <sys/trace.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

int phregs[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};

void display_regs(ti814x_dev_t *ti814x, uint8_t phyid)
{
	uint16_t r;
	int      i, x;

	fprintf(stderr, "Device index %d\n", ti814x->cfg.device_index);
    x = ti814x->cfg.device_index;
    ti814x->cfg.device_index = 1;

	for (i=0; i<sizeof(phregs)/sizeof(int); i++)
	{
		r = ti814x_mdi_read (ti814x, phyid, phregs[i]);
		fprintf(stderr, "[%d] #PHY# reg %d: %x\n", sizeof(phregs)/sizeof(int), phregs[i], r);
	}
    ti814x->cfg.device_index = x;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static struct mbuf *ti814x_defrag (struct mbuf *m)
{
    struct mbuf *m2;

    
    MGET (m2, M_DONTWAIT, MT_DATA);
    if (m2 == NULL) {
    	m_freem (m);
        return (NULL);
    }

    M_COPY_PKTHDR (m2, m);

    MCLGET (m2, M_DONTWAIT);
    if ((m2->m_flags & M_EXT) == 0) {
        m_freem (m);
        m_freem (m2);
        return (NULL);
    }

    /* Paranoid ? */
    if (m->m_pkthdr.len > m2->m_ext.ext_size) {
        m_freem (m);
        m_freem (m2);
        return (NULL);
    }

    m_copydata (m, 0, m->m_pkthdr.len, mtod(m2, caddr_t));
    m2->m_pkthdr.len = m2->m_len = m->m_pkthdr.len;

    m_freem(m);
    
    return (m2);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void ti814x_reap_pkts(ti814x_dev_t *ti814x)
{
    int                     idx;
    struct mbuf             *m;
    struct ifnet            *ifp = &ti814x->ecom.ec_if;
    cppi_desc_t             *desc;

    /* Exit if there's no need to recover used buffers */
    if ((ti814x->tx_q_len == 0) && (ti814x->tx_pidx == ti814x->tx_cidx)) {
        return;
    }

    do {
        idx = ti814x->tx_cidx;
        desc = &ti814x->meminfo.tx_desc[idx];
         
        /* Don't free memory owned by DMA */
        if ((desc->flag_len & DESC_FLAG_SOP)) {
            if (desc->flag_len & DESC_FLAG_OWN){
                break;
            }
        }

        if ((m = ti814x->tx_mbuf[idx]) != NULL) {
            ti814x->tx_mbuf[idx] = NULL;
            ti814x->tx_freed++;
            m_freem (m);
            ifp->if_opackets++;
        }
        ti814x->tx_q_len--;
        ti814x->tx_cidx += 1;
        if (ti814x->tx_cidx == ti814x->num_tx_pkts) {
            ti814x->tx_cidx = 0;
        }
    } while (ti814x->tx_cidx != ti814x->tx_pidx);
}


/*****************************************************************************/
/*  Dump out the contents of a fragment from QNX network stack               */
/*****************************************************************************/
void ba_dump( ti814x_dev_t *ti814x, struct mbuf *m )
{
	int i, c;
	int count;
	int32_t			idx;
	cppi_desc_t		*desc;
	struct mbuf 	*m2;

	fprintf( stderr, "[BA] **** %s ****\n", __FUNCTION__ );

	idx = ti814x->tx_pidx;
	desc = &(ti814x->meminfo.tx_desc[idx]);

	fprintf( stderr, "[BA] ti814x ->\n" );
	fprintf( stderr, "[BA]   tx_pidx     = %d\n", ti814x->tx_pidx );
	fprintf( stderr, "[BA]   num_tx_pkts = %d\n", ti814x->num_tx_pkts );
	fprintf( stderr, "[BA]   tx_q_len    = %d\n", ti814x->tx_q_len );

	count = 0;
	m2 = m;
    while ( m2 != NULL )
    {
    	fprintf( stderr, "[BA] -----------\n" );
    	fprintf( stderr, "[BA] count     = %d\n", count );
    	fprintf( stderr, "[BA] m2        = 0x%08x\n", (uint32_t)m2 );
    	fprintf( stderr, "[BA] m_next    = 0x%08x\n", (uint32_t)m2->m_next );
    	fprintf( stderr, "[BA] m_len     = %d\n", m2->m_len );
    	fprintf( stderr, "[BA] m_data    = 0x%08x\n", (uint32_t)m2->m_data );
    	fprintf( stderr, "[BA] m_owner   = 0x%08x\n", (uint32_t)m2->m_owner );
    	fprintf( stderr, "[BA] m_type    = 0x%08x\n", (uint32_t)m2->m_type );
    	fprintf( stderr, "[BA] m_flags   = 0x%08x\n", (uint32_t)m2->m_flags );
    	fprintf( stderr, "[BA] m_nextpkt = 0x%08x\n", (uint32_t)m2->m_nextpkt );
    	fprintf( stderr, "[BA] m_page    = 0x%08x\n", (uint32_t)m2->m_page );

		fprintf( stderr, "[BA] pkt dump for m = 0x%08x\n", (uint32_t)m2 );
		fprintf( stderr, "[BA] m->m_data = 0x%08x\n", (uint32_t)m2->m_data );
		c = 0;
		for (i=0; i<m2->m_len; i++)
		{
			fprintf(stderr, "%02X ", (uint8_t)m2->m_data[i]);
			c ++;
			if (c == 16)
			{
				fprintf(stderr, "\n");
				c = 0;
			}
		}

    	m2 = m2->m_next;
    	count ++;
    	if ( c != 0 )
    	{
    		fprintf( stderr, "\n" );
    	}
    }
    fprintf( stderr, "\n" );
	fprintf( stderr, "[BA] *********\n" );
}



/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void    ti814x_start (struct ifnet *ifp)
{
    ti814x_dev_t                    *ti814x = ifp->if_softc;
    struct mbuf                     *m, *m2;
    off64_t                         phys;
    int32_t                         idx, idx_start;
    int32_t                         num_free, first = 1;
    int32_t                         num_frags;
    struct _iopkt_self              *iopkt = ti814x->iopkt;
    struct nw_work_thread           *wtp = WTP;
    cppi_desc_t                     *desc, *start_desc;
    int32_t                         len, next_idx;
    uint32_t                        start_phys;
    uint32_t                        devidx = ti814x->cfg.device_index;
    uint32_t                        hdp_idx;
    int32_t							total_length;

    if ((ifp->if_flags_tx & IFF_RUNNING) == 0)
    {
        NW_SIGUNLOCK_P (&ifp->if_snd_ex, ti814x->iopkt, wtp);
        return;
    }

    if (!ti814x->linkup)
    {
        NW_SIGUNLOCK_P (&ifp->if_snd_ex, ti814x->iopkt, wtp);
        return;
    }

    ti814x->start_running = 1;
    ifp->if_flags_tx |= IFF_OACTIVE;

    /* reap packets if we're running low */
    num_free = ti814x->num_tx_pkts - ti814x->tx_q_len;
//    fprintf( stderr, "[BA] num_free=%d  DEFRAG_LIMIT=%d\n", num_free, DEFRAG_LIMIT );
    if (num_free < DEFRAG_LIMIT)
    {
        ti814x_reap_pkts(ti814x);
        /* recalculate num_free post-reaping */
        num_free = ti814x->num_tx_pkts - ti814x->tx_q_len;
//        fprintf( stderr, "[BA] num_free    = %d\n", num_free );
//        fprintf( stderr, "[BA] num_tx_pkts = %d\n", ti814x->num_tx_pkts );
//        fprintf( stderr, "[BA] tx_q_len    = %d\n", ti814x->tx_q_len );
        /* Check again for available descriptors */
        if (!num_free)
        {
            ifp->if_flags_tx |= IFF_OACTIVE;
            ti814x->stats.tx_failed_allocs++;
            goto done;
        }
    }

    /* Grab an outbound packet/mbuf chain */
    IFQ_DEQUEUE (&ifp->if_snd, m);
    /* If none are available break out of the loop */
    if (m == NULL)
    {
        goto done;
    }
    
//    ba_dump( ti814x, m );

    do
    {
    	if ( m->m_len == 0 )
    	{
//    		fprintf( stderr, "[BA] #### !! Weirdness 0 !! ##### m=0x%08x length=0\n", m );
    		break;
    	}

        /* Initialize descriptor tracking variables */
        idx_start = idx = ti814x->tx_pidx;
//        fprintf( stderr, "[BA] idx_start    = %d\n", idx_start );
//        fprintf( stderr, "[BA] idx          = %d\n", idx );
//        fprintf( stderr, "[BA] ti814x->tx_pidx    = %d\n", ti814x->tx_pidx );
        first = 1;
        start_desc = desc = &ti814x->meminfo.tx_desc [idx_start];
        if (ti814x->tx_mbuf[idx] == NULL)
        {
            ti814x->tx_mbuf[idx] = m;
        } 

        /* verify the descriptor isn't currently being used by the DMA engine */
        if (desc->flag_len & DESC_FLAG_SOP)
        {
            if (desc->flag_len & DESC_FLAG_OWN)
            {
                m_free(m);
                goto done;
            }
        }
        desc->flag_len = 0x00000000;
        desc->off_len = 0x00000000;
        desc->buffer = 0x00000000;
        desc->next = 0x00000000;
        start_phys = ti814x->meminfo.tx_phys + (sizeof (cppi_desc_t) * idx);
        m->m_nextpkt = NULL;
        
        /* Count the number of fragments */
        /* [BA] exclude zero length fragments just upset the hardware. */
        m2 = m;
        num_frags = 0;
        total_length = m2->m_pkthdr.len;
        while ( m2 != NULL )
        {
        	if ( m2->m_len > 0 )
        	{
        		num_frags ++;
        	}
        	else
        	{
//        		fprintf( stderr, "[BA] #### !! Weirdness 1 !! ##### m2=0x%08x length=0\n", m2 );
        	}
            m2 = m2->m_next;
        }
//        fprintf( stderr, "[BA] num_frags    = %d\n", num_frags );
//        fprintf( stderr, "[BA] total_length = %d\n", total_length );
//        fprintf( stderr, "[BA] DEFRAG_LIMIT = %d\n", DEFRAG_LIMIT );
              
        /* The packet is massively fragmented or we're lacking the
         * resource to send it, defrag it */
        if ((num_frags > DEFRAG_LIMIT) || (num_free < num_frags))
        {
        	m2 = ti814x_defrag(m);
            if (m2 == NULL)
            {
                NW_EX_LK (&ifp->if_snd_ex, ti814x->iopkt);
                m_free(m);
                goto done;
            }

            /* Re-adjust the number of fragments and total length */
            m2 = m;
            num_frags = 0;
            total_length = m2->m_pkthdr.len;
            while ( m2 != NULL )
            {
            	if ( m2->m_len > 0 )
            	{
            		num_frags ++;
            	}
            	else
            	{
//            		fprintf( stderr, "[BA] #### !! Weirdness 2 !! ##### m2=0x%08x length=0\n", m2 );
            	}
                m2 = m2->m_next;
            }
        }
//		fprintf( stderr, "[BA] setting up CPPI descriptors\n" );
//    	fprintf( stderr, "[BA] num_frags = %d\n", num_frags );

        /* Setting up the CPPI Descriptors.. one loop iteration for every fragment... */
        for (m2 = m; m2; m2 = m2->m_next)
        {
//        	fprintf( stderr, "[BA] m2 = 0x%08x\n", m2 );
//        	fprintf( stderr, "[BA] m2->m_len = %d\n", m2->m_len );
            if (!m2->m_len)
            {
//            	fprintf( stderr, "[BA] m2->m_len = %d,  continuing...\n", m2->m_len );
//            	fprintf( stderr, "[BA] #### !! Weirdness 3 !! ##### m=0x%08x length=0\n", m2 );
                num_frags--;
                continue;
            }
//            fprintf( stderr, "[BA] num_frags = %d\n", num_frags );
            phys = mbuf_phys (m2);
            CACHE_FLUSH (&ti814x->meminfo.cachectl, m2->m_data, phys, m2->m_len);

            /* Build the descriptor chaining if there is more than one fragment */
            if ( num_frags > 1 )
            {
                next_idx = idx + 1;
                if (next_idx == ti814x->num_tx_pkts)
                {
                    next_idx = 0;
                }
                desc->next = ti814x->meminfo.tx_phys + (sizeof (cppi_desc_t) * next_idx);
            }
            else
            {
                desc->next = 0;
            }
            /* Set the buffer pointer to the actual physical memory*/
            desc->buffer = (unsigned int) phys;
            
            /* Data contained in just this part of the frame, not the total frame size */
            desc->off_len = m2->m_len & 0x00001fff;
//            fprintf( stderr, "[BA] off_len  = %d\n", desc->off_len );
//            fprintf( stderr, "[BA] first    = %d\n", first );
//            fprintf( stderr, "[BA] m2->m_pkthdr.len = %d\n", m2->m_pkthdr.len );

            /* packet length is only valid on the start-of-packet descriptor */
            if (first)
            {

            	len = m2->m_pkthdr.len & 0x1fff;
                if (len < MIN_PKT_SIZE)
                {
//                	fprintf( stderr, "[BA] setting len = MIN_PKT_SIZE(%d)\n", MIN_PKT_SIZE );
                    len = MIN_PKT_SIZE;
                }
                desc->flag_len = len | DESC_FLAG_OWN;
                first = 0;
            }
            else
            {
                /* Only valid on SOP */
                desc->flag_len = 0;
            }
/*
            fprintf( stderr, "[BA] :: \n" );
            fprintf( stderr, "[BA] desc = 0x%08x\n", desc );
            fprintf( stderr, "[BA]   next     = 0x%08x\n", desc->next );
            fprintf( stderr, "[BA]   buffer   = 0x%08x\n", desc->buffer );
            fprintf( stderr, "[BA]   off_len  = 0x%08x\n", desc->off_len );
            fprintf( stderr, "[BA]     off    = %d\n", 0xffff & (desc->off_len >> 16) );
            fprintf( stderr, "[BA]     len    = %d\n", 0xffff & (desc->off_len >> 0) );
            fprintf( stderr, "[BA]   flag_len = 0x%08x\n", desc->flag_len );
            fprintf( stderr, "[BA]     flag   = 0x%04x\n", 0xffff & (desc->flag_len >> 16) );
            fprintf( stderr, "[BA]     len    = %d\n", 0xffff & (desc->flag_len >> 0) );
            fprintf( stderr, "[BA] == \n" );
*/
            /* Finished the current descriptor, now move onto the next descriptor... */
            ti814x->tx_q_len++;
            idx += 1;
            if (idx == ti814x->num_tx_pkts)
            {
                idx = 0;
            }
            desc = &ti814x->meminfo.tx_desc [idx];

//            fprintf( stderr, "[BA] idx      = %d\n", idx );
//            fprintf( stderr, "[BA] ti814x->num_tx_pkts = %d\n", ti814x->num_tx_pkts );

            /* Verify the descriptor is available */
            if (desc->flag_len & DESC_FLAG_SOP)
            {
                if (desc->flag_len & DESC_FLAG_OWN)
                {
                	// [BA] Warning: no descriptor available!!
                    goto done;
                }
            }
        }   /* end for loop - fragment iteration */

        /* Return [desc] pointer to the previous descriptor */
        ti814x->tx_pidx = idx;
        idx -= 1;
        if (idx < 0)
        {
            idx = ti814x->num_tx_pkts - 1;
        }
//        fprintf( stderr, "[BA] ti814x->tx_pidx      = %d\n", ti814x->tx_pidx );
//        fprintf( stderr, "[BA] idx      = %d\n", idx );

        /* The descriptor is ready to be sent */
        /* Mark the last descriptor as end-of-packet and NULL terminate the descriptor chain */
        desc = &ti814x->meminfo.tx_desc [idx];
        desc->flag_len |= DESC_FLAG_EOP;
        desc->next = NULL;
        start_desc->flag_len |= (DESC_FLAG_OWN | DESC_FLAG_SOP | DESC_FLAG_TOPORT_EN |
                                    (((devidx+1)&0x3) << 16));
//    	fprintf( stderr, "[BA] start_desc->flag_len = 0x%08x\n", start_desc->flag_len );

        /* [BA 2012/02/09]
         * The minimum size of an Ethernet frame is 64 bytes (including the 4 byte CRC)
         * which means 60 bytes of header + payload.  The QNX network stack passes
         * us each Ethernet frame as 1 or more (usually 2) sections (header + data)
         * from which we generate hardware descriptors.  Each hardware descriptor has
         * a length, the total of which must add up to at least 60 bytes.  When a small
         * frame is passed (< 60 bytes) we must add padding bytes to bring the total
         * frame length up to 60 bytes - this is not done automatically by TI's hardware!
         * We do this by adding zero padding to just the LAST descriptor, which is what
         * the following code fragment does.  */
        if ( total_length < MIN_PKT_SIZE )
		{
        	len = desc->off_len & 0x1fff;
//        	fprintf( stderr, "[BA] Patching length... \n" );
//        	fprintf( stderr, "[BA]   m2->m_pkthdr.len = %d. \n", total_length );
//        	fprintf( stderr, "[BA]   old len  = %d. \n", len );
        	len += (MIN_PKT_SIZE - total_length);
        	desc->off_len = (desc->off_len & 0xffff0000) | len;
//        	fprintf( stderr, "[BA]   new len  = %d. \n", len );
//        	fprintf( stderr, "[BA]   off_len = %d. \n", desc->off_len );
		}

        /* Find an available DMA channel */
        /* [BA] Warning: Potential stall here if all slots are busy! */
        hdp_idx = TX0_HDP;
        while (in32(ti814x->cpsw_regs + hdp_idx) != 0x00000000)
        {
            hdp_idx += 4; /* Move to the next TX head DMA pointer */

            if (hdp_idx > TX7_HDP)
                hdp_idx = TX0_HDP;
        }
//        fprintf( stderr, "[BA] hdp_idx = %d\n", hdp_idx );

//        fprintf(stderr, "[1]hdp_idx %d: %x\n", hdp_idx, in32(ti814x->cpsw_regs + hdp_idx));
        out32(ti814x->cpsw_regs + hdp_idx, start_phys);
//        fprintf(stderr, "[2]hdp_idx %d: %x\n", hdp_idx, in32(ti814x->cpsw_regs + hdp_idx));

/*       {
			int i, c;
			fprintf(stderr, "pkt dump for m=0x%p\n", m);
			fprintf(stderr, "m->m_data=0x%p\n", m->m_data);

			c = 0;
			for (i=0; i<m->m_len; i++)
			{
				fprintf(stderr, "%02X ", (uint8_t)m->m_data[i]);
				c ++;
				if (c == 16)
				{
					fprintf(stderr, "\n");
					c = 0;
				}
			}
	    	if ( c != 0 )
	    	{
	    		fprintf( stderr, "\n" );
	    	}
       }
*/
//        display_regs(ti814x, 0);

#if NBPFILTER > 0
        /* Pass the packet to any BPF listeners */
        if (ifp->if_bpf)
        {
            bpf_mtap (ifp->if_bpf, m);
        }
#endif
        /* Grab an outbound packet/mbuf chain */
        IFQ_DEQUEUE (&ifp->if_snd, m);
    } while(m!=NULL); /* end while(1) */
done:

    ti814x->start_running = 0;
    ifp->if_flags_tx &= ~IFF_OACTIVE;
    NW_SIGUNLOCK_P (&ifp->if_snd_ex, iopkt, wtp);
}
