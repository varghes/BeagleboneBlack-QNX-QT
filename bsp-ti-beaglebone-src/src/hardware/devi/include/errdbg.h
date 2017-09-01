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




#ifndef _ERRDBG_H_INCLUDED
#define _ERRDBG_H_INCLUDED

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* 
 * examples of the DEBUG macro in action
 *
 * DBG(printf("This val should be: %d\n", val););
 *
 * DBG(assert(x == someval););
 *
 * DBG(if (x == someval) 
         print("Oops");
       else 
         x = newval;
   );
 *
 *
 */

#ifdef DEBUG
#define DBG(code_fragment) do { code_fragment } while (0)
#else
#define DBG(code_fragment)  // debug: do nothing
#endif

#define VBOSE(a) if(verbose) printf("%s\n",(a))

#define err_abort(code, text) do { \
		     fprintf (stderr, "%s at \"%s\":%d %s\n", \
			      text, __FILE__, __LINE__, strerror(code)); \
		     abort(); \
		     } while (0)

#define errno_abort(text) do { \
		     fprintf (stderr, "%s at \"%s\":%d: %s\n", \
			      text, __FILE__, __LINE__, strerror(errno)); \
		     abort(); \
		     } while (0)


#define errno_exit(text) do { \
		     fprintf (stderr, "%s at \"%s\":%d: %s\n", \
			      text, __FILE__, __LINE__, strerror(errno)); \
		     exit(EXIT_FAILURE); \
		     } while (0)


#define errno_print(text) do { \
		     fprintf (stderr, "%s at \"%s\":%d: %s\n", \
			      text, __FILE__, __LINE__, strerror(errno)); \
		     } while (0)




#endif
