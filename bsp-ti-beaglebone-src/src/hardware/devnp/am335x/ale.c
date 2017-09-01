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

#include    "ti814x.h"


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_set_port_num(uint32_t *entry, int32_t port)
{
    /* Secure bit is bit67:66 in the table entry */
    /* Table word 2 bit 3:2 represents this bit */
    entry[2] &= 0xfffffff3;
    entry[2] |= ((port & 0x00000003) << 2);
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_set_blocked(uint32_t *entry, int32_t blocked)
{
    /* Secure bit is bit65 in the table entry */
    /* Table word 2 bit 1 represents this bit */
    entry[2] &= 0xfffffffd;
    entry[2] |= ((blocked & 0x00000001) << 1);
    return(EOK);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_set_secure(uint32_t *entry, int32_t secure)
{
    /* Secure bit is bit64 in the table entry */
    /* Table word 2 bit 0 represents this bit */
    entry[2] &= 0xfffffffe;
    entry[2] |= (secure & 0x00000001);
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_get_ucast_type(uint32_t *entry)
{
    /* unicast type field is located in bits 63:62  */
    /* ALE Table Word 1 bits 31:30 represent these bits */
    return((entry[1]>>30)&0x3);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_set_ucast_type(uint32_t *ale_entry, uint8_t type)
{
    /* clear unitcast type bits */
    ale_entry[1] &= 0x3fffffff;
    ale_entry[1] |= (type & 0x3) << 30;
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_set_mcast_state(uint32_t *ale_entry, uint8_t state)
{
    /* mcast state field is located in bits 63:62 */
    /* ALE Table Word 1 bits 31:30 represent these bits */
    /* clear entry type bits */
    ale_entry[1] &= 0x3fffffff;
    ale_entry[1] |= (state & 0x3)<<30;
    return(EOK);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_set_addr(uint32_t *entry, uint8_t *addr)
{
    entry[0] = addr[0] | (addr[1]<<8) | (addr[2]<<16) | (addr[3]<<24);
    /* clear ad-dress bits from T1 */
    entry[1] &= 0xffff0000;
    entry[1] |= (0x0000ffff & (addr[4] | (addr[5]<<8)));
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_get_addr(uint32_t *entry, uint8_t *addr)
{
    addr[0] = (entry[0]      )  & 0x000000ff;
    addr[1] = (entry[0] >> 8 )  & 0x000000ff;
    addr[2] = (entry[0] >> 16)  & 0x000000ff;
    addr[3] = (entry[0] >> 24)  & 0x000000ff;
    addr[4] = (entry[1]      )  & 0x000000ff;
    addr[5] = (entry[1] >> 8 )  & 0x000000ff;
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_set_entry_type(uint32_t *entry, uint8_t entry_type)
{
    /* Entry type field is located in bits 61:60 */
    /* ALE Table Word 1 bits 29:28 represent bits 61:60 of the table entry */
    /* clear entry type bits */
    entry[1] &= 0xcfffffff;
    entry[1] |= (entry_type & 0x3)<<28;
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_get_entry_type(uint32_t *entry)
{
    /* Entry type field is located in bits 61:60 */
    /* ALE Table Word 1 bits 29:28 represent bits 61:60 of the table entry */
    /* 63.62.61.60-59.58.57.56 / 55.54.53.52-51.50.49.48 / 47.46.45.44-43.42.41.40 / 39.38.37.36-35.34.33.32 */
    /* 31.30.29.28-27.26.25.24 / 23.22.21.20-19.18.17.16 / 15.14.13.12-11.10.09.08 / 07.06.05.04-03.02.01.00 */
    return((entry[1]>>28)&0x3);
    
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int8_t ti814x_ale_get_port_mask(uint32_t *entry)
{
    /* Depending on the table entry, bits 66, 67, 68 will return the port number
     * or the port field or reseverd bits, for this information to be valid the
     * type of table entry MUST be considered */

    /* port field is located in bits 68:66 of the table entry */
    /* ALE Table Word 2 bits 7:0 define table entry bits 71:64 */
    return((entry[2] >> (PORT_MASK_BIT_OFFSET-TBLW2_START_BIT)) & TBLW2_PORT_MASK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int8_t ti814x_ale_set_port_mask(uint32_t *entry, int32_t port_mask)
{
    entry[2] = ((port_mask & 0x7) << (PORT_MASK_BIT_OFFSET-TBLW2_START_BIT));
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_init(ti814x_dev_t *ti814x)
{
    /* Enable ALE and Clear ALE Table */
    outle32(ti814x->cpsw_regs + ALE_CONTROL, ENABLE_ALE|CLEAR_TABLE|ALE_BYPASS);
    outle32(ti814x->cpsw_regs + ALE_PORTCTL0, PORT_STATE_FORWARD);
    outle32(ti814x->cpsw_regs + ALE_PORTCTL1, PORT_STATE_FORWARD);
    outle32(ti814x->cpsw_regs + ALE_PORTCTL2, PORT_STATE_FORWARD);
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int32_t ti814x_ale_read (ti814x_dev_t *ti814x, uint32_t idx, uint32_t *entry)
{
    if (ti814x->cfg.verbose) {
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - idx=%d",
               __FUNCTION__, __LINE__, idx);
    }
    if (idx > 1024) {
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - invalid ALE table index",
               __FUNCTION__, __LINE__);
        return(-1);
    }

    /* Write index to ALE table control to load table entry into table-word registers */
    out32(ti814x->cpsw_regs + ALE_TBLCTL, idx & ENTRY_MASK);

    /* Store table entry */
    entry[0] = in32(ti814x->cpsw_regs + ALE_TBLW0);
    entry[1] = in32(ti814x->cpsw_regs + ALE_TBLW1);
    entry[2] = in32(ti814x->cpsw_regs + ALE_TBLW2);
    slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - %08x-%08x-%08x",
               __FUNCTION__, __LINE__, entry[2],entry[1],entry[0]);
    
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
uint32_t ti814x_ale_write (ti814x_dev_t *ti814x, uint32_t idx, uint32_t *entry)
{
    slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - idx=%d %08x-%08x-%08x",
               __FUNCTION__, __LINE__, idx, entry[2],entry[1],entry[0]);
    /* Populate the table words */
    out32(ti814x->cpsw_regs + ALE_TBLW0, entry[0]);
    out32(ti814x->cpsw_regs + ALE_TBLW1, entry[1]);
    out32(ti814x->cpsw_regs + ALE_TBLW2, entry[2]);

    /* Write to ALE table control to push data to the ALE table */
    out32(ti814x->cpsw_regs + ALE_TBLCTL, WRITE_RDZ_WRITE | (idx & ENTRY_MASK));
    
    return(EOK);
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void ti814x_ale_flush_mcast(uint32_t *entry, int32_t port_mask)
{
    //int32_t mask;
    slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d",
               __FUNCTION__, __LINE__);
    //mask = ti814x_ale_get_port_field(entry);
    //if (!(mask & port_mask)) {
    //    return; /* table port mask doesn't make the port mask we're looking for */
    //}
    //mask &= ~port_mask;
    /* free if only remaining port is host port */
    //if (mask == BIT(ale->ale_ports))
    ti814x_ale_set_entry_type(entry, ALE_TYPE_FREE);
    //else
    //    cpsw_ale_set_port_mask(ale_entry, mask);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
void ti814x_ale_flush_ucast(uint32_t *ale_entry, int port_mask)
{
        //int port;

        //port = cpsw_ale_get_port_num(ale_entry);
        //if ((BIT(port) & port_mask) == 0)
        //        return; /* ports dont intersect, not interested */
        ti814x_ale_set_entry_type(ale_entry, ALE_TYPE_FREE);
}

#if 0
static int cpsw_ale_dump_mcast(uint32_t *entry, char *buf, int len)
{
        int outlen = 0;
        static const char *str_mcast_state[] = {"f", "blf", "lf", "f"};
        int mcast_state = cpsw_ale_get_mcast_state(ale_entry);
        int port_mask   = cpsw_ale_get_port_mask(ale_entry);
        int super       = cpsw_ale_get_super(ale_entry);

        outlen += snprintf(buf + outlen, len - outlen,
                           "mcstate: %s(%d), ", str_mcast_state[mcast_state],
                           mcast_state);
        outlen += snprintf(buf + outlen, len - outlen,
                           "port mask: %x, %ssuper\n", port_mask,
                           super ? "" : "no ");
        return outlen;
   
}
#endif
#if 0
static int cpsw_ale_dump_ucast(u32 *ale_entry, char *buf, int len)
{
        int outlen = 0;
        static const char *str_ucast_type[] = {"persistant", "untouched",
                                               "oui", "touched"};
        int ucast_type  = cpsw_ale_get_ucast_type(ale_entry);
        int port_num    = cpsw_ale_get_port_num(ale_entry);
        int secure      = cpsw_ale_get_secure(ale_entry);
        int blocked     = cpsw_ale_get_blocked(ale_entry);

        outlen += snprintf(buf + outlen, len - outlen,
                           "uctype: %s(%d), ", str_ucast_type[ucast_type],
                           ucast_type);
        outlen += snprintf(buf + outlen, len - outlen,
                           "port: %d%s%s\n", port_num, secure ? ", Secure" : "",
                           blocked ? ", Blocked" : "");
        return outlen;

}
#endif
#if 0
static int cpsw_ale_dump_entry(int idx, u32 *ale_entry, char *buf, int len)
{

        int type, outlen = 0;
        u8 addr[6];
        static const char *str_type[] = {"free", "addr", "vlan", "vlan+addr"};

        type = cpsw_ale_get_entry_type(ale_entry);
        if (type == ALE_TYPE_FREE)
                return outlen;

        if (idx >= 0) {
                outlen += snprintf(buf + outlen, len - outlen,
                                   "index %d, ", idx);
        }

        outlen += snprintf(buf + outlen, len - outlen, "raw: %08x %08x %08x, ",
                           ale_entry[0], ale_entry[1], ale_entry[2]);

        outlen += snprintf(buf + outlen, len - outlen,
                           "type: %s(%d), ", str_type[type], type);

        cpsw_ale_get_addr(ale_entry, addr);
        outlen += snprintf(buf + outlen, len - outlen,
                           "addr: " ADDR_FMT_STR ", ", ADDR_FMT_ARGS(addr));

        if (type == ALE_TYPE_VLAN || type == ALE_TYPE_VLAN_ADDR) {
                outlen += snprintf(buf + outlen, len - outlen, "vlan: %d, ",
                                   cpsw_ale_get_vlan_id(ale_entry));
        }

        outlen += cpsw_ale_get_mcast(ale_entry) ?
                  cpsw_ale_dump_mcast(ale_entry, buf + outlen, len - outlen) :
                  cpsw_ale_dump_ucast(ale_entry, buf + outlen, len - outlen);

        return outlen;

}
#endif

// index shoud be -1 unless you absolutely know where the entry shoud go.
int32_t ti814x_ale_add_ucast(ti814x_dev_t *ti814x, uint8_t *addr, int32_t index, int32_t port, int32_t flags)
{

        uint32_t ale_entry[ALE_ENTRY_WORDS] = {0, 0, 0};
        int32_t idx;

        ti814x_ale_set_entry_type(ale_entry, ALE_TYPE_ADDR);
        ti814x_ale_set_addr(ale_entry, addr);
        ti814x_ale_set_ucast_type(ale_entry, ALE_UNICAST);
        ti814x_ale_set_secure(ale_entry, (flags & ALE_SECURE_FLAG) ? 1 : 0);
        ti814x_ale_set_blocked(ale_entry, (flags & ALE_BLOCKED_FLAG) ? 1 : 0);
        ti814x_ale_set_port_num(ale_entry, port);

        /* If an index was suppied with the call to this function assume the user knows
         * best and write the entry information to that index in the table, otherwise, verify
         * if one that matches exists already, faiing that find a spot to put it */
        if (index<0) {
            idx = ti814x_ale_match_addr(ti814x, addr);
            if (idx < 0)
                    idx = ti814x_ale_match_free(ti814x);
            if (idx < 0)
                    idx = ti814x_ale_find_ageable(ti814x);
            if (idx < 0)
                    return -ENOMEM;
        } else {
            idx = index;
        }

        ti814x_ale_write(ti814x, idx, ale_entry);
        return 0;

}

#if 0
int cpsw_ale_del_ucast(struct cpsw_ale *ale, u8 *addr, int port)
{

        u32 ale_entry[ALE_ENTRY_WORDS] = {0, 0, 0};
        int idx;

        idx = cpsw_ale_match_addr(ale, addr);
        if (idx < 0)
                return -ENOENT;

        cpsw_ale_set_entry_type(ale_entry, ALE_TYPE_FREE);
        cpsw_ale_write(ale, idx, ale_entry);
        return 0;

}
#endif

int32_t ti814x_ale_add_mcast(ti814x_dev_t *ti814x, uint8_t *addr, int32_t port_mask)
{
        uint32_t ale_entry[ALE_ENTRY_WORDS] = {0, 0, 0};
        int32_t idx, mask;

        ti814x_ale_print_table(ti814x);
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - adding %02x:%02x:%02x:%02x:%02x:%02x:",
               __FUNCTION__, __LINE__, addr[5],addr[4],addr[3],addr[2],addr[1],addr[0]);        
        
        if (ti814x->cfg.verbose > 1) {
            slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d", __FUNCTION__, __LINE__);
        }

        idx = ti814x_ale_match_addr(ti814x, addr);
        if (idx >= 0) {
            slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - Found a match @ %d",
               __FUNCTION__, __LINE__, idx);
                ti814x_ale_read(ti814x, idx, ale_entry);
        }
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - %d",
               __FUNCTION__, __LINE__, idx);
        
        ti814x_ale_set_entry_type(ale_entry, ALE_TYPE_ADDR);
        ti814x_ale_set_addr(ale_entry, addr);
        ti814x_ale_set_mcast_state(ale_entry, ALE_STATE_FWD_2);

        mask = ti814x_ale_get_port_mask(ale_entry);
        port_mask |= mask;
        ti814x_ale_set_port_mask(ale_entry, port_mask);

        if (idx < 0){
            /* Find a free table entry */
            idx = ti814x_ale_match_free(ti814x);
            slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - idx = %d",
               __FUNCTION__, __LINE__, idx);
        }
        if (idx < 0){
            /* If there are no free table entries find one that is ageable */
            idx = ti814x_ale_find_ageable(ti814x);
        }
        if (idx < 0){
            /* If all table entries are taken return error */
            return -ENOMEM;
        }
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - Write MCAST entry to idx=%d",
               __FUNCTION__, __LINE__, idx);
        /* If we found a place to put it write the mutlicast table entry */
        ti814x_ale_write(ti814x, idx, ale_entry);
        return 0;
}

#if 0
int cpsw_ale_del_mcast(struct cpsw_ale *ale, u8 *addr, int port_mask)
{
#if 0
        u32 ale_entry[ALE_ENTRY_WORDS] = {0, 0, 0};
        int idx, mask;

        idx = cpsw_ale_match_addr(ale, addr);
        if (idx < 0)
                return -EINVAL;

        cpsw_ale_read(ale, idx, ale_entry);
        mask = cpsw_ale_get_port_mask(ale_entry);
        port_mask = mask & ~port_mask;

        if (port_mask == BIT(ale->ale_ports))
                cpsw_ale_set_entry_type(ale_entry, ALE_TYPE_FREE);
        else
                cpsw_ale_set_port_mask(ale_entry, port_mask);

        cpsw_ale_write(ale, idx, ale_entry);
        return 0;
#endif
}
#endif

int32_t ti814x_ale_match_addr(ti814x_dev_t *ti814x, uint8_t* addr)
{
        uint32_t entry[ALE_ENTRY_WORDS];
        int32_t type, idx;

        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - looking for match to %02x:%02x:%02x:%02x:%02x:%02x",
               __FUNCTION__, __LINE__, addr[5],addr[4],addr[3],addr[2],addr[1],addr[0]);
        
        for (idx = 0; idx < ALE_ENTRIES; idx++) {
                uint8_t entry_addr[6];

                ti814x_ale_read(ti814x, idx, entry);
                type = ti814x_ale_get_entry_type(entry);
                if (type != ALE_TYPE_ADDR && type != ALE_TYPE_VLAN_ADDR){
                    slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - continue idx=%d", __FUNCTION__, __LINE__, idx);
                        continue;
                }
                ti814x_ale_get_addr(entry, entry_addr);
                if (memcmp(entry_addr, addr, 6) == 0) {
                    slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - matched address at idx=%d", __FUNCTION__, __LINE__, idx);
                    return idx;
                }
        }
        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - returning idx=%d", __FUNCTION__, __LINE__, idx);
        return -ENOENT;
}

int32_t ti814x_ale_match_free(ti814x_dev_t *ti814x)
{
        uint32_t ale_entry[ALE_ENTRY_WORDS];
        int32_t type, idx;

        for (idx = 0; idx < ALE_ENTRIES; idx++) {
                ti814x_ale_read(ti814x, idx, ale_entry);
                type = ti814x_ale_get_entry_type(ale_entry);
                if (type == ALE_TYPE_FREE){
                    slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - matched FREE at idx=%d", __FUNCTION__, __LINE__, idx);
                        return idx;
                }
        }
        return -ENOENT;
}

int32_t ti814x_ale_find_ageable(ti814x_dev_t *ti814x)
{
        uint32_t ale_entry[ALE_ENTRY_WORDS];
        int type, idx;

        for (idx = 0; idx < ALE_ENTRIES; idx++) {
                ti814x_ale_read(ti814x, idx, ale_entry);
                type = ti814x_ale_get_entry_type(ale_entry);
                if ((type == ALE_TYPE_ADDR) || (type == ALE_TYPE_VLAN_ADDR)) {
                    type = ti814x_ale_get_ucast_type(ale_entry);
                    if ((type == ALE_UNICAST_AGE_NOT_TOUCHED) ||
                        (type == ALE_UNICAST_AGE_TOUCHED)){
                        slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "%s:%d - Found AGEABLE at idx=%d", __FUNCTION__, __LINE__, idx);
                        return idx;
                    }
                }
        }
        return -ENOENT;
}

int32_t ti814x_ale_print_table(ti814x_dev_t *ti814x)
{
    uint32_t idx, type;
    uint32_t ale_entry[ALE_ENTRY_WORDS];
    uint8_t addr[6];
    
    for (idx = 0; idx < ALE_ENTRIES; idx++) {
        ti814x_ale_read(ti814x, idx, ale_entry);
        type = ti814x_ale_get_entry_type(ale_entry);
        switch(type) {
            case ALE_TYPE_FREE:
                //slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "idx=%04d free entry", __FUNCTION__, __LINE__, idx)
                break;
            case ALE_TYPE_ADDR:
                ti814x_ale_get_addr(ale_entry,addr);
                slogf (_SLOGC_NETWORK, _SLOG_DEBUG1, "idx=%04d Address Entry %02x:%02x:%02x:%02x:%02x:%02x type=0x%x",
                      idx, addr[5],addr[4],addr[3],addr[2],addr[1],addr[0], type);
                break;
        }
    }
    return(0);
}