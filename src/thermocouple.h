/*
 * thermocouple.h
 *
 *  Created on: 6 Apr 2012
 *      Author: sjm
 */

#ifndef THERMOCOUPLE_H_
#define THERMOCOUPLE_H_

typedef struct {
  float T ; /* Compensated hot junction temperature */
  float Tcj ; /* Measured cold junction temperature */
  float V ; /* EMF */
  int type ; /* Junction type */
} tc_struct ;

typedef enum {
  tc_type_B = 0,
  tc_type_E,
  tc_type_J,
  tc_type_K,
  tc_type_N,
  tc_type_R,
  tc_type_S,
  tc_type_T
} tc_types ;

int getThermocoupleT(tc_struct *tc);

#endif /* THERMOCOUPLE_H_ */
