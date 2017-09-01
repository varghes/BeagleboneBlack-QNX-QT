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

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <hw/i2c.h>

#include <arm/beaglebone.h>

#define STRLEN		64

typedef struct
{
	int		fd;
	int		i2c_address;
	char	devname[STRLEN];

} AM335X_BOARDID, *AM335X_PBOARDID;

static int bdid_read(AM335X_BOARDID *bdid, BDIDENT *bdident, int showerr)
{
    struct {
        i2c_send_t      hdr;
        uint8_t	        reg[2];
    } msgreg;

    struct {
        i2c_recv_t      hdr;
        uint8_t 		val[256];
    } msgval;
    int rbytes;
    int r;

    msgreg.hdr.slave.addr = bdid->i2c_address;
    msgreg.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msgreg.hdr.len        = 2;
    msgreg.hdr.stop       = 0;
    msgreg.reg[0]         = 0;
    msgreg.reg[1]         = 0;

    r = devctl(bdid->fd, DCMD_I2C_SEND, &msgreg, sizeof(msgreg), NULL);
    if (r)
    {
    	if (showerr) printf("I2C write failed (DCMD_I2C_SEND), error %d\n", r);
    	return (-1);
    }

    msgval.hdr.slave.addr = bdid->i2c_address;
    msgval.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msgval.hdr.len        = 256;
    msgval.hdr.stop       = 1;

    r = devctl(bdid->fd, DCMD_I2C_RECV, &msgval, sizeof(msgval), &rbytes);
    if (r)
    {
    	if (showerr) printf("I2C read failed (DCMD_I2C_RECV), error %d\n", r);
        return (-1);
    }

//    // Dump board ID
//    {
//    	int i;
//		for (i=0; i<256; i+=8)
//		printf("%08x %08x %08x %08x %08x %08x %08x %08x\n", msgval.val[i+0],
//    msgval.val[i+1], msgval.val[i+2], msgval.val[i+3], msgval.val[i+4],
//    msgval.val[i+5], msgval.val[i+6], msgval.val[i+7]);
//    }
    /*
     * Board ID can be found in section 7.11, Table 7 of
     * BeagleBone reference: http://beagleboard.org/static/BONESRM_latest.pdf
     */

    // Reference: Beaglebone Manual (http://beagleboard.org/static/BONESRM_latest.pdf)
    //   Ch 7.11  EEPROM               - beaglebone main board
    //   Ch 8.1.3 EEPROM Data Format   - beaglebone cape
    memcpy(&bdident->header , &msgval.val[0]                         , AM335X_BDID_HEADER_LEN);
	memcpy(&bdident->bdname , &msgval.val[AM335X_BDID_BDNAME_OFFSET] , AM335X_BDID_BDNAME_LEN);
	bdident->bdname[AM335X_BDID_BDNAME_LEN] = 0;
	memcpy(&bdident->version, &msgval.val[AM335X_BDID_VERSION_OFFSET], AM335X_BDID_VERSION_LEN);
	bdident->version[AM335X_BDID_VERSION_LEN] = 0;
	memcpy(&bdident->serial , &msgval.val[AM335X_BDID_SERIAL_OFFSET] , AM335X_BDID_SERIAL_LEN);
	bdident->serial[AM335X_BDID_SERIAL_LEN] = 0;
	memcpy(&bdident->config , &msgval.val[AM335X_BDID_CONFIG_OFFSET] , AM335X_BDID_CONFIG_LEN);
	bdident->config[AM335X_BDID_CONFIG_LEN] = 0;
	memcpy(&bdident->macaddr, &msgval.val[AM335X_BDID_MAC1_OFFSET]   , AM335X_BDID_MAC_LEN*AM335X_MACS);

    return (rbytes);
}

int read_boardid(int address, BDIDENT *bdident, int showerr)
{
	AM335X_BOARDID	bdid;
	int				r;

	sprintf(bdid.devname, BOARDID_I2C_DEVICE);
	bdid.fd = open(bdid.devname, O_RDWR);
	if (bdid.fd == -1)
	{
		printf("Unable to open %s (errno=%d)\n", bdid.devname, errno);
		return -1;
	}

	r=devctl(bdid.fd, DCMD_I2C_LOCK, NULL, 0, NULL);
	if (r)
	{
		printf("Unable to lock %s (errno=%d)\n", bdid.devname, errno);
		close(bdid.fd);
		return -1;
	}

	bdid.i2c_address = address;
	r = bdid_read(&bdid, bdident, showerr);
	if (r<0)
	{
		if (showerr) printf("Unable to read %s (errno=%d)\n", bdid.devname, errno);
		close(bdid.fd);
		return -1;
	}

	r=devctl(bdid.fd, DCMD_I2C_UNLOCK, NULL, 0, NULL);
	if (r)
	{
		printf("Unable to unlock %s (errno=%d)\n", bdid.devname, errno);
		close(bdid.fd);
		return -1;
	}

	close(bdid.fd);

	return 0;
}

// Reference: Beaglebone Manual (http://beagleboard.org/static/BONESRM_latest.pdf)
//   Ch 7.11  EEPROM               - beaglebone main board
//   Ch 8.1.3 EEPROM Data Format   - beaglebone cape
void dump(BDIDENT *bdident)
{
    printf("header:  %x\n", bdident->header);
    printf("name:    %s\n", bdident->bdname);
    printf("version: %s\n", bdident->version);
    printf("serial:  %s\n", bdident->serial);
    if (bdident->config[0])
    	printf("config:  %s\n"  , bdident->config);
    if ((bdident->macaddr[0][0])&&!((bdident->macaddr[0][0]==0xFF)&&
    		(bdident->macaddr[0][1]==0xFF)&&(bdident->macaddr[0][2]==0xFF)&&
    		(bdident->macaddr[0][3]==0xFF)&&(bdident->macaddr[0][4]==0xFF)&&
    		(bdident->macaddr[0][5]==0xFF)))
    	printf("mac1:    %x.%x.%x.%x.%x.%x\n", bdident->macaddr[0][0],
    			bdident->macaddr[0][1], bdident->macaddr[0][2],
    			bdident->macaddr[0][3], bdident->macaddr[0][4],
    			bdident->macaddr[0][5]);
    if ((bdident->macaddr[1][0])&&!((bdident->macaddr[1][0]==0xFF)&&
    		(bdident->macaddr[1][1]==0xFF)&&(bdident->macaddr[1][2]==0xFF)&&
    		(bdident->macaddr[1][3]==0xFF)&&(bdident->macaddr[1][4]==0xFF)&&
    		(bdident->macaddr[1][5]==0xFF)))
    	printf("mac2:    %x.%x.%x.%x.%x.%x\n", bdident->macaddr[1][0],
    			bdident->macaddr[1][1], bdident->macaddr[1][2],
    			bdident->macaddr[1][3], bdident->macaddr[1][4],
    			bdident->macaddr[1][5]);
    if ((bdident->macaddr[2][0])&&!((bdident->macaddr[2][0]==0xFF)&&
    		(bdident->macaddr[2][1]==0xFF)&&(bdident->macaddr[2][2]==0xFF)&&
    		(bdident->macaddr[2][3]==0xFF)&&(bdident->macaddr[2][4]==0xFF)&&
    		(bdident->macaddr[2][5]==0xFF)))
   		printf("mac3:    %x.%x.%x.%x.%x.%x\n", bdident->macaddr[2][0],
   				bdident->macaddr[2][1], bdident->macaddr[2][2],
   				bdident->macaddr[2][3], bdident->macaddr[2][4],
   				bdident->macaddr[2][5]);
}

int main(int argc, char *argv[])
{
	int     i, r;
	BDIDENT bd;

	printf("Board ID\n");

	// Read Board ID. Every beaglebone and cape has an EEPROM with ID information
	// that can be read over I2C0. Capes may be plugged in or not, by the main
	// board's EEPROM must be there, so print errors if there are any (parameter 3).
	r = read_boardid(0x50, &bd, 1 /* print errors */);
	if (!r) dump(&bd);

	for (i=0; i<AM335X_I2C0_MAXCAPES; i++)
	{
		// Read optional cape ID. Don't print errors, likely caused by cape absence.
		r = read_boardid(AM335X_I2C0_CAPE0+i, &bd, 0 /* don't print errors */);
		if (!r) dump(&bd);
	}

	return 0;
}

