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

#include    "ti814x.h"


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
uint16_t ti814x_mdi_read (void *hdl, uint8_t phyid, uint8_t reg)
{
    ti814x_dev_t    *ti814x = (ti814x_dev_t *) hdl;
    int             timeout = 1000;
    uint16_t        data = 0;
    uint32_t        mdio_useraccess_reg;

    switch(ti814x->cfg.device_index) {
        case 0: mdio_useraccess_reg = MDIOUSERACCESS0; break;
        case 1: mdio_useraccess_reg = MDIOUSERACCESS1; break;
        default: return(0xffff);
    }

    while (timeout) {
        if (! (inle32 (ti814x->cpsw_regs + mdio_useraccess_reg) & GO))
            break;
        nanospin_ns (1000);
        timeout--;
    }
    
    if (timeout <= 0) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d timeout waiting for ready",    __FUNCTION__, __LINE__);
        return (0xffff);
    }
    
    outle32 (ti814x->cpsw_regs + mdio_useraccess_reg, GO | REGADR(reg) | PHYADR(phyid));

    while (timeout) {
        if (! (inle32 (ti814x->cpsw_regs + mdio_useraccess_reg) & GO))
            break;
        nanospin_ns (1000);
        timeout--;
    }
    
    if (timeout <= 0) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d timeout waiting for data",    __FUNCTION__, __LINE__);
        return (0xffff);
    }

    // if (!(inle32(mdio+mdio_useraccess_reg) & ACK))
    //     slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d ACK BIT NOT SET: BAD",    __FUNCTION__, __LINE__);
    data = ((uint16_t) inle32 (ti814x->cpsw_regs + mdio_useraccess_reg) & 0xffff);
    if (ti814x->cfg.verbose & DEBUG_MII_RW) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d devid=%d phyid=%d reg=%d data=0x%04x",
              __FUNCTION__, __LINE__, ti814x->cfg.device_index, phyid, reg, data);
    }
    return data;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void ti814x_mdi_write (void *hdl, uint8_t phyid, uint8_t reg, uint16_t data)
{
    ti814x_dev_t    *ti814x = (ti814x_dev_t *) hdl;
    int             timeout = 1000;
    uint32_t        mdio_useraccess_reg;

    if (ti814x->cfg.verbose & DEBUG_MII_RW) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - devid=%d phyid=%d reg=%d data=0x%04x",
            __FUNCTION__, __LINE__, ti814x->cfg.device_index, phyid, reg, data);
    }

    switch(ti814x->cfg.device_index) {
        case 0: mdio_useraccess_reg = MDIOUSERACCESS0; break;
        case 1: mdio_useraccess_reg = MDIOUSERACCESS1; break;
        default:
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d -Invalid device index %d",
                        __FUNCTION__, __LINE__, ti814x->cfg.device_index);
            return;
    }

    while (timeout) {
        if (! (inle32 (ti814x->cpsw_regs + mdio_useraccess_reg) & GO))
            break;
        nanospin_ns (1000);
        timeout--;
    }
    if (timeout <= 0)
        return;

    outle32 (ti814x->cpsw_regs + mdio_useraccess_reg, GO | WRITE | REGADR(reg) | PHYADR(phyid) | data);

    timeout = 1000;
    while (timeout) {
        if (!(inle32(ti814x->cpsw_regs + mdio_useraccess_reg) & GO))
            break;
        nanospin_ns(1000);
        timeout--;
    }

    if (timeout <= 0) {
         slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - write operation timeout",
          __FUNCTION__, __LINE__);
    }
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void ti814x_mdi_callback (void *handle, uint8_t phyaddr, uint8_t linkstate)
{
    ti814x_dev_t        *ti814x = handle;
    int             i, mode, gig_en=0;
    uint32_t        reg, mac_control_reg;
    char            *s = 0;
    struct ifnet    *ifp = &ti814x->ecom.ec_if;
    nic_config_t    *cfg = &ti814x->cfg;

    if (cfg->verbose & DEBUG_MII) {
        slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d phyaddr=%d linkstate=%d devidx=%d",
                __FUNCTION__, __LINE__, phyaddr, linkstate, cfg->device_index);
    }
    
    switch (linkstate) {
        case    MDI_LINK_UP:
        if ((i = MDI_GetActiveMedia (ti814x->mdi, cfg->phy_addr, &mode)) != MDI_LINK_UP) {
            if (cfg->verbose & DEBUG_MII)
                slogf(_SLOGC_NETWORK, _SLOG_INFO, "MDI_GetActiveMedia returned %x phy_addr: %d", i, cfg->phy_addr);
            mode = 0;
        }   

        switch (mode) {
            case    MDI_10bTFD:
                cfg->media_rate = 10000L;
                break;
            case    MDI_10bT:
                s = "10 BaseT Half Duplex";
                cfg->duplex = 0;
                cfg->media_rate = 10000L;
                break;
            case    MDI_100bTFD:
                s = "100 BaseT Full Duplex";
                cfg->duplex = 1;
                cfg->media_rate = 100000L;
                break;
            case    MDI_100bT:
                s = "100 BaseT Half Duplex";
                cfg->duplex = 0;
                cfg->media_rate = 100000L;
                break;
            case    MDI_100bT4:
                s = "100 BaseT4";
                cfg->duplex = 0;
                cfg->media_rate = 100000L;
                break;
            case MDI_1000bT:
			    s = "1000 BaseT Half Duplex !!!NOT SUPPORTED!!!";
                cfg->duplex = 0;
                cfg->media_rate = 1000 * 1000L;
                gig_en=1;
                break;
            case MDI_1000bTFD:
			    s = "1000 BaseT Full Duplex";
                cfg->duplex = 1;
                cfg->media_rate = 1000 * 1000L;
                gig_en=1;
                break;
            default:
                s = "Unknown";
                cfg->duplex = 0;
                cfg->media_rate = 0L;
                break;
            }
        if (cfg->verbose & DEBUG_MASK) {
            slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:%d - Link up (%s) if=%d", __FUNCTION__, __LINE__, s, cfg->device_index);
        }

        /* Set Duplex on MAC */
        switch (ti814x->cfg.device_index) {
            case 0: mac_control_reg = ti814x->cpsw_regs + SL1_MAC_CONTROL; break;
            case 1: mac_control_reg = ti814x->cpsw_regs + SL2_MAC_CONTROL; break;
            default: 
                slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:%d - Invalid device index (%d)",
                                __FUNCTION__, __LINE__, ti814x->cfg.device_index );
                return;
        }
        reg = inle32 (mac_control_reg);
        if (cfg->duplex)
            reg |= FULLDUPLEX;
        else
            reg &= ~FULLDUPLEX;

        if (cfg->media_rate == 100000L)
            reg |= MACIFCTL_A;
        else
            reg &= ~MACIFCTL_A;

        if (gig_en)
            reg |= GIG_MODE|EXT_EN;
        
        outle32 (mac_control_reg, reg);

        /* Enable ALE and Clear ALE Table */
        outle32(ti814x->cpsw_regs + ALE_CONTROL, ENABLE_ALE|CLEAR_TABLE|ALE_BYPASS);

        cfg->flags &= ~NIC_FLAG_LINK_DOWN;
        ti814x->linkup = 1;
        if_link_state_change(ifp, LINK_STATE_UP);
        break;

    case    MDI_LINK_DOWN:
        cfg->media_rate = cfg->duplex = -1;
        MDI_AutoNegotiate(ti814x->mdi, cfg->phy_addr, NoWait);
        cfg->flags |= NIC_FLAG_LINK_DOWN;
        ti814x->linkup = 0;

        if (cfg->verbose & DEBUG_MASK) {
            slogf(_SLOGC_NETWORK, _SLOG_INFO, "%s:%d: Link down %d",
                  __FUNCTION__, __LINE__, cfg->lan);
        }

        if_link_state_change(ifp, LINK_STATE_DOWN);
        break;

    default:
        slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:%d: Unknown link state %hhu",
               __FUNCTION__, __LINE__, linkstate);
        break;
        }
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void ti814x_MDI_MonitorPhy (void *arg)
{
    ti814x_dev_t *ti814x = arg;

    // Only monitor the link if:
    //   1) the user forces it, or
    //   2) the link state is unknown, or
    //   3) there's no traffic.
   
    if (ti814x->probe_phy ||
       (ti814x->cfg.flags & NIC_FLAG_LINK_DOWN) ||
       (ti814x->cfg.media_rate <= 0) ||
       !ti814x->pkts_received) {

        MDI_MonitorPhy(ti814x->mdi);
    }
    ti814x->pkts_received = 0;

    if (ti814x->stop_miimon)
        callout_stop(&ti814x->mii_callout);
    else
        callout_msec(&ti814x->mii_callout, 3 * 1000, ti814x_MDI_MonitorPhy, ti814x);
}

/*****************************************************************************/
/* MII PHY Interface Routines...                                             */
/*****************************************************************************/
#define NIC_MDI_PRIORITY 10 // mii timer
int ti814x_findphy(ti814x_dev_t *ti814x)
{
    int                 timeout, an_capable, status, rc, i;
    int32_t             phy_found = 0;
    uint16_t            reg;
    volatile uint32_t   phyid;
    uint32_t            devidx;
    nic_config_t        *cfg = &ti814x->cfg;

    if (ti814x->mdi) {
        MDI_DeRegister ((mdi_t **)&ti814x->mdi);
    }

    outle32 (ti814x->cpsw_regs + MDIOCONTROL, MDIO_ENABLE | MDIO_FAULT | MDIO_FAULTENB | MDIO_CLK_DIVISOR);

    if ((rc=MDI_Register_Extended (ti814x, ti814x_mdi_write, ti814x_mdi_read,
                ti814x_mdi_callback, (mdi_t **)&ti814x->mdi, NULL,
                0, 0)) != MDI_SUCCESS)
    {
         slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - MDI_Register_Extended failed rc=%d",
                    __FUNCTION__, __LINE__, rc);
         return(-1);
    }
    callout_init (&ti814x->mii_callout);

    /* Wait for MDIO to be IDLE */
    timeout = 1000;
    while (timeout) {
        if (!(inle32 (ti814x->cpsw_regs + MDIOCONTROL) & MDIO_IDLE))
            break;
        nanospin_ns (1000);
        timeout--;
    }
    if (timeout <= 0) {
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "PHY init timeout");
        return (-1);
    }

     /* Look for an active PHY */
    devidx = ti814x->cfg.device_index;
    /* 4.3 mdio_fs_v1p5p1: Each of the 32 bits of this register (ALIVE) is set if the
     * most recent access to the PHY with address corresponding to the register
     * bit number was acknowledged by the PHY, the bit is reset if the PHY
     * fails to acknowledge the access.
     */
    /* As such we need to iterate through all 32 possible PHYS, read from them
     * and only then will the ALIVE bits be representative of what's actually
     * present on the board */
    for (i = 0; i < 32; i++) {
        ti814x_mdi_read(ti814x, i, MDI_BMSR);
        nanospin_ns (50000);
    }

    devidx = cfg->device_index;
    phyid = inle32 (ti814x->cpsw_regs + MDIOALIVE);
    if (cfg->verbose & DEBUG_MII) {
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "MDIOALIVE=0x%08x", phyid);
    }
    if ((phyid) && (phyid != 0xffffffff)) {
        for (i = 0; i < 32; i++) {
            if (phyid & (1 << i)) {
                if (!devidx)    /* Phy matched device index */
                    break;
                else
                    devidx--;   /* decrement phy index for next loop */
            }
        }
        phyid = i;
        cfg->phy_addr = phyid;
        phy_found = 1;
        if (cfg->verbose & DEBUG_MII) {
            slogf(_SLOGC_NETWORK, _SLOG_ERROR,
                "%s:%d - MII transceiver found at address %d",
                __FUNCTION__, __LINE__, cfg->phy_addr);
        }
    }

    if ((!phy_found) || (phyid==32)) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - No PHY found",
                    __FUNCTION__, __LINE__);
        return (-1);
    }
    

    cfg->connector = NIC_CONNECTOR_MII;
    an_capable = ti814x_mdi_read(ti814x, cfg->phy_addr, MDI_BMSR) & BMSR_AN_ABILITY;

    if (cfg->verbose & DEBUG_MASK) {
        slogf (_SLOGC_NETWORK, _SLOG_INFO,
          "%s:%d: an_capable 0x%X, force_link %d, media_rate %d, duplex %d",
          __FUNCTION__, __LINE__, an_capable, ti814x->force_link, 
          ti814x->cfg.media_rate, ti814x->cfg.duplex);
    }

    /* Permit the MDIO State machine to monitor the link status */
    switch (ti814x->cfg.device_index) {
        case 0: outle32 (ti814x->cpsw_regs + MDIOUSERPHYSEL0, 0); break;
        case 1: outle32 (ti814x->cpsw_regs + MDIOUSERPHYSEL1, 0); break;
        default: slogf (_SLOGC_NETWORK, _SLOG_ERROR, "Invalid Device Index"); break;
    }
    

    if (MDI_InitPhy(ti814x->mdi, phyid) != MDI_SUCCESS) {
        slogf (_SLOGC_NETWORK, _SLOG_ERROR, "InitPhy failed");
        return (-1);
    }

    if (ti814x->force_link != -1 || !an_capable) {
        reg = ti814x_mdi_read(ti814x, ti814x->cfg.phy_addr, MDI_BMCR);

        reg &= ~(BMCR_RESTART_AN | BMCR_SPEED_100 | BMCR_SPEED_1000 | BMCR_FULL_DUPLEX);
        if (an_capable && ti814x->force_link != 0) {
            /*
             * If we force the speed, but the link partner
             * is autonegotiating, there is a greater chance
             * that everything will work if we advertise with
             * the speed that we are forcing to.
             */
            MDI_SetAdvert(ti814x->mdi, ti814x->cfg.phy_addr, ti814x->force_link);

            reg |= BMCR_RESTART_AN | BMCR_AN_ENABLE;

            if (cfg->verbose & DEBUG_MASK) {
                slogf (_SLOGC_NETWORK, _SLOG_INFO,
                    "%s:%d: Restricted autonegotiate (%dMbps only)",
                    __FUNCTION__, __LINE__, 
                    ti814x->cfg.media_rate / 1000);
            }
        }
        else {

            reg &= ~BMCR_AN_ENABLE;

            if (cfg->verbose & DEBUG_MASK) {
                slogf (_SLOGC_NETWORK, _SLOG_INFO,
                    "%s:%d: Forcing the link (%dMbps %s Duplex)", __FUNCTION__, __LINE__,
                    ti814x->cfg.media_rate/1000, (cfg->duplex > 0) ? "Full" : "Half");
            }

            if (cfg->media_rate == 100 * 1000) {
                reg |= BMCR_SPEED_100;
            }
            else {
                if (cfg->media_rate == 1000 * 1000) {
                    reg |= BMCR_SPEED_1000;
                }
            }

            if (cfg->verbose & DEBUG_MII) {
                slogf (_SLOGC_NETWORK, _SLOG_INFO,
                  "%s:%d: writing reg 0x%X to MDI_BMCR", __FUNCTION__, __LINE__, reg);
                }
        }
        ti814x_mdi_write (ti814x, cfg->phy_addr, MDI_BMCR, reg);

        if (reg & BMCR_AN_ENABLE) {
            MDI_EnableMonitor (ti814x->mdi, 1);
        } else {
            MDI_DisableMonitor (ti814x->mdi);
        }
    }
    else {  // not forcing the link
        cfg->flags |= NIC_FLAG_LINK_DOWN;

        if ((inle32(ti814x->cpsw_regs + MDIOLINK) & (1<<phyid)) &&
                ti814x->cfg.verbose & DEBUG_MASK) {
            slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - Link is up link=0x%08x phyid=%d",
                __FUNCTION__, __LINE__, inle32(ti814x->cpsw_regs+MDIOLINK),cfg->phy_addr);
        }

        MDI_AutoNegotiate(ti814x->mdi, cfg->phy_addr, NoWait);

        status = MDI_EnableMonitor(ti814x->mdi, 1);
        if (status != MDI_SUCCESS) {
            slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:%d: MDI_EnableMonitor returned %d",
                   __FUNCTION__, __LINE__, status);
        }
    }
    return(EOK);
}


