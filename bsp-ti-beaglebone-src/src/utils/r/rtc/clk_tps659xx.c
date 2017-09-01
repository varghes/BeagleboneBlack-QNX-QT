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
 * TPS659xx RTC driver
 *
 *  NOTE:  This driver assumes UNSECURE mode!
 */

/*
 * TPS659xx registers
 */
#define TPS659xx_SECONDS_REG        0x1c
#define TPS659xx_MINUTES_REG        0x1d
#define TPS659xx_HOURS_REG          0x1e
#define TPS659xx_DAYS_REG           0x1f
#define TPS659xx_MONTHS_REG         0x20
#define TPS659xx_YEARS_REG          0x21
#define TPS659xx_WEEKS_REG          0x22
#define TPS659xx_ALRM_SECS_REG      0x23
#define TPS659xx_ALRM_MINS_REG      0x24
#define TPS659xx_ALRM_HRS_REG       0x25
#define TPS659xx_ALRM_DAYS_REG      0x26
#define TPS659xx_ALRM_MNTHS_REG     0x27
#define TPS659xx_ALRM_YRS_REG       0x28
#define TPS659xx_RTC_CTRL_REG       0x29
#define TPS659xx_RTC_STATUS_REG     0x2a
#define TPS659xx_RTC_INTS_REG       0x2b
#define TPS659xx_RTC_COMP_LSB_REG   0x2c
#define TPS659xx_RTC_COMP_MSB_REG   0x2d

/*
 * TPS659xx register offsets 
 */
#define TPS659xx_SECONDS_REG_OFFSET         0x00
#define TPS659xx_MINUTES_REG_OFFSET         0x01
#define TPS659xx_HOURS_REG_OFFSET           0x02
#define TPS659xx_DAYS_REG_OFFSET            0x03
#define TPS659xx_MONTHS_REG_OFFSET          0x04
#define TPS659xx_YEARS_REG_OFFSET           0x05
#define TPS659xx_WEEKS_REG_OFFSET           0x06
#define TPS659xx_ALRM_SECS_REG_OFFSET       0x07
#define TPS659xx_ALRM_MINS_REG_OFFSET       0x08
#define TPS659xx_ALRM_HRS_REG_OFFSET        0x09
#define TPS659xx_ALRM_DAYS_REG_OFFSET       0x0a
#define TPS659xx_ALRM_MNTHS_REG_OFFSET      0x0b
#define TPS659xx_ALRM_YRS_REG_OFFSET        0x0c
#define TPS659xx_RTC_CTRL_REG_OFFSET        0x0d
#define TPS659xx_RTC_STATUS_REG_OFFSET      0x0e
#define TPS659xx_RTC_INTS_REG_OFFSET        0x0f
#define TPS659xx_RTC_COMP_LSB_REG_OFFSET    0x10
#define TPS659xx_RTC_COMP_MSB_REG_OFFSET    0x11

/*  AM/PM bit in status register */
#define TPS659xx_RTD_PM		    0x08
#define TPS659xx_RESET_EVENT    0x80
#define TPS659xx_GET_TIME       0x1 << 6
#define TPS659xx_STOP_BIT       0x1

#define TPS659XX_I2C_DEVNAME    "/dev/i2c0"

#define TWL4030_ADDR_GRP1   0x4b

/* TWL4030 I2S clock speed */
#define TWL4030_I2C_SPEED     100000

// Global file descriptor 
static int fd = -1;

/*************************************************************************
 *  twl4030_i2c_write_i2c_read
 *************************************************************************/
static int tps659xx_i2c_read(uint8_t addr, uint8_t reg, uint8_t * val, int num)
{
    int error;
    int i;

    struct send_recv
    {
        i2c_sendrecv_t hdr;
        uint8_t buf[9];
    } twl4030_rd_data;

    /*Read the Registers Current Value */
    twl4030_rd_data.buf[0] = reg;
    twl4030_rd_data.hdr.send_len = 1;
    twl4030_rd_data.hdr.recv_len = num;

    twl4030_rd_data.hdr.slave.addr = addr;
    twl4030_rd_data.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
    twl4030_rd_data.hdr.stop = 1;

    error = devctl( fd, DCMD_I2C_SENDRECV, &twl4030_rd_data, sizeof(twl4030_rd_data), NULL);
    if (error != EOK)
    {
        fprintf( stderr, "RTC:  BEAGLE RTC: twl4030_i2c_read fail, error code %d\n", error );
        fprintf( stderr, "RTC, error means %s \n", strerror( error ) );
        return error;
    }

    for (i = 0; i < num; i++)
    {
        val[i] = twl4030_rd_data.buf[i];
    }

    return 0;
}

/*************************************************************************
 *  tps659xx_i2c_write
 *************************************************************************/
static int tps659xx_i2c_write(uint8_t addr, uint8_t reg, uint8_t *val, int num)
{
    int i;
    int error;

    struct send
    {
        i2c_send_t hdr;
        uint8_t buf[9];
    } twl4030_wr_data;

    // Increment the # of bytes to send to account for slave reg addy.
    num++;

    twl4030_wr_data.hdr.slave.addr = addr;
    twl4030_wr_data.hdr.slave.fmt = I2C_ADDRFMT_7BIT;
    twl4030_wr_data.hdr.len = num;
    twl4030_wr_data.hdr.stop = 1;

    twl4030_wr_data.buf[0] = reg;

    for (i = 1; i < num; i++)
    {
        // the val buffer is 1 place behind buf[] due to buf[0] == reg addy
        twl4030_wr_data.buf[i] = val[i - 1];
    }

    error = devctl( fd, DCMD_I2C_SEND, &twl4030_wr_data, sizeof(twl4030_wr_data), NULL);

    if (error != EOK)
    {
        fprintf( stderr, "RTC:  BEAGLE RTC: twl4030_i2c_write fail\n" );
        fprintf( stderr, "RTC, error means %s \n", strerror( error ) );
        return error;
    }

    return 0;

}

/*************************************************************************
 *  tps659xx init
 *************************************************************************/
int RTCFUNC(init,tps659xx)(struct chip_loc *chip, char *argv[])
{
    unsigned speed;
    uint8_t regval = 0;

    //fprintf(stdout, "+RTCFunc, tps659xx init \n");

    fd = open( TPS659XX_I2C_DEVNAME, O_RDWR);
    if (fd < 0)
    {
        fprintf( stderr, "RTC:  Unable to open I2C device\n" );
        return -1;
    }

    /*
     * Set the I2C speed
     */
    speed = TWL4030_I2C_SPEED;
    if (devctl( fd, DCMD_I2C_SET_BUS_SPEED, &speed, sizeof(speed), NULL) != EOK)
    {
        fprintf( stderr, "RTC:  Unable to set I2C speed\n" );
        return -1;
    }

    // Now try starting the RTC
    regval = 0x1;
    if (tps659xx_i2c_write( TWL4030_ADDR_GRP1, TPS659xx_RTC_CTRL_REG, &regval, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write control reg to I2C device\n" );
        return -1;
    }

    // Now write the power-up register to clear it.
    regval = 0x80;
    if (tps659xx_i2c_write( TWL4030_ADDR_GRP1, TPS659xx_RTC_STATUS_REG, &regval, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write status data to I2C device\n" );
        return -1;
    }

    //fprintf(stdout, "-RTCFunc, tps659xx init \n");

    return 0;
}

/*************************************************************************
 *  tps659xx get
 *************************************************************************/
int RTCFUNC(get,tps659xx)(struct tm *tm, int cent_reg)
{
    uint8_t ctrlreg = 0;
    uint8_t val[7];

    //fprintf(stdout, "+RTCFunc, tps659xx, get \n");

    if (tps659xx_i2c_read( TWL4030_ADDR_GRP1, TPS659xx_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC: Unable to read data from I2C device \n" );
        return -1;
    }

    ctrlreg |= TPS659xx_GET_TIME;

    // Now try setting the 'get time' bit
    if (tps659xx_i2c_write( TWL4030_ADDR_GRP1, TPS659xx_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write 'get time bit' to I2C device\n" );
        return -1;
    }

    memset( val, 0x0, sizeof(val) );

    // Now get the time.
    if (tps659xx_i2c_read( TWL4030_ADDR_GRP1, TPS659xx_SECONDS_REG, val, 7 ) != EOK)
    {
        fprintf( stderr, "RTC: Unable to read date information from I2C device \n" );
        return -1;
    }

    //Now parse time
    tm->tm_sec = BCD2BIN(val[TPS659xx_SECONDS_REG_OFFSET] );
    tm->tm_min = BCD2BIN(val[TPS659xx_MINUTES_REG_OFFSET] );
    tm->tm_hour = BCD2BIN(val[TPS659xx_HOURS_REG_OFFSET] ) & 0x3f;
    tm->tm_mday = BCD2BIN(val[TPS659xx_DAYS_REG_OFFSET]);
    tm->tm_wday = BCD2BIN(val[TPS659xx_WEEKS_REG_OFFSET]);
    tm->tm_mon = BCD2BIN(val[TPS659xx_MONTHS_REG_OFFSET ]) - 1;
    tm->tm_year = BCD2BIN(val[TPS659xx_YEARS_REG_OFFSET]);

    /* adjust hour data if RTC is using a 12-hour clock */
    if ((ctrlreg & TPS659xx_RTD_PM))
    {
        tm->tm_hour += 12;
    }

    /* no century bit in RTC, so this will have to do for now */
    if (tm->tm_year < 70)
        tm->tm_year += 100;

    return (0);
}

/*************************************************************************
 *  tps659xx set
 *************************************************************************/
int RTCFUNC(set,tps659xx)(struct tm *tm, int cent_reg)
{
    uint8_t status = 0;
    uint8_t ctrlreg = 0;
    uint8_t val[7];

    //fprintf(stdout, "+RTCFunc, tps659xx, set \n");

    // Stop the RTC to set the registers.

    if (tps659xx_i2c_read( TWL4030_ADDR_GRP1, TPS659xx_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC: Unable to read data from I2C device \n" );
        return -1;
    }

    ctrlreg &= ~TPS659xx_STOP_BIT;

    // Now try setting the 'get time' bit
    if (tps659xx_i2c_write( TWL4030_ADDR_GRP1, TPS659xx_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write 'get time bit' to I2C device\n" );
        return -1;
    }

    if (tps659xx_i2c_read( TWL4030_ADDR_GRP1, TPS659xx_RTC_CTRL_REG, &status, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Set: Unable to read status from I2C device\n" );
        return -1;
    }

    memset( val, 0x0, sizeof(val) );

    /* adjust hour data if RTC is using a 12-hour clock */
    if (status & TPS659xx_RTD_PM)
    {
        val[TPS659xx_HOURS_REG_OFFSET] = (tm->tm_hour < 12) ? BIN2BCD(tm->tm_hour)
                : (TPS659xx_RTD_PM | BIN2BCD(tm->tm_hour - 12));
    }
    else
    {
        val[TPS659xx_HOURS_REG_OFFSET] = BIN2BCD(tm->tm_hour);
    }

    // Now stuff the buffer with the appropriate values
    val[TPS659xx_SECONDS_REG_OFFSET] = BIN2BCD(tm->tm_sec );
    val[TPS659xx_MINUTES_REG_OFFSET] = BIN2BCD(tm->tm_min );
    val[TPS659xx_DAYS_REG_OFFSET] = BIN2BCD(tm->tm_mday);
    val[TPS659xx_WEEKS_REG_OFFSET] = BIN2BCD(tm->tm_wday);
    val[TPS659xx_MONTHS_REG_OFFSET] = BIN2BCD(tm->tm_mon + 1);
    val[TPS659xx_YEARS_REG_OFFSET] = BIN2BCD(tm->tm_year % 100);

    if (tps659xx_i2c_write( TWL4030_ADDR_GRP1, TPS659xx_SECONDS_REG, val, 7 ) != EOK)
    {
        fprintf( stderr, "RTC Set: Unable to write date to I2C device\n" );
        return -1;
    }

    // Now restart the RTC
    ctrlreg |= TPS659xx_STOP_BIT;

    if (tps659xx_i2c_write( TWL4030_ADDR_GRP1, TPS659xx_RTC_CTRL_REG, &ctrlreg, 1 ) != EOK)
    {
        fprintf( stderr, "RTC Init: Unable to write restart to rtc. \n" );
        return -1;
    }

    return (0);
}
