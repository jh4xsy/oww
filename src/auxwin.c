/* auxwin.c */

/* Dr. Simon J. Melhuish
   Oww project

   Maintainance of auxillary readout window

   GUI functions provided by auxwin_ro / auxwin_un
*/

#include <stdio.h>
#include "wstypes.h"
#include "auxwin.h"
#include "auxwin_t.h"
#include "setup.h"
#include "devices.h"
#include "convert.h"
#include "meteo.h"
#include "weather.h"
#include "globaldef.h"
#include "intl.h"

extern int
auxwin_g_start(void) ;

extern int
auxwin_g_end(void) ;

/*extern int
auxwin_g_print(char* format, ...) ;*/

int
auxwin_g_write(char *name, float val, int prec, char *unit) ;

int
auxwin_init(void)
{
  return 0 ;
}

static char *
indexed(char *stem, int i)
{
  static char buffer[20] ;
  sprintf(buffer, "%s%d", stem, i+1) ;
  return buffer ;
}

int
auxwin_update(wsstruct *wd)
{
  int i ;

  if (0 != auxwin_g_start()) return -1 ;

  /* Update all valid fields */

  /* Wind vane */

  if (devices_have_vane())
  {
    float primary_T ;

    auxwin_g_write(
      _("Wind Speed"),
      convert_speed(wd->anem_speed, setup_unit_speed),
      1,
      //(setup_mph) ? _("mph") : _("km/h")) ;
      convert_unit_short_name(convert_unit_kph + setup_unit_speed)) ;
    if (setup_gust_time != 0)
    {
      auxwin_g_write(
        _("Wind Gust"),
        convert_speed(wd->anem_int_gust, setup_unit_speed),
        1,
        //(setup_mph) ? _("mph") : _("km/h")) ;
        convert_unit_short_name(convert_unit_kph + setup_unit_speed)) ;
    }
    auxwin_g_write(
      _("Wind Dir."),
      22.5F * (float) (wd->vane_bearing-1), 1,  DEGSYMB) ;

    primary_T = weather_primary_T(NULL) ;
    if (primary_T != UNDEFINED_T)
    {
      int rh ;

      auxwin_g_write(
        _("Wind Chill"),
        convert_temp(
          meteo_wind_chill(primary_T, wd->anem_speed, setup_wctype), setup_f),
        1,
        (setup_f) ? "F" : "C") ;

      /* We need RH for heat index */
      rh = weather_primary_rh(NULL) ;
      if (-1 != rh)
      {
        auxwin_g_write(
          _("Heat Index"),
          convert_temp((float)
            meteo_hi((double) primary_T,
                     (double) wd->RH[rh],
                      setup_hitype),
            setup_f),
          1,
          (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
      }
    }
  }

  /* Rain gauge */

  if (devices_have_rain())
  {
    auxwin_g_write(
      _("Rain"),
      convert_mm(wd->rain, setup_mm),
      (setup_mm) ? 1 : 2,
      (setup_mm) ? "mm" : "\"") ;
    auxwin_g_write(
      _("Rain Rate"),
      convert_mm(wd->rain_rate, setup_mm),
      (setup_mm) ? 1 : 2,
      (setup_mm) ? _("mm/hour") : _("\"/hour")) ;
  }

  /* Thermometers */
  for (i=0; i<MAXTEMPS; ++i)
  {
    if (!devices_have_temperature(i)) continue ;

    auxwin_g_write(
      indexed("T ", i),
      convert_temp(wd->T[i], setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
  }

  /* Soil Thermometers */
  for (i=0; i<MAXSOILTEMPS; ++i)
  {
    if (!devices_have_soil_temperature(i)) continue ;

    auxwin_g_write(
      indexed("Tsoil ", i),
      convert_temp(wd->soilT[i], setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
  }

  /* Indoor Thermometers */
  for (i=0; i<MAXINDOORTEMPS; ++i)
  {
    if (!devices_have_indoor_temperature(i)) continue ;

    auxwin_g_write(
      indexed("Tindoor ", i),
      convert_temp(wd->indoorT[i], setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
  }

  /* RH sensors */
  for (i=0; i<MAXHUMS; ++i)
  {
    if (!devices_have_hum(i)) continue ;

    auxwin_g_write(
      indexed(_("RH "), i),
      wd->RH[i], 0, "%") ;

    auxwin_g_write(
      indexed(_("T rh "), i),
      convert_temp(wd->Trh[i], setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;

    auxwin_g_write(
      indexed(_("T dew "), i),
      convert_temp(
        meteo_dew_point(wd->Trh[i], wd->RH[i]), setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
  }


  /* Barometers */
  for (i=0; i<MAXBAROM; ++i)
  {
    int bp_prec[] = {1, 3, 2, 4} ;

    if (!devices_have_barom(i)) continue ;

    auxwin_g_write(
      indexed(_("Barom "), i),
      convert_barom(wd->barom[i], setup_unit_bp),
        bp_prec[setup_unit_bp],
        convert_unit_short_name(convert_unit_millibar + setup_unit_bp)) ;
      //(setup_hg) ? 2 : 0,
      //(setup_hg) ? "\"Hg" : "mB") ;

    auxwin_g_write(
      indexed(_("T b "), i),
      convert_temp(wd->Tb[i], setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
  }

  /* GPCs */
  for (i=0; i<MAXGPC; ++i)
  {
    if (!devices_have_gpc(i)) continue ;

    auxwin_g_write(
      indexed(_("GPC "), i),
      wd->gpc[i].gpc,
      -1,
      "") ;
  }

  /* Solar sensors */
  for (i=0; i<MAXSOL; ++i)
  {
    if (!devices_have_solar_data(i)) continue ;

    auxwin_g_write(
      indexed(_("Solar "), i),
      wd->solar[i],
      0,
      WpSqM) ;
  }

  /* UV sensors */
  for (i=0; i<MAXUV; ++i)
  {
    if (!devices_have_uv_data(i)) continue ;

    auxwin_g_write(
      indexed(_("UVI "), i),
      wd->uvi[i],
      1,
      "") ;

//    auxwin_g_write(
//      indexed(_("T UV "), i),
//      convert_temp(wd->uviT[i], setup_f),
//      1,
//      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
  }

  /* ADC sensors */
  for (i=0; i<MAXADC; ++i)
  {
    if (!devices_have_adc(i)) continue ;

    auxwin_g_write(
      indexed(_("ADC Vdd "), i),
      wd->adc[i].V*0.001,
      3,
      "V") ;

    auxwin_g_write(
      indexed(_("ADC Vsns "), i),
      wd->adc[i].I,
      2,
      "mV") ;

    auxwin_g_write(
      indexed(_("ADC Acc "), i),
      wd->adc[i].Q,
      1,
      "mV.s") ;

    auxwin_g_write(
      indexed(_("T adc "), i),
      convert_temp(wd->adc[i].T, setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
  }

  /* Thermocouples */
  for (i=0; i<MAXTC; ++i)
  {
    if (!devices_have_thrmcpl(i)) continue ;

    auxwin_g_write(
      indexed(_("Ttc "), i),
      convert_temp(wd->thrmcpl[i].T, setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;

    auxwin_g_write(
      indexed(_("Tcj "), i),
      convert_temp(wd->thrmcpl[i].Tcj, setup_f),
      1,
      (setup_f) ? DEGSYMB "F" : DEGSYMB "C") ;
  }

  /* Soil moistures */
  for (i=0; i<MAXMOIST*4; ++i)
  {
    int sm_prec[] = {0, 2, 1, 3} ;

    if (!devices_have_soil_moist(i)) continue ;

    auxwin_g_write(
      indexed(_("Moist "), i),
      convert_barom(10.0F*wd->soil_moist[i], setup_unit_bp),
        sm_prec[setup_unit_bp],
        convert_unit_short_name(convert_unit_millibar + setup_unit_bp)) ;
  }

  /* Leaf wetness values */
  for (i=0; i<MAXMOIST*4; ++i)
  {
    if (!devices_have_leaf_wet(i)) continue ;

    auxwin_g_write(
      indexed(_("Leaf "), i),
      wd->leaf_wet[i],
      0, "%") ;
  }

  auxwin_g_end() ;

  return 0 ;
}

int
auxwin_new_datasource(int ds)
{
  return 0 ;
}
