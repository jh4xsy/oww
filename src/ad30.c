/*
*  C Implementation: ad30
*
* Description: 
*
*
* Author: Simon Melhuish <simon@melhuish.info>, (C) 2007
*
* Copyright: See COPYING file that comes with this distribution
*
*/

/* Read DS2760 (family code 0x30) ADC */

#include <stdio.h>
#include <math.h>
#include "werr.h"
//#include "mlan.h"
#include "ownet.h"
#include "applctn.h"
#include "devices.h"
#include "ad30.h"
#include "setup.h"
#include "intl.h"

/*
* Read a block from the DS2670
*
*/
int ad30_read(
  int adcDevice,
  adc_struct *result
)
{
  uchar block[8];
  int i;
  
  if (!devices_access(adcDevice))
    return -1;

  block[0] = 0x69; /* Read Data */
  block[1] = 0x0c; /* Start of voltage/current data */
  for (i=2; i<8; block[i++]=0xff);

  if(owBlock(devices_portnum(adcDevice), FALSE, block, 8))
  {
    int value;

    value = (block[2] << 3) | (block[3] >> 5);
    if (value & 0x400) // Check sign bit
      value |= ~0x7ff; // Set all bits above bit 10
    result->V = 4.88F * (float) value; // Convert to mV

    value = (block[4] << 5) | (block[5] >> 3);
    if (value & 0x1000) // Check sign bit
      value |= ~0x1fff; // Set all bits above bit 12
    result->I = 15.625e-3F * (float) value; // Convert to mV

    value = (block[6] << 8) | block[7];
    if (value & 0x8000) // Check sign bit
      value |= ~0xffff ; // Set all bits above bit 15
    result->Q = 22.5F * (float) value; // Resulting unit is mV.s

    if (!devices_access(adcDevice))
      return -1;

    block[0] = 0x69; /* Read Data */
    block[1] = 0x18; /* Start of temperature data */
    for (i=2; i<4; block[i++]=0xff);

    if (owBlock(devices_portnum(adcDevice), FALSE, block, 4))
    {
      value = (block[2] << 3) | (block[3] >> 5);
      if (value & 0x400) // Check sign bit
        value |= ~0x7ff; // Set all bits above bit 10
      result->T = 0.125F * (float) value;
      
      return 0; /* Ok */
    }
      
    return 0; /* Ok */
  }

  return -1 ; /* Failed */
}

