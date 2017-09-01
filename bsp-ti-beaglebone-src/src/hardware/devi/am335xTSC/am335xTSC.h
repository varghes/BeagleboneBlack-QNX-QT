/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems.
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

#ifndef AM335XTSC_H_
#define AM335XTSC_H_

#include <sys/slog.h>
#include <sys/slogcodes.h>

/*
 * Types 
 */
typedef struct
{
	_uint32		x;
	_uint32		y;
	
} t_point;

/* 
 * Defines
 */
#define AM335XTSC_VERSION 100

/* Debug and system log macros */
#define AM335XTSC_TRACE 0
#if AM335XTSC_TRACE
 #define TRACE(fmt, arg...) (void)fprintf(stderr, ":am335xTSC:" fmt"\n", ##arg)
#else
 #define TRACE(fmt, arg...) ((void)0)
#endif

#define SLINFO(fmt, arg...) (void)slogf(_SLOG_SETCODE(_SLOGC_INPUT, 1), _SLOG_INFO, fmt, ##arg)

#endif
