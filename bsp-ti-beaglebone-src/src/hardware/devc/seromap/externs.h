/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
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








#ifdef DEFN
	#define EXT
	#define INIT1(a)				= { a }
#else
	#define EXT extern
	#define INIT1(a)
#endif

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <termios.h>
#include <devctl.h>
#include <sys/dcmd_chr.h>
#include <sys/iomsg.h>
#include <atomic.h>
#include <hw/inout.h>
#include <arm/omap.h>
#include <sys/io-char.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>

#include <sys/trace.h>

char	*user_parm;

/* Activate code for BT and Powermanagement */
#ifdef VARIANT_winbt
    #ifndef WINBT
            #define WINBT
    #endif
#endif


#ifdef WINBT
/* Increase the UART size to include the SYSC register */
#undef 	OMAP_UART_SIZE
#define OMAP_UART_SIZE 	0x60
#define OMAP_UART_SYSC 	0x54
#define OMAP_UART_WER 	0x5C
#endif

#define FIFO_TRIG_8     1
#define FIFO_TRIG_16    2
#define FIFO_TRIG_32    3
#define FIFO_TRIG_56    4
#define FIFO_TRIG_60    5

#define FIFO_SIZE       64  /* size of the rx and tx fifo's */

typedef struct dev_omap {
	TTYDEV			tty;
	struct dev_omap	*next;
	unsigned		intr;
	int				iid;
	unsigned		clk;
	unsigned		div;
	unsigned char	fifo;
	uintptr_t		port[OMAP_UART_SIZE];

	unsigned		kick_maxim;

	unsigned		brd;		/* Baud rate divisor */
	unsigned		lcr;
	unsigned		efr;
	unsigned		baud;

	pmd_attr_t		driver_pmd;	/* Power Management attributes associated with each device */
	struct sigevent	pwm_event;	/* Power Management event associated with each device */
	volatile unsigned	pwm_flag;
#define	SEROMAP_PWM_PAGED	0x1	/* Flag to tell tto not to transmit data */
	unsigned		auto_rts_enable;
#ifdef WINBT
	intrspin_t idle_spinlock;
	uint32_t physbase;
	uint32_t clkctrl_phys;
	uintptr_t clkctrl_base;
	uintptr_t pinmux_base;
	uintptr_t gpio3_base;
	int		idle; 				/* idle flag signaled by a devctl command */
	int     signal_oband_notification;
#endif
} DEV_OMAP;

typedef struct ttyinit_omap {
	TTYINIT		tty;
	unsigned	pwm_init;
	unsigned	loopback; 	/*loopback mode*/
	unsigned	auto_rts_enable;

}TTYINIT_OMAP;

#define	SEROMAP_NUM_POWER_MODES	4

EXT TTYCTRL						ttyctrl;

/* Should put into arm/omap.h */
#define	OMAP_SCR_WAKEUPEN		(1 << 4)
#define	OMAP_SSR_WAKEUP_STS		(1 << 1)

#define	SEROMAP_NOKICK			55

#include "proto.h"
#include <variant.h>

__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-j5-evm/1.0.0/latest/hardware/devc/seromap/externs.h $ $Rev: 503296 $" )
