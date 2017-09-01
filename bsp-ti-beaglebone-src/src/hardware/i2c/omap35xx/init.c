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

#include "proto.h"

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
    fprintf(stderr, "omap_i2c_recover: Release SDA line by sending %d SCK and a STOP\n", clk_cnt);

    /* Disable system test mode and back to Normal mode */
	out16(dev->regbase + OMAP_I2C_SYSTEST,
		in16(dev->regbase + OMAP_I2C_SYSTEST) & ~OMAP_I2C_SYSTEST_ST_EN);

	return 0;
}


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
		fprintf(stderr, "omap_i2c_reset timed out: OMAP_I2C_SYSS %x\n", in16(dev->regbase + OMAP_I2C_SYSS));
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
        fprintf(stderr, "omap_i2c_reset: Bus busy after reset, try to recover it by sending SCK and STOP on the bus\n");    

		/* try to send SCK and STOP to release I2C bus that hold by a slave device */
		if (-1 == omap_i2c_bus_recover(dev)) {
			fprintf(stderr, "omap_i2c_reset: Can not recover SDA line from low level\n");
			return (-1);
		}
		delay(1);

		/* clear the status register */
		out16(dev->regbase + OMAP_I2C_STAT, in16(dev->regbase + OMAP_I2C_STAT));

        /* Check bus busy again */
        if (in16(dev->regbase + OMAP_I2C_STAT) & OMAP_I2C_STAT_BB) {
            fprintf(stderr, "omap_i2c_reset: Bus still busy after bus recovering\n");
            return (-1);
        }
    }
    
    /* enable interrupts */
    out16(dev->regbase + OMAP_I2C_IE, OMAP_I2C_IE_MASK);

	return 0;
}

void *
omap_init(int argc, char *argv[])
{
    omap_dev_t      *dev;
	uint16_t		s;

    if (-1 == ThreadCtl(_NTO_TCTL_IO, 0)) {
        perror("ThreadCtl");
        return NULL;
    }

    dev = malloc(sizeof(omap_dev_t));
    if (!dev)
        return NULL;
	
#if !defined(VARIANT_j5)
		dev->speed = 100000; 
#endif
	
    if (-1 == omap_options(dev, argc, argv)) {
       	fprintf(stderr, "omap_options: parse I2C option failed\n");
        goto fail_free_dev;
    }

    dev->regbase = mmap_device_io(dev->reglen, dev->physbase);
    if (dev->regbase == (uintptr_t)MAP_FAILED) {
        perror("mmap_device_io");
        goto fail_free_dev;
    }

    /* Initialize interrupt handler */
	if ((dev->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
        perror("ChannelCreate");
        goto fail_unmap_io;
    }

	if ((dev->coid = ConnectAttach(0, 0, dev->chid, _NTO_SIDE_CHANNEL, 0)) == -1) {
        perror("ConnectAttach");
        goto fail_chnl_dstr;
    }
    
	dev->intrevent.sigev_notify   = SIGEV_PULSE;
	dev->intrevent.sigev_coid     = dev->coid;
	dev->intrevent.sigev_code     = OMAP_I2C_EVENT;
	dev->intrevent.sigev_priority = 21;

	/*
	 * Attach interrupt
	 */
	dev->iid = InterruptAttach(dev->intr, i2c_intr, dev, 0, _NTO_INTR_FLAGS_TRK_MSK);
    if (dev->iid == -1) {
        perror("InterruptAttachEvent");
        goto fail_con_dtch;
    }
	
	/* Set up the fifo size - Get total size */
	s = (in16(dev->regbase + OMAP_I2C_BUFSTAT) >> 14) & 0x3;
	dev->fifo_size = 0x8 << s;
	/* Set up notification threshold as half the total available size. */
	dev->fifo_size >>=1;

	if(omap_i2c_reset(dev)==-1){
		fprintf(stderr, "omap_i2c_reset: reset I2C interface failed\n");
		goto fail_intr_dtch;
	}
    return dev;
fail_intr_dtch:
    InterruptDetach(dev->iid);
fail_con_dtch:
	ConnectDetach(dev->coid);
fail_chnl_dstr:
	ChannelDestroy(dev->chid);
fail_unmap_io:
    munmap_device_io(dev->regbase, dev->reglen);
fail_free_dev:
    free(dev);
    return NULL;
 
}

__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-j5-evm/1.0.0/latest/hardware/i2c/omap35xx/init.c $ $Rev: 514806 $" );
