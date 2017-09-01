/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software.   Free development licenses are
 * available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include "externs.h"
#include "clock_toggle.h"

int
winbt_devctl(resmgr_context_t * ctp, io_devctl_t * msg, iofunc_ocb_t * ocb)
{
	DEV_OMAP* dev = (DEV_OMAP*)ocb->attr;
	int* data = NULL;

	switch (msg->i.dcmd) {
		case DCMD_CHR_IDLE:
			// if active disable the clocks and set the flag for idle
			omap_clock_disable(dev);
			break;
		case DCMD_CHR_RESUME:
			// if idle enable the clocks and deassert the idle flag
			omap_clock_enable(dev);
			break;
		case DCMD_CHR_FORCE_RTS:
			// force RTS to low or high. If low, it will put RTS control back
			// in the hands of the device's auto RTS functionality
			data = _DEVCTL_DATA(msg->i);
			InterruptLock(&dev->idle_spinlock);
			omap_force_rts(dev, *data);
			InterruptUnlock(&dev->idle_spinlock);
			break;
		default:
			return ENOTTY;
	}

	return EOK;
}

