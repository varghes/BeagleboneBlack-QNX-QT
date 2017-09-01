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


#include "startup.h"
#include <stdio.h>
#include "arm/am335x.h"

//
// Get the current time from the RTC chip. 
//
unsigned long
rtc_time(void)
{
	uint32_t rtc_addr = AM335X_RTC_BASE;

	// Stop RTC
	out32(rtc_addr+AM335x_RTC_CTRL_REG,
				in32(rtc_addr+AM335x_RTC_CTRL_REG) &
				!AM335x_RTC_CTRL_START_RTC );


	// Disable RTC
	out32(rtc_addr+AM335x_RTC_CTRL_REG,
				in32(rtc_addr+AM335x_RTC_CTRL_REG) |
				AM335x_RTC_CTRL_RTC_DISABLE );

	// Enable RTC
	out32(rtc_addr+AM335x_RTC_CTRL_REG,
				in32(rtc_addr+AM335x_RTC_CTRL_REG)
				& !AM335x_RTC_CTRL_RTC_DISABLE );

	// Write to KICK Registers
	out32(rtc_addr+AM335x_RTC_KICK0R, AM335x_KICK_DATA0);
	out32(rtc_addr+AM335x_RTC_KICK1R, AM335x_KICK_DATA1);

	// Set Oscillator clocks to use external crystal and resistor
	out32(rtc_addr+AM335x_RTC_OSC_REG,
			AM335x_RTC_OSC_32CLK_SEL |
			AM335x_RTC_OSC_RES_SELECT);

	// Turn on Clocks - 0->1 bit change enables the clocks
	out32(rtc_addr+AM335x_RTC_OSC_REG,
			AM335x_RTC_OSC_32CLK_EN |
			in32(rtc_addr+AM335x_RTC_OSC_REG));

	// Start RTC
	out32(rtc_addr+AM335x_RTC_CTRL_REG,
					in32(rtc_addr+AM335x_RTC_CTRL_REG) |
					AM335x_RTC_CTRL_START_RTC);

	return rtc_time_none();
}

__SRCVERSION( "$URL$ $Rev$" );
