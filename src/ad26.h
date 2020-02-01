#ifndef HUMIDITY_H
# define HUMIDITY_H 1
  extern int Volt_AD(int vdd, /*uchar **/ int adc);
  extern int ad26_current_reading(int adc, int depth, float *result) ;
  extern float Volt_Reading(int vdd, /*uchar **/ int adc, int depth);
  extern int ad26_start_tempconv(/*uchar **/ int adc) ;
  extern int Get_Temperature(/*uchar **/ int adc, float *T);
#endif
