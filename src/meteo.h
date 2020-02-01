/* meteo.h */

/*
   Repository for meteorological functions
*/

#ifndef METEO_H
#  define METEO_H 1

#define METEO_WINDCHILL_STEADMAN  1
#define METEO_WINDCHILL_NWS_1992B 2
#define METEO_WINDCHILL_NEW_WCT   3

#define METEO_HI_HEATIND 1
#define METEO_HI_HUMIDEX 2
#define METEO_HI_HEATIND16 3

  /* meteo_wind_chill

     Wind chill temperature (deg C), given actual temperature (deg C)
     and wind speed (mph)
  */

  float
  meteo_wind_chill(float T, float wsp_mph, int formula) ;

  /* takes temp in C and relative humidity (0<rh<=100)
   returns dewpoint temp in C */

  double
  meteo_dew_point(double Tc, double rh) ;

/* 16-term heat index or Canadian Humidex */
double meteo_hi(double T, double H, int formula) ;

#endif
