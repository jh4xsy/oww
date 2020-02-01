//
// C++ Interface: ad30
//
// Description: 
//
//
// Author: Simon Melhuish <simon@melhuish.info>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef AD30_H
#define AD30_H 1

typedef struct {
  float V ; /* Voltage (mV) */
  float I ; /* Current (mA) */
  float Q ; /* Accumulated charge (Coulomb) */
  float T ; /* Temperature (degrees C) */
} adc_struct ;

int ad30_read(
  int adcDevice,
  adc_struct *result
);
#endif /* AD30_H */
