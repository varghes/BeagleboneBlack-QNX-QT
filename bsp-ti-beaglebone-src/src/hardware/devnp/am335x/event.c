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
#include    <netinet/in.h>

#if NBPFILTER > 0
#include    <net/bpf.h>
#include    <net/bpfdesc.h>
#endif

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
int ti814x_process_interrupt (void *arg, struct nw_work_thread *wtp)
{
    ti814x_dev_t                    *ti814x = arg;

    ti814x_receive(ti814x, wtp);
    ti814x->pkts_received = 1;
    
    return (1);
}

/**************************************************************************/
/*                                                                        */
/**************************************************************************/
const struct sigevent *ti814x_isr (void *arg, int iid)
{
    ti814x_dev_t                *ti814x = arg;
    struct _iopkt_inter         *ient = &ti814x->inter;

    outle32 (ti814x->cpsw_regs + RX_INTMASK_CLEAR, RX0_PEND_MASK);
    outle32(ti814x->cpsw_regs + CPDMA_EOI_VECTOR, 0x1);
    
    return  interrupt_queue(ti814x->iopkt, ient);
}

/**************************************************************************/
/* device_enable_interrupt - enables rx interrupts on the J5 platform      */
/**************************************************************************/
int ti814x_enable_interrupt (void *arg)
{
    ti814x_dev_t *ti814x = arg;
    outle32 (ti814x->cpsw_regs + RX_INTMASK_SET, RX0_PEND_MASK);
    return 1;
}


/**************************************************************************/
/*                                                                        */
/**************************************************************************/
void ti814x_hk_callout (void *arg)
{
    ti814x_dev_t            *ti814x = arg;

    callout_msec(&ti814x->hk_callout, 1 * 1000, ti814x_hk_callout, ti814x);
}

