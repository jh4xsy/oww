/*
 * wsi603a.c
 *
 *  Created on: 17-Jun-2009
 *      Author: sjm
 */

/* Read DS2760 (family code 0x30) ADC */

#include <stdio.h>
#include <math.h>
#include "werr.h"
//#include "mlan.h"
#include "ownet.h"
#include "applctn.h"
#include "devices.h"
#include "wsi603a.h"
#include "setup.h"
#include "intl.h"

#define WSI603A_TRIES 4
#define CRAZYSPEED 250.0F

int get_crc(uchar *block, int length)
{
  int i, sum=0;
  for (i=0; i<length; ++i)
	sum += block[i];

  sum &= 0xff;

  return sum;
}

int crc_ok(uchar *block, int length)
{
  return ((get_crc(block,length)&0xff) == block[length]);
}

/*
* Read a block from the DS2670
*
*/
int wsi603a_read_once(int device,  wsi603a_struct *result)
{
  uchar block[9];
//  uchar block2[9];
  int i;

  if (!devices_access(device))
    return -1;

  block[0] = 0x69; /* Read Data */
  block[1] = 0x88; /* Start of SRAM */
  for (i=2; i<=8; block[i++]=0xff);

  if(owBlock(devices_portnum(device), FALSE, block, 9))
  {
    int value, value2, diff;

    // Check the crc
    if (!crc_ok(&block[2], 6))
      return -1;

    result->windSpeed = 1.6294F * (float) block[3]; // Result in mph - Ignore cal for now

    if (result->windSpeed >= CRAZYSPEED)
      return -1;

    result->windPoint = (int) block[4]; // Result is 1 - 16

    if ((block[4] & 0xf0) != 0) return -1;

    result->intensity = (float) block[6];

//    printf("sp = %f, pt = %d, int = %f", result->windSpeed, result->windPoint, result->intensity);

    if (!devices_access(device))
      return -1;

    block[0] = /*block2[0] =*/ 0x69; /* Read Data */
    block[1] = /*block2[1] =*/ 0x18; /* Start of temperature data */
    for (i=2; i<4; ++i)
      block[i]=/*block2[i]=*/0xff;

    if (owBlock(devices_portnum(device), FALSE, block, 4))
    {
      if ((block[3] & 0x01F) != 0)
      {
    	werr(WERR_DEBUG0, _("Bad T data read from %s"), devices_list[device].menu_entry);
    	return -1; /* Bad data from T read - failed */
      }

//      if (!devices_access(device))
//        return -1;

  	value = (block[2] << 3) | (block[3] >> 5);
  	if (value & 0x400) // Check sign bit
  	  value |= ~0x7ff; // Set all bits above bit 10
	result->T = 0.125F * (float) value;

	return 0; // Ok

//  	// Read again to check
//      if (owBlock(devices_portnum(device), FALSE, block2, 4))
//      {
////    	if ((block2[3] & 0x01F) != 0)
////        {
////      	  printf("Bad data on 2nd read\n");
////      	  if ((block[3] & 0x01F) != 0)
////      		printf("!!! Bad data on 1st and 2nd reads !!!\n");
//////    	  return -1; /* Bad data on 2nd read - failed */
////        }
//
////    	if ((block[3] != block2[3]) || (block[2] != block2[2]))
////        {
////      	  printf("Data not repeatable - failed - %02x%02x %02x%02x\n", block[2], block[3], block2[2], block2[3]);
////    	  return -1; /* Data not repeatable - failed */
////        }
//
//    	value = (block[2] << 3) | (block[3] >> 5);
//    	if (value & 0x400) // Check sign bit
//    	  value |= ~0x7ff; // Set all bits above bit 10
////    	result->T = 0.125F * (float) value;
//
//    	value2 = (block2[2] << 3) | (block2[3] >> 5);
//    	if (value2 & 0x400) // Check sign bit
//    	  value2 |= ~0x7ff; // Set all bits above bit 10
//
//    	diff = value - value2;
//
//    	// We insist on T repeating to within 2x LSB (0.25 deg)
//
//    	if ((diff>2) || (diff<-2)) {
////      	  printf("Data not repeatable - failed - %02x%02x %02x%02x\n", block[2], block[3], block2[2], block2[3]);
//    	  return -1; /* Data not repeatable - failed */
//    	}
//    	result->T = 0.125F * (float) value;
//
//    	//      printf(", T = %f\n", result->T);
////    	if (result->T <= 0.0)
////    	  printf("T = %f\n", result->T);
//
//    	return 0; /* Ok */
//      }
    }
  }

  return -1 ; /* Failed */
}

int wsi603a_read(int device,  wsi603a_struct *result)
{
  int i;

  for (i=0; i<WSI603A_TRIES; ++i)
  {
	if (0==wsi603a_read_once(device, result))
	  return 0;
	msDelay(50);
  }

  werr(WERR_DEBUG0, "Too many retries on WSI603A");
  return -1;
}

int wsi603a_set_led_mode(int device, int mode, int status, int level, int threshold)
{
  uchar block[9];

  if (!devices_access(device))
	return -1;

  block[0] = 0x6c; /* Write Data */
  block[1] = 0x80; /* Start of SRAM */
  block[2] = 0xA2; /* LED control */
  block[3] = 0xFF & mode;
  block[4] = 0xFF & status;
  block[5] = 0xFF & level;
  block[6] = 0xFF & threshold;
  block[7] = 0xFF & get_crc(&block[2],5);
  block[8] = 0x2E; /* End byte */

  return (owBlock(devices_portnum(device), FALSE, block, 9))?0:-1;
}

int wsi603a_special_handler(int device)
{
  int mode=0, status=0, threshold=0, level=0;

  printf("Enter mode:");
  scanf("%d", &mode);

  if (mode == 0)
  {
	printf("Enter status:");
	scanf("%d", &status);
  }
  else
  {
	printf("Enter level:");
	scanf("%d", &level);
	printf("Enter threshold:");
	scanf("%d", &threshold);
  }

  return wsi603a_set_led_mode(device, mode, status, threshold, level);
}
