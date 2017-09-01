/*
 * $QNXLicenseC:
 * Copyright 2011, QNX Software Systems. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "externs.h"

#ifdef WINBT

#define CONTROL_CORE_PAD0_UART2_CTS_PAD1_UART2_RTS 0x4a100118

#define OMAP44XX_GPIO_REG_SIZE 0x1000

/* UART2_RTS/GPIO124 is in the 4th GPIO block */
#define OMAP44XX_GPIO3_BASE 0x48059000

/* 4th GPIO block holds GPIOs 96-127, 124 is the 28th GPIO in the block */
#define OMAP44XX_GPIO124_MASK (1 << 28)

#define OMAP44XX_CLEARDATAOUT_REG 0x190
#define OMAP44XX_SETDATAOUT_REG 0x194

void
omap_force_rts(DEV_OMAP* dev, int level)
{
	if (level) {
		/*
		 * Set UART2_RTS I/O to GPIO and set data out to ensure the BT chip
		 * sees our RTS as high and doesn't transmit until we're fully awake
		 */
		out32(dev->pinmux_base, 0x01030118);
		out32((dev->gpio3_base + OMAP44XX_SETDATAOUT_REG), OMAP44XX_GPIO124_MASK);
	} else {
		/* Revert GPIO124 to its primary function (UART2_RTS) */
		out32((dev->gpio3_base + OMAP44XX_CLEARDATAOUT_REG), OMAP44XX_GPIO124_MASK);
		out32(dev->pinmux_base, 0x00000118);
	}
}

int
omap_force_rts_init(DEV_OMAP* dev)
{
	dev->pinmux_base = mmap_device_io(4, CONTROL_CORE_PAD0_UART2_CTS_PAD1_UART2_RTS);
	if (dev->pinmux_base == (uintptr_t)MAP_FAILED) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "%s: mmap_device_io pinmux_base: %s\n", __FUNCTION__, strerror(errno));
		goto fail1;
	}

	dev->gpio3_base = mmap_device_io(OMAP44XX_GPIO_REG_SIZE, OMAP44XX_GPIO3_BASE);
	if (dev->gpio3_base == (uintptr_t)MAP_FAILED) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "%s: mmap_device_io gpio3_base: %s\n", __FUNCTION__, strerror(errno));
		goto fail2;
	}

	return 0;

fail2:
	munmap_device_io(dev->pinmux_base, 4);
	dev->pinmux_base = NULL;

fail1:
	return -1;

}
#endif /* WINBT */

__SRCVERSION( "$URL: $ $Rev: $" );
