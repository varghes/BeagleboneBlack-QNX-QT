/*
 * $QNXLicenseC:
 * Copyright 2007, QNX Software Systems. All Rights Reserved.
 * 
 * You must obtain a written license from and pay applicable license fees to QNX 
 * Software Systems before you may reproduce, modify or distribute this software, 
 * or any work that includes all or part of this software.   Free development 
 * licenses are available for evaluation and non-commercial purposes.  For more 
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *  
 * This file may contain contributions from others.  Please review this entire 
 * file for other proprietary rights or license notices, as well as the QNX 
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/ 
 * for other information.
 * $
 */




#ifndef __SERIOBT_H_INCLUDED
#define __SERIOBT_H_INCLUDED

#include <devctl.h>

#define _DCMD_SERIOBT  _DCMD_MISC

#define DCMD_SERIOBT_SETADDR             __DIOT(_DCMD_SERIOBT, 1, char[17])

/**
 * Mechanism to signal to the devc-seriobt that the open/close
 * override should signal to the bluetooth stack to create/destroy a service
 * based upon the open/close resource manager override methods
 * byte - 0 - do not link open/close with a service connection
 * byte - 1 - link open/close with a service connection (this is default behavior)
 */
#define DCMD_SERIOBT_CONNECTION_SCHEME   __DIOT(_DCMD_SERIOBT, 2, 1) // expecting a byte identifier

#endif
