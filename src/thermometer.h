//
// C++ Interface: thermometer
//
// Description: 
//
//
// Author: Simon Melhuish <simon@melhuish.info>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef THERMOMETER_H
#define THERMOMETER_H 1

#include "devices.h"

typedef struct {
  int devtype ;          /* What kind of device is this?
                            Should match type in devices_struct */
  int status ;
  int (*update)(devices_struct *dev) ;
  int state ; // Thermometer state
  int power_state ;
  float T ;
} thermometer ;

enum thermometer_states {
  thermometer_init = 0,
  thermometer_conv,
  thermometer_strong,
  thermometer_done
} ;

enum thermometer_power_states {
  thermometer_power_unknown = 0,
  thermometer_power_bus,
  thermometer_power_vdd
} ;

void thermometer_new(devices_struct *dev) ;
int thermometer_probe(void) ;

#endif
