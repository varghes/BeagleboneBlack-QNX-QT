/*
 * $QNXLicenseC:
 * Copyright 2007-2008, QNX Software Systems.
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



/*
 *    aic3106.c
 *      The primary interface into the aic3106 codec.
 */

struct aic3106_context;
#define  MIXER_CONTEXT_T struct aic3106_context

#include "mcasp.h"
#include <hw/i2c.h>

enum
{
	AIC3106_PAGESELECTOR 				= 0,
#define PAGE_SELECT                                 0
	AIC3106_SFT_RESET				 	= 1,
#define SFT_RESET                                   (1<<7)
	AIC3106_CODEC_SAMPLE_RATE_SELEC 	= 2,
	AIC3106_PLL_PROGRAM_REG_A			= 3,
#define REG_A_PLL_CTRL                              (1<<7)
#define REG_A_PLL_Q(i)                              (i<<3)
#define REG_A_PLL_P(i)                              (i<<0)
	AIC3106_PLL_PROGRAM_REG_B			= 4,
#define REG_B_PLL_J(i)                              (i<<2)
	AIC3106_PLL_PROGRAM_REG_C			= 5,
#define REG_C_PLL_D_MOST_BIT(i)                     ((i>>6) & 0xff) //0x36
	AIC3106_PLL_PROGRAM_REG_D			= 6,
#define REG_D_PLL_D_LEAST_BIT(i)                    ((i&0x3f)<<2)//0xb0
	AIC3106_CODEC_DATAPATH_SETUP		= 7,
#define RIGTH_DAC_DATAPATH_CTRL                     (1<<1)
#define LEFT_DAC_DATAPATH_CTRL                      (1<<3)
#define FS_SET                                      (1<<7)
	AIC3106_AUDIO_SER_DATA_INT_CTRL_A	= 8,
#define REG_A_BCLK_DIR_CTRL                         (1<<7)
#define REG_A_WCLK_DIR_CTRL                         (1<<6)
#define REG_A_SERIAL_DOUT_3STATE_CTRL               (1<<5)
	AIC3106_AUDIO_SER_DATA_INT_CTRL_B	= 9,
#define REG_B_BCLK_RATE_CTRL                        (1<<3)
#define REG_B_AUDIO_SERIAL_DATA_MODE                (1<<6)
	AIC3106_AUDIO_SER_DATA_INT_CTRL_C	= 10, 
	AIC3106_AUDIO_CODEC_OVERFLOW_FLAG	= 11,
#define OVERFLOW_FLAG_PLL_R(i)                      (i<<0)
	AIC3106_AUDIO_CODEC_DFC				= 12, //DIGITAL FILTER CONTROL
	AIC3106_HEADSET_PRESS_DETECT_A		= 13,
	AIC3106_HEADSET_PRESS_DETECT_B		= 14,
	AIC3106_LEFT_ADC_PGA_GAIN_CTRL		= 15,
	AIC3106_RIGHT_ADC_PGA_GAIN_CTRL		= 16,
	AIC3106_MIC3LR_TO_LEFT_ADC_CTRL		= 17,
#define MIC3R_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX     (0xf<<0)
#define MIC3L_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX     (0xf<<4)
	AIC3106_MIC3LR_TO_RIGHT_ADC_CTRL	= 18,
#define MIC3R_INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX    (0xf<<0)
#define MIC3L_INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX    (0xf<<4)
	AIC3106_LINE1L_TO_LEFT_ADC_CTRL		= 19,
#define LINE1L_LEFT_ADC_CHANNEL_POWER_CTRL          (1<<2)
#define LINE1L_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX    (0xf<<3)
	AIC3106_LINE2L_TO_LEFT_ADC_CTRL		= 20,
	AIC3106_LINE1R_TO_LEFT_ADC_CTRL		= 21,
	AIC3106_LINE1R_TO_RIGHT_ADC_CTRL	= 22,
#define LINE1R__RIGHT_ADC_CHANNEL_POWER_CTRL        (1<<2)
#define LINE1R__INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX  (0xf<<3)
	AIC3106_LINE2R_TO_RIGHT_ADC_CTRL	= 23,
	AIC3106_LINE1L_TO_RIGHT_ADC_CTRL	= 24,
	AIC3106_MICBIAS_CTRL				= 25,
#define MICBIAS_OFF                                 (0<<6)
#define MICBIAS_20                                  (1<<6)
#define MICBIAS_25                                  (2<<6)
#define MICBIAS_AVDD                                (3<<6)
	AIC3106_LEFT_AGC_CTRL_A				= 26,
	AIC3106_LEFT_AGC_CTRL_B				= 27,
	AIC3106_LEFT_AGC_CTRL_C				= 28,
	AIC3106_RIGHT_AGC_CTRL_A			= 29,
	AIC3106_RIGHT_AGC_CTRL_B			= 30,
	AIC3106_RIGHT_AGC_CTRL_C			= 31,
	AIC3106_LEFT_AGC_GAIN				= 32,
	AIC3106_RIGHT_AGC_GAIN				= 33,
	AIC3106_LEFT_AGC_NOISE_GATE_DEBNC	= 34,
	AIC3106_RIGHT_AGC_NOISE_GATE_DEBNC	= 35,
	AIC3106_ADC_FLAG					= 36,
	AIC3106_DAC_POWER_DRIVER_CTRL		= 37,
#define LEFT_DAC_POWER_CTRL                         (1<<7)
#define RIGHT_DAC_POWER_CTRL                        (1<<6)
	AIC3106_HPOWER_DRIVER_CTRL			= 38,
	AIC3106_RESERVED_REG_1				= 39,
	AIC3106_HIGH_POWER_STAGE_CTRL		= 40,
	AIC3106_DAC_OUTPUT_SWITCH_CTRL		= 41,
#define RIGHT_DAC_OUT_SWITCHING_CTRL                (1<<4)
#define LEFT_DAC_OUT_SWITCHING_CTRL                 (1<<6)
	AIC3106_OUTPUT_DRIVER_POP_REDUCTION	= 42,
	AIC3106_LEFT_DAC_DIG_VOL_CTRL		= 43,
	AIC3106_RIGHT_DAC_DIG_VOL_CTRL		= 44,
	AIC3106_LINE2L_TO_HPLOUT_VOL_CTRL	= 45,
	AIC3106_PGA_L_TO_HPLOUT_VOL_CTRL	= 46,
	AIC3106_DAC_L1_TO_HPLOUT_VOL_CTRL	= 47,
#define HPLOUT_DAC_L1_OUT_ROUTING_CTR               (1<<7)
	AIC3106_LINE2R_TO_HPLOUT_VOL_CTRL	= 48,
	AIC3106_PGA_R_TO_HPLOUT_VOL_CTRL	= 49,
	AIC3106_DAC_R1_TO_HPLOUT_VOL_CTRL	= 50,
	AIC3106_HPLOUT_OUTPUT_LVL_CTRL		= 51,
#define HPLOUT_POWER_CTRL                           (1<<0)
#define HPLOUT_MUTE                                 (1<<3)
	AIC3106_LINE2L_TO_HPLCOM_VOL_CTRL	= 52,
	AIC3106_PGA_L_TO_HPLCOM_VOL_CTRL	= 53,
	AIC3106_DAC_L1_TO_HPLCOM_VOL_CTRL	= 54,
	AIC3106_LINE2R_TO_HPLCOM_VOL_CTRL	= 55,
	AIC3106_PGA_R_TO_HPLCOM_VOL_CTRL	= 56,
	AIC3106_DAC_R1_TO_HPLCOM_VOL_CTRL	= 57,
	AIC3106_HPLCOM_OUTPUT_LVL_CTRL		= 58,
	AIC3106_LINE2L_TO_HPROUT_VOL_CTRL	= 59,
	AIC3106_PGA_L_TO_HPROUT_VOL_CTRL	= 60,
	AIC3106_DAC_L1_TO_HPROUT_VOL_CTRL	= 61,
	AIC3106_LINE2R_TO_HPROUT_VOL_CTRL	= 62,
	AIC3106_PGA_R_TO_HPROUT_VOL_CTRL	= 63,
	AIC3106_DAC_R1_TO_HPROUT_VOL_CTRL	= 64,
#define HPROUT_DAC_R1_OUT_ROUTING_CTRL              (1<<7)
	AIC3106_HPROUT_OUTPUT_LVL_CTRL		= 65,
#define HPROUT_POWER_CTRL                           (1<<0)
#define HPROUT_MUTE                                 (1<<3)
	AIC3106_LINE2L_TO_HPRCOM_VOL_CTRL	= 66,
	AIC3106_PGA_L_HPRCOM_VOL_CTRL		= 67,
	AIC3106_DAC_L1_TO_HPRCOM_VOL_CTRL	= 68,
	AIC3106_LINE2R_TO_HPRCOM_VOL_CTRL	= 69,
	AIC3106_PGA_R_TO_HPRCOM_VOL_CTRL	= 70,
	AIC3106_DAC_R1_TO_HPRCOM_VOL_CTRL	= 71,
	AIC3106_HPRCOM_OUTPUT_LVL_CTRL		= 72,
	AIC3106_LINE2L_TO_MONOLOP_VOL_CTRL	= 73,
	AIC3106_PGA_L_MONOLOP_VOL_CTRL		= 74,
	AIC3106_DAC_L1_TO_MONOLOP_VOL_CTRL	= 75,
	AIC3106_LINE2R_TO_MONOLOP_VOL_CTRL	= 76,
	AIC3106_PGA_R_TO_MONOLOP_VOL_CTRL	= 77,
	AIC3106_DAC_R1_TO_MONOLOP_VOL_CTRL	= 78,
	AIC3106_MONOLOP_OUTPUT_LVL_CTRL		= 79,
	AIC3106_LINE2L_TO_LEFTLOP_VOL_CTRL	= 80,
	AIC3106_PGA_L_TO_LEFTLOP_VOL_CTRL	= 81,
	AIC3106_DAC_L1_TO_LEFTLOP_VOL_CTRL	= 82,
	AIC3106_LINE2R_TO_LEFTLOP_VOL_CTRL	= 83,
	AIC3106_PGA_R_TO_LEFTLOP_VOL_CTRL	= 84,
	AIC3106_DAC_R1_TO_LEFTLOP_VOL_CTRL	= 85,
	AIC3106_LEFTLOP_OUTPUT_LVL_CTRL		= 86,
#define LEFTLOP_LEFT_LOP_M_POWER_STATUS             (1<<0)
#define LEFTLOP_LEFT_LOP_M_MUTE                     (1<<3)
	AIC3106_LINE2L_TO_RIGHTLOP_VOL_CTRL	= 87,
	AIC3106_PGA_L_TO_RIGHTLOP_VOL_CTRL	= 88,
	AIC3106_DAC_L1_TO_RIGHTLOP_VOL_CTRL	= 89,
	AIC3106_LINE2R_TO_RIGHTLOP_VOL_CTRL	= 90,
	AIC3106_PGA_R_TO_RIGHTLOP_VOL_CTRL	= 91,
	AIC3106_DAC_R1_TO_RIGHTLOP_VOL_CTRL	= 92,
	AIC3106_RIGHTLOP_OUTPUT_LVL_CTRL	= 93,
#define RIGHTLOP_RIGHT_LOP_M_POWER_STATUS           (1<<0)
#define RIGHTLOP_RIGHT_LOP_M_MUTE                   (1<<3)
	AIC3106_MODULE_PWR_STAT				= 94,
	AIC3106_OUTPUT_DRIVER_SHORT_CRCT_DETEC_STAT	= 95,
	AIC3106_STICKY_INTERRUPT_FLAGS		=96,
	AIC3106_REALTIME_INTERRUPT_FLAGS	= 97,
	AIC3106_GPIO1_CTRL					= 98,
	AIC3106_GPIO2_CTRL					= 99,
	AIC3106_ADDITIONAL_GPIO_CTRL_A		= 100,
	AIC3106_ADDITIONAL_GPIO_CTRL_B		= 101,
	AIC3106_CLK_GENERATION_CTRL			= 102,
};

typedef struct aic3106_context
{
	ado_mixer_t *mixer;
	HW_CONTEXT_T *hwc;
	int fd;
	int num_of_codecs;
	int tx_voices;
	int rx_voices;
	int i2c_addr1;
	int i2c_addr2;
	int i2c_addr3;
	
	int sel_mcasp;
#define SEL_MCASP1 0
	
	int left_adc_pga_gain_ctrl[3];
	int right_adc_pga_gain_ctrl[3];
	
	int left_dac_vol_ctrl[3];
	int right_dac_vol_ctrl[3];
	
	uint8_t output_routing; // for McASP2 (hp or lineout)
	
	uint8_t	input_mux; /* 0: mic1, 1: mic2, 2: aux*/
#define MIC1_IN_INPUT   0
#define AUX_IN_INPUT    1
#define MIC2_IN_INPUT   2
	
#define SWITCH_LINEOUT  0
#define SWITCH_HPOUT    1
#define DAC_TO_LR1  (0x00)
#define DAC_TO_LR3  (0x50)
#define SWITCH_HP_LINE_MASK (0xf0)
	
	ado_mixer_delement_t *mic1_in;
	ado_mixer_delement_t *aux_in;
}
aic3106_context_t;

/*
 If MCLK is an even multiple of sampling frequency, we do not need
 to turn on internal codec PLL, and we can use 'Q' to obtain desired
 frequency:
			fs = (MCLK) / (128 * Q)
 
 If MCLK is not an even multiple of sampling frequency, we need to 
 use internal PLL and obtain desired frequency using P,R,K=J.D using 
 the following formula:
 
			fs = (MCLK * K * R) / (2048 * P)
			where K = J.D (J is the integer portion of K, D is the fraction)
 
 For example: Given MCLK=24.576MHz, Fs=44.1KHz, and assuming R=1, we have:
 
 P = 2 [10MHz <= (MCLK/P) <= 20MHz]
 
 K = (44100 * 2048 * 2)/(24576000) = 7.35
 Therefore, J = 7 [4 <= J <= 55]
			D = 3500
 
 Finally, the above calculation meet the final requirement:
 80MHz <= (MCLK * K * R / P) <= 110MHz
 

 */
 
#define NUM_CLOCK_ENTRIES 3
static uint32_t codec_pll_dividers[NUM_CLOCK_ENTRIES][6] = {
                             /* MCLK      Q  P  R  J  D */
	/* mclk = 12.0000 MHz */   {12000000, 0, 1, 1, 7, 5264},
	/* mclk = 24.5760 MHz */   {24576000, 0, 2, 1, 7, 3500},
	/* mclk = 45.1584 MHz */   {45158400, 8, 0, 0, 0, 0}
};

#define MCLK_INDEX  0
#define PLL_Q_INDEX 1
#define PLL_P_INDEX 2
#define PLL_R_INDEX 3
#define PLL_J_INDEX 4
#define PLL_D_INDEX 5

static int32_t pcm_devices[1] = {
	0
};

static snd_mixer_voice_t stereo_voices[2] = {
	{SND_MIXER_VOICE_LEFT, 0}, 
	{SND_MIXER_VOICE_RIGHT, 0}
};

static snd_mixer_voice_t quad_voices[4] = {
	{SND_MIXER_VOICE_LEFT, 0},
	{SND_MIXER_VOICE_RIGHT, 0},
	{SND_MIXER_VOICE_REAR_LEFT, 0},
	{SND_MIXER_VOICE_REAR_RIGHT, 0}
};

static snd_mixer_voice_t sixch_voices[6] = {
	{SND_MIXER_VOICE_LEFT, 0},
	{SND_MIXER_VOICE_RIGHT, 0},
	{SND_MIXER_VOICE_REAR_LEFT, 0},
	{SND_MIXER_VOICE_REAR_RIGHT, 0},
	{SND_MIXER_VOICE_CENTER, 0},
	{SND_MIXER_VOICE_WOOFER, 0}
	
};

static struct snd_mixer_element_volume1_range output_range[1] = {
	{0, 127, -6350, 0}
};

static struct snd_mixer_element_volume1_range input_range[1] = {
	{0, 119, 0, 5950}
};

static uint8_t aic3106_rd(MIXER_CONTEXT_T * aic3106, uint8_t regaddr, uint8_t slave_addr)
{
	struct send_recv
	{
		i2c_sendrecv_t hdr;
		uint8_t buf[2];
	} aic3106_rd_data;
	
	/*Read the Registers Current Value */
	aic3106_rd_data.buf[0] = regaddr;
	aic3106_rd_data.hdr.send_len = 1;
	aic3106_rd_data.hdr.recv_len = 1;
	
	aic3106_rd_data.hdr.slave.addr = slave_addr;
	aic3106_rd_data.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	aic3106_rd_data.hdr.stop = 1;
	
	if (devctl(aic3106->fd, DCMD_I2C_SENDRECV, &aic3106_rd_data, sizeof (aic3106_rd_data), NULL))
	{
		ado_error("Failed to write to codec: %s\n", strerror(errno));
	}
	
	ado_debug(DB_LVL_MIXER, "AIC3106 Codec read reg=%x data=%x (%d) ,slave_addr = %x", regaddr, aic3106_rd_data.buf[0],
			  aic3106_rd_data.buf[0], slave_addr);
	return aic3106_rd_data.buf[0];
}

static void aic3106_wr(MIXER_CONTEXT_T * aic3106, uint8_t regaddr, uint8_t data, uint8_t slave_addr)
{
	struct send_recv
	{
		i2c_sendrecv_t hdr;
		uint8_t buf[2];
	} aic3106_rd_data;
	
	/*Read the Registers Current Value */
	aic3106_rd_data.buf[0] = regaddr;
	aic3106_rd_data.buf[1] = data;
	aic3106_rd_data.hdr.send_len = 2;
	aic3106_rd_data.hdr.recv_len = 1;
	
	aic3106_rd_data.hdr.slave.addr = slave_addr;
	aic3106_rd_data.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	aic3106_rd_data.hdr.stop = 1;
	
	if (devctl(aic3106->fd, DCMD_I2C_SENDRECV, &aic3106_rd_data, sizeof (aic3106_rd_data), NULL))
	{
		ado_error("Failed to write to codec: %s\n", strerror(errno));
	}
	
	ado_debug(DB_LVL_MIXER, "AIC3106 Codec write reg=%x data=%x (%d), slave_addr = %x", regaddr, data, data, slave_addr);
	aic3106_rd(aic3106, regaddr, slave_addr);				 /* Dumby read to ensure write is applied */
	aic3106_rd(aic3106, regaddr, slave_addr);
	return;
}

static void
aic3106_dump_regs(MIXER_CONTEXT_T * aic3106)
{
	int regix;
	uint8_t regval;

	for(regix = 0; regix <= AIC3106_CLK_GENERATION_CTRL; regix++)
	{
		regval = aic3106_rd(aic3106, regix, aic3106->i2c_addr1);
		ado_error("aic3106_dump_regs: %3d: 0x%02X", regix, regval);
	}
}


static int32_t
aic3106_output_vol_control(MIXER_CONTEXT_T * aic3106, ado_mixer_delement_t * element, uint8_t set,
						   uint32_t * vol, void *instance_data)
{
	int32_t altered = 0;

	if(set)
	{
		altered = 	(vol[0] != (127 - (aic3106->left_dac_vol_ctrl[0] & 0x7f))) 	||
		(vol[1] != (127 - (aic3106->right_dac_vol_ctrl[0] & 0x7f)));
		if( (aic3106->num_of_codecs == 3) && !altered)
		{
			altered = 	(vol[3] != (127 - (aic3106->left_dac_vol_ctrl[1] & 0x7f))) 	||
			(vol[4] != (127 - (aic3106->right_dac_vol_ctrl[1] & 0x7f))) 	||
			(vol[2] != (127 - (aic3106->left_dac_vol_ctrl[2] & 0x7f))) 	||
			(vol[5] != (127 - (aic3106->right_dac_vol_ctrl[2] & 0x7f)));
		}
		
		// vol[0] and vol[1] for aic3106 #1
		aic3106->left_dac_vol_ctrl[0] &= 0x80;
		aic3106->left_dac_vol_ctrl[0] |= 127 - (vol[0] & 0x7f);
		aic3106->right_dac_vol_ctrl[0] &= 0x80;
		aic3106->right_dac_vol_ctrl[0] |= 127 - (vol[1] & 0x7f);
		aic3106_wr(aic3106, AIC3106_LEFT_DAC_DIG_VOL_CTRL, aic3106->left_dac_vol_ctrl[0], aic3106->i2c_addr1);
		aic3106_wr(aic3106, AIC3106_RIGHT_DAC_DIG_VOL_CTRL, aic3106->right_dac_vol_ctrl[0], aic3106->i2c_addr1);
		
		if(aic3106->num_of_codecs == 3)
		{
			// vol[3] and vol[4] for aic3106 #2
			aic3106->left_dac_vol_ctrl[1] &= 0x80;
			aic3106->left_dac_vol_ctrl[1] |= 127 - (vol[3] & 0x7f);
			aic3106->right_dac_vol_ctrl[1] &= 0x80;
			aic3106->right_dac_vol_ctrl[1] |= 127 - (vol[4] & 0x7f);
			aic3106_wr(aic3106, AIC3106_LEFT_DAC_DIG_VOL_CTRL, aic3106->left_dac_vol_ctrl[1], aic3106->i2c_addr2);
			aic3106_wr(aic3106, AIC3106_RIGHT_DAC_DIG_VOL_CTRL, aic3106->right_dac_vol_ctrl[1], aic3106->i2c_addr2);
			
			// vol[2] and vol[5] for aic3106 #3
			aic3106->left_dac_vol_ctrl[2] &= 0x80;
			aic3106->left_dac_vol_ctrl[2] |= 127 - (vol[2] & 0x7f);
			aic3106->right_dac_vol_ctrl[2] &= 0x80;
			aic3106->right_dac_vol_ctrl[2] |= 127 - (vol[5] & 0x7f);
			aic3106_wr(aic3106, AIC3106_LEFT_DAC_DIG_VOL_CTRL, aic3106->left_dac_vol_ctrl[2], aic3106->i2c_addr3);
			aic3106_wr(aic3106, AIC3106_RIGHT_DAC_DIG_VOL_CTRL, aic3106->right_dac_vol_ctrl[2], aic3106->i2c_addr3);
		}
	}
	else
	{
		vol[0] = 127 - (aic3106->left_dac_vol_ctrl[0] & 0x7f);
		vol[1] = 127 - (aic3106->right_dac_vol_ctrl[0] & 0x7f);
		if(aic3106->num_of_codecs == 3)
		{
			vol[3] = 127 - (aic3106->left_dac_vol_ctrl[1] & 0x7f);
			vol[4] = 127 - (aic3106->right_dac_vol_ctrl[1] & 0x7f);
			vol[2] = 127 - (aic3106->left_dac_vol_ctrl[2] & 0x7f);
			vol[5] = 127 - (aic3106->right_dac_vol_ctrl[2] & 0x7f);
		}
	}

	// EL debug:
	aic3106_dump_regs(aic3106);

	return (altered);
}

static int32_t
aic3106_output_mute_control(MIXER_CONTEXT_T * aic3106, ado_mixer_delement_t * element, uint8_t set,
							uint32_t * val, void *instance_data)
{
	int32_t altered = 0;
	if (set)
	{
		
		if(aic3106->num_of_codecs == 1)
		{
			altered = 	val[0] != ( ((aic3106->left_dac_vol_ctrl[0] & 0x80)>>7) |
								   ((aic3106->right_dac_vol_ctrl[0] & 0x80)>>6) );
		}
		else if(aic3106->num_of_codecs == 3)
		{
			altered = 	val[0] != ( ((aic3106->left_dac_vol_ctrl[0] & 0x80)>>7) |
								   ((aic3106->right_dac_vol_ctrl[0] & 0x80)>>6)|
								   ((aic3106->left_dac_vol_ctrl[1] & 0x80)>>4) |
								   ((aic3106->right_dac_vol_ctrl[1] & 0x80)>>3)|
								   ((aic3106->left_dac_vol_ctrl[2] & 0x80)>>5) |
								   ((aic3106->right_dac_vol_ctrl[2] & 0x80)>>2) );
		}
		// bits 0 and 1 for aic3106 #1
		aic3106->left_dac_vol_ctrl[0] &= 0x7f;
		aic3106->left_dac_vol_ctrl[0] |= (val[0] & SND_MIXER_CHN_MASK_FRONT_LEFT)<<7;
		aic3106->right_dac_vol_ctrl[0] &= 0x7f;
		aic3106->right_dac_vol_ctrl[0] |= (val[0] & SND_MIXER_CHN_MASK_FRONT_RIGHT)<<6;
		aic3106_wr(aic3106, AIC3106_LEFT_DAC_DIG_VOL_CTRL, aic3106->left_dac_vol_ctrl[0], aic3106->i2c_addr1);
		aic3106_wr(aic3106, AIC3106_RIGHT_DAC_DIG_VOL_CTRL, aic3106->right_dac_vol_ctrl[0], aic3106->i2c_addr1);
		
		if(aic3106->num_of_codecs == 3)
		{
			// bits 3 and 4 for aic3106 #2
			aic3106->left_dac_vol_ctrl[1] &= 0x7f;
			aic3106->left_dac_vol_ctrl[1] |= (val[0] & SND_MIXER_CHN_MASK_REAR_LEFT)<<4;
			aic3106->right_dac_vol_ctrl[1] &= 0x7f;
			aic3106->right_dac_vol_ctrl[1] |= (val[0] & SND_MIXER_CHN_MASK_REAR_RIGHT)<<3;
			aic3106_wr(aic3106, AIC3106_LEFT_DAC_DIG_VOL_CTRL, aic3106->left_dac_vol_ctrl[1], aic3106->i2c_addr2);
			aic3106_wr(aic3106, AIC3106_RIGHT_DAC_DIG_VOL_CTRL, aic3106->right_dac_vol_ctrl[1], aic3106->i2c_addr2);
			
			// bits 2 and 5 for aic3106 #3
			aic3106->left_dac_vol_ctrl[2] &= 0x7f;
			aic3106->left_dac_vol_ctrl[2] |= (val[0] & SND_MIXER_CHN_MASK_FRONT_CENTER)<<5;
			aic3106->right_dac_vol_ctrl[2] &= 0x7f;
			aic3106->right_dac_vol_ctrl[2] |= (val[0] & SND_MIXER_CHN_MASK_WOOFER)<<2;
			aic3106_wr(aic3106, AIC3106_LEFT_DAC_DIG_VOL_CTRL, aic3106->left_dac_vol_ctrl[2], aic3106->i2c_addr2);
			aic3106_wr(aic3106, AIC3106_RIGHT_DAC_DIG_VOL_CTRL, aic3106->right_dac_vol_ctrl[2], aic3106->i2c_addr2);
		}
	}
	else
	{
		if(aic3106->num_of_codecs == 1)
		{
			val[0] =  	(aic3106->left_dac_vol_ctrl[0] & 0x80)>>7 	|
			(aic3106->right_dac_vol_ctrl[0] & 0x80)>>6;
		}
		else
		{
			val[0] =  	(aic3106->left_dac_vol_ctrl[0] & 0x80)>>7 	|
			(aic3106->right_dac_vol_ctrl[0] & 0x80)>>6 |
			(aic3106->left_dac_vol_ctrl[1] & 0x80)>>4 |
			(aic3106->right_dac_vol_ctrl[1] & 0x80)>>3 |
			(aic3106->left_dac_vol_ctrl[2] & 0x80)>>5 |
			(aic3106->right_dac_vol_ctrl[2] & 0x80)>>2;
		}
	}
	return (altered);
}

static int32_t
aic3106_hp_get(MIXER_CONTEXT_T * aic3106, ado_dswitch_t * dswitch, snd_switch_t * cswitch, void *instance_data)
{
	cswitch->type = SND_SW_TYPE_BOOLEAN;
	if(aic3106_rd(aic3106, AIC3106_DAC_OUTPUT_SWITCH_CTRL, aic3106->i2c_addr1) & SWITCH_HP_LINE_MASK)
		cswitch->value.enable = SWITCH_LINEOUT;
	else
		cswitch->value.enable = SWITCH_HPOUT;
	
	return (0);
}

static int32_t
aic3106_hp_set(MIXER_CONTEXT_T * aic3106, ado_dswitch_t * dswitch, snd_switch_t * cswitch, void *instance_data)
{
	int32_t altered = 0;
	if((cswitch->value.enable == SWITCH_LINEOUT) && (aic3106->output_routing != SWITCH_LINEOUT))
	{
		// selected LINEOUT
		aic3106_wr(aic3106, AIC3106_DAC_OUTPUT_SWITCH_CTRL, (RIGHT_DAC_OUT_SWITCHING_CTRL | LEFT_DAC_OUT_SWITCHING_CTRL), aic3106->i2c_addr1);
		aic3106->output_routing = SWITCH_LINEOUT;
		altered = 1;
	}
	else
	{
		// selected HPOUT
		aic3106_wr(aic3106, AIC3106_DAC_OUTPUT_SWITCH_CTRL, 0x00, aic3106->i2c_addr1);
		aic3106->output_routing = SWITCH_HPOUT;
		altered = 1;
	}
	
	return altered;
}
static int32_t
aic3106_mux_control(MIXER_CONTEXT_T * aic3106, ado_mixer_delement_t * element, uint8_t set,ado_mixer_delement_t ** inelements, void *instance_data)
{
	int32_t altered = 0;
	uint32_t tmp;
	
	if (set)
	{
		if (aic3106->hwc->pcm_chn[CAP_CHN].pcm_subchn)
			return (EBUSY);						 /* Cannot switch while capture is active */
		
		if(inelements[0] == aic3106->mic1_in)
			tmp = MIC1_IN_INPUT;
		else
			tmp = AUX_IN_INPUT;
		altered = aic3106->input_mux != tmp;
		
		aic3106->input_mux = tmp;
		
		if(MIC1_IN_INPUT == aic3106->input_mux) // Mic
		{
			// disconnect Line1L and Line1R from Left and Right ADC
			aic3106_wr(aic3106, AIC3106_LINE1L_TO_LEFT_ADC_CTRL,(LINE1L_LEFT_ADC_CHANNEL_POWER_CTRL | LINE1L_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX) , aic3106->i2c_addr1);
			aic3106_wr(aic3106, AIC3106_LINE1R_TO_RIGHT_ADC_CTRL, (LINE1R__RIGHT_ADC_CHANNEL_POWER_CTRL | LINE1R__INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX), aic3106->i2c_addr1);
			
			// connect MIC3L to Left ADC and MIC3R to right ADC
			aic3106_wr(aic3106, AIC3106_MIC3LR_TO_LEFT_ADC_CTRL, MIC3R_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX, aic3106->i2c_addr1);
			aic3106_wr(aic3106, AIC3106_MIC3LR_TO_RIGHT_ADC_CTRL, MIC3L_INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX, aic3106->i2c_addr1);
		}
		else // Aux input
		{
			// disconnect MIC3L to Left ADC and MIC3R to right ADC
			aic3106_wr(aic3106, AIC3106_MIC3LR_TO_LEFT_ADC_CTRL,(MIC3R_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX | MIC3L_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX), aic3106->i2c_addr1);
			aic3106_wr(aic3106, AIC3106_MIC3LR_TO_RIGHT_ADC_CTRL,(MIC3R_INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX | MIC3L_INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX), aic3106->i2c_addr1);
			
			// connect Line1L and Line1R from Left and Right ADC
			aic3106_wr(aic3106, AIC3106_LINE1L_TO_LEFT_ADC_CTRL, LINE1L_LEFT_ADC_CHANNEL_POWER_CTRL, aic3106->i2c_addr1);
			aic3106_wr(aic3106, AIC3106_LINE1R_TO_RIGHT_ADC_CTRL, LINE1R__RIGHT_ADC_CHANNEL_POWER_CTRL, aic3106->i2c_addr1);
		}
	}
	else
	{
		if(MIC1_IN_INPUT == aic3106->input_mux)
			inelements[0] = inelements[1] = aic3106->mic1_in;
		else
			inelements[0] = inelements[1] =aic3106->aux_in;
	}
	return (altered);
}


static int32_t
aic3106_input_vol_control(MIXER_CONTEXT_T * aic3106, ado_mixer_delement_t * element, uint8_t set,uint32_t * vol, void *instance_data)
{
	int32_t altered = 0;
	
	if(set)
	{
		altered = ((vol[0] != (aic3106->left_adc_pga_gain_ctrl[0] & 0x7f)) ||
				   (vol[1] != (aic3106->right_adc_pga_gain_ctrl[0] & 0x7f)));
		
		aic3106->left_adc_pga_gain_ctrl[0] &= 0x80;
		aic3106->left_adc_pga_gain_ctrl[0] |= (vol[0] & 0x7f);
		aic3106->right_adc_pga_gain_ctrl[0] &= 0x80;
		aic3106->right_adc_pga_gain_ctrl[0] |= (vol[1] & 0x7f);
		
		aic3106_wr(aic3106, AIC3106_LEFT_ADC_PGA_GAIN_CTRL, aic3106->left_adc_pga_gain_ctrl[0], aic3106->i2c_addr1);
		aic3106_wr(aic3106, AIC3106_RIGHT_ADC_PGA_GAIN_CTRL, aic3106->right_adc_pga_gain_ctrl[0], aic3106->i2c_addr1);
	}
	else
	{
		vol[0] = aic3106->left_adc_pga_gain_ctrl[0] & 0x7f;
		vol[1] = aic3106->right_adc_pga_gain_ctrl[0] & 0x7f;
	}
	
	return (altered);
}

static int32_t
aic3106_input_mute_control(MIXER_CONTEXT_T * aic3106, ado_mixer_delement_t * element, uint8_t set,
						   uint32_t * val, void *instance_data)
{
	int32_t altered = 0;
	
	if(set)
	{
		altered = val[0] != (  ((aic3106->left_adc_pga_gain_ctrl[0] & 0x80)>>7) |
							 ((aic3106->right_adc_pga_gain_ctrl[0] & 0x80)>>6) );
		
		aic3106->left_adc_pga_gain_ctrl[0] &= 0x7f;
		aic3106->left_adc_pga_gain_ctrl[0] |= (val[0] & SND_MIXER_CHN_MASK_FRONT_LEFT)<<7;
		aic3106->right_adc_pga_gain_ctrl[0] &= 0x7f;
		aic3106->right_adc_pga_gain_ctrl[0] |= (val[0] & SND_MIXER_CHN_MASK_FRONT_RIGHT)<<6;
		
		aic3106_wr(aic3106, AIC3106_LEFT_ADC_PGA_GAIN_CTRL, aic3106->left_adc_pga_gain_ctrl[0], aic3106->i2c_addr1);
		aic3106_wr(aic3106, AIC3106_RIGHT_ADC_PGA_GAIN_CTRL, aic3106->right_adc_pga_gain_ctrl[0], aic3106->i2c_addr1);
	}
	else
	{
		val[0] = (aic3106->left_adc_pga_gain_ctrl[0] & 0x80)>>7 |
		(aic3106->right_adc_pga_gain_ctrl[0] & 0x80)>>6;
	}
	return (altered);
}

static int32_t build_aic3106_mixer(MIXER_CONTEXT_T * aic3106, ado_mixer_t * mixer)
{
	int error = 0;
	
	ado_mixer_delement_t *play_vol;
	ado_mixer_delement_t *play_mute;
	ado_mixer_delement_t *play_out;
	ado_mixer_dgroup_t *play_grp;
	
	ado_mixer_delement_t *input_mux = NULL;
	
	ado_mixer_delement_t *input_vol = NULL;
	ado_mixer_delement_t *input_mute = NULL;
	
	ado_mixer_dgroup_t *linein_grp;
	ado_mixer_dgroup_t *micin_grp;
	ado_mixer_dgroup_t *igain_grp;
	
	ado_mixer_delement_t *pcm_out, *pcm_in = NULL;
	
	if(SEL_MCASP1 == aic3106->sel_mcasp)
	{
		/* ################ */
		/* the OUTPUT GROUP */
		/* ################ */
		
		// pcm out
		if (!error && (pcm_out = ado_mixer_element_pcm1(mixer, SND_MIXER_ELEMENT_PLAYBACK,SND_MIXER_ETYPE_PLAYBACK1, 1, &pcm_devices[0])) == NULL)
			error++;
		
		// volume
		if (!error && (play_vol = ado_mixer_element_volume1(mixer, "PCM Volume", 2,output_range, aic3106_output_vol_control, (void *) NULL, NULL)) == NULL)
			error++;
		
		// route pcm_out to volume
		if (!error && ado_mixer_element_route_add(mixer, pcm_out, play_vol) != 0)
			error++;
		
		// mute
		if (!error && (play_mute = ado_mixer_element_sw1(mixer, "HP Mute", 2,aic3106_output_mute_control, (void *) NULL, NULL)) == NULL)
			error++;
		
		// route volume to mute
		if (!error && ado_mixer_element_route_add(mixer, play_vol, play_mute) != 0)
			error++;
		
		// ouput
		if (!error && (play_out = ado_mixer_element_io(mixer, "PCM_OUT", SND_MIXER_ETYPE_OUTPUT, 0, 2, stereo_voices)) == NULL)
			error++;
		if (!error && ado_mixer_element_route_add(mixer, play_mute, play_out) != 0)
			error++;
		
		if (!error && (play_grp = ado_mixer_playback_group_create(mixer, SND_MIXER_PCM_OUT,SND_MIXER_CHN_MASK_STEREO, play_vol, play_mute)) == NULL)
			error++;
		
		if(!error &&
		   ado_mixer_switch_new(mixer, "Headphone Select", SND_SW_TYPE_BOOLEAN, 0, aic3106_hp_get, aic3106_hp_set, NULL, NULL) == NULL)
			error++;
		
		/* ################ */
		/* the INPUT GROUP */
		/* ################ */
		
		// Input mux component
		if(!error && (input_mux = ado_mixer_element_mux1(mixer, "Input Mux", 0, 2, aic3106_mux_control, NULL, NULL)) == NULL)
			error++;
		
		// Input gain group
		if(!error && (input_vol = ado_mixer_element_volume1(mixer, "Input Volume", 2,input_range, aic3106_input_vol_control, (void*) NULL, NULL)) == NULL)
			error++;
		
		// route mux output to volume element
		if(!error && ado_mixer_element_route_add(mixer, input_mux, input_vol) != 0)
			error++;
		
		if(!error && (input_mute = ado_mixer_element_sw1(mixer, "Input Mute", 2,aic3106_input_mute_control, NULL, NULL)) == NULL)
			error++;
		
		// route vol to mute
		if(!error && ado_mixer_element_route_add(mixer, input_vol, input_mute) != 0)
			error++;
		
		// create input gain group
		if(!error && (igain_grp = ado_mixer_capture_group_create(mixer, SND_MIXER_GRP_IGAIN, SND_MIXER_CHN_MASK_STEREO,input_vol, input_mute, NULL, NULL)) == NULL)
			error++;
		
		// Linein group
		if(!error && (aic3106->aux_in = ado_mixer_element_io(mixer, "LINEIN", SND_MIXER_ETYPE_INPUT, 0, 2, stereo_voices)) == NULL)
			error++;
		
		// route linein io to mux input
		if(!error && ado_mixer_element_route_add(mixer, aic3106->aux_in, input_mux) != 0)
			error++;
		
		// create linein group
		if(!error && (linein_grp = ado_mixer_capture_group_create(mixer, SND_MIXER_LINE_IN,SND_MIXER_CHN_MASK_STEREO, NULL, NULL, input_mux, aic3106->aux_in)) == NULL)
			error++;
		
		// Mic group
		if(!error && (aic3106->mic1_in = ado_mixer_element_io(mixer, "MICIN", SND_MIXER_ETYPE_INPUT, 0, 2, stereo_voices)) == NULL)
			error++;
		
		// route mic io to mux input
		if(!error && ado_mixer_element_route_add(mixer, aic3106->mic1_in, input_mux) != 0)
			error++;
		
		// create mic group
		if(!error && (micin_grp = ado_mixer_capture_group_create(mixer, SND_MIXER_MIC_IN,SND_MIXER_CHN_MASK_STEREO, NULL, NULL, input_mux, aic3106->mic1_in)) == NULL)
			error++;
		
		// PCM component
		if(!error && (pcm_in = ado_mixer_element_pcm1(mixer, SND_MIXER_ELEMENT_CAPTURE, SND_MIXER_ETYPE_CAPTURE1, 1, &pcm_devices[0])) == NULL)
			error++;
		
		// route mute to pcm
		if(!error && ado_mixer_element_route_add(mixer, input_mute, pcm_in) != 0)
			error++;
		
		return (!error ? 0 : -1);
	}
	return (0);
}


static ado_mixer_reset_t aic3106_reset;
static int aic3106_reset(MIXER_CONTEXT_T * aic3106)
{
	int i, j;
	uint8_t regVal;
	int ind;
	
	for(j=0, i = aic3106->i2c_addr1; i <= (aic3106->i2c_addr1 + aic3106->num_of_codecs - 1); i++, j++)
	{
		aic3106_wr(aic3106, AIC3106_PAGESELECTOR, PAGE_SELECT, i);		// select Page 0
		aic3106_wr(aic3106, AIC3106_SFT_RESET, SFT_RESET, i); 		// rest codec
		
		if( MASTER == aic3106->hwc->clk_master)
		{
			// BCLK and WCLK as input, slave mode, Place DOUT in high impedance when valid data is not being sent
			aic3106_wr(aic3106, AIC3106_AUDIO_SER_DATA_INT_CTRL_A, REG_A_SERIAL_DOUT_3STATE_CTRL, i);
		}
		else 
		{ // McASP is SLAVE -> Codec is Master
			
			// BCLK and WCLK as output, master mode, Place DOUT in high impedance when valid data is not being sent
			aic3106_wr(aic3106, AIC3106_AUDIO_SER_DATA_INT_CTRL_A,(REG_A_BCLK_DIR_CTRL | REG_A_WCLK_DIR_CTRL | REG_A_SERIAL_DOUT_3STATE_CTRL), i);
			
			for(ind=0; ind < NUM_CLOCK_ENTRIES; ind++)
			{
				if(aic3106->hwc->mclk_freq == codec_pll_dividers[ind][0])
				{
					// found the correct mclk
					
					if(0 == codec_pll_dividers[ind][PLL_Q_INDEX])
					{
						//Q=0 means we need to enable PLL and use the values for P,R,J and D
						aic3106_wr(aic3106, AIC3106_PLL_PROGRAM_REG_A,(REG_A_PLL_CTRL | REG_A_PLL_P(codec_pll_dividers[ind][PLL_P_INDEX])), i);		//Enable PLL, P=2
						aic3106_wr(aic3106, AIC3106_PLL_PROGRAM_REG_B, REG_B_PLL_J(codec_pll_dividers[ind][PLL_J_INDEX]), i);						// J=7
						aic3106_wr(aic3106, AIC3106_PLL_PROGRAM_REG_C, REG_C_PLL_D_MOST_BIT(codec_pll_dividers[ind][PLL_D_INDEX]) , i);			// D=3500
						aic3106_wr(aic3106, AIC3106_PLL_PROGRAM_REG_D, REG_D_PLL_D_LEAST_BIT(codec_pll_dividers[ind][PLL_D_INDEX]) , i);			// D=3500
						aic3106_wr(aic3106, AIC3106_AUDIO_CODEC_OVERFLOW_FLAG, OVERFLOW_FLAG_PLL_R(codec_pll_dividers[ind][PLL_R_INDEX]) , i);	// R=1							
						
					}
					else 
					{
						// A non-zero value for Q means the do not need to use the PLL
						aic3106_wr(aic3106, AIC3106_PLL_PROGRAM_REG_A,REG_A_PLL_Q(codec_pll_dividers[ind][PLL_Q_INDEX]), i);		// Set Q value
					}
					break;

				}
			}
			if(ind == NUM_CLOCK_ENTRIES)
			{
				ado_error("aic3106: invalid MCLK value (%d)", aic3106->hwc->mclk_freq);
				return(-1);
			}
		}

		
		// use DSP mode for num_of_codecs > 1 and Left Justified for num_of_codecs =1
		// 16-bit word
		if(aic3106->num_of_codecs > 1)
			aic3106_wr(aic3106, AIC3106_AUDIO_SER_DATA_INT_CTRL_B, (REG_B_BCLK_RATE_CTRL | REG_B_AUDIO_SERIAL_DATA_MODE), i);
		else
			aic3106_wr(aic3106, AIC3106_AUDIO_SER_DATA_INT_CTRL_B, (0x00), i);
		/**
		 * TODO: offset are still not behaving properly for McASP5
		 * offset: 0 for aic3106#1, 32 for aic3106#2 and 64 for aic3106#3
		 */
		if(aic3106->num_of_codecs > 1)
			aic3106_wr(aic3106, AIC3106_AUDIO_SER_DATA_INT_CTRL_C, (((j)<<6)+1), i);
		
		////////////
		// INPUTS
		///////////
		
		if(SEL_MCASP1 == aic3106->sel_mcasp)
		{
			// connect MIC3L to Left ADC and MIC3R to right ADC
			aic3106_wr(aic3106, AIC3106_MIC3LR_TO_LEFT_ADC_CTRL, MIC3R_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX, i);
			aic3106_wr(aic3106, AIC3106_MIC3LR_TO_RIGHT_ADC_CTRL, MIC3L_INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX, i);
			
			// disconnect Line1L and Line1R from Left and Right ADC (can connect through mux)
			aic3106_wr(aic3106, AIC3106_LINE1L_TO_LEFT_ADC_CTRL, LINE1L_INPUT_LEVEL_CTRL_LEFT_ADC_PGA_MIX, i);
			aic3106_wr(aic3106, AIC3106_LINE1R_TO_RIGHT_ADC_CTRL, LINE1R__INPUT_LEVEL_CTRL_RIGHT_ADC_PGA_MIX, i);
			
		}

		// turn on microphone biasing
		aic3106_wr(aic3106, AIC3106_MICBIAS_CTRL, MICBIAS_AVDD, i);
		
		// set Left and Right ADC to (29.5 db (50%))
		aic3106->left_adc_pga_gain_ctrl[j] = 0x3b;
		aic3106->right_adc_pga_gain_ctrl[j] = 0x3b;
		aic3106_wr(aic3106, AIC3106_LEFT_ADC_PGA_GAIN_CTRL, aic3106->left_adc_pga_gain_ctrl[j], i);
		aic3106_wr(aic3106, AIC3106_RIGHT_ADC_PGA_GAIN_CTRL, aic3106->right_adc_pga_gain_ctrl[j], i);
		
		// Turn on Left and Right ADC
		regVal = aic3106_rd(aic3106, AIC3106_LINE1L_TO_LEFT_ADC_CTRL, i);
		aic3106_wr(aic3106, AIC3106_LINE1L_TO_LEFT_ADC_CTRL, regVal | (1<<2), i);
		regVal = aic3106_rd(aic3106, AIC3106_LINE1R_TO_RIGHT_ADC_CTRL, i);
		aic3106_wr(aic3106, AIC3106_LINE1R_TO_RIGHT_ADC_CTRL, regVal | (1<<2), i);
		
		////////////
		// OUTPUTS
		///////////
		
		// set output vol to 100% (0db)
		aic3106->left_dac_vol_ctrl[j] = 0x0;
		aic3106->right_dac_vol_ctrl[j] = 0x0;
		aic3106_wr(aic3106, AIC3106_LEFT_DAC_DIG_VOL_CTRL, aic3106->left_dac_vol_ctrl[j], i);
		aic3106_wr(aic3106, AIC3106_RIGHT_DAC_DIG_VOL_CTRL, aic3106->right_dac_vol_ctrl[j], i);
		
		// Select DAC output to DAC_L1/R1 path (direct from DAC to HPOUT)
		aic3106_wr(aic3106, AIC3106_DAC_OUTPUT_SWITCH_CTRL, 0x00, i);
		
		// LEFT/LOP/M Vol Ctrl (0db) + not muted + powered on
		//aic3106_wr(aic3106, AIC3106_LEFTLOP_OUTPUT_LVL_CTRL, (LEFTLOP_LEFT_LOP_M_POWER_STATUS | LEFTLOP_LEFT_LOP_M_MUTE), i);
		//aic3106_wr(aic3106, AIC3106_RIGHTLOP_OUTPUT_LVL_CTRL, (RIGHTLOP_RIGHT_LOP_M_POWER_STATUS | RIGHTLOP_RIGHT_LOP_M_MUTE), i);

		// LEFT/LOP/M Vol Ctrl (0db) + muted
		aic3106_wr(aic3106, AIC3106_LEFTLOP_OUTPUT_LVL_CTRL, 0x00, i);
		aic3106_wr(aic3106, AIC3106_RIGHTLOP_OUTPUT_LVL_CTRL, 0x00, i);
		
		if(SEL_MCASP1 == aic3106->sel_mcasp)
		{
			// select DAC_L1 path to HPLOUT and DAC_R1 to HPROUT
			aic3106_wr(aic3106, AIC3106_DAC_L1_TO_HPLOUT_VOL_CTRL, HPLOUT_DAC_L1_OUT_ROUTING_CTR, i);
			aic3106_wr(aic3106, AIC3106_DAC_R1_TO_HPROUT_VOL_CTRL, HPROUT_DAC_R1_OUT_ROUTING_CTRL, i);
			
			// HPL/ROUT Vol Ctrl (0db) + not muted + powered on
			aic3106_wr(aic3106, AIC3106_HPLOUT_OUTPUT_LVL_CTRL, (HPLOUT_POWER_CTRL | HPLOUT_MUTE), i);
			aic3106_wr(aic3106, AIC3106_HPROUT_OUTPUT_LVL_CTRL, (HPROUT_POWER_CTRL | HPROUT_MUTE), i);
		}
		
		// Power up Left and Right DAC
		aic3106_wr(aic3106, AIC3106_DAC_POWER_DRIVER_CTRL, (LEFT_DAC_POWER_CTRL | RIGHT_DAC_POWER_CTRL), i);
		
	}
	return (0);
}


static ado_mixer_destroy_t aic3106_destroy;
static int aic3106_destroy(MIXER_CONTEXT_T * aic3106)
{
	int i;
	ado_debug(DB_LVL_MIXER, "destroying AIC3106 Codec");
	
	for(i = aic3106->i2c_addr1; i <= (aic3106->i2c_addr1 + aic3106->num_of_codecs - 1); i++)
		aic3106_wr(aic3106, AIC3106_SFT_RESET, SFT_RESET, i);
	return (0);
}

/**
 * This function is responsible for configuring the aic3106 chip's
 * sample rates
 */
int
aic3106_set_sample_rate(MIXER_CONTEXT_T *aic3106)
{
	uint32_t	fsref;
	uint32_t	fdiv;
	int i;
	// currently, aic3106 on J5, supports only 44.1KHz
	fsref = 44100;
	fdiv = 0; // fs = fsref / 1;
	
	for(i = aic3106->i2c_addr1; i <= (aic3106->i2c_addr1 + aic3106->num_of_codecs - 1); i++)
	{
		aic3106_wr(aic3106, AIC3106_CODEC_SAMPLE_RATE_SELEC, fdiv,i);
		// Left and Right DAC datapath control + sampling frequency
		aic3106_wr(aic3106, AIC3106_CODEC_DATAPATH_SETUP,(RIGTH_DAC_DATAPATH_CTRL | LEFT_DAC_DATAPATH_CTRL |FS_SET), i); //44.1 kHz
	}
	return 0;
}

int
codec_mixer(ado_card_t * card, HW_CONTEXT_T * hwc)
{
	aic3106_context_t *aic3106;
	int32_t status;
	
	ado_debug(DB_LVL_MIXER, "initializing AIC3106A Codec");
	
	if ((aic3106 = (aic3106_context_t *) ado_calloc(1, sizeof (aic3106_context_t))) == NULL)
	{
		ado_error("aic3106: no memory %s", strerror(errno));
		return (-1);
	}
	if ((status = ado_mixer_create(card, "aic3106", &hwc->mixer, aic3106)) != EOK)
	{
		ado_free(aic3106);
		return (status);
	}
	aic3106->mixer = hwc->mixer;
	aic3106->hwc = hwc;
	aic3106->output_routing = SWITCH_HPOUT; // valid for McASP1, ignored otherwise
	aic3106->input_mux = MIC1_IN_INPUT;
	
	// determine which mcasp is being used
	if(hwc->mcasp_baseaddr == MCASP1_BASEADDR)
		aic3106->sel_mcasp = SEL_MCASP1;

	if(hwc->i2c_dev == 0xff)
	{
		ado_debug(DB_LVL_DRIVER, "No i2c device selected (will assume Mcasp0 --> i2c1)");
		if(SEL_MCASP1 == aic3106->sel_mcasp)
			hwc->i2c_dev = 1;
	}
	
	switch(hwc->i2c_dev)
	{
		case 0:
			if ((aic3106->fd = open("/dev/i2c0", O_RDWR)) < 0)
			{
				ado_error("aic3106: could not open i2c device %s", strerror(errno));
				return (-1);
			}
			break;
		case 1:
			if ((aic3106->fd = open("/dev/i2c1", O_RDWR)) < 0)
			{
				ado_error("aic3106: could not open i2c device %s", strerror(errno));
				return (-1);
			}
			break;
		case 2:
			if ((aic3106->fd = open("/dev/i2c2", O_RDWR)) < 0)
			{
				ado_error("aic3106: could not open i2c device %s", strerror(errno));
				return (-1);
			}
			break;
		default:
			break;
	}
	
	aic3106->tx_voices = hwc->tx_voices;
	aic3106->rx_voices = hwc->rx_voices;
	aic3106->num_of_codecs = aic3106->tx_voices>>1; // each codes supports two channels
	
	aic3106->i2c_addr1 = hwc->codec_i2c_addr;
	aic3106->i2c_addr2 = aic3106->i2c_addr1 + 1; // will be ignored for McASP2
	aic3106->i2c_addr3 = aic3106->i2c_addr2 + 1; // will be ingnored for McASP2
	
	// reset codec(s)
	if(aic3106_reset(aic3106) == -1)
    {
		ado_error("aic3106: could not initialize codec");
		return (-1);
    }
	
	// set sample rate
	aic3106_set_sample_rate(aic3106);
	
	if (build_aic3106_mixer(aic3106, aic3106->mixer))
	{
		ado_free(aic3106);
		return (-1);
	}
	
	ado_mixer_set_reset_func(aic3106->mixer, aic3106_reset);
	ado_mixer_set_destroy_func(aic3106->mixer, aic3106_destroy);
	
	/* setup mixer controls for pcm  */
	ado_pcm_chn_mixer(hwc->pcm, ADO_PCM_CHANNEL_PLAYBACK,
					  hwc->mixer, ado_mixer_find_element(hwc->mixer, SND_MIXER_ETYPE_PLAYBACK1, SND_MIXER_ELEMENT_PLAYBACK, 0),
					  ado_mixer_find_group(hwc->mixer, SND_MIXER_PCM_OUT, 0));
	
	ado_pcm_chn_mixer(hwc->pcm, ADO_PCM_CHANNEL_CAPTURE,
					  hwc->mixer, ado_mixer_find_element(hwc->mixer, SND_MIXER_ETYPE_CAPTURE1, SND_MIXER_ELEMENT_CAPTURE, 0),
					  ado_mixer_find_group(hwc->mixer, SND_MIXER_GRP_IGAIN, 0));
	
	return (0);
}

__SRCVERSION
("$URL: http://svn/product/trunk/hardware/deva/ctrl/dra44x/nto/arm/dll.le.jacinto/aic3106.c $ $Rev: 217582 $");
