/*
 * hobbyboards_uv.h
 *
 *  Created on: 19 May 2012
 *      Author: sjm
 */

#ifndef HOBBYBOARDS_MOIST_H_
#define HOBBYBOARDS_MOIST_H_

typedef struct {
  int sensor[4] ; /* The sensor data read from a moisture board */
  int type[4]   ; /* The type for each sensor channel: 0 => moisture, 1 => leaf wetness */
//  int soilmap[4] ; /* Map soil moisture channels to sensors */
//  int leafmap[4] ; /* Map leaf wetness channels to sensors */
} moist_struct ;

#define HBMOIST_SOIL 0
#define HBMOIST_LEAF 1

// Read the UVI sensor version
int hbmoist_read_version(int uvnum, int *major, int *minor);

// Read the type of the device - this will be 0x01
int hbmoist_read_type(int uvnum, int *type);

// Read sensor data value
int hbmoist_read_sensors(int moistnum, moist_struct *data);

int hbmoist_set_assignments(int device, int *types);

int hbmoist_get_assignments(int device, int *types);

int hbmoist_parser_special(int device);

#endif /* HOBBYBOARDS_MOIST_H_ */
