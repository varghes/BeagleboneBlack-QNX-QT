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

#include "rtc.h"
#include <time.h>
#include <fcntl.h>
#include <hw/i2c.h>

/*
 * TPS65910 RTC driver
 */

/*
 * TPS65910 registers
 */
#define TPS65910_SECONDS_REG        0x00
#define TPS65910_MINUTES_REG        0x01
#define TPS65910_HOURS_REG          0x02
#define TPS65910_DAYS_REG           0x03
#define TPS65910_MONTHS_REG         0x04
#define TPS65910_YEARS_REG          0x05
#define TPS65910_WEEKS_REG          0x06
#define TPS65910_ALRM_SECS_REG      0x08
#define TPS65910_ALRM_MINS_REG      0x09
#define TPS65910_ALRM_HRS_REG       0x0a
#define TPS65910_ALRM_DAYS_REG      0x0b
#define TPS65910_ALRM_MNTHS_REG     0x0c
#define TPS65910_ALRM_YRS_REG       0x0d
#define TPS65910_RTC_CTRL_REG       0x10
#define TPS65910_RTC_STATUS_REG     0x11
#define TPS65910_RTC_INTS_REG       0x12
#define TPS65910_RTC_COMP_LSB_REG   0x13
#define TPS65910_RTC_COMP_MSB_REG   0x14
#define TPS65910_VRTC_REG           0x1E
#define TPS65910_DEVCTRL_REG        0x3F

/*  RTC control register bits */
#define TPS65910_RTC_CTRL_RTC_V_OPT         0x1 << 7
#define TPS65910_RTC_CTRL_MODE_12_24 	    0x1 << 3
#define TPS65910_RTC_CTRL_GET_TIME       	0x1 << 6
#define TPS65910_RTC_CTRL_STOP_BIT       	0x1

/* VRTC_REG register bits */
#define TPS65910_VRTC_OFFMASK               0x1 << 3

/* DEVCTRL register bits */
#define TPS65910_DEVCTRL_CK32K_CTRL         0x1 << 5
#define TPS65910_DEVCTRL_RTC_PWDN           0x1 << 6

/*
 * BCD buf offsets
 */
#define TPS65910_SECONDS_REG_OFFSET        0x00
#define TPS65910_MINUTES_REG_OFFSET        0x01
#define TPS65910_HOURS_REG_OFFSET          0x02
#define TPS65910_DAYS_REG_OFFSET           0x03
#define TPS65910_MONTHS_REG_OFFSET         0x04
#define TPS65910_YEARS_REG_OFFSET          0x05
#define TPS65910_WEEKS_REG_OFFSET          0x06

#define TPS65910_I2C_DEVNAME               "/dev/i2c0"

#define TPS65910_I2C_ADDR                  0x2D

/* TPS65910 I2C clock speed */
#define TPS65910_I2C_SPEED                 100000

// Global file descriptor 
static int fd = -1;

/*************************************************************************
 *  twl4030_i2c_write_i2c_read
 *************************************************************************/
static int tps65910_i2c_read(uint8_t reg, uint8_t * val, int num)
{
    int error;
    int i;

    struct send_recv
    {
        i2c_sendrecv_t hdr;
        uint8_t buf[9];
    } tps65910_rd_data;

    /* Read the Registers Current Value */
    tps65910_rd_data.buf[0] = reg;
    tps65910_rd_data.hdr.send_len = 1;
    tps65910_rd_data.hdr.recv_len = num;

    tps65910_rd_data.hdr.slave.addr = TPS65910_I2C_ADDR;
    tps65910_rd_data.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
    tps65910_rd_data.hdr.stop = 1;

    error = devctl( fd, DCMD_I2C_SENDRECV, &tps65910_rd_data, sizeof(tps65910_rd_data), NULL);
    if (error != EOK)
    {
        fprintf( stderr, "RTC:  TPS65910 RTC: tps65910_i2c_read fail, error code %d\n", error );
        fprintf( stderr, "RTC, error means %s \n", strerror( error ) );
        return error;
    }

    for (i = 0; i < num; i++)
    {
        val[i] = tps65910_rd_data.buf[i];
    }

    return 0;
}

/*************************************************************************
 *  tps65910_i2c_write
 *************************************************************************/
static int tps65910_i2c_write(uint8_t reg, uint8_t *val, int num)
{
    int i;
    int error;

    struct send
    {
        i2c_send_t hdr;
        uint8_t buf[9];
    } tps65910_wr_data;

    /* Increment the # of bytes to send to account for slave reg addy. */
    num++;

    tps65910_wr_data.hdr.slave.addr = TPS65910_I2C_ADDR;
    tps65910_wr_data.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
    tps65910_wr_data.hdr.len = num;
    tps65910_wr_data.hdr.stop = 1;

    tps65910_wr_data.buf[0] = reg;

    for (i = 1; i < num; i++)
    {
        /* The val buffer is 1 place behind buf[] due to buf[0] == reg addy */
        tps65910_wr_data.buf[i] = val[i - 1];
    }

    error = devctl( fd, DCMD_I2C_SEND, &tps65910_wr_data, sizeof(tps65910_wr_data), NULL);

    if (error != EOK)
    {
        fprintf( stderr, "RTC:  TPS65910 RTC: tps65910_i2c_write fail\n" );
        fprintf( stderr, "RTC, error means %s \n", strerror( error ) );
        return error;
    }

    return 0;

}

/*************************************************************************
 *  tps65910 init
 *************************************************************************/
int RTCFUNC(init,tps65910)(struct chip_loc *chip, char *argv[])
{
    unsigned speed;
    uint8_t regval = 0;

    //fprintf(stdout, "+RTCFunc, tps65910 init \n");

    fd = open( TPS65910_I2C_DEVNAME, O_RDWR);
    if (fd < 0)
    {
        fprintf( stderr, "RTC:  Unable to open I2C device\n" );
        return -1;
    }

    /*
     * Set the I2C speed
     */
    speed = TPS65910_I2C_SPEED;
    if (devctl( fd, DCMD_I2C_SET_BUS_SPEED, &speed, sizeof(speed), NULL) != EOK)
    {
        fprintf( stderr, "RTC:  Unable to set I2C speed\n" );
        return -1;
    }

    /* The following settings will only have an effect if not
     * set previously, but set them anyway to ensure RTC is
     * operational: */

    /* Power on RTC domain, set clock source to internal RC */
    if (tps65910_i2c_read( TPS65910_DEVCTRL_REG, &regval, 1 ) != EOK)
	{
		fprintf( stderr, "RTC: Unable to read data from I2C device \n" );
		return -1;
	}
    regval &= ~TPS65910_DEVCTRL_RTC_PWDN;
    regval |= TPS65910_DEVCTRL_CK32K_CTRL;
    if (tps65910_i2c_write( TPS65910_DEVCTRL_REG, &regval, 1 ) != EOK)
	{
		fprintf( stderr, "RTC Init: Unable to write control reg to I2C device\n" );
		return -1;
	}

    /* Enable VRTC during OFF state */
    if (tps65910_i2c_read( TPS65910_VRTC_REG, &regval, 1 ) != EOK)
	{
		fprintf( stderr, "RTC: Unable to read data from I2C device \n" );
		return -1;
	}
    regval |= TPS65910_VRTC_OFFMASK;
    if (tps65910_i2c_write( TPS65910_VRTC_REG, &regval, 1 ) != EOK)
	{
		fprintf( stderr, "RTC Init: Unable to write control reg to I2C device\n" );
		return -1;
	}

    /* Start the RTC, set reads to access the static shadowed registers */
    if (tps65910_i2c_read( TPS65910_RTC_CTRL_REG, &regval, 1 ) != EOK)
	{
		fprintf( stderr, "RTC: Unable to read data from I2C device \n" );
		return -1;
	}
    regval |= TPS65910_RTC_CTRL_STOP_BIT;
    regval |= TPS65910_RTC_CTRL_RTC_V_OPT;
    if (tps65910_i2c_write( TPS65910_RTC_CTRL_REG, &regval, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write control reg to I2C device\n" );
        return -1;
    }

    //fprintf(stdout, "-RTCFunc, tps65910 init \n");

    return 0;
}

/*************************************************************************
 *  tps65910 get
 *************************************************************************/
int RTCFUNC(get,tps65910)(struct tm *tm, int cent_reg)
{
    uint8_t ctrlreg = 0;
    uint8_t val[7];

    //fprintf(stdout, "+RTCFunc, tps65910, get \n");

    /* Update the shadow registers */
    if (tps65910_i2c_read( TPS65910_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC: Unable to read data from I2C device \n" );
        return -1;
    }
    ctrlreg |= TPS65910_RTC_CTRL_GET_TIME;
    if (tps65910_i2c_write( TPS65910_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write 'get time bit' to I2C device\n" );
        return -1;
    }

    memset( val, 0x0, sizeof(val) );

    /* Read out the time */
    if (tps65910_i2c_read( TPS65910_SECONDS_REG, val, 7 ) != EOK)
    {
        fprintf( stderr, "RTC: Unable to read date information from I2C device \n" );
        return -1;
    }

    /* Parse time */
    tm->tm_sec = BCD2BIN(val[TPS65910_SECONDS_REG_OFFSET] );
    tm->tm_min = BCD2BIN(val[TPS65910_MINUTES_REG_OFFSET] );
    tm->tm_hour = BCD2BIN(val[TPS65910_HOURS_REG_OFFSET] ) & 0x3f;
    tm->tm_mday = BCD2BIN(val[TPS65910_DAYS_REG_OFFSET]);
    tm->tm_wday = BCD2BIN(val[TPS65910_WEEKS_REG_OFFSET]);
    tm->tm_mon = BCD2BIN(val[TPS65910_MONTHS_REG_OFFSET ]) - 1;
    tm->tm_year = BCD2BIN(val[TPS65910_YEARS_REG_OFFSET]);

    /* Adjust hour data if RTC is using a 12-hour clock */
    if ((ctrlreg & TPS65910_RTC_CTRL_MODE_12_24))
    {
        tm->tm_hour += 12;
    }

    /* No century bit in RTC, so this will have to do for now */
    if (tm->tm_year < 70)
        tm->tm_year += 100;

    //fprintf(stdout, "-RTCFunc, tps65910, get \n");

    return (0);
}

/*************************************************************************
 *  tps65910 set
 *************************************************************************/
int RTCFUNC(set,tps65910)(struct tm *tm, int cent_reg)
{
    uint8_t ctrlreg = 0;
    uint8_t val[7];

    //fprintf(stdout, "+RTCFunc, tps65910, set \n");

    /* Stop the RTC to set the registers. */
    if (tps65910_i2c_read( TPS65910_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC: Unable to read data from I2C device \n" );
        return -1;
    }
    ctrlreg &= ~TPS65910_RTC_CTRL_STOP_BIT;
    if (tps65910_i2c_write( TPS65910_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write 'get time bit' to I2C device\n" );
        return -1;
    }

    memset( val, 0x0, sizeof(val) );

    /* Adjust hour data if RTC is using a 12-hour clock */
    if (ctrlreg & TPS65910_RTC_CTRL_MODE_12_24)
    {
        val[TPS65910_HOURS_REG_OFFSET] = (tm->tm_hour < 12) ? BIN2BCD(tm->tm_hour)
                : (TPS65910_RTC_CTRL_MODE_12_24 | BIN2BCD(tm->tm_hour - 12));
    }
    else
    {
        val[TPS65910_HOURS_REG_OFFSET] = BIN2BCD(tm->tm_hour);
    }

    /* Now stuff the buffer with the appropriate values */
    val[TPS65910_SECONDS_REG_OFFSET] = BIN2BCD(tm->tm_sec );
    val[TPS65910_MINUTES_REG_OFFSET] = BIN2BCD(tm->tm_min );
    val[TPS65910_DAYS_REG_OFFSET] = BIN2BCD(tm->tm_mday);
    val[TPS65910_WEEKS_REG_OFFSET] = BIN2BCD(tm->tm_wday);
    val[TPS65910_MONTHS_REG_OFFSET] = BIN2BCD(tm->tm_mon + 1);
    val[TPS65910_YEARS_REG_OFFSET] = BIN2BCD(tm->tm_year % 100);

    if (tps65910_i2c_write( TPS65910_SECONDS_REG, val, 7 ) != EOK)
    {
        fprintf( stderr, "RTC Set: Unable to write date to I2C device\n" );
        return -1;
    }

    /* Restart the RTC */
    ctrlreg |= TPS65910_RTC_CTRL_STOP_BIT;
    if (tps65910_i2c_write( TPS65910_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write restart to rtc. \n" );
        return -1;
    }

    //fprintf(stdout, "-RTCFunc, tps65910, set \n");

    return (0);
}
