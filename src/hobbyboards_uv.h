/*
 * hobbyboards_uv.h
 *
 *  Created on: 24 Mar 2012
 *      Author: sjm
 */

#ifndef HOBBYBOARDS_UV_H_
#define HOBBYBOARDS_UV_H_

// Read the UVI sensor version
int hbuvi_read_version(int uvnum, int *major, int *minor);

// Read the type of the device - this will be 0x01
int hbuvi_read_type(int uvnum, int *type);

// Read the device temperature
int hbuvi_read_T(int uvnum, float *T);

//// Set temperature offset
//void hbuvi_set_T_offset(int uvnum, float offset);
//
//// Read temperature offset
//float hbuvi_read_T_offset(int uvnum);

// Read UVI value
int hbuvi_read_UVI(int uvnum, float *uvi);

//// Set offst on UVI values
//void hbuvi_set_UVI_offset(int uvnum, float offset);
//
//// Read offset on UVI values
//float hbuvi_read_UVI_offset(int uvnum);
//
//// Set whether or not a case is used
//void hbuvi_set_in_case(int uvnum, int in_case);
//
//// Read whether or not a case is used
//int hbuvi_read_in_case(int uvnum);



#endif /* HOBBYBOARDS_UV_H_ */
