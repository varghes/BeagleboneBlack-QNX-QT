/*
 * Copyright 2003, QNX Software Systems Ltd. All Rights Reserved.
 *
 * This source code may contain confidential information of QNX Software
 * Systems Ltd.  (QSSL) and its licensors. Any use, reproduction,
 * modification, disclosure, distribution or transfer of this software,
 * or any software which includes or is based upon any of this code, is
 * prohibited unless expressly authorized by QSSL by written agreement. For
 * more information (including whether this source code file has been
 * published) please email licensing@qnx.com.
 */




#ifndef EXTERN_H_INCLUDED
#ifndef EXT
#define EXT extern
#endif

#define EXTERN_H_INCLUDED

#include <sys/devi.h>

extern int		optind, opterr, optopt;
extern char		*optarg;

EXT int			verbosity;

EXT event_bus_line_t	*Bus;	/* pointer to list of event bus lines 
				   managed by devi */

EXT int			IpGroup; /* input group (used by photon i/f) */
EXT char		*IpTargetDev; /* name of photon device to open */
EXT unsigned int        OptFlags; /* global options */
EXT int			IpCurrentFd; /* fd used to send messages via photon */
EXT input_module_t	*ModuleTable[MODULE_TABLE_SIZE]; /* table of currently
							    supported modules */
EXT common_callbacks_t commonCallbacks; /* Global callback functions     */
EXT char                *pServ_name;
EXT char                *pFull_process_name; // Full process name
#endif

