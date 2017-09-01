/*
 * clk_am335x.c
 *
 *  Created on: 10 Jan 2012
 *      Author: Administrator
 */

/*
 * AM335x RTC driver
 */
#include "rtc.h"
#include <time.h>
#include <fcntl.h>
#include <hw/i2c.h>
#include "arm/am335x.h"

int
RTCFUNC(init,am335xrtc)(struct chip_loc *chip, char *argv[])
{
#ifdef	VERBOSE_SUPPORTED
	if (verbose) {
	printf("Init AM335X RTC Called\n");
	}
#endif

	if (chip->phys == NIL_PADDR)
    	/* Set the base address to the AM335X_RTC_BASE that way we can use simple
        register offsets already defined. */
     	chip->phys = AM335X_RTC_BASE;

    	if (chip->century_reg == UNSET)
       	/* There is no century register available year is stored in 12 bits */
        chip->century_reg = -1;

    	if (chip->access_type == NONE)
        	chip->access_type = MEMMAPPED;

    	chip->reg_shift = 0;

  	/* return size of memory to be mapped */
 	 return(AM335X_RTC_SIZE);
 }

int
RTCFUNC(get,am335xrtc)(struct tm *tm, int cent_reg)
{
	uint32_t	data;
	uint32_t hours_to_add = 0;

     	/* read RTC  values */
	/*
	 * Manual states to read the seconds register first, as this
	 * will copy all registers into shadow registers so rollovers won't occur
	 * during the following reads
	 */

	data=chip_read(AM335x_SECONDS_REG,32);    /* seconds after the minute -- [0,61] */
 	tm->tm_sec = BCD2BIN(data);

	data=chip_read(AM335x_YEARS_REG,32);   /* years since 1900 (2 figures)                  */
	tm->tm_year = 100 + BCD2BIN(data); // Assumes All dates are 2000 or later

	data=chip_read(AM335x_MONTHS_REG,32);    /* months since January     -- [0,11] */
	tm->tm_mon = BCD2BIN(data);

	data=chip_read(AM335x_DAYS_REG,32);   /* day of the month         -- [1,31] */
	tm->tm_mday = BCD2BIN(data);

	data=chip_read(AM335x_WEEKS_REG,32);   /* day of the week         -- [0-6] */
	tm->tm_wday = BCD2BIN(data);

	data=chip_read(AM335x_HOURS_REG,32);   /* hours after midnight     -- [0,23] */
	if (chip_read(AM335x_RTC_CTRL_REG,32) & AM335x_RTC_CTRL_MODE_12_24)
	{
		// In PM_AM mode
		if (data & AM335xRTC_24_12_MODE)
		{
			data &= AM335xRTC_24_12_MODE_MASK;
			hours_to_add = 12;
		}

	}
	tm->tm_hour = BCD2BIN(data) + hours_to_add;

	data=chip_read(AM335x_MINUTES_REG,32);    /* minutes after the hour   -- [0,59] */
	tm->tm_min = BCD2BIN(data);


#ifdef	VERBOSE_SUPPORTED
	if (verbose) {
	 	printf("AM335x Get Called\n");
		printf("y:%d m:%d d:%d h:%d m:%d s:%d dow:%d\n",
				tm->tm_year,
				tm->tm_mon,
				tm->tm_mday,
				tm->tm_hour,
				tm->tm_min,
				tm->tm_sec,
				tm->tm_wday);
	}
#endif

	return 0;
}

int
RTCFUNC(set,am335xrtc)(struct tm *tm, int cent_reg)
{
	uint32_t data;

#ifdef	VERBOSE_SUPPORTED
	if (verbose) {
	printf("AM335X Set Called\n");
	printf("y:%d m:%d d:%d h:%d m:%d s:%d\n",
			tm->tm_year,
			tm->tm_mon,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec);
	printf("wd: %d, yd:%d is:%d gm:%ld z:%s\n",
			tm->tm_wday,
			tm->tm_yday,
			tm->tm_isdst,
			tm->tm_gmtoff,
			tm->tm_zone);
	}
#endif

	// Unlock register for writing

	chip_write(AM335x_RTC_KICK0R, AM335x_KICK_DATA0, 32);
	delay(2);
	chip_write(AM335x_RTC_KICK1R, AM335x_KICK_DATA1, 32);


	data = BIN2BCD(tm->tm_sec);
	chip_write(AM335x_SECONDS_REG, data, 32);

	data = BIN2BCD(tm->tm_min);
	chip_write(AM335x_MINUTES_REG, data, 32);

	if (chip_read(AM335x_RTC_CTRL_REG,32) & AM335x_RTC_CTRL_MODE_12_24)
	{
		if(tm->tm_hour >= 12)
		{
			data = BIN2BCD(tm->tm_hour - 12);
			data |= AM335xRTC_24_12_MODE;
		}
		else
		{
			data = BIN2BCD(tm->tm_hour);
		}
	}
	chip_write(AM335x_HOURS_REG, data, 32);

	data = BIN2BCD(tm->tm_mday);
	chip_write(AM335x_DAYS_REG, data, 32);

	data = BIN2BCD(tm->tm_mon);
	chip_write(AM335x_MONTHS_REG, data, 32);

	data = BIN2BCD(tm->tm_year+100);
	chip_write(AM335x_YEARS_REG, data, 32);

	data = BIN2BCD(tm->tm_wday);
	chip_write(AM335x_WEEKS_REG, data, 32);

	return 0;
}


