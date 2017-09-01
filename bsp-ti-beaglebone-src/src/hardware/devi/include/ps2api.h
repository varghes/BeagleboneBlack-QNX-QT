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



/* PS2 controller API specification */

#ifndef _PS2API_H
#define _PS2API_H

/* Controller modes                 */
#define SET_LEGACY_MODE          (1)
#define SET_MULTIPLEX_MODE       (2)


/* Initialization flags             */
#define INIT_MULTIPLEX_MODE      (0x01)
#define INIT_SELF_TEST           (0x02)
/* Next two constant - for IRQ parameters in init_contr */
#define SET_DEFAULT              (0)
#define SET_NONE                 (-1)
/* Initialize PS/2 keyboard controller                  */
int  init_contr(int flags, int nIrq_base, int nIrq_aux, int nDataPort, int nStatusPort);


/* default arguments                                    */
#define DEFAULT_DISPHANDLE       (NULL)
#define DEFAULT_COID             (-1)
/* Register logical device at the PS/2 controller       */
int  register_device(dispatch_t * dp, int (*pulse)(message_context_t *, int, unsigned, void *), 
		    int coid, int nPortNum, void * data);

/* Reset PS/2 device                                    */
int  reset_device(int nPortNum);
/* Disconnect logical device from PS/2 controller       */
int  unregister_device(int nPortNum);

/* Change scheduling priority for this port processing  */
int set_priority(int nPortNum,int nNewPrty);

/* sends command to target device but not     
   expects ACK (useful for RESET and RESET_WRAP_MODE    */
int send_device_command(int nPortNum, unsigned char cmd,
		  unsigned char * pReply, int nReplyLen);

/* Send device/controller specific command              */
int  send_command(int nPortNum, unsigned char cmd, 
		  unsigned char * pReply, int nReplyLen);
/* Send sequence of commands  to device                 */
int  send_command_set(int nPortNum, 
		  unsigned char * pCmds,  int nCmdSetLen,
		  unsigned char * pReply, int nReplyLen);

/* Extract data  comming from interrupt processing program */
unsigned char get_in_data(message_context_t * ctp);

/* Shutdown PS/2 controller                             */
void shutdown_contr();
/* Switch controller to/from multiplexing mode (now just does nothing) */
int  switch_controller_mode(int nNewMode);

/* Prototipes of Controller specific functions          */
typedef int (check_data)(int nPortNum, unsigned char nStatus);
typedef int (get_switch_cmd)(int nPortNum);

#define NOT_DEF_IRQ_TYPE         (0)
#define KBD_IRQ_TYPE             (1)
#define AUX_IRQ_TYPE             (2)

#define RET_DATA_ERROR           (-1)
#define RET_NO_DATA              (-2)
typedef int (validate_data)(int nStatus, int nIrqId); 
typedef int (enable_port_cmd)(int nPortNum, unsigned char cmd);
typedef int (switch_mode)(int nNewMode);

/* Replace controller specific callbacks                */ 
void set_callbacks(check_data * f_chdat, get_switch_cmd * f_getsw, validate_data * f_valid,
		  enable_port_cmd * f_encmd, switch_mode * f_swmode);

#endif // _PS2API_H
