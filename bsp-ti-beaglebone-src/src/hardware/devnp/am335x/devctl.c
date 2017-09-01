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

#include    <ti814x.h>
#include    <net/ifdrvcom.h>
#include    <sys/sockio.h>


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void ti814x_filter(ti814x_dev_t *ti814x)
{
    struct ethercom         *ec;
    struct ether_multi      *enm;
    struct ether_multistep  step;
    struct ifnet            *ifp;
    //int                     mcentries = 0;              

    slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d", __FUNCTION__, __LINE__);
    ec = &ti814x->ecom;
    ifp = &ec->ec_if;

    ifp->if_flags &= ~IFF_ALLMULTI;

    ETHER_FIRST_MULTI (step, ec, enm);
    while (enm != NULL) {
        if (memcmp (enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN) != 0) {
            ifp->if_flags |= IFF_ALLMULTI;
        }
        slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d", __FUNCTION__, __LINE__);
        ti814x_ale_add_mcast(ti814x, enm->enm_addrlo, 0);
        ETHER_NEXT_MULTI (step, enm);
    }
    slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d", __FUNCTION__, __LINE__);
    //omap35xx_flush_mcast_bits (omap35xx);
}

#if DM814x
/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void ti814x_dump_stats (ti814x_dev_t *ti814x)
{
    nic_ethernet_stats_t    *estats = &ti814x->stats.un.estats;
    nic_stats_t             *stats = &ti814x->stats;

    stats->rxed_ok              = inle32(ti814x->cpsw_regs + RXGOODFRAMES);
    stats->rxed_broadcast       = inle32(ti814x->cpsw_regs + RXBROADCASTFRAMES);
    stats->rxed_multicast       = inle32(ti814x->cpsw_regs + RXMULTICASTFRAMES);
    stats->octets_rxed_ok       = inle32(ti814x->cpsw_regs + RXOCTETS);

    stats->txed_ok              = inle32 (ti814x->cpsw_regs+ TXGOODFRAMES);
    stats->txed_broadcast       = inle32 (ti814x->cpsw_regs+ TXBROADCASTFRAMES);
    stats->txed_multicast       = inle32 (ti814x->cpsw_regs+ TXMULTICASTFRAMES);
    stats->octets_txed_ok       = inle32 (ti814x->cpsw_regs+ TXOCTETS);

    estats->align_errors        = inle32 (ti814x->cpsw_regs+ RXALIGNCODEERRORS);
    estats->single_collisions   = inle32 (ti814x->cpsw_regs+ TXSINGLECOLLFRAMES);
    estats->multi_collisions    = inle32 (ti814x->cpsw_regs+ TXMULTCOLLFRAMES);
    estats->fcs_errors          = inle32 (ti814x->cpsw_regs+ RXCRCERRORS);
    estats->tx_deferred         = inle32 (ti814x->cpsw_regs+ TXDEFERREDFRAMES);
    estats->late_collisions     = inle32 (ti814x->cpsw_regs+ TXLATECOLLISION);
    estats->xcoll_aborted       = inle32 (ti814x->cpsw_regs+ TXEXCESSIVECOLLISIONS);
    estats->no_carrier          = inle32 (ti814x->cpsw_regs+ TXCARRIERSENSEERRORS);
    estats->oversized_packets   = inle32 (ti814x->cpsw_regs+ RXOVERSIZEDFRAMES);
    estats->jabber_detected     = inle32 (ti814x->cpsw_regs+ RXJABBERFRAMES);
    estats->short_packets       = inle32 (ti814x->cpsw_regs+ RXUNDERSIZEDFRAMES);
    estats->total_collision_frames 
                                = inle32 (ti814x->cpsw_regs+TXCOLLISIONFRAMES);
}
#endif

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int ti814x_ioctl(struct ifnet * ifp, unsigned long cmd, caddr_t data)
{
    int                     error = 0;
    ti814x_dev_t                *ti814x = ifp->if_softc;
    struct drvcom_config    *dcfgp;
    struct drvcom_stats     *dstp;
    struct ifdrv_com        *ifdc;

    switch (cmd) {
        case    SIOCGDRVCOM:
            ifdc = (struct ifdrv_com *)data;
            switch (ifdc->ifdc_cmd) {
                case    DRVCOM_CONFIG:
                    dcfgp = (struct drvcom_config *)ifdc;

                    if (ifdc->ifdc_len != sizeof(nic_config_t)) {
                        error = EINVAL;
                        break;
                    }
                    memcpy(&dcfgp->dcom_config, &ti814x->cfg, sizeof(ti814x->cfg));
                    break;

                case    DRVCOM_STATS:
                    dstp = (struct drvcom_stats *)ifdc;

                    if (ifdc->ifdc_len != sizeof(nic_stats_t)) {
                        error = EINVAL;
                        break;
                        }
#if DM814X
                    ti814x_dump_stats(ti814x);
#endif
                    memcpy(&dstp->dcom_stats, &ti814x->stats, sizeof(ti814x->stats));
                    break;

                default:
                    error = ENOTTY;
                }
            break;


        case    SIOCSIFMEDIA:
        case    SIOCGIFMEDIA: {
            struct ifreq *ifr = (struct ifreq *)data;

            error = ifmedia_ioctl(ifp, ifr, &ti814x->bsd_mii.mii_media, cmd);
            break;
            }

        default:
            error = ether_ioctl(ifp, cmd, data);
            if (error == ENETRESET) {
                /*
                * Multicast list has changed; set the
                * hardware filter accordingly.
                */
                if ((ifp->if_flags_tx & IFF_RUNNING) == 0) {
                    /* Interface is currently down */
                } else {
                    //ti814x_filter(ti814x);
                }
                error = 0;
            }
            break;
        }
    return error;
}
