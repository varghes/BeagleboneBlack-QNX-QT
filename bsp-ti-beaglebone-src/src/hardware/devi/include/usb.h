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




#ifndef __USB_H_INCLUDED__
#define __USB_H_INCLUDED__


#include <sys/usbdi.h>

typedef struct _controlRequest 
{
   _uint8 dir ;    /* Direction (URB_DIR_IN or URB_DIR_OUT)                           */
   _uint8 type;    /* Type (USB_TYPE_STANDARD, USB_TYPE_CLASS or USB_TYPE_VENDOR)     */
   _uint8 rec ;    /* Recipient (USB_RECIPENT_DEVICE, _INTERFACE, _ENDPOINT, _OTHER)  */
   _uint8 req ;    /* Request - vendor specific or one of standard                    */
   _uint16 value;  /* Command parameter                                               */
   _uint16 index;  /* Command index( request specific                                 */
} controlRequest, * pControlRequest;

/* Device status flags                                                                */
#define USB_DEVICE_STATUS_FLAGS            (0xff)
#define USB_DEVICE_ON                      (0x01)   
#define USB_DEVICE_PRESENT                 (0x02)

#endif
