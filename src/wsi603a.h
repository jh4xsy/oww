/*
 * wsi603a.h
 *
 *  Created on: 17-Jun-2009
 *      Author: sjm
 */

#ifndef WSI603A_H_
#define WSI603A_H_

typedef struct {
  float 	windSpeed ; /* Wind speed */
  int		windPoint ; /* Wind point number */
//  float 	windBearing ; /* Wind bearing (degrees) */
  int 		lightStat ; /* LED status */
  float 	intensity ; /* Reading from light sensor */
  float 	T ; /* Temperature (degrees C) */
} wsi603a_struct ;

int wsi603a_read(
  int device,
  wsi603a_struct *result
);

int wsi603a_special_handler(int device);

#endif /* WSI603A_H_ */
