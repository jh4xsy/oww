/* meteo.c */

/*
   Repository for meteorological functions
*/

#include <math.h>
#include "werr.h"
#include "meteo.h"

/* meteo_wind_chill

   Wind chill temperature (deg C), given actual temperature (deg C)
   and wind speed (mph)
*/

float
meteo_wind_chill(float T, float wsp_mph, int formula)
{
  static float T_cached=-200.0F, wsp_cached=-200.0F, wc ;

  if ((T_cached == T) && (wsp_cached == wsp_mph))
    return wc ;

  switch (formula)
  {
    case METEO_WINDCHILL_NWS_1992B:

      /* Old NWS (1992b) formula */

      /* Formula from http://observe.ivv.nasa.gov/nasa/earth/wind_chill/ */
      /* Converted to Celcius */

      /* Formula valid only at 4 mph or higher */
      if (wsp_mph < 4.0F)
      {
        wc = T ; /* Report T for V < 4 mph */
      }
      else
      {
        if (wsp_mph > 40.0F)
        {
          /* Limit wsp_msp to 40 mph */
          wc = (33.0F - 1.57469F * (33.0F - T)) ;
        }
        else
        {
          wc = (33.0F -
                  (0.474677F - 0.020425F * wsp_mph
                   + 0.303107F * (float) sqrt((double) wsp_mph))
                  * (33.0F - T)) ;
        }
      }
      break ;

    case METEO_WINDCHILL_STEADMAN:
    {
      /* Steadman Windchill */

      // From:
      // The Steadman Wind Chill: An Improvement over Present Scales
      // Robert G. Quayle and Robert G. Steadman
      // Weather and Forecasting, 13, 1187-1193.
      // Eqn. (1)

      float V ;
      V = wsp_mph * 0.447040972F ;
      if (V >= 2.0F)
        wc = 1.41F - 1.162F * V + 0.980F * T + 0.0124F * V * V +
             0.0185F * V * T ;
      else
        wc = T ; /* Report T for V < 2 m/s */
      break ;
    }

    case METEO_WINDCHILL_NEW_WCT:
    {
      float V, Vp ;

      V = wsp_mph * 1.6093475F ; /* Wind speed unit is kph */
      if (V > 3.0F)
      {
        Vp = (float) pow((double) V, 0.16) ;
        wc = 13.12F + 0.6215F * T - 11.37F * Vp + 0.3965F * T * Vp ;
      }
      else
      {
        wc = T ; /* Report T for V <= 3 mph */
      }
      break ;
    }

    default:
      wc = T ;
  }

  /* Cache T and wind speed values */
  T_cached = T ;
  wsp_cached = wsp_mph ;

  return wc ;
}

double
meteo_dew_point(double T, double rh)
{
  double alpha, beta, a, b ;

  if (T < 0)
  {
    a = 21.874 ;
    b = 265.5 ;
  }
  else
  {
    a = 17.269 ;
    b = 237.3 ;
  }

  if (rh < 0.1)
      rh = 0.1 ;

  else if (rh >= 100.0)
      return T;

  alpha = a * T / (b + T) + log(rh * 0.01) ;
  beta = a - alpha ;
  if ((beta > 0.00001) || (beta < -0.00001))
    return (b * alpha / beta) ;
  else
    return 0.0 ;
}

/*
This is the 16-term heat index, found at
http://home.hetnet.nl/~vanadovv/Meteo.html
*/

static double meteo_heat_index16(double T, double H)
{
  const double c[16] = {
    -0.429783,
     0.6691767,
     0.0102724,
    -0.000125211,
    -0.0737656,
    -0.000228621,
     0.000000103391,
     0.000868291,
     0.0000146039,
    -0.00000000681059,
     0.00000462416,
    -0.0000000707711,
    -0.000000000156160,
     1.428556,
    -0.00502815,
     0.0000193122
  } ;

  double hi ;

  if (T < 21.11) return T ;
  
  hi = c[0] + H*(c[13]+H*(c[14]+c[15]*H)) +
       + T*(c[1]+H*(c[4]+H*(c[5]+c[6]*H)) + T*(c[2]+H*(c[7]+H*(c[8]+c[9]*H)) +
       + T*(c[3]+H*(c[10]+H*(c[11]+c[12]*H))))) ;
  
  if (hi < T) return T ;
  return hi ;
}

static double meteo_heat_index(double T, double H)
{
  const double c[9] = {
    -8.7846948,
     2.33854898,
    -0.01642498,
     1.61139412,
    -0.01230809,
    -0.146115971,
     0.000725489,
     0.00221173,
    -0.000003582
  } ;
  
  double hi ;
  
  if (T < 21.11) return T ;
    
  hi = c[0] + H*(c[1]+c[2]*H) + T*(c[3] + H*(c[5]+c[6]*H) + T*(c[4] + H*(c[7]+c[8]*H))) ;
  
  if (hi < T) return T ;
  return hi ;
}

static double meteo_humidex(double T, double H)
{
  if (T < 0.0) return T ;
    
  return T + 5.0 * ((6.112 *
    pow(10.0, 7.5 * T / (237.7 + T)) * H / 100.0) - 10.0) / 9.0 ;
}

double meteo_hi(double T, double H, int hi_type)
{
  switch (hi_type)
  {
    case METEO_HI_HEATIND:
      /* Heat Index */
      return meteo_heat_index(T, H) ;

    case METEO_HI_HUMIDEX:
      /* Canadian Humidex */
      return meteo_humidex(T, H) ;
    
    case METEO_HI_HEATIND16:
      /* 16-term Heat Index */
      return meteo_heat_index16(T, H) ;
  }

  return T ;
}
