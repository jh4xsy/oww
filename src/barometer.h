//
// C++ Interface: barometer
//
// Description: 
//
//
// Author: Simon Melhuish <simon@melhuish.info>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef BAROMETER_H
#define BAROMETER_H 1

#include <stdlib.h>
#include "devices.h"

typedef struct {
  int devtype ;          /* What kind of device is this?
                            Should match type in devices_struct */
  int status ;
  int (*update)(devices_struct *dev) ;
  int state ; // Barometer state
  int powered ;
  float T ;
} barometer ;

enum barometer_states {
  barometer_init = 0
} ;

void barometer_new(devices_struct *dev) ;

#endif
