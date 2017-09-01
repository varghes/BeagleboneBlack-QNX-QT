/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include "omap3spi.h"

unsigned int speed_divisor[13] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096};

int omap3_cfg(void *hdl, spi_cfg_t *cfg)
{
	omap3_spi_t	*dev = hdl;
	uint32_t	ctrl;
	uint32_t	calculated_speed, i = 0;
   
	if (cfg == NULL)
		return 0;
	
	i = (cfg->mode & SPI_MODE_CHAR_LEN_MASK) - 1;
	
	if (i > 32 || i < 4)
		return 0;

	ctrl = (i << 7);
		
	if (!(cfg->mode & SPI_MODE_CSPOL_HIGH))
		ctrl |= OMAP3_MCSPI_CSPOL_ACTIVE_LOW;	/* Bit Order */ 
	
    if (!(cfg->mode & SPI_MODE_CKPOL_HIGH))
		ctrl |= OMAP3_MCSPI_POLARIY;	/* Active low polarity */
		
	if (cfg->mode & SPI_MODE_CKPHASE_HALF)
		ctrl |= OMAP3_MCSPI_PHASE;	/* CPHA 1 Phase */	

	/* Since all the modules here will be configured as 
	* SPI masters the "somi" line is to be configured as
	* "input/reception" line and "simo" has to be configured
	* as "output/transmission" line
	*/
    ctrl &= OMAP3_MCSPI_IS_SOMI_INPUT;
    ctrl &= OMAP3_MCSPI_DPE1_SIMO_OUTPUT;
    ctrl |= OMAP3_MCSPI_DPE0_TX_DISABLE;

   if (dev->cs_delay)
   {	
      // Set the CS to start of data delay to 1 1/2 clocks
      ctrl &= OMAP3_MCSPI_CS_DELAY_MASK;
      ctrl |= OMAP3_MCSPI_CS_DELAY_ONE_AND_A_HALF;
	}
  
	
	/* Calculate the SPI target operational speed.
	 * The SPI module is supplie with a 48MHz reference clock.
	 * The SPI transfer speed has to be set by dividing this
	 * reference clock appropriately.
	 */
	 for(i = 0; i < 13;)
	 {
	 	calculated_speed = (dev->clock) / speed_divisor[i];
	 	if(calculated_speed < cfg->clock_rate)
	 		break;
	 	i++;		
	 }		
	ctrl |= (i << 2);

 	return ctrl;
}

__SRCVERSION( "$URL$ $Rev$" );

