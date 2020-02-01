/*
 * hobbyboards_uv.c
 *
 *  Created on: 24 Mar 2012
 *      Author: sjm
 */


// Include Files
#include <stdio.h>
#include <math.h>
#include "werr.h"
//#include "mlan.h"
#include "ownet.h"
#include "applctn.h"
#include "devices.h"
#include "setup.h"
#include "intl.h"
#include "werr.h"
#include "hobbyboards_uv.h"

#define COMM_READVER 0x11
#define COMM_READTYP 0x12
#define COMM_READT   0x21
#define COMM_SETTO   0x22
#define COMM_READTO  0x23
#define COMM_READUVI 0x24
#define COMM_SETUVO  0x25
#define COMM_READUVO 0x26
#define COMM_SETINC  0x27
#define COMM_READINC 0x28

// Read the UVI sensor version
int hbuvi_read_version(int device, int *major, int *minor) {
  uchar block[3];

  if (devices_access(device))
  {
	block[0] = COMM_READVER;
	block[1] = block[2] = 0xFF;

	if(owBlock(devices_portnum(device), FALSE, block, 3))
	{
	  if (block[1] == 0xFF) return -1;

	  *major = block[2];
	  *minor = block[1];

	  return 0; // Ok
	}
  }

  werr(
    WERR_DEBUG0,
    _("Failed to read %s version"),
    devices_list[device].menu_entry) ;

  return -1; // Failed
}

// Read the type of the device - this will be 0x01
int hbuvi_read_type(int device, int *type) {
  uchar block[2];

  if (devices_access(device))
  {
	block[0] = COMM_READTYP;
	block[1] = 0xFF;

	if(owBlock(devices_portnum(device), FALSE, block, 2))
	{
	  if (block[1] == 0xFF) return -1;

	  *type = block[1];

	  return 0; // Ok
	}
  }

  werr(
    WERR_DEBUG0,
    _("Failed to read %s type"),
    devices_list[device].menu_entry) ;

  return -1; // Failed

}

// Read the device temperature - returned as a flot in C, resolution 0.5C
int hbuvi_read_T(int device, float *T) {
  uchar block[3];

  if (devices_access(device))
  {
	block[0] = COMM_READT;
	block[1] = block[2] = 0xFF;

	if(owBlock(devices_portnum(device), FALSE, block, 3))
	{
	  int16_t hdt = block[1] | block[2]<<8;

	  *T = hdt * 0.5F;

	  return 0; // Ok
	}
  }

  werr(
    WERR_DEBUG0,
    _("Failed to read %s temperature"),
    devices_list[device].menu_entry) ;

  return -1; // Failed
}

//// Set temperature offset
//void hbuvi_set_T_offset(int device, float offset) {
//
//}
//
//// Read temperature offset
//float hbuvi_read_T_offset(int device) {
//  return 0.0F;
//}

// Read UVI value - returns UV index, resultion 0.1
int hbuvi_read_UVI(int device, float *uvi) {
  uchar block[2];

  if (!devices_access(device)) {
	printf("UV access failed\n");
    return -1;
  }

  block[0] = COMM_READUVI;
  block[1] = 0xFF;

  if(owBlock(devices_portnum(device), FALSE, block, 2))
  {
    if (block[1] == 0xFF) return -1;

    *uvi = block[1]*0.1F;

//    printf("UV = %f\n", *uvi);

    return 0; // Ok
  }

  printf("UV block failed\n");

  return -1; // Failed
}

//// Set offst on UVI values
//void hbuvi_set_UVI_offset(int device, float offset) {
//
//}
//
//// Read offset on UVI values
//float hbuvi_read_UVI_offset(int device) {
//  return 0.0F;
//}
//
//// Set whether or not a case is used
//void hbuvi_set_in_case(int device, int in_case) {
//
//}
//
//// Read whether or not a case is used
//int hbuvi_read_in_case(int device) {
//  return FALSE;
//}

