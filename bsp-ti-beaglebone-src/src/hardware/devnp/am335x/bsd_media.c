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


#include <ti814x.h>
#include <sys/malloc.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <device_qnx.h>


#include <sys/mman.h>


//
// this is a callback, made by the bsd media code.  We passed
// a pointer to this function during the ifmedia_init() call
// in bsd_mii_initmedia()
//
void
bsd_mii_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
    ti814x_dev_t *ti814x = ifp->if_softc;

    ti814x->bsd_mii.mii_media_active = IFM_ETHER;
    ti814x->bsd_mii.mii_media_status = IFM_AVALID;

    if (ti814x->force_link) {

        if (ti814x->linkup) {
            ti814x->bsd_mii.mii_media_status |= IFM_ACTIVE;
        }

        // report back the previously forced values
        switch(ti814x->cfg.media_rate) {
            case 0:
            ti814x->bsd_mii.mii_media_active |= IFM_NONE;
            break;  

            case 1000*10:
            ti814x->bsd_mii.mii_media_active |= IFM_10_T;
            break;

            case 1000*100:
            ti814x->bsd_mii.mii_media_active |= IFM_100_TX;
            break;

            case 1000*1000:
            ti814x->bsd_mii.mii_media_active |= IFM_1000_T;
            break;

            default:    // this shouldnt really happen, but ...
            ti814x->bsd_mii.mii_media_active |= IFM_NONE;
            break;
        }
        if (ti814x->cfg.duplex) {
            ti814x->bsd_mii.mii_media_active |= IFM_FDX;
        }

        // we dont set flow ctrl bits in mii_media_active

    } else if (ti814x->linkup) {  // link is auto-detect and up

        ti814x->bsd_mii.mii_media_status |= IFM_ACTIVE;

        switch(ti814x->cfg.media_rate) {
            case 1000*10:
            ti814x->bsd_mii.mii_media_active |= IFM_10_T;
            break;

            case 1000*100:
            ti814x->bsd_mii.mii_media_active |= IFM_100_TX;
            break;

            case 1000*1000:
            ti814x->bsd_mii.mii_media_active |= IFM_1000_T;
            break;

            default:    // this shouldnt really happen, but ...
            ti814x->bsd_mii.mii_media_active |= IFM_NONE;
            break;
        }

        if (ti814x->cfg.duplex) {
            ti814x->bsd_mii.mii_media_active |= IFM_FDX;
        }

        // these media state variables are set by the link interrupt
        if (ti814x->pause_xmit || ti814x->pause_receive) {
            ti814x->bsd_mii.mii_media_active |= IFM_FLOW;
        
            if (ti814x->pause_xmit) {
                ti814x->bsd_mii.mii_media_active |= IFM_ETH_TXPAUSE;
            }
    
            if (ti814x->pause_receive) {
                ti814x->bsd_mii.mii_media_active |= IFM_ETH_RXPAUSE;
            }
        }

        // could move this to event.c so there was no lag
        ifmedia_set(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO);

    } else {    // link is auto-detect and down
        ti814x->bsd_mii.mii_media_active |= IFM_NONE;
        ti814x->bsd_mii.mii_media_status = 0;

        // could move this to event.c so there was no lag
        ifmedia_set(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
    }

    // stuff parameter values with hoked-up bsd values
    ifmr->ifm_status = ti814x->bsd_mii.mii_media_status;
    ifmr->ifm_active = ti814x->bsd_mii.mii_media_active;
}


//
// this is a callback, made by the bsd media code.  We passed
// a pointer to this function during the ifmedia_init() call
// in bsd_mii_initmedia().  This function is called when
// someone makes an ioctl into us, we call into the generic
// ifmedia source, and it make this callback to actually 
// force the speed and duplex, just as if the user had
// set the cmd line options
//
int
bsd_mii_mediachange(struct ifnet *ifp)
{
    ti814x_dev_t        *ti814x         = ifp->if_softc;
    int             old_media_rate  = ti814x->cfg.media_rate;
    int             old_duplex      = ti814x->cfg.duplex;
    int             old_force_link  = ti814x->force_link;
    struct ifmedia  *ifm            = &ti814x->bsd_mii.mii_media;
    int             user_duplex     = ifm->ifm_media & IFM_FDX ? 1 : 0;
    int             user_media      = ifm->ifm_media & IFM_TMASK;

    if (!(ifp->if_flags & IFF_UP)) {
        slogf(_SLOGC_NETWORK, _SLOG_WARNING,
          "%s(): network interface isn't up, ioctl ignored", __FUNCTION__);
        return 0;
    }

    if (!(ifm->ifm_media & IFM_ETHER)) {
        slogf(_SLOGC_NETWORK, _SLOG_WARNING,
          "%s(): network interface - bad media: 0x%X", 
          __FUNCTION__, ifm->ifm_media);
        return 0;   // should never happen
    }

    switch (user_media) {
        case IFM_AUTO:      // auto-select media
        ti814x->force_link        =  0;
        ti814x->cfg.media_rate    = -1;
        ti814x->cfg.duplex        = -1;
        ifmedia_set(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO);
        break;

        case IFM_NONE:      // disable media
        //
        // forcing the link with a speed of zero means to disable the link
        //
        ti814x->force_link        = 1;
        ti814x->cfg.media_rate    = 0;
        ti814x->cfg.duplex        = 0;
        ifmedia_set(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
        break;

        case IFM_10_T:      // force 10baseT
        ti814x->force_link        = 1;
        ti814x->cfg.media_rate    = 10 * 1000;
        ti814x->cfg.duplex        = user_duplex;
        ifmedia_set(&ti814x->bsd_mii.mii_media,
          user_duplex ? IFM_ETHER|IFM_10_T|IFM_FDX : IFM_ETHER|IFM_10_T);
        break;

        case IFM_100_TX:    // force 100baseTX
        ti814x->force_link        = 1;
        ti814x->cfg.media_rate    = 100 * 1000;
        ti814x->cfg.duplex        = user_duplex;
        ifmedia_set(&ti814x->bsd_mii.mii_media,
          user_duplex ? IFM_ETHER|IFM_100_TX|IFM_FDX : IFM_ETHER|IFM_100_TX);
        break;

        case IFM_1000_T:    // force 1000baseT
        // only GigE full duplex supported
        ti814x->force_link        = 1;
        ti814x->cfg.media_rate    = 1000 * 1000;
        ti814x->cfg.duplex        = user_duplex;
        ifmedia_set(&ti814x->bsd_mii.mii_media,
          user_duplex ? IFM_ETHER|IFM_1000_T|IFM_FDX : IFM_ETHER|IFM_1000_T);
        break;

        default:            // should never happen
        slogf(_SLOGC_NETWORK, _SLOG_WARNING,
          "%s(): network interface - unknown media: 0x%X", 
          __FUNCTION__, user_media);
        return 0;
        break;
    }

    // does the user want something different than it already is?
    if ((ti814x->cfg.media_rate != old_media_rate)    ||
        (ti814x->cfg.duplex     != old_duplex)        ||
        (ti814x->force_link     != old_force_link)    ||
        (ti814x->cfg.flags      &  NIC_FLAG_LINK_DOWN) ) {
        
        // re-initialize hardware with new parameters
        ifp->if_init(ifp);

    }
    return 0;
}


//
// called from ti814x_attach() in init.c to hook up
// to the bsd media structure.  Not entirely unlike kissing
// a porcupine, we must do so carefully, because we do not
// want to use the bsd mii management structure, because
// this driver uses link interrupt
//
void
bsd_mii_initmedia(ti814x_dev_t *ti814x)
{
    ti814x->bsd_mii.mii_ifp = &ti814x->ecom.ec_if;

    ifmedia_init(&ti814x->bsd_mii.mii_media, IFM_IMASK, bsd_mii_mediachange,
      bsd_mii_mediastatus);

    // we do NOT call mii_attach() - we do our own link management

    //
    // must create these entries to make ifconfig media work
    // see lib/socket/public/net/if_media.h for defines
    //

    // ifconfig wm0 none (x22)
    ifmedia_add(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);

    // ifconfig wm0 auto (x20)
    ifmedia_add(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_AUTO, 0, NULL);

    // ifconfig wm0 10baseT (x23 - half duplex)
    ifmedia_add(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_10_T, 0, NULL);

    // ifconfig wm0 10baseT-FDX (x100023)
    ifmedia_add(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);

    // ifconfig wm0 100baseTX (x26 - half duplex)
    ifmedia_add(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX, 0, NULL);

    // ifconfig wm0 100baseTX-FDX (x100026 - full duplex)
    ifmedia_add(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);

    // ifconfig wm0 1000baseT (x30 - half duplex)
    ifmedia_add(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T, 0, NULL);

    // ifconfig wm0 1000baseT mediaopt fdx (x100030 - full duplex)
    ifmedia_add(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);

    // add more entries to support flow control via ifconfig media

    // link is initially down
    ifmedia_set(&ti814x->bsd_mii.mii_media, IFM_ETHER|IFM_NONE);
}


