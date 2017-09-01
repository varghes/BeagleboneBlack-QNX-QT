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


#include "externs.h"
#include "proto.h"

int
tto(TTYDEV *ttydev, int action, int arg1) {
	TTYBUF 			*bup = &ttydev->obuf;
	DEV_OMAP		*dev = (DEV_OMAP *)ttydev;
	const uintptr_t	*port = dev->port;
	unsigned char 	c;
	
#ifdef WINBT

	if (dev->idle) {

		/* Check the client lists for notify conditions */
		return(tto_checkclients(&dev->tty));
	}
#endif

	switch(action) {
	case TTO_STTY:
//            if (dev->driver_pmd.cur_mode == PM_MODE_ACTIVE)
			ser_stty(dev);
		return (0);

	case TTO_CTRL:

		if(arg1 & _SERCTL_BRK_CHG)
			set_port(port[OMAP_UART_LCR], OMAP_LCR_BREAK, arg1 &_SERCTL_BRK ? OMAP_LCR_BREAK : 0);

		if(arg1 & _SERCTL_DTR_CHG)
			set_port(port[OMAP_UART_MCR], OMAP_MCR_DTR, arg1 & _SERCTL_DTR ? OMAP_MCR_DTR : 0);

		if(arg1 & _SERCTL_RTS_CHG)
		{
			if (dev->auto_rts_enable)
			{
				/* For auto-rts enable/disable RX & LS interrupts to assert/clear 
				 * input flow control (the FIFO will automatically handle the RTS line) 
				*/
				if (arg1 & _SERCTL_RTS)
					write_omap(port[OMAP_UART_IER], read_omap(port[OMAP_UART_IER] ) | OMAP_IER_RHR | OMAP_IER_LS );
				else
					write_omap(port[OMAP_UART_IER], read_omap(port[OMAP_UART_IER] ) & ~(OMAP_IER_RHR | OMAP_IER_LS ) );
			}
			else
				set_port(port[OMAP_UART_MCR], OMAP_MCR_RTS, arg1 & _SERCTL_RTS ? OMAP_MCR_RTS : 0);
		}

#ifdef WINBT
        if (arg1 & _CTL_TIMED_CHG) {

            slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "%s: Turning clocks OFF due to CTS glitch\n", __FUNCTION__);
            dev->signal_oband_notification = 0;
            omap_clock_disable(dev);
        }
#endif
		return (0);

	case TTO_LINESTATUS:
		return (((read_omap(port[OMAP_UART_MSR]) << 8) | read_omap(port[OMAP_UART_MCR])) & 0xf003);

	case TTO_DATA:
	case TTO_EVENT:
		break;

	default:
		return (0);
	}


	while (bup->cnt > 0 && (!(read_omap(port[OMAP_UART_SSR]) & OMAP_SSR_TXFULL)) )
	{
		/*
    		* If the OSW_PAGED_OVERRIDE flag is set then allow
   		* transmit of character even if output is suspended via
    		* the OSW_PAGED flag. This flag implies that the next
    		* character in the obuf is a software flow control
    		* charater (STOP/START).
    		* Note: tx_inject sets it up so that the contol
    		*       character is at the start (tail) of the buffer.
    		*
		*/
	

		if (dev->tty.flags & (OHW_PAGED | OSW_PAGED) && !(dev->tty.xflags & OSW_PAGED_OVERRIDE))
			break;

		/* Get character from obuf and do any output processing */
		dev_lock(&dev->tty);
		c = tto_getchar(&dev->tty);
		dev_unlock(&dev->tty);
	    	
		/* Print the character */
		dev->tty.un.s.tx_tmr = 3;       /* Timeout 3 */
		write_omap(port[OMAP_UART_THR], c);

		/* Clear the OSW_PAGED_OVERRIDE flag as we only want
      		* one character to be transmitted in this case.
     		*/
    		if (dev->tty.xflags & OSW_PAGED_OVERRIDE){
           		atomic_clr(&dev->tty.xflags, OSW_PAGED_OVERRIDE);
			break;
 		}	
 	}

	/* If there is still data in the obuf and we are not in a flow 
	 * controlled state then turn TX interrupts back on to notify us
	 * when the hardware is ready for more characters.
	 */
	if (bup->cnt > 0 && !(dev->tty.flags & (OHW_PAGED|OSW_PAGED)) )
	 { 
		// enable all interrupts
		set_port(dev->port[OMAP_UART_IER], OMAP_IER_THR, OMAP_IER_THR);
	 }
	 

	/* Check the client lists for notify conditions */
	return(tto_checkclients(&dev->tty));
}

void
ser_stty(DEV_OMAP *dev) {
	unsigned char	lcr = 0;
	unsigned char	efr = 0;
	const uintptr_t *port = dev->port;
	unsigned		brd = 0;
	int     		multiple;

#ifdef WINBT
	if (dev->idle) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "%s: Called while idle, ignoring\n", __FUNCTION__);
		return;
	}
#endif

	/* determine data bits */
	switch (dev->tty.c_cflag & CSIZE) {
		case CS8: ++lcr;
		case CS7: ++lcr;
		case CS6: ++lcr;
	}

	/* determine stop bits */
	if (dev->tty.c_cflag & CSTOPB)
		lcr |= OMAP_LCR_STB2;
	/* determine parity bits */
	if (dev->tty.c_cflag & PARENB)
		lcr |= OMAP_LCR_PEN;
	if ((dev->tty.c_cflag & PARODD) == 0)
		lcr |= OMAP_LCR_EPS;

	if (dev->tty.c_cflag & OHFLOW)
		efr = OMAP_EFR_AUTO_CTS;
	if (dev->tty.c_cflag & IHFLOW && dev->auto_rts_enable)
		efr |= OMAP_EFR_AUTO_RTS;

	/* Apply EFR value if changed */
	if (dev->efr != efr)
	{
		/* Switch to Config mode B to access the Enhanced Feature Register (EFR) */
		write_omap(port[OMAP_UART_LCR],0xbf);
		/* turn off S/W flow control, Config AUTO hw flow control, enable writes to MCR[7:5], FCR[5:4], and IER[7:4] */
		set_port(port[OMAP_UART_EFR], efr, efr);
		/* Switch back to operational mode */
		write_omap(port[OMAP_UART_LCR],0);
		/* Restore LCR config values */
		write_omap(port[OMAP_UART_LCR], lcr);
		dev->lcr = lcr;
		dev->efr = efr;
	}

	if (dev->tty.baud != dev->baud)
	{
		/* Get acces to Divisor Latch registers  */
		write_omap(port[OMAP_UART_LCR], OMAP_LCR_DLAB);

#ifdef OMAP5910
		/*
		 * Oscillator frequency is fixed at 12MHz. This normally wouldn't allow a 
		 * 115200 baud rate, since divisor = Fin / (16 * baud), which, for 115200, = 6.5,
		 * which is out of tolerance. There is a special register with a bit which, when
		 * set, automatically enables a 6.5 divisor value in hardware, and the DLH / DLL
		 * registers can just be programmed with a 1.
		 */

		if (dev->tty.baud == 115200)
		{
			brd = 1;
			write_omap(port[OMAP_UART_DLL], brd);
			write_omap(port[OMAP_UART_DLH], (brd >> 8) & 0xff);
			write_omap(port[OMAP_UART_OSC_12M_SEL], 0x01);
		}
		else
		{
			brd = (dev->tty.baud == 0)? 0 : (12000000 / (16 * dev->tty.baud));
			write_omap(port[OMAP_UART_DLL], brd);
			write_omap(port[OMAP_UART_DLH], (brd >> 8) & 0xff);
			write_omap(port[OMAP_UART_OSC_12M_SEL], 0x00);
		}
#else
		/* MODE_SELECT must be DISABLED before modifying the DLL and DLH registers */
		set_port(port[OMAP_UART_MDR1], OMAP_MDR1_MODE_MSK, OMAP_MDR1_MODE_DISABLE); /* Disable UART */
		multiple = dev->tty.baud > 230400 ? 13 : 16;

		brd = (dev->tty.baud == 0)? 0 : (48000000 / (multiple * dev->tty.baud));
		write_omap(port[OMAP_UART_DLL], brd);
		write_omap(port[OMAP_UART_DLH], (brd >> 8) & 0xff);
		if (multiple == 13)
    		set_port(port[OMAP_UART_MDR1], OMAP_MDR1_MODE_MSK, OMAP_MDR1_MODE_13X); /* Enable UART in 13x mode */
		else
	    	set_port(port[OMAP_UART_MDR1], OMAP_MDR1_MODE_MSK, OMAP_MDR1_MODE_16X); /* Enable UART in 16x mode */
#endif
		write_omap(port[OMAP_UART_LCR],lcr);	/* Restore LCR config values */
		dev->lcr  = lcr;
		dev->baud = dev->tty.baud;
		dev->brd  = brd;
	}

	/* Apply LCR value if changed */
	if (dev->lcr != lcr)
	{
		write_omap(port[OMAP_UART_LCR], lcr);
		dev->lcr  = lcr;
	}

	if (dev->kick_maxim) {
		unsigned kick_cnt = dev->baud / 10000;
		if (kick_cnt)
			dev->kick_maxim = kick_cnt;
	}

}


int drain_check(TTYDEV *ttydev, uintptr_t *count) {
	TTYBUF 			*bup = &ttydev->obuf;
	DEV_OMAP		*dev = (DEV_OMAP *)ttydev;
	const uintptr_t	*port = dev->port;

#ifdef WINBT
	if (dev->idle) {
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "%s: Called while idle, ignoring\n", __FUNCTION__);
		return 0;
	}
#endif

	// if the device has DRAINED, return 1
	if (bup->cnt == 0 &&
		(read_omap(port[OMAP_UART_LSR]) & OMAP_LSR_TSRE)) return 1;

	// if the device has not DRAINED, set a timer based on 50ms counts
	// wait for the time it takes for one character to be transmitted
	// out the shift register.  We do this dynamically since the
	// baud rate can change.
	if (count != NULL)
		*count = (ttydev->baud == 0) ? 0 : ((IO_CHAR_DEFAULT_BITSIZE * 20) / ttydev->baud) + 1;
	
	return 0;
}

__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-j5-evm/1.0.0/latest/hardware/devc/seromap/tto.c $ $Rev: 567817 $" );
