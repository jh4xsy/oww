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
#define COMM_SENSDAT 0x21
#define COMM_SETLEAF 0x22
#define COMM_GETLEAF 0x23

// Read the moisture sensor version
int hbmoist_read_version(int device, int *major, int *minor) {
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

// Read the type of the device - this will be 0x02
int hbmoist_read_type(int device, int *type) {
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

// Read moisture sensor channels
int hbmoist_read_sensors(int device, moist_struct *data) {
  uchar block[5] = {COMM_SENSDAT, 0xFF, 0xFF, 0xFF, 0xFF};

  if (devices_access(device)) {
	//  block[0] = COMM_SENSDAT;
	//  block[1] = block[2] = block[3] = block[4] = 0xFF;

	  if(owBlock(devices_portnum(device), FALSE, block, 5))
	  {
		int i;

		for (i=0; i<4; ++i)
		{
		  if (block[1+i] == 0xFF) return -1;
		  data->sensor[i] = block[1+i];
	//	  printf("Moist ch. %d = %d\n", i, data->sensor[i]);
		}

		hbmoist_get_assignments(device, data->type);

	    return 0; // Ok
	  }
  }


  werr(
    WERR_DEBUG0,
    _("Failed to read %s"),
    devices_list[device].menu_entry) ;

  return -1; // Failed
}

// Get channel assignments
int hbmoist_get_assignments(int device, int *types) {
  uchar block[2] = {COMM_GETLEAF, 0xFF};

  if (devices_access(device)) {
	  if(owBlock(devices_portnum(device), FALSE, block, 2))
	  {
		int i;
	//	int mask = 1;
	//	printf("block[1] = %d\n", block[1]);

		for (i=0; i<4; ++i)
		{
		  types[i] = 1 & (block[1] >> i);
	//	  printf("hbmoist_get_assignments chan %d is a %s sensor\n", i+1, (types[i]==HBMOIST_LEAF)?"leaf":"soil");
		}

	    return 0; // Ok
	  }
  }


  werr(
    WERR_DEBUG0,
    _("Failed to read %s assignments"),
    devices_list[device].menu_entry) ;


  return -1; // Failed
}

// Get channel assignments
int hbmoist_set_assignments(int device, int *types) {
  uchar block[2] ;
  int mask=1;
  int bits=0;
  int i;

  if (devices_access(device)) {
	  for (i=0; i<4; ++i) {
		bits |= (types[i]==HBMOIST_LEAF)?mask:0;
		mask *= 2;
	  }

	  block[0] = COMM_SETLEAF;
	  block[1] = bits ;

	  if(owBlock(devices_portnum(device), FALSE, block, 2))
	  {
//		printf("Moisture assingment %d\n", bits);
	    return 0; // Ok
	  }
  }


  werr(
    WERR_DEBUG0,
    _("Failed to set %s assignments"),
    devices_list[device].menu_entry) ;

  return -1; // Failed
}

/**
 * Handler for parser special command
 */
int hbmoist_parser_special(int device)
{
  int chan, type;
  int types[4];
  char t[12];
  char c;

  printf("Set type of which channel [1-4]?\n");
  scanf("%d", &chan);

  if ((chan<1)||(chan>4)) {
	printf("The channel number must be from 1 to 4\n");
	return -1;
  }

  printf("Choose the type for channel %d [m or l]:\n");
  scanf("%11s", t);
  c = toupper(t[0]);

  switch (c) {
  case 'M':
  case 'S':
	type = HBMOIST_SOIL;
	break;

  case 'L':
  case 'W':
	type = HBMOIST_LEAF;
	break;
  }

  hbmoist_get_assignments(device, types);
  types[chan-1] = type;
  hbmoist_set_assignments(device, types);

  return 0;
}


