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

#include <sys/devi.h>
#include "am335xTSC.h"

/* module specific flags and constants */
#define FLAG_INIT						0x00001  	/* module has been initialized */

/* Registers */
#define AM335X_TSC_REVISION				0x000
#define AM335X_TSC_SYSCONFIG			0x010
#define AM335X_TSC_IRQEOI				0x020
#define AM335X_TSC_RAWIRQSTATUS			0x024
#define AM335X_TSC_IRQSTATUS			0x028
#define AM335X_TSC_IRQENABLE			0x02C
#define AM335X_TSC_IRQWAKEUP			0x034
#define AM335X_TSC_CTRL					0x040
#define AM335X_TSC_FSM					0x044
#define AM335X_TSC_CLKDIV				0x04C
#define AM335X_TSC_STEPENB				0x054
#define AM335X_TSC_IDLECONFIG			0x058
#define AM335X_TSC_CHARGECONFIG			0x05C
#define AM335X_TSC_CHARGEDELAY			0x060
#define AM335X_TSC_STEPCONFIG(n)		(0x64 + ((n-1) * 8))
#define AM335X_TSC_STEPDELAY(n)			(0x68 + ((n-1) * 8))
#define AM335X_TSC_FIFO0CNT				0xE4
#define AM335X_TSC_FIFO0THR				0xE8
#define AM335X_TSC_FIFO1CNT				0xF0
#define AM335X_TSC_FIFO1THR				0xF4
#define AM335X_TSC_FIFO0				0x100
#define AM335X_TSC_FIFO1				0x200

/*	Register bits	*/
#define TSC_IRQWKUP_ENB					(1 << 0)

#define TSC_STPENB_STEPENB				0x1FFF

#define TSC_IRQENB_FIFO0THRES			(1 << 2)
#define TSC_IRQENB_FIFO1THRES			(1 << 5)
#define TSC_IRQENB_PENUP				(1 << 9)

#define TSC_STEPCONFIG_MODE_HWSYNC		0x2
#define TSC_STEPCONFIG_2SAMPLES_AVG		(1 << 4)
#define TSC_STEPCONFIG_XPP				(1 << 5)
#define TSC_STEPCONFIG_XNN				(1 << 6)
#define TSC_STEPCONFIG_YPP				(1 << 7)
#define TSC_STEPCONFIG_YNN				(1 << 8)
#define TSC_STEPCONFIG_XNP				(1 << 9)
#define TSC_STEPCONFIG_YPN				(1 << 10)
#define TSC_STEPCONFIG_RFP				(1 << 12)
#define TSC_STEPCONFIG_INM				(1 << 18)
#define TSC_STEPCONFIG_INP_4			(1 << 19)
#define TSC_STEPCONFIG_INP_5			(1 << 21)
#define TSC_STEPCONFIG_FIFO1			(1 << 26)
#define TSC_STEPCONFIG_IDLE_INP			(1 << 22)

#define SAMPLE_DELAY(dly)				(dly << 24)
#define OPEN_DELAY(dly)					(dly & 0x3FFFF)

#define TSC_STEPCHARGE_INM				(1 << 16)
#define TSC_STEPCHARGE_INP				(1 << 20)
#define TSC_STEPCHARGE_RFM				(1 << 23)
#define TSC_STEPCHARGE_DELAY			0x1

#define TSC_CNTRLREG_TSCSSENB			(1 << 0)
#define TSC_CNTRLREG_STEPID				(1 << 1)
#define TSC_CNTRLREG_STEPCONFIGWRT		(1 << 2)
#define TSC_CNTRLREG_TSCENB				(1 << 7)
#define TSC_CNTRLREG_4WIRE				(0x1 << 5)
#define TSC_CNTRLREG_5WIRE				(0x1 << 6)
#define TSC_CNTRLREG_8WIRE				(0x3 << 5)

#define TSC_FSM_STEPID					0x10
#define TSC_FSM_FSM						(1 << 5)

#define __delay(x) {volatile unsigned _delay = x; while (_delay--); } /* 5+ cycles per loop */


/* Defaults for command line option variables: */
#define AM335X_TSC_BASE					0x44E0D000
#define AM335X_TSC_SZ					0x00002000

#define AM335X_TSC_IRQ					16
#define PRIORITY_PEN_DOWN_IRQ			15

#define CLK_M_OSC						24000000
#define ADC_CLK							3000000

#define TSC_STEPCONFIG_OPENDLY			0x98
#define TSC_STEPCONFIG_SAMPLEDLY		0x0

/* Other device specific defines: */
#define FIFO_SAMPLE_THRESHOLD			5


/* Types */
struct private_data
{
	uint32_t	flags;

	uint32_t	tscadc_pbase;
	volatile uintptr_t	tscadc_vbase;
	uint32_t	irq;
	uint32_t	iid;
	int			irq_pulse_code;
	uint32_t	open_delay;
	uint32_t	sample_delay;
	uint32_t	sample_clk;
	uint32_t	master_osc_clk;
	uint8_t		num_wires;

	uint8_t		pen_down;
	t_point		point;			/* (x & y co-ords) */

};


/* forward decl */
static int am335xTSC_dev_init(input_module_t *module);
static int am335xTSC_dev_reset(input_module_t *module);
static int am335xTSC_dev_output(input_module_t *module, void *data, int count);
static int am335xTSC_dev_pulse(message_context_t *, int, unsigned, void *);
static int am335xTSC_dev_parm(input_module_t *module, int opt, char *optarg);
static int am335xTSC_dev_devctrl(input_module_t *module, int event, void *ptr);
static int am335xTSC_dev_shutdown(input_module_t *module, int ms);


/* Our device module is represented by the following input_module_t
 * data structure.  We always create one static instance of the module.
 * If more than one are needed, ie in multiple bus lines; then the system
 * will allocate a new module and copy the contents of the static one
 * into it.
 */
input_module_t	am335xTSC_dev =
{
	NULL,						/* up module in bus, filled in at runtime */
	NULL,						/* down module in busline */
	NULL,						/* event bus line we belong to, runtime */
	0,							/* flags, leave as zero */
	DEVI_CLASS_NONE|DEVI_MODULE_TYPE_DEVICE, /* module type and class */
	"dev1",						/* name of module, limited to 12 chars
	 	 	 	 	 	 	 	 * Must match what you specify on the
	 	 	 	 	 	 	 	 * cmd line when invoking this module
	 	 	 	 	 	 	 	 */
	__DATE__,					/* date of compilation, useful in versioning,
	 	 	 	 	 	 	 	 * output when the -l option is given */
	"b:i:d:m:s:o:",				/* command line parameters for the module */
	NULL,						/* private data, initialized in init() cb */
	am335xTSC_dev_init,		/* init callback, required */
	am335xTSC_dev_reset,		/* reset callback, required */
	NULL,						/* input, device modules don't need this
	 	 	 	 	 	 	 	 * as it is invoked by lower modules to pass
	 	 	 	 	 	 	 	 * data to higher modules */
	am335xTSC_dev_output,	/* used by higher modules to send data to
							 	 * the device we control  */
	am335xTSC_dev_pulse,		/* our pulse callback, used for the interrupt */
	am335xTSC_dev_parm,		/* parm, required, used to parse any command
							 	 * line params to the modules */
	am335xTSC_dev_devctrl,	/* devctrl, used by higher layer modules to
							 	 * request non-io services of the device */
	am335xTSC_dev_shutdown,	/* shutdown, required, called when module
							 	 * is being shutdown */
};


/* am335xTSC_dev_init
 *
 * This is our init() callback specified in the input_module_t structure.
 * Here we typically allocate space for our private data and assign it
 * to the data member of the module structure.  This callback is called by
 * the input runtime system *before* the module is linked into an event
 * bus line, ie you can't communicate with other modules in your bus line
 * at this point.
 *
 * Return:  0 if ok
 *         -1 on error
 */
static int
am335xTSC_dev_init(input_module_t *module)
{
	struct private_data *dp = module->data;

	TRACE("am335xTSC_dev_init -->");

	if(!module->data)
	{

		dp = module->data = malloc(sizeof(struct private_data));
		if (!dp)
		{
			errno_print("malloc");
			return (-1);
		}

		memset(dp, 0, sizeof(struct private_data));

		dp->irq_pulse_code    = DEVI_PULSE_ALLOC;
		dp->iid = 0;
		dp->pen_down = 0;
		dp->point.x = 0;
		dp->point.y = 0;

		/* sensible defaults */
		dp->tscadc_pbase = AM335X_TSC_BASE;
		dp->irq = AM335X_TSC_IRQ;
		dp->open_delay = TSC_STEPCONFIG_OPENDLY;
		dp->sample_delay = TSC_STEPCONFIG_SAMPLEDLY;
		dp->sample_clk = ADC_CLK;
		dp->master_osc_clk = CLK_M_OSC;
		dp->num_wires = 4;
	}

	return (0);
}

/* am335x_tsc_idle_setup
 *
 * Set up the idling steps in the touch screen controller
 *
 * Return:  0 if ok
 *         -1 on error
 */
static int am335x_tsc_idle_setup(struct private_data *dp)
{
	uint32_t idleconfig_val;

	idleconfig_val = ( TSC_STEPCONFIG_YNN |
						TSC_STEPCONFIG_XNN |
						TSC_STEPCONFIG_INM );

	switch(dp->num_wires)
	{
	case 4:
		idleconfig_val |= TSC_STEPCONFIG_IDLE_INP;
		break;
	case 5:
		idleconfig_val |= TSC_STEPCONFIG_INP_5;
		break;
	case 8:
		idleconfig_val |= TSC_STEPCONFIG_INP_4;
		break;
	}
	out32(dp->tscadc_vbase + AM335X_TSC_IDLECONFIG, idleconfig_val);

	return 0;
}

/* am335x_tsc_step_setup
 *
 * Set up the sampling steps in the touch screen controller
 *
 * Return:  0 if ok
 *         -1 on error
 */
static int am335x_tsc_step_setup(struct private_data *dp)
{
	uint32_t stepconfigx, stepconfigy, stepdelay, chargeconfig;
	uint8_t stepno;


	stepdelay = ( SAMPLE_DELAY(dp->sample_delay) | OPEN_DELAY(dp->open_delay) );

	stepconfigx = ( TSC_STEPCONFIG_MODE_HWSYNC |
					TSC_STEPCONFIG_2SAMPLES_AVG |
					TSC_STEPCONFIG_XPP |
					TSC_STEPCONFIG_YPN );
	switch(dp->num_wires)
	{
	case 4:
		stepconfigx |= TSC_STEPCONFIG_INP_4;
		break;
	case 5:
		stepconfigx |= ( TSC_STEPCONFIG_YPP |
							TSC_STEPCONFIG_YNN |
							TSC_STEPCONFIG_INP_5 );
		break;
	case 8:
		stepconfigx |= TSC_STEPCONFIG_INP_4;
		break;
	}
	for (stepno = 1; stepno < 7; stepno++)
	{
		out32(dp->tscadc_vbase + AM335X_TSC_STEPCONFIG(stepno), stepconfigx);
		out32(dp->tscadc_vbase + AM335X_TSC_STEPDELAY(stepno), stepdelay);
	}

	stepconfigy = ( TSC_STEPCONFIG_MODE_HWSYNC |
					TSC_STEPCONFIG_2SAMPLES_AVG |
					TSC_STEPCONFIG_YNN |
					TSC_STEPCONFIG_INM |
					TSC_STEPCONFIG_FIFO1 );
	switch (dp->num_wires)
	{
	case 4:
		stepconfigy |= TSC_STEPCONFIG_XNP;
		break;
	case 5:
		stepconfigy |= ( TSC_STEPCONFIG_XPP |
							TSC_STEPCONFIG_XNP |
							TSC_STEPCONFIG_YPN |
							TSC_STEPCONFIG_INP_5 );
		break;
	case 8:
		stepconfigy |= TSC_STEPCONFIG_XNP;
		break;
	}
	for (stepno = 7; stepno < 13; stepno++)
	{
		out32(dp->tscadc_vbase + AM335X_TSC_STEPCONFIG(stepno), stepconfigy);
		out32(dp->tscadc_vbase + AM335X_TSC_STEPDELAY(stepno), stepdelay);
	}

	chargeconfig = ( TSC_STEPCONFIG_XPP |
						TSC_STEPCONFIG_YNN |
						TSC_STEPCONFIG_RFP |
						TSC_STEPCHARGE_INM |
						TSC_STEPCHARGE_INP |
						TSC_STEPCHARGE_RFM );
	out32(dp->tscadc_vbase + AM335X_TSC_CHARGECONFIG, chargeconfig);
	out32(dp->tscadc_vbase + AM335X_TSC_CHARGEDELAY, TSC_STEPCHARGE_DELAY);

	out32(dp->tscadc_vbase + AM335X_TSC_STEPENB, TSC_STPENB_STEPENB);

	return 0;
}

/* am335xTSC_dev_reset
 *
 * The reset callback is called when our module is linked into an
 * event bus line.  Now communication is possible with the other
 * modules in the bus line.  In here we will setup our device for
 * action.  We will map in our IO range, attach to our irq, and setup
 * our timers.
 *
 * Return   0 if ok
 *         -1 on error
 *
 */
static int
am335xTSC_dev_reset(input_module_t *module)
{
	uint32_t clk_value, ctrl, irqenable;
	struct private_data	*dp = module->data;

	TRACE("am335xTSC_dev_reset -->");

    if((dp->flags & FLAG_INIT) == 0)
	{
		if ((dp->tscadc_vbase = (uintptr_t)mmap_device_memory(0, AM335X_TSC_SZ,
				PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, dp->tscadc_pbase)) == (uintptr_t)MAP_FAILED)
		{
			printf("Unable to mmap TSC (%s) \n", strerror(errno));
			goto fail1;
		}

		/* set up irq handling */
		if (verbosity >= 2)
		{
			TRACE("Attaching to irq %d\n", dp->irq);
		}
		/* We use the library call devi_register_interrupt() to attach
		 * our irq.  This call will setup a pulse to send to the input
		 * runtime system when the specified irq triggers.  The pulse will
		 * be sent at the priority specified in the second argument and with
		 * the code returned to us in the third argument.
		 * Note we initialized the dp->irq_pc member to DEVI_PULSE_ALLOC to
		 * tell the devi_register_interrupt() call to allocate a number for
		 * pulse itself.
		 * When the input runtime system receives the pulse it
		 * will call our pulse() callback.
		 */
		dp->iid = devi_register_interrupt(dp->irq, PRIORITY_PEN_DOWN_IRQ, &dp->irq_pulse_code, module, NULL, 0);
		if (dp->iid < 0)
		{
			errno_print("Unable to register interrupt\n");
			//dp->fd = -1;
			goto fail2;
		}
		TRACE("pulsecode: %x, iid %x", dp->irq_pulse_code, dp->iid);

		/* Note nothing is preventing you from not using these calls.  If
		 * you want to have an actual interrupt handler, feel free to
		 * call InterruptAttach() and assign it a handler.
		 */

		/* Must set TSCADC_CLKDIV to divider value - 1 */
		clk_value = (dp->master_osc_clk / dp->sample_clk) - 1;
		out32(dp->tscadc_vbase + AM335X_TSC_CLKDIV, clk_value);

		/* Enable wake up from pen events */
		out32(dp->tscadc_vbase + AM335X_TSC_IRQWAKEUP, TSC_IRQWKUP_ENB);

		/* Set the control register bits */
		ctrl = ( TSC_CNTRLREG_STEPCONFIGWRT |
					TSC_CNTRLREG_TSCENB |
					TSC_CNTRLREG_STEPID );
		switch (dp->num_wires) {
		case 4:
			ctrl |= TSC_CNTRLREG_4WIRE;
			break;
		case 5:
			ctrl |= TSC_CNTRLREG_5WIRE;
			break;
		case 8:
			ctrl |= TSC_CNTRLREG_8WIRE;
			break;
		}
		out32(dp->tscadc_vbase + AM335X_TSC_CTRL, ctrl);

		/* Set up idle configuration */
		am335x_tsc_idle_setup(dp);

		/* IRQ Enable */
		irqenable = TSC_IRQENB_FIFO1THRES;
		out32(dp->tscadc_vbase + AM335X_TSC_IRQENABLE, irqenable);

		am335x_tsc_step_setup(dp);

		out32(dp->tscadc_vbase + AM335X_TSC_FIFO1THR, FIFO_SAMPLE_THRESHOLD);

		ctrl |= TSC_CNTRLREG_TSCSSENB;
		out32(dp->tscadc_vbase + AM335X_TSC_CTRL, ctrl);

		dp->flags |= FLAG_INIT;
	}

    return (0);

fail2:
	munmap_device_memory((void*)dp->tscadc_vbase, AM335X_TSC_SZ);
fail1:
	return -1;
}

/* am335xTSC_dev_output
 *
 * This callback is usually called by higher layer modules asking
 * for data to be sent to the device.
 *
 * Return  0 if ok
 *        -1 on error
 */
static int
am335xTSC_dev_output(input_module_t *module, void *data, int count)
{
	struct private_data *dp = module->data;
	/* send output to device */
	/* :TODO: Don't need this */
	dp = dp;

	return (0);
}

/* am335xTSC_dev_pulse
 *
 * Usually implemented in device class modules. This callback is automatically activated
 * each time that a registered interrupt handler wants to notify a device module about
 * input activity.
 *
 * Return 0.
 */
static int
am335xTSC_dev_pulse(message_context_t *ctp, int code, unsigned flags, void *data)
{
	input_module_t *module = (input_module_t *) data;
	struct private_data *dp = module->data;
	input_module_t *up = module->up;

	int32_t fsm = 0, fifo0count = 0, fifo1count = 0;
	int32_t irqstatus_clear = 0, irqstatus = 0;
	uint32_t readx1 = 0, ready1 = 0;
	uint32_t prev_val_x = ~0, prev_val_y = ~0;
	uint32_t prev_diff_x = ~0, prev_diff_y = ~0;
	uint32_t cur_diff_x = 0, cur_diff_y = 0;
	uint32_t val_x = 0, val_y = 0, diffx = 0, diffy = 0;
	uint32_t sampleix;

	TRACE("am335xTSC_dev_pulse --> code=%d", code);

	if (code == dp->irq_pulse_code)
	{
		irqstatus = in32(dp->tscadc_vbase + AM335X_TSC_IRQSTATUS);

		if (irqstatus & TSC_IRQENB_FIFO1THRES)
		{
			fifo0count = in32(dp->tscadc_vbase + AM335X_TSC_FIFO0CNT);
			fifo1count = in32(dp->tscadc_vbase + AM335X_TSC_FIFO1CNT);

			/* Go through FIFOs and skip excess samples */
			for (sampleix = 0; sampleix < fifo0count; sampleix++)
			{
				readx1 = in32(dp->tscadc_vbase + AM335X_TSC_FIFO0);
				readx1 &= 0xfff;
				if (readx1 > prev_val_x)
					cur_diff_x = readx1 - prev_val_x;
				else
					cur_diff_x = prev_val_x - readx1;

				if (cur_diff_x < prev_diff_x) {
					prev_diff_x = cur_diff_x;
					val_x = readx1;
				}
				prev_val_x = readx1;

				ready1 = in32(dp->tscadc_vbase + AM335X_TSC_FIFO1);
				ready1 &= 0xfff;
				if (ready1 > prev_val_y)
					cur_diff_y = ready1 - prev_val_y;
				else
					cur_diff_y = prev_val_y - ready1;

				if (cur_diff_y < prev_diff_y) {
					prev_diff_y = cur_diff_y;
					val_y = ready1;
				}
				prev_val_y = ready1;
			}

			if (val_x > dp->point.x)
			{
				diffx = val_x - dp->point.x;
				diffy = val_y - dp->point.y;
			}
			else
			{
				diffx = dp->point.x - val_x;
				diffy = dp->point.y - val_y;
			}
			dp->point.x = val_x;
			dp->point.y = val_y;

			if (dp->pen_down)
			{
				if ((diffx < 15) && (diffy < 15))
				{
					TRACE("am335xTSC_dev_pulse: sending pen down input x=%d, y=%d",
							dp->point.x, dp->point.y);

					(up->input)( up, sizeof(t_point), (void*)&dp->point );
				}
			}
			irqstatus_clear |= TSC_IRQENB_FIFO1THRES;
		}

		__delay(3000);

		irqstatus = in32(dp->tscadc_vbase + AM335X_TSC_RAWIRQSTATUS);
		if (irqstatus & TSC_IRQENB_PENUP)
		{
			fsm = in32(dp->tscadc_vbase + AM335X_TSC_FSM);
			/* Is FSM idle? */
			if (fsm == 0x10)
			{
				dp->pen_down = 0;

				TRACE("am335xTSC_dev_pulse: sending pen up input");

				/* Encode a pen-up event by sending x=0, y=0 */
				dp->point.x = 0;
				dp->point.y = 0;
				(up->input)( up, sizeof(t_point), (void*)&dp->point );
			}
			else
			{
				dp->pen_down = 1;

				TRACE("pen up, FSM running\n\n");
			}
			irqstatus_clear |= TSC_IRQENB_PENUP;
		}

		out32(dp->tscadc_vbase + AM335X_TSC_IRQSTATUS, irqstatus_clear);

		/* check pending interrupts */
		out32(dp->tscadc_vbase + AM335X_TSC_IRQEOI, 0x0);

		out32(dp->tscadc_vbase + AM335X_TSC_STEPENB, TSC_STPENB_STEPENB);
	}

	InterruptUnmask(dp->irq, -1);
	return NULL;
}

/* am335xTSC_dev_parm
 *
 * This callback is called by the input runtime system to parse any
 * command line parameters given to the module.  The argument opt
 * is the command line option given and optarg contains the argument
 * to the option if any.  This callback is called *before* the module
 * is linked into an event bus line.
 *
 * Return  0 if ok
 *        -1 on error
 */
static int
am335xTSC_dev_parm(input_module_t *module, int opt, char *optarg)
{
	struct private_data *dp = module->data;

	TRACE("am335xTSC_dev_parm --> opt=%c", opt);

	switch (opt) {

	/* user specified irq on command line */
	case 'b':
		dp->tscadc_pbase = strtol(optarg, 0, 0);
		SLINFO("TSCADC base: 0x%08X", dp->tscadc_pbase);
		break;
	case 'i':
		dp->irq = strtol(optarg, 0, 0);
		SLINFO("IRQ: %d", dp->irq);
		break;
	case 'd':
		dp->sample_delay = strtol(optarg, 0, 0);
		SLINFO("Sampling delay in clock cycles: %d cycles", dp->sample_delay);
		break;
	case 'm':
		dp->master_osc_clk = strtol(optarg, 0, 0);
		SLINFO("Master oscillator clock freq: %d Hz", dp->master_osc_clk);
		break;
	case 's':
		dp->sample_clk = strtol(optarg, 0, 0);
		SLINFO("Sample clock freq: %d Hz", dp->sample_clk);
		break;
	case 'o':
		dp->open_delay = strtol(optarg, 0, 0);
		SLINFO("Open delay in clock cycles: %d cycles", dp->open_delay);
		break;
	default:
		SLINFO("unknown device option");
		return (-1);
	}

	return (0);
}

/* am335xTSC_dev_devctrl
 *
 * The devctrl() callback is used by modules in an event bus line to
 * send commands to each other.  The current list of devctrl events
 * are listed in the header const.h in the include directory.
 * The third argument ptr is a pointer to any data associated with
 * the event.  These data types are listed in the file devi.h
 * and prefixed with devctl, eg struct devctl_led is used when sending
 * a command to the keyboard device modules asking it to turn on/off its
 * lights.
 *
 * Return  0 if ok
 *        -1 on error
 */
static int
am335xTSC_dev_devctrl(input_module_t *module, int event, void *ptr)
{
	struct private_data *dp = module->data;

	TRACE("am335xTSC_dev_devctrl -->");

	dp = dp;
	switch(event)
	{
		case DEVCTL_GETCOORDRNG:
		{
			struct devctl_coord_range *range = ptr;
			range->min = 0;
			range->max = 8191; /* 12 bit sampling */
			break;
		}
		default:
			return (-1);
	}

	return (EOK);
}

/* am335xTSC_dev_shutdown
 *
 * This callback is called when the input runtime system is shutting down.
 * Perform any device specific shutdown here.
 *
 * Return  0 if ok
 *        -1 on error
 */
static int
am335xTSC_dev_shutdown(input_module_t *module, int ms)
{
	struct private_data *dp = module->data;

	TRACE("am335xTSC_dev_shutdown -->");

	InterruptDetach( dp->iid );

	/* release interrupt controller memory mapping */
	munmap_device_io( dp->tscadc_vbase, AM335X_TSC_SZ );

	free( module->data );
	module->data = NULL;

	return (0);
}


