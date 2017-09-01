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




#ifndef __PHOTON_H_INCLUDED__
#define __PHOTON_H_INCLUDED__

#include <Ph.h>
#include <photon/PhT.h>
#include <photon/PhSystem.h>
#include <photon/PhService.h>

extern struct 		_Ph_ctrl *_Ph_;

#define MAX_QUERY_RIDS	100

/* injection constants */

#define INJECTED		1
#define NOT_INJECTED	        ~INJECTED
#define MAX_COORD		50

#define SIG_INPUT_EVENT SIGRTMIN
#define SIG_PH_EVENT SIGRTMIN+1


typedef struct _input_data {

  struct {
	unsigned char		type;
	unsigned char		flags;
	long 			reply_pid;
   } hdr;

  union {
	struct packet_kbd	        kbd;
	struct packet_rel	        rel;
	struct packet_abs	        abs;
	char 				data[1];
  } u;
} input_data_t;


typedef struct {

	int			flags;
  	PhRid_t			rid;
  	PhRect_t	        rect;
  	PhRegion_t		region;
  	int			throttle;
} input_region_data_t;

typedef	struct {

  unsigned                      millisec;
  unsigned                      abs_max;

  union {
    PhRawPtrEvent_t		ptr;
    char			padding[offsetof(PhRawPtrEvent_t, coord) + 
					MAX_COORD * sizeof(PhRawPtrCoord_t)];
  } u;

} inject_event_t;


/* Protos */

int		photon_connect(char *, input_region_data_t *);
void		inject(buffer_t *, input_data_t *, input_region_data_t *);
void 		trig_ptr(PhRawPtrEvent_t *, unsigned, PhRid_t);
void 		trig_kbd(PhRawKeyEvent_t *, unsigned, PhRid_t); 
void *		ph_thread(void *);
void		injector_specific_destroy(void *);
int 		photon_input_pulse(input_region_data_t *);
int		query_grafx_region(int, PhRect_t *);
void		slay_photon(unsigned);
pid_t		photon_pid(void);

#endif



