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

#include <gulliver.h>
#include <pthread.h>
#include <time.h>
#include <sys/devi.h>
#include "am335xTSC.h"

#define FLAG_INIT					0x1000

#define XOFF						210		  	/* Lowest touch screen x value observed */
#define YOFF						290		  	/* Lowest touch screen y value observed */
#define XMAX						3980		/* Highest touch screen x value observed */
#define YMAX						3860		/* Lowest touch screen y value observed */

#define DEFAULT_SCREEN_X			800	 	 	/* Default X Scale factor from touch to image */
#define DEFAULT_SCREEN_Y			480			/* Default Y Scale factor from touch to image */


struct data
{
    _uint16				flags;
	_uint16				buttons;
	uint32_t			xdim;
	uint32_t			ydim;
	float				xscale_factor;
	float				yscale_factor;
    struct packet_abs	tp;
    uint8_t				xreverse;
    uint8_t				yreverse;

    /* next structure's fields we use to send command to SmartSet controller and to receive response */
    pthread_mutex_t		mutex;
    pthread_cond_t		cond;
};


static int am335xTSC_proto_init(input_module_t *module);
static int am335xTSC_proto_reset(input_module_t *module);
static int am335xTSC_proto_input(input_module_t *module, int num, void *ptr);
static int am335xTSC_proto_parm(input_module_t *module,int opt,char *optarg);
static int am335xTSC_proto_devctrl(input_module_t *module,int event,void *ptr);
static int am335xTSC_proto_shutdown(input_module_t *module, int delay);


input_module_t  am335xTSC_proto =
{
    NULL,
    NULL,
    NULL,
    0,
    DEVI_CLASS_ABS|DEVI_MODULE_TYPE_PROTO,
    "pro1",
    __DATE__,
    "x:y:h:v:",				/* arguments */
    NULL,                   /* private data initialized in init */
    am335xTSC_proto_init,
    am335xTSC_proto_reset,
    am335xTSC_proto_input,
    NULL,
    NULL,
    am335xTSC_proto_parm,
    am335xTSC_proto_devctrl,
    am335xTSC_proto_shutdown,
};


/* Description: callback initialisation function; it is called when input module
 *              initializes input system
 * Input      : input_module_t * module - pointer to module descriptor
 * Output     : None
 * Return     : 0 if OK, otherwise - (-1)
 */
int am335xTSC_proto_init(input_module_t *module)
{
    struct data *dp = module->data;

	TRACE("am335xTSC_proto_init -->");
	SLINFO("am335xTSC touchscreen driver, version %d", AM335XTSC_VERSION);

    if(!module->data)
    {
        if(!(dp = module->data = calloc(sizeof *dp, 1)))
        {
            return (-1);
        }
        dp->flags = ABSOLUTE | ABS_UNCALIBRATED;
		dp->buttons = 0;

		dp->xdim = DEFAULT_SCREEN_X;
		dp->ydim = DEFAULT_SCREEN_Y;

		if((XMAX-XOFF) > dp->xdim)
			dp->xscale_factor = 1.0 / ((float)(XMAX-XOFF) / dp->xdim);
		else
			dp->xscale_factor = (float)dp->xdim / (XMAX-XOFF);

		if((YMAX-YOFF) > dp->ydim)
			dp->yscale_factor = 1.0 / ((float)(YMAX-YOFF) / dp->ydim);
		else
			dp->yscale_factor = (float)dp->ydim / (YMAX-YOFF);

		TRACE("am335xTSC_proto_init: default scale factors x=%f, y=%f",
				dp->xscale_factor, dp->yscale_factor);

		memset(&(dp->tp), 0, sizeof(struct packet_abs));
    }
    return (0);
}


/* Description: this callback function is called when the module is linked into the
 *              event bus;it is used to set initial module state on the protocol
 *              level
 * Input      : input_module_t * module - pointer to module descriptor
 * Output     : None
 * Return     : 0 if OK, otherwise -1
 */
int am335xTSC_proto_reset(input_module_t * pModule)
{
    struct data *dp = pModule->data;

	TRACE("am335xTSC_proto_reset -->");

    if((dp->flags & FLAG_INIT) == 0)
    {
        dp->flags |= FLAG_INIT;
    }

    return (0);
}



/* Description: main protocol processing function.  It will be called  by the
 *              device layer to pass it data to process.  Its job is to interpret
 *              the data according to the ADS784x touch screen protocol, create
 *              a data structure, fill it in and hand it off to the  filter layer
 *              module above it. The protocol can be processed using simple state
 *              machine.  State info is kept in module's private data
 * Input      : input_module_t * module - pointer to module descriptor
 *              int num - number of bytes to process
 *              void * ptr - raw data to process
 * Output     : None
 * Return     : 0 if OK, otherwise -1
 *
 * NOTE		  : ptr MUST be word aligned; i.e. DO NOT PASS AN UNALIGNED STATIC VAR
 *
 */
int am335xTSC_proto_input(input_module_t *module, int num, void *ptr)
{
    struct data		*dp = module->data;
    input_module_t	*up = module->up;

	TRACE("am335xTSC_proto_input-->");

	if(dp->flags & FLAG_INIT)
	{
		_uint16 x, y;
		t_point *point;

		point = (t_point*)ptr;
		x = point->x;
		y = point->y;

		TRACE("am335xTSC_proto_input: received input x=%d y=%d",  point->x,  point->y);

		dp->tp.flags = dp->flags;

		if ( (x == 0) && (y == 0) )
		{
			/* 'Mouse up' event, reset our internal sampling variables and return the
			 * last accepted coords (dp->tp) with the button released.
			 */
			dp->buttons = 0;

			dp->tp.z = 0;
			dp->tp.buttons = dp->buttons;
			clk_get(&dp->tp.timestamp);

			TRACE("am335xTSC_proto_input: sending UP event");

			(up->input)(up, 1, &dp->tp);
		}
		else
		{
			dp->buttons = _POINTER_BUTTON_LEFT;
			
			/* scale to fit the screen */
			if (x < XOFF)
				x = 0;
			else
			{
				//x = ((x-XOFF) * dp->xdim) / (XMAX - (dp->xscale_factor*XOFF));

				x -= XOFF;
				x = (float)x * dp->xscale_factor;
				if (dp->xreverse)
					x = dp->xdim - x;
			}
			if (y < YOFF)
				y = 0;
			else
			{
				//y = ((y-YOFF) * dp->ydim) / (YMAX - (dp->yscale_factor*YOFF));

				y -= YOFF;
				y = (float)y * dp->yscale_factor;
				if (dp->yreverse)
					y = dp->ydim - y;
			}

			dp->tp.x = x;
			dp->tp.y = y;
			dp->tp.z = 0;
			dp->tp.buttons = dp->buttons;

			TRACE("am335xTSC_proto_input: sending DOWN event x=%d y=%d", dp->tp.x, dp->tp.y);

			clk_get(&dp->tp.timestamp);
			(up->input)(up, 1, &dp->tp);
		}
	}

	return (0);
}


/* Description: this is a callback function for command line parameter processing
 *              (all valid parameters for device module are listed in am335xTSC_proto.args
 * Input      : input_module_t * module - pointer to module descriptor
 *              int opt  - parameter code
 *              char * optarg - optional parameter value
 * Output     : None
 * Return     : 0 if OK, otherwise -1
 */
int am335xTSC_proto_parm(input_module_t *module, int opt, char *optarg)
{
    struct data *dp = module->data;

	TRACE("am335xTSC_proto_parm --> opt=%c", opt);
	dp = dp;

    switch (opt)
	{
		case 'x':
			dp->xdim = strtol(optarg,0,0);
			if((XMAX-XOFF) > dp->xdim)
				dp->xscale_factor = 1.0 / ((float)(XMAX-XOFF) / dp->xdim);
			else
				dp->xscale_factor = (float)dp->xdim / (XMAX-XOFF);
			SLINFO("x scale factor: %f", dp->xscale_factor);
			break;
		case 'y':
			dp->ydim = strtol(optarg,0,0);
			if((YMAX-YOFF) > dp->ydim)
				dp->yscale_factor = 1.0 / ((float)(YMAX-YOFF) / dp->ydim);
			else
				dp->yscale_factor = (float)dp->ydim / (YMAX-YOFF);
			SLINFO("y scale factor: %f", dp->yscale_factor);
			break;
		case 'h':
			dp->xreverse = (strtol(optarg,0,0) == 0) ? 0 : 1;
			SLINFO("Reversing left-right");
			break;
		case 'v':
			dp->yreverse = (strtol(optarg,0,0) == 0) ? 0 : 1;
			SLINFO("Reversing up-down");
			break;
        default:
			SLINFO("unknown protocol option");
            return (-1);
    }
	TRACE("am335xTSC_proto_parm <-");
    return (0);
}

/* Description: this is a callback function for DEVCTRL command processing
 * Input      : input_module_t * module - pointer to module descriptor
 *              int event  - DEVCTRL command code
 *              void * ptr - pointer to data exchange block
 * Output     : None
 * Return     : 0 if OK, otherwise -1
 */
int am335xTSC_proto_devctrl(input_module_t *module, int event, void *ptr)
{
    struct data     *dp = module->data;
    input_module_t  *down = module->down;

	TRACE("am335xTSC_proto_devctrl --> event=0x%x", event);
    switch(event) {

	case DEVCTL_GETDEVFLAGS:
		*(unsigned short *)ptr = (dp->flags & FLAGS_GLOBAL);
		break;
	case DEVCTL_GETPTRBTNS:
		*(unsigned long *)ptr = dp->buttons;
		break;
	case DEVCTL_GETPTRCOORD:
		*(unsigned char *)ptr = '\02';
		break;
	case DEVCTL_GETPTRPRESS:
		*(unsigned char *)ptr = 0;
		break;
	case DEVCTL_GETCOORDRNG: {
		struct devctl_coord_range *range = ptr;

		/* we don't know this, pass it on maybe the device knows ? */
		if((down->devctrl)(down, DEVCTL_GETCOORDRNG, ptr) == -1)
		{
			/* the lower layer doesn't know.  probably a serial device
			 * well, let's try and guess.  If there is pressure data
			 * then it must be 12 bit.  Otherwise we'll assume that
			 * it's 8 bit
			 */
			if(dp->flags & ABS_PRESS_DATA)
			{
				range->min = 0;
				range->max = 8191;
			}
			else
			{
				range->min = 0;
				range->max = 8191;
			}
		}
		break;
	}
	default:
		if(down && down->devctrl) {
			return((down->devctrl)(down, event, ptr));
		} else {
			return (-1);
		}
    }

    return (0);
}

/* Description: this is a callback function which is called when resource manager
 *              is shutting down
 * Input      : input_module_t * module - pointer to module descriptor
 *              int delay  - program doesn't use this parameter
 * Output     : None
 * Return     : 0 if OK, otherwise -1
 * Comment    : Does nothing for the protocol level
 */
int am335xTSC_proto_shutdown(input_module_t *module, int delay)
{
	TRACE("am335xTSC_proto_shutdown -->");
    return (0);
}
