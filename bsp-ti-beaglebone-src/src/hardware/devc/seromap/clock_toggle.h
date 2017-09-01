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

#ifndef CLOCK_TOGGLE_H_
#define CLOCK_TOGGLE_H_

// We use these to determine which CLKCTRL to use
#define OMAP_UART1_PHYSBASE 0x4806A000
#define OMAP_UART2_PHYSBASE 0x4806C000
#define OMAP_UART3_PHYSBASE 0x48020000
#define OMAP_UART4_PHYSBASE 0x4806E000

#define CM_L4PER_UART1_CLKCTRL 0x4A009540
#define CM_L4PER_UART2_CLKCTRL 0x4A009548
#define CM_L4PER_UART3_CLKCTRL 0x4A009550
#define CM_L4PER_UART4_CLKCTRL 0x4A009558

#define OMAP_CLKCTRL_MODMODE_MASK 0x3
#define OMAP_CLKCTRL_MODMODE_ENABLE 0x2
#define OMAP_CLKCTRL_MODMODE_DISABLE 0x0
#define OMAP_CLKCTRL_IDLEST_MASK 0x30000
#define OMAP_CLKCTRL_IDLEST_FUNCTIONAL (0x0 << 16)

#define OMAP_UART_SYSC_IDLEMODE_MASK 		0x18
#define OMAP_UART_SYSC_IDLEMODE_SMART 		(0x2 << 3)
#define OMAP_UART_SYSC_IDLEMODE_NONE 		(0x1 << 3)
#define OMAP_UART_SYSC_IDLEMODE_FORCE 		(0x0 << 3)
#define OMAP_UART_SYSC_IDLEMODE_SMARTWAKEUP (0x3 << 3)

#define OMAP_UART_SYSC_ENAWAKEUP_DISABLE	(0x0 << 2)
#define OMAP_UART_SYSC_ENAWAKEUP_ENABLE		(0x1 << 2)

#define OMAP_UART_SYSC_AUTOIDLE_MASK 0x1
#define OMAP_UART_SYSC_AUTOIDLE_ENABLE 0x1
#define OMAP_UART_SYSC_AUTOIDLE_DISABLE 0x0

#define OMAP_UART_WER_CTS_ENABLE			(0x1 << 0)
#define OMAP_UART_WER_CTS_DISABLE			(0x0 << 0)

#endif /* CLOCK_TOGGLE_H_ */

__SRCVERSION( "$URL: $ $Rev: $" );
