/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "omapl1xx.h"

enum {O_HSW, O_HFP, O_HBP, O_VSW, O_VFP, O_VBP, O_LPP, O_SYNCC, O_SYNCE, O_IVS, O_IHS, O_IPC, O_BIAS, O_ACB, O_ACBI, O_PCD, O_TYPE, O_LCDX, O_LCDY, O_IRQ, O_LR, O_TB, O_MODE3, O_DITH, O_DISPLAY, O_REFRESH, END};

static char *omapl1xx_opts[] = {
	[O_HSW]			=	"hsw",		// Horizontal Sync Pulse Width
	[O_HFP]			=	"hfp",		// Horizontal Front Porch
	[O_HBP]			=	"hbp",		// Horizontal Back Porch
	[O_VSW]			=	"vsw",		// Vertical Sync Pulse Width
	[O_VFP]			=	"vfp",		// Vertical Front Porch
	[O_VBP]			=	"vbp",		// Vertical Back Porch
	[O_LPP]			=	"lpp",		// Lines Per Panel
	[O_SYNCC]		=	"syncc",	// Horizontal and Vertical Sync Control
	[O_SYNCE]		=	"synce",	// Horizontal and Vertical Sync Edge
	[O_IVS]			=	"ivs",		// Invert Frame Clock
	[O_IHS]			=	"ihs",		// Invert Line Clock
	[O_IPC]			=	"ipc",		// Invert Pixel Clock
	[O_BIAS]		=	"bias",		// Invert AC Bias
	[O_ACB]			=	"acb",		// AC Bias Pin Frequency
	[O_ACBI]		=	"acbi",		// Number of AC Bias output Transition Counts
	[O_PCD]			=	"pcd",		// Pixel Clock Divisor
	[O_TYPE]		=	"type",		// TFT or STN mode
	[O_LCDX]		=	"lcdx",		// Horizontal Resolution (in Pixels)
	[O_LCDY]		=	"lcdy",		// Vertical Resolution (in Pixels)
	[O_IRQ]			=	"irq",		// LCD IRQ number
	[O_LR]			=   "lr",		// Left-right swap
	[O_TB]			=   "tb",		// Top-bottom swap
	[O_MODE3]		=   "md3",		// Mode3
	[O_DITH]		=   "dith",		// Dither
	[O_DISPLAY]		=   "display",	// Display type
	[END]			=	NULL
};


int
get_config_data(omapl1xx_context_t *omapl1xx, const char *filename)
{
	FILE	*fin = NULL;
	char	buf[512], *c, *opt, *value;
	slogf(_SLOGC_GRAPHICS, _SLOG_INFO, "get_config_data");
	if (filename == NULL) {
		disp_printf(omapl1xx->adapter,
		    "No config file specified, using default parameters\n");
	} else if ((fin = fopen(filename, "r")) == NULL) {
		disp_printf(omapl1xx->adapter,
		    "Could not open config file \"%s\": %s\n",
		    filename, strerror(errno));
	}

	/* Default settings for OMAPL1xx reference boards, Override them if a configuration file is specified */
	omapl1xx->hsw = 22;
	omapl1xx->hfp = 54;
	omapl1xx->hbp = 2; 
	omapl1xx->vsw = 2; 
	omapl1xx->vfp = 17;
	omapl1xx->vbp = 4;
	omapl1xx->lpp = 273;
	omapl1xx->sync_ctrl = 0;
	omapl1xx->sync_edge = 0;
	omapl1xx->ivs = 1;
	omapl1xx->ihs = 1;
	omapl1xx->ipc = 0;
	omapl1xx->bias = 0;
	omapl1xx->acb = 0;
	omapl1xx->acbi = 0;
	omapl1xx->pcd = 20;
	omapl1xx->type = 1;
	omapl1xx->lr = 0;
	omapl1xx->tb = 0;
	omapl1xx->mode3 = 0;
	omapl1xx->dith = 0;
	omapl1xx->display = 0;
	omapl1xx->refresh = 60;
	omapl1xx->lcd_xres = OMAPL1xx_LCD_XRES;
	omapl1xx->lcd_yres = OMAPL1xx_LCD_YRES;
	omapl1xx->pixel_format = DISP_LAYER_FORMAT_RGB565;

	if (fin == NULL) {
		return 0;
	}

	while (fgets(buf, sizeof (buf), fin) != NULL) {
		c = buf;
		while (*c == ' ' || *c == '\t')
			c++;
		if (*c == '\015' || *c== '\032' || *c == '\0' || *c == '\n' || *c == '#')
			continue;
		opt = c;
		while (*c != '\015' && *c != '\032' && *c != '\0' && *c != '\n' && *c != '#')
			c++;
		*c = '\0';
		break;
	}

	while (*opt != '\0') {
		c = opt;
		switch (getsubopt(&opt, omapl1xx_opts, &value)) {
			case O_HSW:
				omapl1xx->hsw = strtol(value, NULL, 0);
				break;
			case O_HFP:
				omapl1xx->hfp = strtol(value, NULL, 0);
				break;
			case O_HBP:
				omapl1xx->hbp = strtol(value, NULL, 0);
				break;
			case O_VSW:
				omapl1xx->vsw = strtol(value, NULL, 0);
				break;
			case O_VFP:
				omapl1xx->vfp = strtol(value, NULL, 0);
				break;
			case O_VBP:
				omapl1xx->vbp = strtol(value, NULL, 0);
				break;
			case O_LPP:
				omapl1xx->lpp = strtol(value, NULL, 0);
				break;
			case O_SYNCC:
				omapl1xx->sync_ctrl = strtol(value, NULL, 0);
				break;
			case O_SYNCE:
				omapl1xx->sync_edge = strtol(value, NULL, 0);
				break;
			case O_IVS:
				omapl1xx->ivs = strtol(value, NULL, 0);
				break;
			case O_IHS:
				omapl1xx->ihs = strtol(value, NULL, 0);
				break;
			case O_IPC:
				omapl1xx->ipc = strtol(value, NULL, 0);
				break;
			case O_BIAS:
				omapl1xx->bias = strtol(value, NULL, 0);
				break;
			case O_ACB:
				omapl1xx->acb = strtol(value, NULL, 0);
				break;
			case O_ACBI:
				omapl1xx->acbi = strtol(value, NULL, 0);
				break;
			case O_PCD:
				omapl1xx->pcd = strtol(value, NULL, 0);
				break;
			case O_TYPE:
				omapl1xx->type = strtol(value, NULL, 0);
				break;
			case O_LR:
				omapl1xx->lr = strtol(value, NULL, 0);
				break;
			case O_TB:
				omapl1xx->tb = strtol(value, NULL, 0);
				break;
			case O_MODE3:
				omapl1xx->mode3 = strtol(value, NULL, 0);
				break;
			case O_DITH:
				omapl1xx->dith = strtol(value, NULL, 0);
				break;
			case O_DISPLAY:
				if (stricmp(value, "lcd")==0)
					omapl1xx->display = 0;
				else
				if (stricmp(value, "hdmi")==0)
					omapl1xx->display = 1;
				else
					disp_printf(omapl1xx->adapter, "Unknown value %s for 'display' option\n", value);
				break;
			case O_REFRESH:
				omapl1xx->refresh = strtol(value, NULL, 0);
				break;
			case O_LCDX:
				omapl1xx->lcd_xres = strtol(value, NULL, 0);
				break;
			case O_LCDY:
				omapl1xx->lcd_yres = strtol(value, NULL, 0);
				break;
			case O_IRQ:
	//			omapl1xx->irq = strtol(value, NULL, 0);
				break;
			default:
				disp_printf(omapl1xx->adapter, "Unknown option %s\n", c);
				break;
		}
	}
	omapl1xx->vidbase=IMAGE_DISPLAY_PA;
	omapl1xx->width=OMAPL1xx_LCD_XRES;
	omapl1xx->height=OMAPL1xx_LCD_YRES;
	omapl1xx->pixel_format = DISP_LAYER_FORMAT_RGB565;
	fclose(fin);

	return 0;
}
