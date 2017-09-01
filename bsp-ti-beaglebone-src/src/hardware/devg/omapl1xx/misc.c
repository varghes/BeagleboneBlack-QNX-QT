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

#include "omapl1xx.h"




void
omapl1xx_module_info(disp_adapter_t *adp, disp_module_info_t *info)
{
	info->description = "TI OMAPL1xx Internal LCD Raster Controller";
	info->ddk_version_major = DDK_VERSION_MAJOR;
	info->ddk_version_minor = DDK_VERSION_MINOR;
	info->ddk_rev = DDK_REVISION;
	info->driver_rev = 0;
}

static int
omapl1xx_misc_init(disp_adapter_t *adapter, char *optstring)
{
	return 0;
}


static void
omapl1xx_misc_fini(disp_adapter_t *adapter)
{
	;
}


static int
omapl1xx_attach_external(disp_adapter_t *adapter, disp_aperture_t aper[])
{
	omapl1xx_context_t	*omapl1xx = adapter->shmem;

	adapter->ms_ctx = omapl1xx;
	return 0;
}


static int
omapl1xx_detach_external(disp_adapter_t *adapter)
{
	return 0;
}


static int
omapl1xx_recover(disp_adapter_t *adapter)
{
	return 0;
}


static int
omapl1xx_misc_wait_idle(disp_adapter_t *adapter)
{
	omapl1xx_context_t             *omapl1xx = adapter->shmem;
	return 0;
}



int
devg_get_miscfuncs(disp_adapter_t *adp, disp_draw_miscfuncs_t *funcs, int tabsize)
{
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, init, omapl1xx_misc_init, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, fini, omapl1xx_misc_fini, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, module_info, omapl1xx_module_info, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, get_corefuncs_sw, ffb_get_corefuncs, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, get_contextfuncs_sw, ffb_get_contextfuncs, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, attach_external, omapl1xx_attach_external, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, detach_external, omapl1xx_detach_external, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, recover, omapl1xx_recover, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, wait_idle, omapl1xx_misc_wait_idle, tabsize);

	return 0;
}

