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




#ifndef __HID_H_INCLUDED__
#define __HID_H_INCLUDED__

#include <sys/hidut.h>
#include <sys/hiddi.h>

typedef struct _hid_keyboard_data {
  _uint16 nLeds;                  /* Number of leds                       */
  _uint16 nKeys;                  /* Size of keyboard array               */
  _uint16 nRate;                  /* Time interval to repeat (in msecs)   */
  _uint16 nDelay;                 /* Delay time interval (in msecs)       */
} hid_keyboard_data_t, * pHid_keyboard_data_t;

typedef struct _mouse_data {
  _uint16 nButtons;               /* Number of buttons                    */
#define HID_MOUSE_HAS_WHEEL       (0x01)
#define HID_MOUSE_WHEEL_ON        (0x02)
  _uint8  flags;                  /* Flags                                */
} mouse_data_t, * pMouse_data_t;

typedef struct _mouse_raw_data {
  _uint8  btnStates;              /* Buttons states (each bit == 1 corresponds to pressed button */
  _int16 x;                       /* pointer x-movement                                          */
  _int16 y;                       /* pointer y-movement                                          */
  _int16 z;                       /* wheel movement (if wheel exists)                            */
} mouse_raw_data_t, * pMouse_raw_data_t;

typedef struct _joystick_data {
  _uint16 nButtons_1;             /* Number of buttons                                           */
  _uint8  btnStates_1;            /* Buttons states (each bit == 1 corresponds to pressed button */
  _uint16 nButtons_2;             /* Number of buttons                                           */
  _uint8  btnStates_2;            /* Buttons states (each bit == 1 corresponds to pressed button */
  /* Pointer    */
  _uint32 x;
  _uint32 y;
  /* Hat switch */
  _uint32 hatSwitch;
  /* Throttle   */
  _uint32 throttle;
} joystick_data_t, * pJoystick_data_t;

typedef struct _touch_raw_data {
  _uint8  touched;                /* 1 if screen yes, otherwise 0                                */
  _int16 x;                       /* Absolute  x-position                                        */
  _int16 y;                       /* Absolute y-position                                         */
} touch_raw_data_t, * pTouch_raw_data_t;



#endif
