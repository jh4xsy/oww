/* convert.h */

/* Temperature, mm and speed conversion */

#ifndef CONVERT_H
#define CONVERT_H 1

float convert_temp(float c, int f) ;
float convert_speed(float s, int m) ;
float convert_barom(float b, int inchesHg) ;
float convert_mm(float r, int mm) ;
char *convert_mm_string(float r, int mm, int comma) ;
float convert_unit(float val, int unit) ;
char *convert_unit_short_name(int unit) ;

#define CONVERT_NO_COMMA   0
#define CONVERT_WITH_COMMA 1

#define CONVERT_MILLIBAR  0
#define CONVERT_INCHES_HG 1
#define CONVERT_KPA       2
#define CONVERT_ATM       3

#define CONVERT_FAHRENHEIT 1
#define CONVERT_CELSIUS    0

#define CONVERT_KPH  0
#define CONVERT_MPH  1
#define CONVERT_MPS  2
#define CONVERT_KNOT 3

#define CONVERT_MM     1
#define CONVERT_INCHES 0

enum convert_units {
  convert_unit_none = 0,
  convert_unit_c,
  convert_unit_f,
  convert_unit_kph,
  convert_unit_mph,
  convert_unit_mps,
  convert_unit_knot,
  convert_unit_inches,
  convert_unit_mm,
  convert_unit_millibar,
  convert_unit_inches_hg,
  convert_unit_kpa,
  convert_unit_atm
} ;

#endif
