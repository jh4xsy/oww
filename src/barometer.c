//
// C Implementation: barometer
//
// Description: 
//
//
// Author: Simon Melhuish <simon@melhuish.info>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "wstypes.h"
#include "barometer.h"
#include "devices.h"
#include "werr.h"
#include "ownet.h"
#include "progstate.h"
#include "ad26.h"

#define VOLT_READING_TRIES 6

//extern wsstruct ws;


void
barometer_new(devices_struct *dev)
{
  barometer *B ;
  B = (barometer *) calloc(1, sizeof(barometer)) ;
  if (!B) exit(1) ;
  dev->local = B ;
  B->devtype = devtype_barom ;
}
