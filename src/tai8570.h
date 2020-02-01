/* tai8570.h */

// Headers for tai8570 barometer functions
// For the Oww project
// Simon Melhuish, derived from code by AAG
// 2003

int tai8570_read_cal(int device_r, int device_w, int cal[]) ;
int tai8570_read_vals(int device_r, int device_w, int cal[], float *Pressure, float *Temperature) ;
int tai8570_check_alloc(void) ;
