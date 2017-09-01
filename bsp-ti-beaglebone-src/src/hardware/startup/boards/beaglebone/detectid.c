/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
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


/* Note that the i2c routines below have been taken from the i2c driver (in    */
/* src/hardware/i2c/omap35xx) and the boardid driver (in src/hardware/boardid/ */
/* am335x. Changes needed to be made to make it wotk in startup, eg without    */
/* interrupts, signals, fprintf, delay, nanospin_ns, and other functions.      */
/* The idea is to keep changes to a minimum so that it's easy to merge any     */
/* i2c/boardid driver updates to this file.                                    */

#include "startup.h"
#include <arm/omap.h>
#include <arm/am335x.h>
#include <arm/beaglebone.h>

/* In proto.h, unistd.h is #included. This causes a 'getopt' conflict with startup.h. */
/* To resolve this, rather than changing proto.h, we pretend it's included already:   */
#define _UNISTD_H_INCLUDED

#include "../../i2c/omap35xx/proto.h"
/* We need the 'correct' offsets.h file to get the correct i2c registers set: */
#undef __OFFSETS_H_INCLUDED
#include "../../i2c/omap35xx/arm/j5/offsets.h"
#include <hw/i2c.h>

#define VARIANT_j5				1

#define DEFAULT_PROFILE			0

#define OMAP_I2C_STAT_MASK \
	       (OMAP_I2C_STAT_XUDF  | \
	       OMAP_I2C_STAT_RDR | \
	       OMAP_I2C_STAT_XDR | \
             OMAP_I2C_STAT_ROVR | \
             OMAP_I2C_STAT_AAS | \
             OMAP_I2C_STAT_GC | \
             OMAP_I2C_STAT_XRDY | \
             OMAP_I2C_STAT_RRDY | \
             OMAP_I2C_STAT_ARDY | \
             OMAP_I2C_STAT_NACK | \
             OMAP_I2C_STAT_AL)

/////////////////////////////////////////////////////////////////////////////
// To replace timer based waits (delay, nanospin_ns)

void waitloops(int loops)
{
	volatile int i,j;

	for (i=0; i<loops; i++) { j++; j--; }
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/wait.c, with no changes.

int
omap_wait_bus_not_busy(omap_dev_t *dev, unsigned int stop)
{
    unsigned        tries = 1000000;

	if(dev->re_start) {
		if (stop){
			dev->re_start = 0;
		}
		return 0;
	}else {
		while (in16(dev->regbase + OMAP_I2C_STAT) & OMAP_I2C_STAT_BB){
			if (tries-- == 0){
				/* reset the controller to see if it's able to recover*/
				if (omap_i2c_reset(dev) == -1) {
					return -1;		// CAN NOT recover form bus busy!
				}else{
					break;
				}
			}
		}
		/* The I2C_STAT register should be 0, otherwise reset the I2C interface */
		if (in16(dev->regbase + OMAP_I2C_STAT)) {
			if (omap_i2c_reset(dev) == -1) {
				return -1;
			}
		}
		if(!stop) {
			dev->re_start = 1;
		}
		return 0;
	}

	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/wait.c. This is the i2c_intr function
// with some small changes:
//  * function name change (i2c_intr -> am335x_i2c_xfer)
//  * function type is void instead of int
//  * function parameter is a 'omap_dev_t *dev' instead of 'void *area, int id'
//  * removed OMAP_I2C_STAT_ROVR error
//  * removed return statements

void am335x_i2c_xfer(omap_dev_t *dev)
{
//	omap_dev_t *dev = area;
	uint16_t	stat;

	if ((stat = in16(dev->regbase + OMAP_I2C_STAT)) & OMAP_I2C_STAT_MASK) {

		// check errors and transaction done
		if (stat & OMAP_I2C_STAT_NACK) {
			dev->status |= I2C_STATUS_NACK ;
			out16(dev->regbase + OMAP_I2C_CON,
				   in16(dev->regbase + OMAP_I2C_CON) | OMAP_I2C_CON_STP);
			dev->re_start = 0;
		}
		if (stat & OMAP_I2C_STAT_AL) {
			dev->status |= I2C_STATUS_ARBL;
		}
		if (stat & (/*OMAP_I2C_STAT_ROVR |*/ OMAP_I2C_STAT_XUDF)) {
			dev->status |= I2C_STATUS_ERROR;
			out16(dev->regbase + OMAP_I2C_CON,
				   in16(dev->regbase + OMAP_I2C_CON) | OMAP_I2C_CON_STP);
			dev->re_start = 0;
		}
		if (stat & OMAP_I2C_STAT_ARDY) {
			dev->status |= I2C_STATUS_DONE;
		}

		// check receive interrupt
		if (stat & (OMAP_I2C_STAT_RRDY | OMAP_I2C_STAT_RDR)) {
			int num_bytes = 1;
			if (dev->fifo_size) {
				if (stat & OMAP_I2C_STAT_RRDY)
					num_bytes = dev->fifo_size;
				else
					num_bytes = (in16(dev->regbase+OMAP_I2C_BUFSTAT) & OMAP_I2C_BUFSTAT_RXSTAT) >> 8;
			}
			while (num_bytes) {
				num_bytes--;
				if (dev->xlen) {
					*dev->buf++ = (uint8_t)(in16(dev->regbase + OMAP_I2C_DATA) & 0x00FF);
					dev->xlen--;
				} else {
					break;
				}
			}
		}

		// check transmit interrupt
		if (stat & (OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_XDR)) {
			int num_bytes = 1;
			if (dev->fifo_size) {
				if (stat & OMAP_I2C_STAT_XRDY)
					num_bytes = dev->fifo_size;
				else
					num_bytes = in16(dev->regbase+OMAP_I2C_BUFSTAT) & OMAP_I2C_BUFSTAT_TXSTAT;
			}
			while (num_bytes) {
				num_bytes--;
				if (dev->xlen) {
					out8(dev->regbase + OMAP_I2C_DATA, *dev->buf++);
					dev->xlen--;
				} else {
					break;
				}
			}
		}
	}

	//clear the status
	out16(dev->regbase + OMAP_I2C_STAT, stat);

	// check transaction done
	if ((dev->status & I2C_STATUS_DONE) && dev->intexpected) {
		dev->intexpected = 0;
//		return &dev->intrevent;
	}

//	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/wait.c, changed completely. Only the
// name remains. As we are not using interrupts in startup, we don't wait for
// an interrupt to appear. Instead we simply poll the status flags until we
// received/sent all the data.

uint32_t omap_wait_status(omap_dev_t *dev)
{
	unsigned int waited=0;
	unsigned int xlen;

	xlen = dev->xlen;
	while ((dev->xlen)&&(waited<1000))
	{
		waitloops(250);
		waited++;
		am335x_i2c_xfer(dev);
		/* Reset the wait counter if we actually read/write some data */
		if (xlen != dev->xlen)
		{
			waited=0;
		}
	}

	/* Any data left to read/write? */
	if (dev->xlen)
	{
		/* If there's an error logged, we return that */
		if (dev->status) return dev->status;
		/* No error logged, verify if we exceeded 1000 loops */
		if (waited>=1000) return I2C_STATUS_BUSY;
		/* No error logged, less than 1000 loops, should be OK then */
	}
	return (dev->status);
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/recv.c, with no changes.

i2c_status_t
omap_recv(void *hdl, void *buf, unsigned int len, unsigned int stop)
{
    omap_dev_t      *dev = hdl;
    i2c_status_t    ret;

    if (len <= 0)
        return I2C_STATUS_DONE;

    if (-1 == omap_wait_bus_not_busy(dev, stop))
        return I2C_STATUS_BUSY;

	dev->xlen = len;
	dev->buf = buf;
	dev->status = 0;
	dev->intexpected = 1;

    /* set slave address */
    out16(dev->regbase + OMAP_I2C_SA, dev->slave_addr);

    /* set data count */
    out16(dev->regbase + OMAP_I2C_CNT, len);

	/* Clear the FIFO Buffers */
	out16(dev->regbase + OMAP_I2C_BUF, in16(dev->regbase + OMAP_I2C_BUF)| OMAP_I2C_BUF_RXFIF_CLR | OMAP_I2C_BUF_TXFIF_CLR);

    /* set start condition */
    out16(dev->regbase + OMAP_I2C_CON,
          OMAP_I2C_CON_EN |
          OMAP_I2C_CON_MST |
          OMAP_I2C_CON_STT |
          (stop? OMAP_I2C_CON_STP : 0)|
          (in16(dev->regbase + OMAP_I2C_CON)&OMAP_I2C_CON_XA));

	ret=  omap_wait_status(dev);

    return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/send.c, with no changes.

i2c_status_t
omap_send(void *hdl, void *buf, unsigned int len, unsigned int stop)
{
    omap_dev_t      *dev = hdl;
    i2c_status_t    ret = I2C_STATUS_ERROR;

    if (len <= 0)
        return I2C_STATUS_DONE;

    if (-1 == omap_wait_bus_not_busy(dev, stop))
        return I2C_STATUS_BUSY;

	dev->xlen = len;
	dev->buf = buf;
	dev->status = 0;
	dev->intexpected = 1;

    /* set slave address */
    out16(dev->regbase + OMAP_I2C_SA, dev->slave_addr);

    /* set data count */
    out16(dev->regbase + OMAP_I2C_CNT, len);

	/* Clear the FIFO Buffers */
	out16(dev->regbase + OMAP_I2C_BUF, in16(dev->regbase + OMAP_I2C_BUF)| OMAP_I2C_BUF_RXFIF_CLR | OMAP_I2C_BUF_TXFIF_CLR);

    /* set start condition */
    out16(dev->regbase + OMAP_I2C_CON,
          OMAP_I2C_CON_EN |
          OMAP_I2C_CON_MST |
          OMAP_I2C_CON_TRX |
          OMAP_I2C_CON_STT |
          (stop? OMAP_I2C_CON_STP : 0)|
          (in16(dev->regbase + OMAP_I2C_CON)&OMAP_I2C_CON_XA));

	ret=  omap_wait_status(dev);

    return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/bus_speed.c, with 1 change:
//  * fprintf     -> kprintf

int
omap_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed)
{
    omap_dev_t      *dev = hdl;
    unsigned long   iclk;
    unsigned        scll;

  /* This driver support bus speed range from 8KHz to 400KHz
     * limit the low bus speed to 8KHz to protect SCLL/SCLH from overflow(large than 0xff)
     * if speed=8KHz, iclk=4MHz, then SCLL=0xf3, SCLH=0xf5
     */
    if (speed > 400000 || speed < 8000) {
        kprintf("i2c-omap35xx:  Invalid bus speed(%d)\n", speed);
        errno = EINVAL;
        return -1;
    }

    /* Set the I2C prescaler register to obtain the maximum I2C bit rates
     * and the maximum period of the filtered spikes in F/S mode:
     * Stander Mode: I2Ci_INTERNAL_CLK = 4 MHz
     * Fast Mode:    I2Ci_INTERNAL_CLK = 9.6 MHz
     */

#if defined(VARIANT_j5)
    if (speed <= 100000) {
		out16(dev->regbase + OMAP_I2C_PSC, 11);     // I2Ci_INTERNAL_CLK = 4 MHz
		iclk = OMAP_I2C_ICLK;
	} else {
		out16(dev->regbase + OMAP_I2C_PSC, 4);      // I2Ci_INTERNAL_CLK = 9.6 MHz
		iclk = OMAP_I2C_ICLK_9600K;
	}
#else
    if (speed <= 100000) {
        out16(dev->regbase + OMAP_I2C_PSC, 23);     // I2Ci_INTERNAL_CLK = 4 MHz
        iclk = OMAP_I2C_ICLK;
    } else {
        out16(dev->regbase + OMAP_I2C_PSC, 9);      // I2Ci_INTERNAL_CLK = 9.6 MHz
        iclk = OMAP_I2C_ICLK_9600K;
    }
#endif

	/* OMAP_I2C_PSC is already set */
    /* Set clock for "speed" bps */
	scll = iclk/(speed << 1) - 7;

    out16(dev->regbase + OMAP_I2C_SCLL, scll);
    out16(dev->regbase + OMAP_I2C_SCLH, scll + 2);

	 dev->speed = iclk / ((scll + 7) << 1);
	 if (ospeed)
        *ospeed = dev->speed;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/init.c, with 2 changes:
//  * nanospin_ns -> waitloops
//  * fprintf     -> kprintf

#define nanospin_ns(x)			waitloops(x)
#define delay(x)				waitloops(x*1000)

/* The I2C bus error recovering function by togglling SCL line to recover
 * the SDA line that has been hold to low by a slave device
 */
static int omap_i2c_bus_recover(omap_dev_t *dev) {
	int clk_cnt = 0;

	/* Enable system test mode, Loop back mode + SDA/SCL IO mode */
	out16(dev->regbase + OMAP_I2C_SYSTEST,
		in16(dev->regbase + OMAP_I2C_SYSTEST) | OMAP_I2C_SYSTEST_ST_EN | OMAP_I2C_SYSTEST_TMODE(3));

	/* set SCL and SDA to high */
	out16(dev->regbase + OMAP_I2C_SYSTEST,
		in16(dev->regbase + OMAP_I2C_SYSTEST) | OMAP_I2C_SYSTEST_SCL_O | OMAP_I2C_SYSTEST_SDA_O);

	/* check the SDA line; if SDA has been hold to low by a slave device, toggle SCL line */
	while (0 == (in16(dev->regbase + OMAP_I2C_SYSTEST) & OMAP_I2C_SYSTEST_SDA_I))  {
		/* set SCL to high */
		out16(dev->regbase + OMAP_I2C_SYSTEST,
			in16(dev->regbase + OMAP_I2C_SYSTEST) | OMAP_I2C_SYSTEST_SCL_O);
        nanospin_ns(5000);
		/* reset SCL to low */
		out16(dev->regbase + OMAP_I2C_SYSTEST,
			in16(dev->regbase + OMAP_I2C_SYSTEST) & ~OMAP_I2C_SYSTEST_SCL_O);
        nanospin_ns(5000);
        clk_cnt++;

		if (clk_cnt > 20) {		// CAN NOT RECOVER I2C BUS ERROR
			/* set SCL and SDA to high */
			out16(dev->regbase + OMAP_I2C_SYSTEST,
				in16(dev->regbase + OMAP_I2C_SYSTEST) | OMAP_I2C_SYSTEST_SCL_O | OMAP_I2C_SYSTEST_SDA_O);
			/* Disable system test mode and back to Normal mode */
			out16(dev->regbase + OMAP_I2C_SYSTEST,
				in16(dev->regbase + OMAP_I2C_SYSTEST) & ~OMAP_I2C_SYSTEST_ST_EN);
			return (-1);
		}
    }

    /* send a STOP signal */
	out16(dev->regbase + OMAP_I2C_SYSTEST,							// reset SDA to low
		in16(dev->regbase + OMAP_I2C_SYSTEST) & ~OMAP_I2C_SYSTEST_SDA_O);
    nanospin_ns(5000);
	out16(dev->regbase + OMAP_I2C_SYSTEST,							// set SCL to high
		in16(dev->regbase + OMAP_I2C_SYSTEST) | OMAP_I2C_SYSTEST_SCL_O);
    nanospin_ns(5000);
	out16(dev->regbase + OMAP_I2C_SYSTEST,							// set SDA to high
		in16(dev->regbase + OMAP_I2C_SYSTEST) | OMAP_I2C_SYSTEST_SDA_O);
    nanospin_ns(5000);
    kprintf("omap_i2c_recover: Release SDA line by sending %d SCK and a STOP\n", clk_cnt);

    /* Disable system test mode and back to Normal mode */
	out16(dev->regbase + OMAP_I2C_SYSTEST,
		in16(dev->regbase + OMAP_I2C_SYSTEST) & ~OMAP_I2C_SYSTEST_ST_EN);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/init.c, with 4 changes:
//  * delay       -> waitloops
//  * fprintf     -> kprintf
//  * dont't enable interrupts
//  * added 'omap_set_bus_speed(dev, dev->speed, &speed)'

int omap_i2c_reset(omap_dev_t *dev){
	int 			timeout = 2000;
	unsigned int	speed;

    /* Disable interrupts */
    out16(dev->regbase + OMAP_I2C_IE, 0);
    /* Disable DMA */
    out16(dev->regbase + OMAP_I2C_BUF, 0);
    /* Disable test mode */
    out16(dev->regbase + OMAP_I2C_SYSTEST, 0);

    /* Reset module */
    if (in16(dev->regbase + OMAP_I2C_CON) & OMAP_I2C_CON_EN) {
        out16(dev->regbase + OMAP_I2C_CON, 0);
    }
	out16(dev->regbase+OMAP_I2C_SYSC, 0x2);
	out16(dev->regbase + OMAP_I2C_CON, OMAP_I2C_CON_EN);
    /* poll for reset complete */
    while (0 == (in16(dev->regbase + OMAP_I2C_SYSS) & OMAP_I2C_SYSS_RDONE) && timeout--){
        delay(1);
    }
	if(timeout<=0){
		kprintf("omap_i2c_reset timed out: OMAP_I2C_SYSS %x\n", in16(dev->regbase + OMAP_I2C_SYSS));
		return -1;
	}
	out16(dev->regbase+OMAP_I2C_SYSC, 0x215); //Smart idle mode, Fclock, Enable Wakeup, autoidle
	/*
	 * Enabling all wakup sources to stop I2C freezing on
	 * WFI instruction.
	 * REVISIT: Some wkup sources might not be needed.
	 */
	out16(dev->regbase + OMAP_I2C_WE, 0x636f);

	out16(dev->regbase + OMAP_I2C_CON, 0);

#if !defined(VARIANT_j5)
	/* Set I2C bus speed */
    omap_set_bus_speed(dev, dev->speed, &speed);
#endif

	/* Set FIFO for Tx/RX Note: setup required fifo size - 1*/
	out16(dev->regbase + OMAP_I2C_BUF, (dev->fifo_size - 1) << 8 | /* RTRSH */
									OMAP_I2C_BUF_RXFIF_CLR |
									(dev->fifo_size - 1) | /* XTRSH */
									OMAP_I2C_BUF_TXFIF_CLR);

    /* Set Own Address */
    out16(dev->regbase + OMAP_I2C_OA, dev->own_addr);
    /* Take module out of reset */
    out16(dev->regbase + OMAP_I2C_CON, OMAP_I2C_CON_EN);
	delay(1);

   /* Check bus busy */
    if (in16(dev->regbase + OMAP_I2C_STAT) & OMAP_I2C_STAT_BB) {
        kprintf("omap_i2c_reset: Bus busy after reset, try to recover it by sending SCK and STOP on the bus\n");

		/* try to send SCK and STOP to release I2C bus that hold by a slave device */
		if (-1 == omap_i2c_bus_recover(dev)) {
			kprintf("omap_i2c_reset: Can not recover SDA line from low level\n");
			return (-1);
		}
		delay(1);

		/* clear the status register */
		out16(dev->regbase + OMAP_I2C_STAT, in16(dev->regbase + OMAP_I2C_STAT));

        /* Check bus busy again */
        if (in16(dev->regbase + OMAP_I2C_STAT) & OMAP_I2C_STAT_BB) {
            kprintf("omap_i2c_reset: Bus still busy after bus recovering\n");
            return (-1);
        }
    }

//    /* enable interrupts */
//    out16(dev->regbase + OMAP_I2C_IE, OMAP_I2C_IE_MASK);

    /* added: */
    omap_set_bus_speed(dev, dev->speed, &speed);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/i2c/omap35xx/init.c, with 3 changes:
//  * nanospin_ns -> waitloops
//  * delay       -> waitloops
//  * fprintf     -> kprintf
//  * dont't enable interrupts
//  * added 'omap_set_bus_speed(dev, dev->speed, &speed)'

void *omap_init(int argc, char *argv[])
{
	static omap_dev_t	stdev;
	omap_dev_t   	   *dev = &stdev;
	uint16_t			s;

	/* defaults */
	dev->intr        = AM335X_I2C0_IRQ;
	dev->iid         = -1;
	dev->physbase    = AM335X_I2C0_BASE;
	dev->reglen      = OMAP_I2C_SIZE;
	dev->own_addr    = OMAP_I2C_ADDRESS;
	dev->slave_addr  = TWL4030_AUDIO_SLAVE_ADDRESS; /* audio codec */
	dev->options     = 0;
	dev->re_start    = 0;
    dev->regbase     = dev->physbase;
	dev->speed       = 100000;

	dev->intrevent.sigev_notify   = SIGEV_PULSE;
	dev->intrevent.sigev_coid     = dev->coid;
	dev->intrevent.sigev_code     = OMAP_I2C_EVENT;
	dev->intrevent.sigev_priority = 21;

	/* Set up the fifo size - Get total size */
	s = (in16(dev->regbase + OMAP_I2C_BUFSTAT) >> 14) & 0x3;
	dev->fifo_size = 0x8 << s;
	/* Set up notification threshold as half the total available size. */
	dev->fifo_size >>=1;

	if (omap_i2c_reset(dev)==-1)
	{
		kprintf("omap_i2c_reset: reset I2C interface failed\n");
	}

    return dev;
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/boardid/am335x/boardid.c, with major changes. No longer
// i2c is accessed via the driver, but directly.

static int bdid_read(omap_dev_t *dev, int address, BDIDENT *bdident, int showerr)
{
    uint8_t		val[256]= { 0, 0 };
    uint32_t	stop = 0;
    int			rw, rr;

    dev->slave_addr = address;

    /* Write request to Board ID EEPROM */
    stop = 0;
    rw   = omap_send(dev, val, 2, stop);

    /* We ignore the return value, and try to read anyway.                */
    /* If there's a write error, we rely on a read timeout or read error. */

    memset(val, 0, 256);

    /* Read reply from Board ID EEPROM */
    stop = 1;
    rr   = omap_recv(dev, val, 256, stop);
    if (rr)
    {
    	if (showerr)
    	{
			if (rw)
			{
				kprintf("I2C write failed, error %d\n", rw);
				return (-1);
			}
			kprintf("I2C read failed, error %d\n", rr);
    	}
       	return (-1);
    }

    memcpy(&bdident->header , &val[0]                         , AM335X_BDID_HEADER_LEN);
	memcpy(&bdident->bdname , &val[AM335X_BDID_BDNAME_OFFSET] , AM335X_BDID_BDNAME_LEN);
	bdident->bdname[AM335X_BDID_BDNAME_LEN] = 0;
	memcpy(&bdident->version, &val[AM335X_BDID_VERSION_OFFSET], AM335X_BDID_VERSION_LEN);
	bdident->version[AM335X_BDID_VERSION_LEN] = 0;
	memcpy(&bdident->serial , &val[AM335X_BDID_SERIAL_OFFSET] , AM335X_BDID_SERIAL_LEN);
	bdident->serial[AM335X_BDID_SERIAL_LEN] = 0;
	memcpy(&bdident->config , &val[AM335X_BDID_CONFIG_OFFSET] , AM335X_BDID_CONFIG_LEN);
	bdident->config[AM335X_BDID_CONFIG_LEN] = 0;
	memcpy(&bdident->macaddr, &val[AM335X_BDID_MAC1_OFFSET]   , AM335X_BDID_MAC_LEN*AM335X_MACS);

    return (0);
}

/////////////////////////////////////////////////////////////////////////////
// Copied from src/hardware/boardid/am335x/boardid.c, with 1 change:
// * replaced printf by kprintf

void iddump(BDIDENT *bdident)
{
    kprintf("header:  %x\n", bdident->header);
    kprintf("name:    %s\n", bdident->bdname);
    kprintf("version: %s\n", bdident->version);
    kprintf("serial:  %s\n", bdident->serial);
    if (bdident->config[0])
       kprintf("config:  %s"  , bdident->config);
    if (bdident->macaddr[0][0])
   	   kprintf("mac1:    %x.%x.%x.%x.%x.%x\n", bdident->macaddr[0][0], bdident->macaddr[0][1], bdident->macaddr[0][2], bdident->macaddr[0][3], bdident->macaddr[0][4], bdident->macaddr[0][5]);
    if (bdident->macaddr[1][0])
       kprintf("mac2:    %x.%x.%x.%x.%x.%x\n", bdident->macaddr[1][0], bdident->macaddr[1][1], bdident->macaddr[1][2], bdident->macaddr[1][3], bdident->macaddr[1][4], bdident->macaddr[1][5]);
    if (bdident->macaddr[2][0])
	   kprintf("mac3:    %x.%x.%x.%x.%x.%x\n", bdident->macaddr[2][0], bdident->macaddr[2][1], bdident->macaddr[2][2], bdident->macaddr[2][3], bdident->macaddr[2][4], bdident->macaddr[2][5]);
}

void dump(unsigned int address, BDIDENT *bdident)
{
	kprintf("// Board ID [%x] ////////\n", address);
	if (bdident->header == 0xee3355aa)
	{
		iddump(bdident);
	}
	else
	{
		kprintf("Invalid header [%x], expected %x\n", bdident->header, 0xee3355aa);
	}
	kprintf("/////////////////////////\n", address);
}

/////////////////////////////////////////////////////////////////////////////

void get_boardid_i2c(BEAGLEBONE_ID *boneid)
{
	int			 r;
	int			 i;
	BDIDENT		 bd;
	omap_dev_t	*dev;

	if (boneid==NULL)
	{
		kprintf("Parameter NULL: can't return board id information\n");
		return;
	}

	memset(boneid, 0, sizeof(BEAGLEBONE_ID));

	boneid->basebd_type     = bb_not_detected;
	for (i=0; i<AM335X_I2C0_MAXCAPES; i++)
	{
		boneid->cape_type[i] = ct_not_detected;
	}

	dev = omap_init(0, NULL);
	if (dev != NULL)
	{
		r = bdid_read(dev, AM335X_I2C0_BBID, &bd, 1);
		if (!r)
		{
			//dump(0x50, &bd);
			if (strcmp(bd.bdname, "A335BONE")==0) boneid->basebd_type = bb_BeagleBone;
			else                                  boneid->basebd_type = bb_unknown;
		}

		for (i=0; i<AM335X_I2C0_MAXCAPES; i++)
		{
			r = bdid_read(dev, AM335X_I2C0_CAPE0+i, &bd, 0);
			if (!r)
			{
				//dump(0x51, &bd);
				                                  boneid->cape_type[i] = ct_unknown;
			}
		}
	}

	if (boneid->basebd_type == bb_BeagleBone)
	{
		kprintf("BeagleBone detected\n");
	}
	else
	{
		kprintf("Not a BeagleBone??\n");
	}

	for (i=0; i<AM335X_I2C0_MAXCAPES; i++)
	{
		if (boneid->cape_type[i] == ct_unknown) kprintf("Cape #%d detected\n", i);
	}
}

unsigned long detect_frequency_using_power()
{
	omap_dev_t	*dev;
	uint32_t	stop = 0;
	uint8_t		val[40]= { 0, 0 };
	int			rw, rr;
	unsigned long retval = BEAGLEBONE_MPUFREQ; // Default value assumes USB Powered

	dev = omap_init(0, NULL);
	if (dev != NULL)
	{
		dev->slave_addr = 0x24;
		/* Write request to Power Chip */
		stop = 0;
		rw   = omap_send(dev, val, 2, stop);
		/* Read reply from Power Chip */
		stop = 1;
		rr   = omap_recv(dev, val, 0x20, stop);
		if (rr)
		{
			if (rw)
			{
				kprintf("I2C write failed (power), error %d\n", rw);
			}
			kprintf("I2C read failed (power), error %d\n", rr);
		}
		else
		{
			/*
			kprintf("TI Power Registers:\n");
			for (i=0;i<0x20;i++)
			{
				kprintf("%x ",(int)val[i]);
				if (!((i+1)%8))
				{
					kprintf("\n");
				}
			}
			kprintf("\n");
			*/
			// By experiment, this bit is set when BeagleBoard powered by 5V,
			// and clear when powered by USB
			if (val[9] & 0x8)
			{
				retval = BEAGLEBONE_MPUFREQ_5V;
			}
		}
	}

	return retval;
}

