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

#include    "bpfilter.h"
#include    "ti814x.h"

#if NBPFILTER > 0
#include    <net/bpf.h>
#include    <net/bpfdesc.h>
#endif

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static  int ti814x_add_rx_desc(ti814x_dev_t *ti814x, int idx, struct mbuf *new)
{
    off64_t         phys;
   
    ti814x->meminfo.rx_mbuf[idx] = new;
    phys = mbuf_phys(new);
    CACHE_INVAL (&ti814x->meminfo.cachectl, new->m_data, phys, new->m_ext.ext_size);

    ti814x->meminfo.rx_desc[idx].buffer = (unsigned int) phys;
    ti814x->meminfo.rx_desc[idx].off_len = MAX_PKT_SIZE;
    ti814x->meminfo.rx_desc[idx].flag_len = DESC_FLAG_OWN;

    return (EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void ti814x_receive(ti814x_dev_t *ti814x, struct nw_work_thread *wtp)
{
    struct mbuf             *new, *m;
    int32_t                 pkt_len;
    struct ifnet            *ifp;
    int                     cidx;
    uint32_t                status;
    uint32_t                verbose = ti814x->cfg.verbose;

    while (1) {
        cidx = ti814x->rx_cidx;
        status = ti814x->meminfo.rx_desc[cidx].flag_len;
        if (ti814x->num_if == 2) {
            ifp = &ti814x->common_ecom[((status>>16)&0x3)-1]->ec_if;
        } else {
            ifp = &ti814x->ecom.ec_if;
        }

         /* Descriptor is still owned by the port... */   
        if (status & DESC_FLAG_OWN) {
            break;
        }

        pkt_len = status & 0xffff;
        m = ti814x->meminfo.rx_mbuf[cidx];

        /* advance consumer index for the next loop */
        ti814x->rx_cidx = cidx + 1;
        if (ti814x->rx_cidx == ti814x->num_rx_pkts) {
            ti814x->rx_cidx = 0;
        }
        
        /* Get a packet/buffer to replace the one that was filled */
        new = m_getcl_wtp (M_DONTWAIT, MT_DATA, M_PKTHDR, wtp);
        if (new == NULL) {
            if (verbose & DEBUG_MASK) {
                slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d m_getcl_wtp returned NULL",  __FUNCTION__, __LINE__);
            }
            ti814x_add_rx_desc(ti814x, cidx, m);
            ifp->if_ierrors++;
            ti814x->stats.rx_failed_allocs++;
            goto next;
        }
        
        m->m_pkthdr.len = pkt_len;
        m->m_len = pkt_len;
        m->m_pkthdr.rcvif = ifp;
        m->m_flags |= M_HASFCS;     /* length includes 4 byte crc */

#if NBPFILTER > 0
        /* Pass this up to any BPF listeners. */
        if (ifp->if_bpf)
            bpf_mtap (ifp->if_bpf, m);
#endif

        if (verbose & DEBUG_DUMP_RX) {
            int i;
            fprintf(stderr, "pkt dump for m=0x%p\n", m);
            for (i=0; i<m->m_len; i++) {
                fprintf(stderr, "%02X ", (uint8_t)m->m_data[i]);
                if (!(i % 8)) fprintf(stderr, "   ");
                if (!(i % 16)) fprintf(stderr, "\n");
            }
        }
        
        /* pass rxd mbuf up to io-pkt */
        ifp->if_ipackets++;
        (*ifp->if_input)(ifp, m);
        ti814x_add_rx_desc(ti814x, cidx, new);

        next:
        /* Tell the DMA engine that we're done with this descriptor */
        outle32 (ti814x->cpsw_regs + RX0_CP, in32(ti814x->cpsw_regs + RX0_CP));
    } // while
}
