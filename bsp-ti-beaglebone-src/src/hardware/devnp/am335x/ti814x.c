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

#include <ti814x.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <device_qnx.h>
#include <sys/mman.h>

int ti814x_entry(void *dll_hdl, struct _iopkt_self *iopkt, char *options);
int ti814x_parse_options (ti814x_dev_t *device, const char *optstring, nic_config_t *cfg);

struct _iopkt_drvr_entry IOPKT_DRVR_ENTRY_SYM(ti814x) = IOPKT_DRVR_ENTRY_SYM_INIT(ti814x_entry);

#ifdef VARIANT_a
#include <nw_dl.h>
/* This is what gets specified in the stack's dl.c */
struct nw_dll_syms device_syms[] = {
        {"iopkt_drvr_entry", &IOPKT_DRVR_ENTRY_SYM(ti814x)},
        {NULL, NULL}
};
#endif

struct attach_args {
    struct _iopkt_self  *iopkt;
    uint32_t            busvendor;
    uint32_t            busdevice;
    uint32_t            num_if;
    int32_t             busindex;
    void                *dll_hdl;
    char                *options;
    nic_config_t        cfg;
    uintptr_t           cpsw_base;
    uintptr_t           cppi_base;
    uintptr_t           cmr_base;
    struct ethercom     *common_ecom[2];
    meminfo_t           meminfo;
    int32_t				ethif;
};

int32_t  ti814x_attach(struct device *, struct device *, void *);
int32_t  ti814x_detach(struct device *, int how);
CFATTACH_DECL(ti814x,
    sizeof(ti814x_dev_t),
    NULL,
    ti814x_attach,
    ti814x_detach,
    NULL);


/* No options yet, place holder */
static char *ti814x_opts[] = {
    NULL
};


int32_t  ti814x_init(struct ifnet *);
int32_t  ti814x_setup_descriptors(uintptr_t cppi_base, meminfo_t *meminfo);
int32_t  ti814x_hw_config(uintptr_t cpsw_regs, struct attach_args *attach_args);
void     ti814x_stop(struct ifnet *ifp, int disable);
void     ti814x_hw_reset(uintptr_t cpsw_regs);
int32_t  ti814x_read_mac_addr(ti814x_dev_t *ti814x);
void     ti814x_sliver_reset(uintptr_t cpsw_regs, uint8_t sid);
void     ti814x_shutdown(void *);

static uint32_t run_once = 1;

/*****************************************************************************/
/* ti814x_mapmem  : map in register and buffer descriptor memories           */
/*		- memories are common to both interfaces which is why it is done at    */
/*         such an early stage. Pointers are later propagated to interface   */
/*         handles in the dev_attach function.									     */
/*****************************************************************************/
int32_t ti814x_mapmem(struct attach_args *args, nic_config_t *cfg)
{
    uint32_t verbose = 0; /* verbose option is not active yet, variable needed to enable */
    int32_t  err;

    /* Map in CPSW  base */
    args->cpsw_base = mmap_device_io(cfg->io_window_size[0], cfg->io_window_base[0]);
    if (args->cpsw_base == (uintptr_t)MAP_DEVICE_FAILED) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d: Unable to mmap_device_io() errno=%d",
                __FUNCTION__, __LINE__, errno);
        err = errno;
        return(err);
    } else {
        args->cppi_base = args->cpsw_base + 0x2000;
        /* Display hardware versions if Verbose is enabled */
        if (verbose) {
            uint32_t id_ver = in32(args->cpsw_base + CPSW_ID_VER);
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d CPSW mapped to 0x%08x",
                    __FUNCTION__, __LINE__, args->cpsw_base);
            slogf(_SLOGC_NETWORK, _SLOG_INFO,
                    "%s:%d: CPSW ID:0x%02x RTL_VER:0x%02x MAJOR:0x%01x MINOR:0x%01x ",
                    __FUNCTION__, __LINE__, CPSW_ID_VER_ID(id_ver), CPSW_ID_VER_RTLVER(id_ver),
                    CPSW_ID_VER_MAJOR(id_ver), CPSW_ID_VER_MINOR(id_ver) );
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d CPPI RAM mapped to 0x%08x",
                    __FUNCTION__, __LINE__, args->cppi_base);
        }
    }

    args->cmr_base = mmap_device_io(cfg->io_window_size[1], cfg->io_window_base[1]);
    if (args->cmr_base == (uintptr_t)MAP_FAILED) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d: Unable to mmap_device_io() errno=%d",
                __FUNCTION__, __LINE__, errno);
        err = errno;
        /* Unmap CPSW regs now as the driver will abort after returning an error */
        munmap_device_io(args->cpsw_base, TI814X_CPSW_SIZE);
        return(err);
    } else if (verbose) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d CMR Registers mapped to 0x%08x",
              __FUNCTION__, __LINE__, args->cmr_base);
    }
    return(EOK);
}



/*****************************************************************************/
/* ti814x_parse_options: no driver specific options yet, this function is    */
/*    parses the default drvr options and the rest is a place holder.        */
/*****************************************************************************/
int ti814x_parse_options(ti814x_dev_t *ti814x, const char *optstring, nic_config_t *cfg)
{
    char        *value;
    int32_t     opt;
    char        *options, *freeptr;
    char        *c;
    int32_t     rc = 0;
    int32_t     err = EOK;

    if (optstring == NULL)
        return 0;

    /* getsubopt() is destructive */
    freeptr = options = strdup (optstring);

    while (options && *options != '\0') {
        c = options;
        if ((opt = getsubopt (&options, ti814x_opts, &value)) == -1) {
            if (nic_parse_options (cfg, value) == EOK)
                continue;
            goto error;
        }

        /* Driver specific option will go here */
        switch (opt) {
            default:
                break;
            }

error:
        slogf (_SLOGC_NETWORK, _SLOG_WARNING, "%s:%d - unknown option %s",
                    __FUNCTION__, __LINE__, c);
        err = EINVAL;
        rc = -1;
        }

    free (freeptr, M_DEVBUF);

    errno = err;

    return (rc);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int ti814x_read_mac_addr(ti814x_dev_t *ti814x)
{
    uint32_t    mac_hi, mac_lo;

    if (ti814x->cfg.verbose & DEBUG_MASK) {
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - read_mac_addr for devidx=%d",
            __FUNCTION__, __LINE__, ti814x->cfg.device_index);
    }

    /* MAC Address was specified at the command line, copy current to permanent then return
       right away */
    if (memcmp(ti814x->cfg.current_address, "\0\0\0\0\0\0", MAC_LEN))
    {
        if (ti814x->cfg.device_index && (ti814x->num_if > 1)) { /* i.e. the second interface */
            /* increment the last digit of the mac address to ensure uniqueness.*/
            ti814x->cfg.current_address[5]++;
        }  
         
        if (ti814x->cfg.verbose) {
            slogf (_SLOGC_NETWORK, _SLOG_DEBUG1,
                   "%s - MAC specified at the command line [%02x-%02x-%02x-%02x-%02x-%02x] for devid=%d",
                __FUNCTION__, ti814x->cfg.current_address[0], ti814x->cfg.current_address[1],
                ti814x->cfg.current_address[2], ti814x->cfg.current_address[3],
                ti814x->cfg.current_address[4], ti814x->cfg.current_address[5],
                ti814x->cfg.device_index);
        }  
    } else { 
        /* Grab MAC address from NVRAM */
        switch (ti814x->cfg.device_index) {
            case 0:
                mac_lo = in32(ti814x->cmr_base + MAC_ID0_LO);
                mac_hi = in32(ti814x->cmr_base + MAC_ID0_HI);
                break;
            case 1:
                mac_lo = in32(ti814x->cmr_base + MAC_ID1_LO);
                mac_hi = in32(ti814x->cmr_base + MAC_ID1_HI);
                break;
            default:
                slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - invalid device index",
                            __FUNCTION__, __LINE__);
                return(0);
        }

        ti814x->cfg.current_address[5] = (mac_lo >> 8) & 0xff;
        ti814x->cfg.current_address[4] = mac_lo & 0xff;
        ti814x->cfg.current_address[3] = (mac_hi>>24) & 0xff;
        ti814x->cfg.current_address[2] = (mac_hi>>16) & 0xff;
        ti814x->cfg.current_address[1] = (mac_hi>>8) & 0xff;
        ti814x->cfg.current_address[0] = (mac_hi) & 0xff;

        if (ti814x->cfg.verbose) {
            slogf (_SLOGC_NETWORK, _SLOG_DEBUG1,
                   "%s - MAC taken from NVRAM [%02x-%02x-%02x-%02x-%02x-%02x] for devid=%d",
                __FUNCTION__, ti814x->cfg.current_address[0], ti814x->cfg.current_address[1],
                ti814x->cfg.current_address[2], ti814x->cfg.current_address[3],
                ti814x->cfg.current_address[4], ti814x->cfg.current_address[5],
                ti814x->cfg.device_index);
        }
    }

    /* Store MAC address in permanent_address (for ifconfig) and store it also in hardware */
    memcpy(ti814x->cfg.permanent_address, ti814x->cfg.current_address, MAC_LEN);
    switch(ti814x->cfg.device_index) {
        case 0:
            outle32(ti814x->cpsw_regs + SL1_SA_LO, get_mac_lo(ti814x->cfg.permanent_address));
            outle32(ti814x->cpsw_regs + SL1_SA_HI, get_mac_hi(ti814x->cfg.permanent_address));
            break;
        case 1:
            outle32(ti814x->cpsw_regs + SL2_SA_LO, get_mac_lo(ti814x->cfg.permanent_address));
            outle32(ti814x->cpsw_regs + SL2_SA_HI, get_mac_hi(ti814x->cfg.permanent_address));
            break;
        default:
            slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - invalid device index",
                            __FUNCTION__, __LINE__);
    }                        	
    return (0);
}


/*****************************************************************************/
/* Resets all modules.                                                       */
/*****************************************************************************/
void ti814x_hw_reset(uintptr_t cpsw_regs)
{  
    /* Reset CPSW module */
    out32(cpsw_regs + CPSW_SOFT_RESET, CPSW_SOFT_RESET_BIT);
    /* Poll for Reset completion */
    while (in32(cpsw_regs + CPSW_SOFT_RESET) != 0x0) {
        delay(100);
        slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:%d - CPSW reset timedout", __FUNCTION__, __LINE__);
    }

    /* Reset DMA module */
    out32(cpsw_regs + DMA_SOFT_RESET, DMA_SOFT_RESET_BIT);
    /* Poll for Reset completion */
    while (in32(cpsw_regs + DMA_SOFT_RESET) != 0x0) {
        delay(100);
        slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:%d - CPSW reset timedout", __FUNCTION__, __LINE__);
    }
    
    /* Reset subsystem module */
    out32(cpsw_regs + CPSW_SS_SOFT_RESET, CPSW_SS_SOFT_RESET_BIT);
    while (in32(cpsw_regs + CPSW_SS_SOFT_RESET) != 0x0) {
        delay(100);
        slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:%d - CPSWSS reset timedout", __FUNCTION__, __LINE__);
    }

    /* Reset both slivers */
    ti814x_sliver_reset(cpsw_regs, 0);
    ti814x_sliver_reset(cpsw_regs, 1);

    return;
}

/*****************************************************************************/
/* Reset a specific interface                                                */
/*****************************************************************************/
void ti814x_sliver_reset(uintptr_t cpsw_regs, uint8_t sid)
{
    uint32_t sl_soft_reset;
    
    switch(sid) {
        case 0: sl_soft_reset = SL1_SOFT_RESET; break;
        case 1: sl_soft_reset = SL2_SOFT_RESET; break;
        default:
            slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - Invalid device index", __FUNCTION__, __LINE__);
            return;
    }
    
    out32(cpsw_regs + sl_soft_reset, SL_SOFT_RESET_BIT);
    while (in32(cpsw_regs + sl_soft_reset) != 0x0) {
        delay(100);
        slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s:%d - SL1 reset timedout", __FUNCTION__, __LINE__);
    }
}

/*****************************************************************************/
/* Shutdown all interfaces and disable host interface                        */
/*****************************************************************************/
void ti814x_stop (struct ifnet *ifp, int disable)
{
    ti814x_dev_t                *ti814x;
    struct nw_work_thread       *wtp;
    struct  _iopkt_self         *iopkt;
    uintptr_t                   mac_ctl = 0;
    uint32_t                    mac_ctl_data;

    ti814x      = ifp->if_softc;
    wtp         = WTP;
    iopkt       = ti814x->iopkt;
    
    if (ti814x->cfg.verbose & DEBUG_MASK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - if=%d disable=%d",
              __FUNCTION__, __LINE__, ti814x->cfg.device_index, disable);
    }

    if (disable) {
        /* Disable the specific interface */
        if (ti814x->cfg.device_index == 0)
            mac_ctl = ti814x->cpsw_regs + SL1_MAC_CONTROL;
        else if (ti814x->cfg.device_index == 1)
            mac_ctl = ti814x->cpsw_regs + SL2_MAC_CONTROL;
        else {
            slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - Invalid device index (%d)",
                __FUNCTION__, __LINE__, ti814x->cfg.device_index);
            return;
        }
        mac_ctl_data = in32(mac_ctl);
        outle32 (mac_ctl, (mac_ctl_data & ~GMII_EN));

        /* Stop monitoring phy */
        ti814x->stop_miimon = 1;
    }
    
    /* Stop and reset the specific interface */
    ti814x_sliver_reset(ti814x->cpsw_regs, ti814x->cfg.device_index);
 
    /* Mark the interface as down */
    ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

/*****************************************************************************/
/* Stop, reset, detach and cleanup                                           */
/*****************************************************************************/
int ti814x_detach_cleanup(ti814x_dev_t *ti814x, int how)
{
    struct  ifnet       *ifp;
    struct  _iopkt_self *iopkt;

    if (ti814x->cfg.verbose & DEBUG_MASK){
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - if=%d how=%d",
            __FUNCTION__, __LINE__, ti814x->cfg.device_index, how);
    }
    ifp   = &ti814x->ecom.ec_if;
    iopkt = ti814x->iopkt;

    /* Don't init() while we're dying. */
    ti814x->dying = 1;

    /* Reset and stop the hardware */
    ti814x_hw_reset(ti814x->cpsw_regs);
    
    /* detach from the interrupt */
    if (ti814x->iid[0] != -1) {
        InterruptDetach(ti814x->iid[0]);
        ti814x->iid[0] = -1;
    }
    interrupt_entry_remove(&ti814x->inter, NULL);    /* Must be 'the stack' to call this */ 
    
    /* Detach Interface */
    ether_ifdetach (ifp);
    if_detach (ifp);

    /* Free up allocated resources */
    if (ti814x->tx_mbuf) {
        free(ti814x->tx_mbuf, M_DEVBUF);
        ti814x->tx_mbuf = NULL;
    }

    if (ti814x->meminfo.rx_mbuf) {
        free(ti814x->meminfo.rx_mbuf, M_DEVBUF);
        ti814x->meminfo.rx_mbuf = NULL;
    }
    
    cache_fini(&ti814x->meminfo.cachectl);
    
    if (ti814x->cmr_base) {
        munmap_device_io(ti814x->cmr_base, CMR_MODULE_SIZE);
        ti814x->cmr_base = NULL;
    }

    if (ti814x->cpsw_regs) {
        munmap_device_io(ti814x->cpsw_regs, TI814X_CPSW_SIZE);
        ti814x->cpsw_regs = NULL;
    }
    
    return (EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_hw_config(uintptr_t cpsw_regs, struct attach_args *attach_args)
{
    uint32_t        i=0;
    uint32_t        val;

   /* Initialize DMA Host 2.2.1.1 cpdma 1.8.1 */
    /* 2. Initialize Rx_HDP registers to Zero */
    for (i = 0; i < 8; i++) {
        outle32 (cpsw_regs + TX0_HDP + (4 * i), 0);
        outle32 (cpsw_regs + RX0_HDP + (4 * i), 0);
    }
    /* 3. Enable the desired receive interrupt in the Int Mask Register */
    outle32(cpsw_regs + RX_INTMASK_SET, RX0_PEND_MASK);
    /* 4. Write the rx_buffer_offset register value */
    outle32(cpsw_regs + RX_BUFFER_OFFSET, 0);
    /* 5. setup the receive channels buffer descriptors as required by CPPI 3.0 */
    if (ti814x_setup_descriptors(attach_args->cppi_base, &attach_args->meminfo)) {
        slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s: setup_descriptors error", __FUNCTION__);
        return (-1);
    }
    /* 6. Enable the RX DMA controller by setting the RX_EN bit in the RX_Control register */
    outle32(cpsw_regs + RX_CONTROL, RX_EN);
    outle32(cpsw_regs + TX_CONTROL, TX_EN);
#if DM814x
    /* clear statistics registers */
    for (i = RXGOODFRAMES; i < (RXDMAOVERRUNS + 4); i += 4)
        outle32(cpsw_regs + i, 0);
#endif
    /* Enable statistics for Port 0/1 */
    outle32(cpsw_regs + CPSW_STAT_PORT_EN, P1_STAT_EN|P0_STAT_EN|P2_STAT_EN);
    /* enable receive interrupts */
    outle32(cpsw_regs + C0_RX_EN, RX_EN_CH0);
    /* Configure CMR GMII_SET */
    switch (attach_args->ethif)
    {
    case RGMII:
       val = GMII0_SEL_RGMII| GMII1_SEL_RGMII|
                                         //GMII0_ID_MODE  | GMII1_ID_MODE  | /* remove to allow gigabit ethernet to work...*/
                                         RGMII0_EN      | RGMII1_EN;
       slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s: ethif='RGMII' [gmii_sel=0x%x]", __FUNCTION__, val);
       break;
    case GMII:
    	val = PORT1_GMII_MODE | PORT2_GMII_MODE;
        slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s: ethif='GMII' [gmii_sel=0x%x]", __FUNCTION__, val);
    	break;
    default:
    	val = PORT1_RMII_MODE | PORT2_RMII_MODE;
        slogf (_SLOGC_NETWORK, _SLOG_INFO, "%s: ethif='RMII' [gmii_sel=0x%x]", __FUNCTION__, val);
    	break;
    }
    outle32(attach_args->cmr_base + GMII_SET, val);

    /* Tell the DMA engine where to put the RX Data */
    out32(cpsw_regs + RX0_HDP, CPPI_DESC_PHYS);

    return (EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_setup_descriptors(uintptr_t cppi_base, meminfo_t *meminfo)
{
    int32_t                 i,j, size=0, err=0;
    uint32_t                desc_phys;
    off64_t                 phys;
    cppi_desc_t             *desc;
    struct mbuf             *m;
    struct nw_work_thread   *wtp;
    
    wtp = WTP;

    if (cache_init (0, &meminfo->cachectl, NULL) == -1) {
        slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d: cache_init() failed",
                    __FUNCTION__, __LINE__);
        err = errno;
        return (err);
    }

    /* Allocate array of mbuf pointers for receiving */
    size = DEFAULT_NUM_RX_PKTS * sizeof(meminfo->rx_mbuf);
    if ((meminfo->rx_mbuf = malloc(size, M_DEVBUF, M_NOWAIT)) == NULL) {
        err = errno;
        return (err);
    }
    memset (meminfo->rx_mbuf, 0, size);

    /* index 0 gets the first 128 buffers, index 1 gets the latter chunk */
    desc_phys = CPPI_DESC_PHYS;
    desc = (cppi_desc_t *)cppi_base;

    for (i = 0; i < DEFAULT_NUM_RX_PKTS; i++, desc++) {
        if (meminfo->rx_mbuf [i] != NULL)
            continue;
  
        m = m_getcl_wtp (M_DONTWAIT, MT_DATA, M_PKTHDR, wtp);
        if (m == NULL) {
            return (-1);
        }
        meminfo->rx_mbuf [i] = m;
        desc_phys += sizeof (cppi_desc_t); 
        if (i == (DEFAULT_NUM_RX_PKTS - 1)) {
            desc->next = CPPI_DESC_PHYS;
        } else {
            desc->next = desc_phys;
        }
        phys = pool_phys(m->m_data, m->m_ext.ext_page);
        CACHE_INVAL (&meminfo->cachectl, m->m_data, phys, m->m_ext.ext_size);
        desc->buffer = (uint32_t) phys;
        desc->off_len = 8000; //MAX_PKT_SIZE;
        desc->flag_len = DESC_FLAG_OWN;
    }
    /* Setup/Initialize Transmit buffer descriptor memory space */
    /* TX descriptors start at 0x4a103000 */
    /* Initialze all descriptors, max=128 (0x80*32=0x1000). We'll carve up the pool
     * per interface later in the initialization */
    desc = (cppi_desc_t*)(cppi_base + (uintptr_t)CPPI_TX_DESC_OFFSET);
    for (j = 0; j < TOTAL_NUM_TX_PKTS; j++, desc++) {
            desc->next = 0;
            desc->buffer = 0;
            desc->off_len = 0;
            desc->flag_len = 0;
    }
    return (0);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
void ti814x_shutdown (void *arg)
{
    ti814x_dev_t  *ti814x;
    ti814x = (ti814x_dev_t *)arg;
    
    if (ti814x->cfg.verbose & DEBUG_MASK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d",    __FUNCTION__, __LINE__);
    }

    ti814x_stop(&ti814x->ecom.ec_if, 1);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int ti814x_detach (struct device *dev, int flags)
{
    struct ti814x_dev *idev;
    
    idev = (struct ti814x_dev *)dev;

    return ti814x_detach_cleanup(idev->sc_ti814x, -1);

}

/*****************************************************************************/
/* Initial driver entry point.                                               */
/* Returns -1 on failure; errno will be set to ENODEV if no devices detected.*/
/*****************************************************************************/
int ti814x_entry(void *dll_hdl, struct _iopkt_self *iopkt, char *options)
{
    int32_t                 single;
    int32_t                 instance;
    int32_t                 err;
    uintptr_t               cpsw_regs;
    struct device           *dev[2] = {NULL, NULL};;
    struct attach_args      attach_args;

    /* initialize arguments/parameters for ti814x_attach() */
    memset(&attach_args, 0x00, sizeof(attach_args));

    /* Initialize device_index to -1 to see if parse_option sets it */
    attach_args.cfg.device_index = -1;
    attach_args.iopkt = iopkt;
    attach_args.options = options;
    single = 0;         // get one i/f working for now

    /* Configure Interrupts and memory mappings */
    attach_args.cfg.num_io_windows = 2;
    attach_args.cfg.io_window_base[0] = TI814X_CPSW_BASE;   /* 0x4a100000 */
    attach_args.cfg.io_window_size[0] = TI814X_CPSW_SIZE;   /* 0x4000 */
    attach_args.cfg.io_window_base[1] = CMR_BASE;	         /* 0x48140000 */
    attach_args.cfg.io_window_size[1] = CMR_MODULE_SIZE;    /* 0x2000 */
    attach_args.cfg.num_irqs          = 1; 
    attach_args.cfg.irq[0]            = TI814x_RX_INT0;
    attach_args.ethif                 = GMII;
    
    if (options != NULL) {
        if ((err = ti814x_parse_options(NULL, options, &attach_args.cfg)) != EOK) {
            return(err);
        }
    }

    if ((err = ti814x_mapmem(&attach_args, &attach_args.cfg)) != EOK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d Problem mapping memories",
            __FUNCTION__, __LINE__);
        return(err);
    }
    cpsw_regs = attach_args.cpsw_base;
    ti814x_hw_reset(cpsw_regs);

    /* Initialize all CPPI memory before splitting off for each initialized interface */
    memset((void*)attach_args.cppi_base, 0x00, CPPI_DESC_MEM_SIZE);
  
    instance = 0;       /* Start at fist instance */
    /* deviceindex was not specified on the command line, configure all interfaces */
    if (attach_args.cfg.device_index == -1) {
        attach_args.num_if = 2; /* Attach two devices */
        
    } else {
        /* configure only the interface specified */
        attach_args.num_if = 1;                  /* Attach only one device */
        single = 1;
        /* start at the index specified at the command line */
        //instance = attach_args.cfg.device_index;
    }
    
    ti814x_hw_config(cpsw_regs, &attach_args);
     
    /* Call dev_attach for every instance found */
    while(instance < attach_args.num_if) {
        if (!single)
            attach_args.cfg.device_index = instance;
        if (dev_attach("dm", options, &ti814x_ca, &attach_args,
                    &single, &dev[instance], NULL) != EOK) {
            break;
        }

        dev[instance]->dv_dll_hdl = dll_hdl;

        instance++;
        if (single) {
            break;
        }
    }

    /* If there are more than one interfaces enabled we should store their ecom structure
     * for use in the RX interrupt processing since one interface may be receiving for the other,
     * which forces us to have the ability to tell io-pkt what interface the packet originated
     * from */
    if (!single) {
        /* Only run this code if two interfaces were detected, i.e. dev structure is populated
         * by dev_attach */
        ((ti814x_dev_t*)dev[0])->common_ecom[0] = &((ti814x_dev_t*)dev[0])->ecom;
        ((ti814x_dev_t*)dev[0])->common_ecom[1] = &((ti814x_dev_t*)dev[1])->ecom;
        ((ti814x_dev_t*)dev[1])->common_ecom[0] = &((ti814x_dev_t*)dev[0])->ecom;
        ((ti814x_dev_t*)dev[1])->common_ecom[1] = &((ti814x_dev_t*)dev[1])->ecom;
    }
    
    if (instance > 0) {
        return(EOK);
    }
    return(ENODEV);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int ti814x_attach (struct device *parent, struct device *self, void *aux)
{
    ti814x_dev_t            *ti814x;
    struct  ifnet           *ifp;
    struct  attach_args     *attach_args;
    struct _iopkt_self      *iopkt;
    char                    *options;  
    int32_t                 err, tx_desc_offset=0, size=0;
    attach_args             = aux;
    iopkt                   = attach_args->iopkt;
    options                 = attach_args->options;  
    ti814x                  = (ti814x_dev_t*)self;
    ti814x->iopkt           = iopkt;    
    ifp                     = &ti814x->ecom.ec_if;
    ifp->if_softc           = ti814x;

    if (ti814x->cfg.verbose & DEBUG_MASK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d devid=%d self=%p",
              __FUNCTION__, __LINE__, attach_args->cfg.device_index, self);
    }

    /* Copy memory/buffer information intitalized in ti814x_init */
    memcpy(&ti814x->meminfo, &attach_args->meminfo, sizeof(meminfo_t));
    /* Copy various config parameters initilized in ti814x_init */
    memcpy(&ti814x->cfg, &attach_args->cfg, sizeof(nic_config_t));
    /* Setin interface name */    
    strcpy (ifp->if_xname, ti814x->dev.dv_xname);
    strcpy ((char *) ti814x->cfg.uptype, "en");
    strcpy ((char *) ti814x->cfg.device_description, "ti814x");
    /* Store mmap information initialized in ti814x_init */
    ti814x->cpsw_regs = attach_args->cpsw_base;
    ti814x->cppi_base = attach_args->cppi_base;
    ti814x->cmr_base  = attach_args->cmr_base;

    /* Ethernet stats we are interested in */
    ti814x->stats.un.estats.valid_stats =
        NIC_ETHER_STAT_INTERNAL_TX_ERRORS       |
        NIC_ETHER_STAT_INTERNAL_RX_ERRORS       |
        NIC_ETHER_STAT_NO_CARRIER               |
        NIC_ETHER_STAT_XCOLL_ABORTED            |
        NIC_ETHER_STAT_SINGLE_COLLISIONS        |
        NIC_ETHER_STAT_MULTI_COLLISIONS         |
        NIC_ETHER_STAT_LATE_COLLISIONS          |
        NIC_ETHER_STAT_TOTAL_COLLISION_FRAMES   |
        NIC_ETHER_STAT_TX_DEFERRED              |
        NIC_ETHER_STAT_ALIGN_ERRORS             |
        NIC_ETHER_STAT_FCS_ERRORS               |
        NIC_ETHER_STAT_JABBER_DETECTED          |
        NIC_ETHER_STAT_OVERSIZED_PACKETS        |
        NIC_ETHER_STAT_SHORT_PACKETS;

    /* Generic networking stats we are interested in */
    ti814x->stats.valid_stats =
        NIC_STAT_TX_FAILED_ALLOCS       |
        NIC_STAT_RX_FAILED_ALLOCS       |
        NIC_STAT_RXED_MULTICAST         |
        NIC_STAT_RXED_BROADCAST         |
        NIC_STAT_TXED_BROADCAST         |
        NIC_STAT_TXED_MULTICAST;

    ti814x->cfg.priority = IRUPT_PRIO_DEFAULT;  
    ti814x->cfg.lan = ti814x->dev.dv_unit;                       
    ti814x->cfg.media_rate = -1;
    ti814x->cfg.duplex = -1;
    ti814x->force_link = -1;
    ti814x->num_rx_pkts = DEFAULT_NUM_RX_PKTS;
    /* Number of tx descriptors needs to be split up between interfaces (2)*/
    ti814x->num_tx_pkts = DEFAULT_NUM_TX_PKTS;
    ti814x->cfg.mtu = ETH_MAX_PKT_LEN;
    ti814x->cfg.mru = ETH_MAX_PKT_LEN;
    ti814x->cfg.flags |= NIC_FLAG_MULTICAST;
    ti814x->cfg.mac_length = ETH_MAC_LEN;
    ti814x->num_if = attach_args->num_if;
    ti814x->meminfo.rx_desc = (cppi_desc_t*)attach_args->cppi_base;

    /* Allocate array of mbuf pointers for tracking pending transmit packets */
    size = DEFAULT_NUM_TX_PKTS * sizeof(ti814x->tx_mbuf);
    if ((ti814x->tx_mbuf = malloc(size, M_DEVBUF, M_NOWAIT)) == NULL) {
        err = errno;
        return (err);
    }
    memset(ti814x->tx_mbuf, 0, size);
    
    /* Adjust tx descriptor pointers so that they are interface exclusive */
    tx_desc_offset = ti814x->cfg.device_index * ti814x->num_tx_pkts * sizeof(cppi_desc_t);
    ti814x->meminfo.tx_desc = (cppi_desc_t*)(ti814x->cppi_base + CPPI_TX_DESC_OFFSET + tx_desc_offset);
    ti814x->meminfo.tx_phys = CPPI_DESC_PHYS + CPPI_TX_DESC_OFFSET + tx_desc_offset;
    
    /* hook up so media devctls work */
    bsd_mii_initmedia(ti814x);
    
    /* Setup ethercomm */
    ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
    ifp->if_ioctl = ti814x_ioctl;
    ifp->if_start = ti814x_start;
    ifp->if_init  = ti814x_init;
    ifp->if_stop  = ti814x_stop;
    IFQ_SET_READY(&ifp->if_snd);

    /* Setup interrupt related info */
    ti814x->inter.func = ti814x_process_interrupt;
    ti814x->inter.enable = ti814x_enable_interrupt;
    ti814x->isrp = ti814x_isr;
    ti814x->inter.arg = ti814x;
    ti814x->iid[0] = ti814x->iid[1] = -1; /* Not yet attached */

    /* Parse options */
    if (ti814x_parse_options(ti814x, options, &ti814x->cfg) == -1) {
        err = errno;
        ti814x_detach_cleanup(ti814x, 1);
        return (err);
    }

    if (ti814x->cfg.verbose & DEBUG_MASK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - Attaching if=%s unit=%d ti814x=%p sizeof(ti814x)=%d",
                __FUNCTION__, __LINE__, ti814x->dev.dv_xname, ti814x->cfg.device_index, ti814x, sizeof(ti814x_dev_t));
    }

    /* If media was not specified on cmdline, default to NIC_MEDIA_802_3 */
    if (ti814x->cfg.media == -1) {
        ti814x->cfg.media = NIC_MEDIA_802_3;
        ti814x->stats.media = ti814x->cfg.media;
    }
    else {
        ti814x->stats.media = ti814x->cfg.media;
    }

    if (ti814x->cfg.mtu == 0 || ti814x->cfg.mtu > ETH_MAX_PKT_LEN) {
        ti814x->cfg.mtu = ETH_MAX_PKT_LEN;
    }

    if (ti814x->cfg.mru == 0 || ti814x->cfg.mru > ETH_MAX_PKT_LEN) {
        ti814x->cfg.mru = ETH_MAX_PKT_LEN;
    }

    if (ti814x->cfg.media_rate != -1) {
        if (ti814x->cfg.media_rate == 100 * 1000) {
            ti814x->force_link = (ti814x->cfg.duplex > 0) ? MDI_100bTFD : MDI_100bT;
        }
        else {
            ti814x->force_link = (ti814x->cfg.duplex > 0) ? MDI_10bTFD : MDI_10bT;
        }
    }
 
    ti814x->cfg.flags |= NIC_FLAG_LINK_DOWN;

    evcnt_attach_dynamic (&ti814x->ev_txdrop, EVCNT_TYPE_MISC, NULL, ifp->if_xname, "txdrop");
    
    if (ti814x_read_mac_addr(ti814x)) {
        slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d: hw_config failed",
                    __FUNCTION__, __LINE__);
        ti814x_detach_cleanup(ti814x, 4);
        return (errno);
    }

    /* Todo: somewhat of a hack here, we definitly only want to attach once and not for every
     * interface, but doign this any earlier makes providing the ti814x data structure to the
     * isr difficult, find a way to place this in _entry() */
    if (run_once) {
        run_once = 0;
        if (ti814x->iid[0] == -1) {
            if ((err = InterruptAttach_r(ti814x->cfg.irq[0], ti814x->isrp,
                            ti814x, sizeof(*ti814x), _NTO_INTR_FLAGS_TRK_MSK)) < 0) {
                err = -err;
                slogf (_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d InterruptAttach_r error (%d) ",
                    __FUNCTION__, __LINE__, err);
                return (err);
            }
            ti814x->iid[0] = err;
        }
    }

    /* Settup interrupt entry */
    if ((err = interrupt_entry_init (&ti814x->inter, 0, NULL,
                                        IRUPT_PRIO_DEFAULT)) != EOK) {
        ti814x_detach_cleanup(ti814x, 7);
        return (err);
    }
            
    if_attach (ifp);
    ether_ifattach (ifp, ti814x->cfg.current_address);

    /* Start House-keeping periodic callout */
    callout_init(&ti814x->hk_callout);
    
    if (ti814x->cfg.verbose)
        nic_dump_config (&ti814x->cfg);

    ti814x->sd_hook = shutdownhook_establish(ti814x_shutdown, ti814x);

    return (EOK);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int ti814x_init (struct ifnet *ifp)
{
    ti814x_dev_t                *ti814x;
    struct nw_work_thread       *wtp;
    struct _iopkt_self          *iopkt;

    ti814x = ifp->if_softc;
    iopkt = ti814x->iopkt;
    wtp = WTP;

    if (ti814x->cfg.verbose & DEBUG_MASK) {
        slogf(_SLOGC_NETWORK, _SLOG_ERROR, "%s:%d - Start devidx=%d",
            __FUNCTION__, __LINE__,ti814x->cfg.device_index);
    }

    ti814x->tx_tracked = ti814x->tx_freed = ti814x->tx_alloc = 0;
    
    if (ti814x->dying == 1)
        return (0);

    /* Set slave MAC Address */
    /* MAC address needs to be set in init because SLx_SA_LO/HI get cleared when
     * the slaves get reset, which can happen often with dhcp, etc ... */
    switch(ti814x->cfg.device_index) {
        case 0:
            /* init receive buffer offset and max length */
            outle32 (ti814x->cpsw_regs + SL1_RX_MAXLEN, 8000);//MAX_PKT_SIZE);
            outle32 (ti814x->cpsw_regs + SL1_MAC_CONTROL, in32(ti814x->cpsw_regs+SL1_MAC_CONTROL)|GMII_EN);
            break;
        case 1:
            /* init receive buffer offset and max length */
            outle32 (ti814x->cpsw_regs + SL2_RX_MAXLEN, 8000); //MAX_PKT_SIZE);
            outle32 (ti814x->cpsw_regs + SL2_MAC_CONTROL, in32(ti814x->cpsw_regs+SL2_MAC_CONTROL)|GMII_EN);
            break;
        default:
            slogf (_SLOGC_NETWORK, _SLOG_ERROR,
                   "%s:%d: set slave MAC ADDR: invalid device index (%d)",
                    __FUNCTION__, __LINE__, ti814x->cfg.device_index);
            return(-1);
    }
   
    /* Enable ALE and Clear ALE Table */
    ti814x_ale_init(ti814x);

    /* Init the PHY */
    if (ti814x_findphy(ti814x)) {
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "PHY init failure");
        return (-1);
    }
    
    callout_msec (&ti814x->mii_callout, 3 * 1000, ti814x_MDI_MonitorPhy, ti814x);
    ti814x->stop_miimon = 0;
    
    ifp->if_flags_tx |= IFF_RUNNING;
    ifp->if_flags_tx &= ~IFF_OACTIVE;
    NW_SIGUNLOCK_P (&ifp->if_snd_ex, iopkt, wtp);
    ifp->if_flags |= IFF_RUNNING;

    callout_msec (&ti814x->hk_callout, 10 * 1000, ti814x_hk_callout, ti814x);
    
    return(EOK);
}

