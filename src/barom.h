/* barom.h */

/* Oww project
   Dr. Simon J. Melhuish
*/

#ifndef BAROM_H
#define BAROM_H 1

/* Constants for barometer claibration */

/*
   This is for the default calibration of
   950 mB to 1050 mB (100-mB range)
   1.25 V to 8.25 V  (7-V range)

   Slope:  100 mB / 7 V = 14.2857 mB/V
   Offset: 950 mB - 1.25 V * 14.2857 mB/V = 932.1429 mB
*/

#define Barom_slope  14.2857F
#define Barom_offset 932.1429F

#endif
