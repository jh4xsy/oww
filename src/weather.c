/* weather.c */

/* For Oww project
   This is a (very) hacked version of mweather.c by Dallas Semiconductor
   In fact, it's barely recognizable by now!
*/

/*---------------------------------------------------------------------------
   Copyright (C) 1999 Dallas Semiconductor Corporation, All Rights Reserved.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
   OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Dallas Semiconductor
   shall not be used except as stated in the Dallas Semiconductor
   Branding Policy.
  ---------------------------------------------------------------------------

    MWeather.C - Test application to read the MicroLAN Weather Station.
                 The weather station consists of the following devices
                 on the trunk of the MicroLAN:
                   DS1820 - temperature measurement
                   DS2423 - counter for reading wind speed (page 15)
                   DS2407 - switch to isolate 8 DS2401's for wind
                            direction on channel B.
                 This test uses MLanNetU.c and MLanTrnU.c
                 using the low level Win 32-bit functions in WIN32Lnk.c.

    Version: 1.03

    History: 1.01 -> 1.02  Changed to generic OpenCOM/CloseCOM for easier
                             use with other platforms.
                           Corrected ReadCounter to read the right counter
                             page of DS2423.
                           Changed to accomodate more than 1 DS1820
                             temperature reading.
             1.02 -> 1.03  Removed caps in #includes for Linux capatibility
                           Changed to use msGettick() instead of
                             GetTickCount()
                           Changed reading loop to be infinite in non-win32
                           Changed to use Aquire/Release 1-Wire Net functions
*/

/* This version modified by Dr. S. J. Melhuish for RISC OS !OWW project */

#ifdef WIN32
#include <conio.h>
#endif

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
# ifdef TIME_WITH_SYS_TIME
#  include <time.h>
# endif
#else
# include <time.h>
#endif

#include <math.h>

#include "ownet.h"
#include "werr.h"

#include "wstypes.h"
#include "weather.h"
#include "setup.h"

#include "progstate.h"

#include "rainint.h"
#include "globaldef.h"

#include "devices.h"

#include "arne.h"
#include "oww_trx.h"
#include "ad26.h"
#include "atod20.h"
#include "applctn.h"
#include "stats.h"
#include "config.h"

#include "barom.h"
#include "txtserve.h"
#include "swt1f.h"
#include "tai8570.h"
#include "tai8590.h"
#include "thermometer.h"
#include "intl.h"
#include "wsi603a.h"
#include "hobbyboards_uv.h"
#include "hobbyboards_moist.h"

/* defines */
#define FALSE              0
#define TRUE               1
#define VOLT_READING_TRIES 6
#define SWITCH_TRIES     100
#define ANEM_TRIES        20
#define GPC_TRIES         20
#define RAIN_TRIES        20
#define WEATHER_DBLEV WERR_DEBUG1
#define MAX_DEVS_SEARCH  100

int hygrochron_read(int devnum, float *RH, float *Trh) ;

/* local funcitons */
char *PrintSerialNum (void);
int FindWeatherStation (void);
static int ReadCounter_robust (int device,
                               int CounterPage,
                               uint32_t *Count, int depth);
static int ReadCounter (int, int, uint32_t *);
//int ReadTemperature (uchar *, float *);
int set_switch (int device, int State);
static int set_switch_robust (int device, int State, int depth);
static int ReadWindDirection_ADC (int);
static int ReadWindDirection (int);
static int weather_read_ds2438_thermometer (void) ;
static int weather_read_ws_end_tempconv (void) ;

extern wsstruct ws;

int wv_known = 0, port_opened = 0;

time_t reset_time = 0L;

/* range is 0.06v above each minimum */
static double points_voltage_minimum[]= {
	2.66,
	6.49,
	5.96,
	9.35,
	9.27,
	9.50,
	8.48,
	8.98,
	7.57,
	7.95,
	4.28,
	4.59,
	0.89,
	2.20,
	1.54,
	3.54
};



static int
weather_strong_off (void)
{
  /* turn off the MicroLAN strong pull-up */
  if (owLevel (devices_portnum_var, MODE_NORMAL) != MODE_NORMAL)
    return FALSE;

  return TRUE;
}

void
weather_initialize_vals (void)
{
  /* Blitz the wsstruct so we don't get any surprises */

  memset (&ws, 0, sizeof (wsstruct));

  ws.anem_speed_max = -1;	/* Flag - catches cases with no stats file */
}

int
weather_acquire (void)
{
  char com[36], port[2] ;
  strcpy (com, setup_driver);
  port[0] = '0' + setup_port;
  port[1] = '\0';
# ifdef RISCOS
  strcat (com, port);
# endif

  werr (WERR_DEBUG0, "owAcquireEx: com = %s", com);
//  printf("weather_acquire -> owAcquireEx(\"%s\")\n", com);
  devices_portnum_var = owAcquireEx (com) ;
  if (devices_portnum_var == -1)
  {
    werr_report_owerr() ;

    port_opened = 0;
    return 0;
  }
  werr(WERR_DEBUG0, "devices_portnum_var = 0x%x", devices_portnum_var) ;

  werr (WERR_DEBUG0, "owAcquireEx OK");
  port_opened = 1;

  return 1;
}

int
weather_init (void)
{
  /* Initialize weather station */


  ws.anem_start_count = ws.anem_end_count =ws.rain_count_old = 0;
  ws.rain_count = COUNT_NOT_SET;
  ws.anem_speed = ws.anem_gust = ws.anem_speed_max = 0.0F;
  ws.rain = -1.0F;
  /*ws.NumTemps = 0 ; */

  if (port_opened) return 1 ;

  return weather_acquire ();
}

int
weather_shutdown (void)
{
  /* Done with 1-wire net and weather station */

  if (!port_opened)
    return 1;	/* Nothing to do */

  werr(WERR_AUTOCLOSE|WERR_WARNING, "Shutting down port. Please wait.");
  msDelay(1050);

  /* release the 1-Wire Net */
  owRelease(devices_portnum_var) ;
  port_opened = 0;

  return 1;
}

/* weather_primary_T */
/* Return the temperature for main window or Web upload, &c */

float
weather_primary_T (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (means == NULL)
  {
    /* User chose to use Trh? Return the first one we can find. */
    if (setup_report_Trh1)
      for (i = 0; i < MAXHUMS; ++i)
        if (devices_have_hum (i))
          return ws.Trh[i];

    /* Otherwise use the value from the first DS1820 */
    for (i = 0; i < MAXTEMPS; ++i)
      if (devices_have_temperature(i))
        return ws.T[i];

    /* We have no thermometer */

    return UNDEFINED_T;	/* Non-physical temperature signals error */
  }

  /* We have been provided with a set of means */
  /* User chose to use Trh? Return the first one we can find. */
  if (setup_report_Trh1)
    for (i = 0; i < MAXHUMS; ++i)
      if (means->haveRH[i])
        return means->meanTrh[i];

  /* Otherwise use the value from the first DS1820 */
  for (i = 0; i < MAXTEMPS; ++i)
    if (means->haveT[i])
      return means->meanT[i];

  /* We have no thermometer */

  return UNDEFINED_T;	/* Non-physical temperature signals error */
}

/* weather_primary_soilT */
/* Return the soil temperature index for main window or Web upload, &c,
 * or -1 if none available */

int
weather_primary_soilT (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXSOILTEMPS; ++i)
      if (devices_have_soil_temperature(i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXSOILTEMPS; ++i)
    if (means->haveSoilT[i])
      return i;

  return -1;
}

/* weather_primary_indoorT */
/* Return the indoor temperature index for main window or Web upload, &c,
 * or -1 if none available */

int
weather_primary_indoorT (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXINDOORTEMPS; ++i)
      if (devices_have_indoor_temperature(i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXINDOORTEMPS; ++i)
    if (means->haveIndoorT[i])
      return i;

  return -1;
}

/* weather_primary_rh */
/* Return the number of RH sensor for main window or Web upload, &c */
/* Returns -1 if no RH sensor available */

int
weather_primary_rh (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXHUMS; ++i)
      if (devices_have_hum (i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXHUMS; ++i)
    if (means->haveRH[i])
      return i;

  return -1;
}

/* weather_primary_barom */
/* Return the number of barom sensor for main window or Web upload, &c */
/* Returns -1 if no barom sensor available */

int
weather_primary_barom (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXBAROM; ++i)
      if (devices_have_barom (i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXBAROM; ++i)
    if (means->havebarom[i])
      return i;

  return -1;
}

/* weather_primary_solar */
/* Return the number of solar sensor for main window or Web upload, &c */
/* Returns -1 if no solar sensor available */

int
weather_primary_solar (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXSOL; ++i)
      if (devices_have_solar_data (i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXSOL; ++i)
    if (means->havesol[i])
      return i;

  return -1;
}

/* weather_primary_uv */
/* Return the number of uv sensor for main window or Web upload, &c */
/* Returns -1 if no uv sensor available */

int
weather_primary_uv (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXUV; ++i)
      if (devices_have_uv_data (i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXUV; ++i)
    if (means->haveuvi[i])
      return i;

  return -1;
}

int
weather_primary_soilmoisture (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXMOIST*4; ++i)
      if (devices_have_soil_moist(i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXMOIST*4; ++i)
    if (means->haveMoisture[i])
      return i;

  return -1;
}

int
weather_primary_leafwetness (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXMOIST*4; ++i)
      if (devices_have_leaf_wet(i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXMOIST*4; ++i)
    if (means->haveLeaf[i])
      return i;

  return -1;
}

int
weather_primary_adc (statsmean * means)
{
  int i;

  /* Live ws values or means? */
  if (!means)
  {
    for (i = 0; i < MAXADC; ++i)
      if (devices_have_adc (i))
        return i;

    return -1;
  }

  /* We have been provided with a set of means */
  for (i = 0; i < MAXADC; ++i)
    if (means->haveADC[i])
      return i;

  return -1;
}

char *
weather_rain_since (void)
{
  static int time_used = 0;
  static time_t prev_t_rain = 0xffffffff;
  int time_needed;
  struct tm *tm_rain;

  tm_rain = localtime (&ws.t_rain[0]);

  /* Create rain date string if necessary */
  /* Use the reset time if within 24h, or the date otherwise */

  time_needed = (ws.t <= ws.t_rain[0] + 86400);

  if ((prev_t_rain != ws.t_rain[0]) || (time_used != time_needed))
  {
    if (0 == strftime (ws.rain_date, 128, time_needed ? setup_format_raintime : setup_format_raindate, tm_rain))	/* strftime failed? */
      ws.rain_date[0] = '\0';	/* Null off undefined string */
    time_used = time_needed;
    prev_t_rain = ws.t_rain[0];
  }

  return ws.rain_date;
}

/*static int
have_rain_gauge(void)
{
  return (devices_list[devices_rain].id[0] == COUNT_FAMILY) ;
}*/

/* Reset the rain gauge offsets */

static void
weather_reset_rain (void)
{
  /* Check the age of each rain gauge offset */
  int i, j, days_old;
  uint32_t new_rain_offset[8];
  time_t new_t_rain[8], t_ref;
  struct tm tm_t;

  /* Update rain offset and times only if we have a rain gauge */
  if (devices_have_rain())
  {
    /* What was the unix time at 00:00:00? */
    tm_t = *localtime (&ws.t);
    tm_t.tm_sec = tm_t.tm_min = tm_t.tm_hour = 0;
    t_ref = mktime (&tm_t);

    /* Take copies of times and offsets */

    memcpy ((void *) new_t_rain,
            (void *) ws.t_rain, sizeof (new_t_rain));

    memcpy ((void *) new_rain_offset,
            (void *) ws.rain_offset, sizeof (new_rain_offset));

    /* Start with the oldest value */
    for (i = 7; i >= 0; --i)
    {
      /* Get integer part of age in days */
      days_old = (86399 + t_ref - ws.t_rain[i]) / 86400;
      if (days_old < 0)
      {
        werr (WERR_DEBUG0, "Clock has changed");
        break;
      }

      /* Copy over the older values */
      /* Don't leave any gaps */

      for (j = (days_old > 6) ? 6 : days_old; j > i; --j)
      {
        new_t_rain[j] = ws.t_rain[i];
        new_rain_offset[j] = ws.rain_offset[i];
      }
    }

    /* Write back the updated offsets and times */
    memcpy ((void *) &(ws.t_rain[0]),
            (void *) &(new_t_rain[0]), 8 * sizeof (time_t));

    memcpy ((void *) &(ws.rain_offset[0]),
            (void *) &(new_rain_offset[0]), 8 * sizeof (uint32_t));

    /* Check for monthly rain reset */
    /* But only once we have a legitimate count */
    tm_t.tm_mday = 1;	/* 00:00:00 on 1st of the month */
    t_ref = mktime (&tm_t);

    if ((t_ref > ws.t_rain[7]) && (ws.t_rain[0] > 0))
    {
      /* This is a new month */
      ws.t_rain[7] = t_ref;
      ws.rain_offset[7] = ws.rain_count;
      werr (WERR_DEBUG0, "Monthly rain reset");

      /* Update the offset for today's rain */
      ws.t_rain[0] = ws.t;
      ws.rain_offset[0] = ws.rain_count;
      weather_rain_since ();

      /* This can catch t_rain[i] == 0 cases too */
      for (i = 1; i < 7; ++i)
        if (ws.t_rain[i] == 0)
        {
          ws.t_rain[i] = ws.t_rain[i - 1];
          ws.rain_offset[i] =
            ws.rain_offset[i - 1];
        }
    }
    else
    {
      /* Update the offset for today's rain */
      ws.t_rain[0] = ws.t;
      ws.rain_offset[0] = ws.rain_count;
      werr (WERR_DEBUG0, "Rain reset. Count: %ld", ws.rain_count);
      weather_rain_since ();
    }
  }

}

/* Reset the dailyrain counter offset (for resets at local midnight) */
void
weather_reset_dailyrain(void)
{
  ws.dailyrain_offset = ws.rain_count;
  ws.t_dailyrain = ws.t;

  /* Save the updated values - only for local data though */
  if (setup_datasource == data_source_local)
	  setup_save_stats ();

  werr (WERR_DEBUG0, "dailyrain reset. Count: %ld", ws.rain_count);
}

/* Reset the rain gauge and other statistics */

void
weather_reset_stats (void)
{
  int i;

  /* The main variable for reset time is now reset_time */

  reset_time = ws.t;

//  printf("weather_reset_stats\n");
  werr(WERR_DEBUG0, "weather_reset_stats");

  weather_reset_rain ();

  /* Reset each GPC */
  for (i = 0; i < MAXGPC; ++i)
  {
    if (!devices_have_gpc (i))
      continue;

    /* We have this one */

    ws.gpc[i].count_offset = ws.gpc[i].count;
    ws.gpc[i].gpc = ws.gpc[i].max_sub_delta = 0.0F;
    ws.gpc[i].event_count = 0L ;
    ws.gpc[i].time_reset = ws.t;
  }

  /* Reset primary temperature max / min */
  if ((ws.T[0] != UNDEFINED_T) && (ws.T[0] != BAD_T))
  {
	// T[0] is good, so use it for min/max
	ws.Tmin = ws.Tmax = ws.T[0];
  }
  else
  {
	// T[0] is bad. Use extreme values for min/max for now.
	// They will come good when we get a good T[0].
	ws.Tmin = -UNDEFINED_T;
	ws.Tmax = UNDEFINED_T;
  }

  /* Update max wind speed */
  ws.anem_speed_max = ws.anem_speed;

  /* Save the updated values - only for local data though */
  if (setup_datasource == data_source_local)
	  setup_save_stats ();
}

static int
ReadCounter_robust (int device,
                    int CounterPage, uint32_t *Count, int depth)
{
  while (depth >= 0)
  {
    if (ReadCounter (device, CounterPage, Count))
      return TRUE;

    --depth;
    //msDelay(10) ;
  }

  return FALSE;
}


static int
do_first_thermometer (void)
{
  if (MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS==0) return 0;

  for (ws.Tnum = 0; ws.Tnum < MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS; ++ws.Tnum)
  {
    if (devices_have_thermometer (ws.Tnum))
    {
      state_new (state_starttempconv);
      return 1;	/* OK */
    }
  }

  return 0;		/* No thermometers */
}

/* do_first_barometer added by GNC */
static int
do_first_barometer (void)
{
  for (ws.Bnum = 0; ws.Bnum < MAXBAROM; ++ws.Bnum)
  {
    if (devices_have_barom (ws.Bnum))
    {
      state_new (state_read_barometer);
      return 1;	/* OK */
    }
  }
  return 0;		/* No barometers */
}

static int
do_first_humidity (void)
{
  for (ws.Hnum = 0; ws.Hnum < MAXHUMS; ++ws.Hnum)
  {
    if (devices_have_hum (ws.Hnum))
    {
      state_new (state_read_humidity);
      return 1;	/* OK */
    }
  }
  return 0;		/* No thermometers */
}

static int
do_first_gpc (void)
{
  for (ws.gpcnum = 0; ws.gpcnum < MAXGPC; ++ws.gpcnum)
  {
    if (devices_have_gpc (ws.gpcnum))
    {
      state_new (state_read_gpc);
      return 1;	/* OK */
    }
  }
  return 0;		/* No thermometers */
}

static int
do_first_solar (void)
{
  for (ws.solnum = 0; ws.solnum < MAXSOL; ++ws.solnum)
  {
    if (devices_have_solar_sensor (ws.solnum))
    {
      state_new (state_read_solar);
      return 1;	/* OK */
    }
  }
  return 0;		/* No solar sensors */
}

static int
do_first_uv (void)
{
  for (ws.uvnum = 0; ws.uvnum < MAXUV; ++ws.uvnum)
  {
    if (devices_have_uv_sensor (ws.uvnum))
    {
      state_new (state_read_uv);
      return 1;	/* OK */
    }
  }
  return 0;		/* No uv sensors */
}

/* Change state ready to read next device */

static int
weather_next_device(int old_state)
{
  switch (old_state)
  {
  case state_readanemstart:	// After reading anemometer
    if (do_first_thermometer ())
      return 0;	// Ok
      /* no break */

  case state_starttempconv:     // Finished with a fast thermometer
  case state_endtempconv:	// Finished reading thermometers
    if (do_first_humidity ())
      return 0;	// Ok
    /* no break */

  case state_read_humidity:	// Finished reading hygrometers
    if (do_first_barometer ())
      return 0;	// Ok
    /* no break */

  case state_read_barometer: // Finished reading barometers
    if (do_first_gpc())
      return 0 ; // Ok
    /* no break */

  case state_read_gpc:	// Finished reading GPCs
    if (do_first_solar ())
      return 0;	// Ok
    /* no break */

  case state_read_solar:	// Finished reading solar sensors
	   if (do_first_uv ())
	      return 0;	// Ok
	      /* no break */

  case state_read_uv:	// Finished reading UV sensors
    state_new (state_read_adc);
    break ;

  case state_read_adc:	// Finished reading ADC sensors
    state_new (state_read_tc);
    break ;

  case state_read_tc:	// Finished reading thermocouples
    state_new (state_read_moist);
    break ;

  case state_read_moist:	// Finished reading moisture boards
    state_new (state_readanemend);
    break ;

  default:
    werr(WERR_DEBUG0, "weather_next_device called with old_state = %d", old_state) ;
    state_new (state_readanemend);
    /* no break */
  }

  return 0 ;
}

static int
weather_read_ws_anem_start (void)
{
  /* If we have a WSI603A read it now. We'll skip the individual wind instruments */
  if (devices_have_wsi603a()) {
	wsi603a_struct result;
	if (-1 != wsi603a_read(devices_wsi603a, &result))
	{
	  ws.anem_speed = result.windSpeed * devices_list[devices_wsi603a].calib[2];
	  ws.vane_bearing = result.windPoint;
	  ws.T[0] = devices_list[devices_wsi603a].calib[0] * result.T + devices_list[devices_wsi603a].calib[1];
	  ws.solar[0] = devices_list[devices_wsi603a].calib[3] * result.intensity + devices_list[devices_wsi603a].calib[4];
	}

	  weather_next_device(state_readanemstart) ;

	  return 1;		/* OK */
  }

  /* Reset interval gust value */
  //ws.anem_int_gust = 0;

  /* Read the initial anemometer count */

  /* read the current counter */
  /* use this reference to calculate wind speed later */
  if (ws.anem_end_count > 0)
  {
    /* Have previous count - don't bother to read counter */
    ws.anem_start_count = ws.anem_end_count;
    ws.anem_start_time = ws.anem_end_time;
  }
  else
  {
    if (devices_list[devices_anem].id[0] == COUNT_FAMILY)
    {
      /* First read of anemometer - set normal and gust values */
      if (!ReadCounter_robust (devices_anem, 15,
                               &(ws.anem_start_count),
                               ANEM_TRIES))
      {
        werr (WERR_WARNING + WERR_AUTOCLOSE,
              "Error reading anemometer (start). Device present? : %s",
              owVerify (devices_portnum(devices_anem), FALSE) ? "Yes" : "No");
        //state_new (state_wsdead);
        return 0;
      }
      /* get a time stamp (mS) */
      ws.anem_start_time = ws.gust_start_time = msGettick ();
      ws.gust_start_count = ws.anem_start_count;
    }
  }

  weather_next_device(state_readanemstart) ;

  return 1;		/* OK */
}

static int
weather_read_ws_start_tempconv (void)
{
  /* Start a temperature conversion */

  int p ;
  int powered = 0 ;

  assert((ws.Tnum>=0)&&(ws.Tnum<MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS));

  //return (thermometer_probe() == 0) ;

  p = devices_portnum(devices_T1 + ws.Tnum) ;

  if (!devices_have_thermometer (ws.Tnum))
  {
    werr (0,
          "Attempt to start temp conv on missing thermometer - %s",
          devices_list[devices_T1+ws.Tnum].menu_entry);
    do ++ws.Tnum;
    while ((ws.Tnum < MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS) && !devices_have_thermometer (ws.Tnum));
    if (ws.Tnum == MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS)
      return 0;
  }

  // Check thermometer power status
  if (devices_access(devices_T1+ws.Tnum))
  {
    /* send the Read Power Supply command */
    owTouchByte (p, 0xB4);
    powered = owReadByte(p) ;

    werr(WERR_DEBUG1, "%s power status %d",
      devices_list[devices_T1+ws.Tnum].menu_entry, powered);
  }
  else
  {
    werr(WERR_WARNING, "Failed to access %s", devices_list[devices_T1+ws.Tnum].menu_entry);
    return 0 ;
  }

  // What kind of thermometer is this?
  switch (devices_list[devices_T1+ws.Tnum].id[0])
  {
    // For slow thermometers, start tempconv, kick in strong pullup and wait
    case TEMP_FAMILY:
    case TEMP_FAMILY_DS1822:
    case TEMP_FAMILY_DS18B20:
    {
      /* access the device */
      if (!devices_access (devices_T1 + ws.Tnum))
        return 0;

      /* send the convert temperature command */
      owTouchByte (p, 0x44);

      if (powered)
      {
        /* Poll until temperature conversion finished */
//        int dtime = 0 ;

        while (owReadByte(p) == 0)
        {
          //printf("%04d %03d\n", dtime,res);
//          dtime += 10 ;
          msDelay(10) ;
        }
//        werr(WERR_DEBUG0, "dtime = %d", dtime) ;
        return weather_read_ws_end_tempconv() ;
      }
      else
      {
        /* set the MicroLAN to strong pull-up */
        if (owLevel (p, MODE_STRONG5) != MODE_STRONG5)
          return 0;

        /* Now sleep for 1 second */

        state_new (state_waiting_tempconv);

        // Rest of this case handled in end_tempconv

        return 1;		/* OK */
      }
    }

    // For fast sensors, start tempconv, poll busybyte, get result, and set for next sensor
    case SBATTERY_FAMILY:
    {
      if (!weather_read_ds2438_thermometer ())
        return FALSE ;

      // Skip missing thermometers
      do
      {
        ++ws.Tnum;
      }
      while ((ws.Tnum < MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS) && !devices_have_thermometer (ws.Tnum));

      if (ws.Tnum == MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS)
      {
        /* All thermometers read */
        ws.Tnum = 0;

        weather_next_device(state_starttempconv) ;
      }

      return 1;
    }
  }

  return FALSE ;
}

// Read an AAG TAI-8570 barometer

static int
weather_read_tai8570_barom (void)
{
  static int cal[MAXBAROM][6];
  static float lastval[MAXBAROM] ;
  static int runcount[MAXBAROM] ;
  static int initialized = FALSE;
  float pressure, temperature;

  assert((ws.Bnum>=0)&&(ws.Bnum<MAXBAROM));

  if (!initialized)
  {
    initialized = TRUE;
    memset ((void *) cal, 0, MAXBAROM * 6 * sizeof (int));
    memset ((void *) lastval, 0, MAXBAROM * sizeof (int));
    memset ((void *) runcount, 0, MAXBAROM * sizeof (int));
  }

  if (cal[ws.Bnum][0] == 0)	// Need cal?
  {
    if (tai8570_read_cal
        (devices_BAR1 + ws.Bnum, devices_tai8570w1 + ws.Bnum,
         cal[ws.Bnum]) < 0)
    {
      werr (WERR_WARNING + WERR_AUTOCLOSE,
            "%s: could not read cal values",
            devices_list[devices_BAR1 +
                         ws.Bnum].menu_entry);
      return 0; // Failed
    }
  }

  // Have cal - Read values
  if (tai8570_read_vals (devices_BAR1 + ws.Bnum,
                         devices_tai8570w1 + ws.Bnum,
                         cal[ws.Bnum], &pressure, &temperature) < 0)
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "%s: could not read barometer values",
          devices_list[devices_BAR1 + ws.Bnum].menu_entry);
    return 0; // Failed
  }

  /* Check for run of equal values */
  if (pressure == lastval[ws.Bnum])
  {
    if (++runcount[ws.Bnum] >= 20) // 20 in a row registers failure
    {
      werr(WERR_WARNING, "TAI8570 - bad run") ;
      werr(WERR_WARNING, "pressure = %f", pressure) ;
      werr(WERR_WARNING, "temperature = %f", temperature) ;

      /* Restore start-up state */
      lastval[ws.Bnum] = 0.0 ;
      runcount[ws.Bnum] = 0 ;

      return 0 ; // Failed
    }
  }
  else
  {
    runcount[ws.Bnum] = 0 ;
    lastval[ws.Bnum] = pressure ;
  }

  /* Pressure sanity check */
  if ((pressure >= 300.0F) && (pressure <= 1100.0F))
    ws.barom[ws.Bnum] =
      pressure * devices_list[devices_BAR1 +
                              ws.Bnum].calib[0];

  /* Corrected pressure sanity check */
  if ((ws.barom[ws.Bnum] < 500.0F) ||
      (ws.barom[ws.Bnum] > 1500.0F))
    return 0 ; // Failed

  /* Temperature sanity check */
  if ((temperature >= -75.0F) && (temperature <= 75.0F))
    ws.Tb[ws.Bnum] = temperature;

  return 1;		// Ok
}

static int
weather_read_ds2438_thermometer (void)
{
  float Vad, /*Vdd, */ T;

  assert((ws.Tnum>=0)&&(ws.Tnum<MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS));

  /* Start the temperature conversion */
  if (ad26_start_tempconv (devices_T1 + ws.Tnum) < 0)
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "%s: could not start temperature conversion",
          devices_list[devices_T1 + ws.Tnum].menu_entry);
    return 0;
  }

  /* Copy scratchpad */
  Vad = Volt_Reading (FALSE, devices_T1 + ws.Tnum,
                      VOLT_READING_TRIES);
  if (Vad == -1.0F)
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "Failed to read Vad of %s",
          devices_list[devices_T1 + ws.Tnum].menu_entry);
    return 0;
  }
  //msDelay(1) ;

  if (0 == Get_Temperature (devices_T1 + ws.Tnum, &T))
  {
    //if (T == 85.0F) return 0 ; /* Failed */

    ws.T[ws.Tnum] = T *
                     devices_list[devices_T1 + ws.Tnum].calib[0] +
                     devices_list[devices_T1 + ws.Tnum].calib[1];
  }
  else
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "Failed to read temperature of %s",
          devices_list[devices_T1 + ws.Tnum].menu_entry);
    return 0;
  }

  return 1;
}

int
weather_read_ds2438_barometer (void)
{
  float Vad, /*Vdd, */ T;

  assert((ws.Bnum>=0)&&(ws.Bnum<MAXBAROM));

  /* Start the temperature conversion */
  if (ad26_start_tempconv (devices_BAR1 + ws.Bnum) < 0)
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "%s: could not start temperature conversion",
          devices_list[devices_BAR1 + ws.Bnum].menu_entry);
    return 0;
  }

  /* Get Vad */
  Vad = Volt_Reading (FALSE, devices_BAR1 + ws.Bnum,
                      VOLT_READING_TRIES);
  if (Vad == -1.0F)
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "Failed to read Vad of %s",
          devices_list[devices_BAR1 + ws.Bnum].menu_entry);
    return 0;
  }
  //msDelay(1) ;

  if (0 == Get_Temperature (devices_BAR1 + ws.Bnum, &T))
  {
    //if (T == 85.0F) return 0 ; /* Failed */

    ws.Tb[ws.Bnum] = T *
                     devices_list[devices_BAR1 + ws.Bnum].calib[2] +
                     devices_list[devices_BAR1 + ws.Bnum].calib[3];
  }
  else
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "Failed to read temperature of %s",
          devices_list[devices_BAR1 + ws.Bnum].menu_entry);
    return 0;
  }

  if (Vad > 0.0F)
  {
    float pressure =
      Vad * devices_list[devices_BAR1 + ws.Bnum].calib[0] +
      devices_list[devices_BAR1 + ws.Bnum].calib[1];
    if (pressure >= -1)
      ws.barom[ws.Bnum] = pressure;
    else
      return 0;
  }
  else
  {
    werr (WERR_DEBUG0,
          "Vad %f for %s",
          Vad, devices_list[devices_BAR1 + ws.Bnum].menu_entry);
    return 0;
  }

  return 1;
}

/* weather_read_ws_read_barom from GNC */
static int
weather_read_ws_read_barom (void)
{
  static int *barom_present = NULL ;

  assert((ws.Bnum>=0)&&(ws.Bnum<MAXBAROM));

  if (barom_present == NULL)
  {
    barom_present = (int *) calloc(MAXBAROM, sizeof(int)) ;
  }

  if (devices_have_barom (ws.Bnum))
  {
    barom_present[ws.Bnum] = 1 ;
    switch (devices_list[devices_BAR1 + ws.Bnum].id[0])
    {
    case SBATTERY_FAMILY:
      if (0 == weather_read_ds2438_barometer ()) return 0 ;
      break;

    case SWITCH_FAMILY:
      if (0 == weather_read_tai8570_barom ()) return 0 ;
      break;
    }
  }
  else
  {
    if (barom_present[ws.Bnum] == 1) // Barometer went AWOL?
      werr(WERR_WARNING, "Barometer %d went AWOL", ws.Bnum) ;
    barom_present[ws.Bnum] = 0 ;
  }

  ++ws.Bnum;
  if (ws.Bnum == MAXBAROM)
  {
    /* All barometers read */
    ws.Bnum = 0;

    /* Next do first gpc or the anemometer */
    //if (!do_first_gpc ())
    /* No - carry on to anemometer */
    //	state_new (state_readanemend);

    weather_next_device(state_read_barometer) ;
  }
  return 1;
}

static int
weather_read_ws_read_humidity (void)
{
  /* Start a humidity conversion */

  float Vdd, Vad, T;

  assert((ws.Hnum>=0)&&(ws.Hnum<MAXHUMS));

  if (devices_have_hum (ws.Hnum))
  {
    switch (devices_list[devices_H1 + ws.Hnum].id[0])
    {
      case SBATTERY_FAMILY:
      {
        /* Start the temperature conversion */
        if (ad26_start_tempconv (	/*devices_list[devices_H1+ws.Hnum].id */
              devices_H1 + ws.Hnum) < 0)
        {
          werr (WERR_WARNING + WERR_AUTOCLOSE,
                "%s: could not start temperature conversion",
                devices_list[devices_H1 + ws.Hnum].menu_entry);
          return 0;
        }
        //msDelay(1) ;

        /* Get Vdd */
        Vdd = Volt_Reading (TRUE,	/*devices_list[devices_H1+ws.Hnum].id */
                            devices_H1 + ws.Hnum, VOLT_READING_TRIES);
        if (Vdd == -1.0F)
        {
          werr (WERR_WARNING + WERR_AUTOCLOSE,
                "Failed to read Vdd of %s",
                devices_list[devices_H1 + ws.Hnum].menu_entry);
          return 0;
        }
        //msDelay(1) ;

        /* Get Vad */
        Vad = Volt_Reading (FALSE,	/*devices_list[devices_H1+ws.Hnum].id */
                            devices_H1 + ws.Hnum, VOLT_READING_TRIES);
        if (Vad == -1.0F)
        {
          werr (WERR_WARNING + WERR_AUTOCLOSE,
                "Failed to read Vad of %s",
                devices_list[devices_H1 + ws.Hnum].menu_entry);
          return 0;
        }
        //msDelay(1) ;

        if (0 == Get_Temperature (devices_H1 + ws.Hnum, &T))
        {
          //if (T == 85.0F) return 0 ; /* Failed */

          ws.Trh[ws.Hnum] = T *
                            devices_list[devices_H1 + ws.Hnum].calib[0] +
                            devices_list[devices_H1 + ws.Hnum].calib[1];
        }
        else
        {
          werr (WERR_WARNING + WERR_AUTOCLOSE,
                "Failed to read temperature of %s",
                devices_list[devices_H1 + ws.Hnum].menu_entry);
          return 0;
        }

        if ((Vad > 0.0F) && (Vdd > 0.0F) && (Vad < Vdd))
        {
          ws.RH[ws.Hnum] =
            (float) ((((double) (Vad / Vdd) -
                      (0.8 / Vdd)) / 0.0062) / (1.0546 -
                                          0.00216 *
                                          ws.Trh[ws.
                                                Hnum]));
          ws.RH[ws.Hnum] =
            ws.RH[ws.Hnum] * devices_list[devices_H1 +
                                          ws.Hnum].
            calib[2] + devices_list[devices_H1 +
                                    ws.Hnum].calib[3];

          if (ws.RH[ws.Hnum] > 100.0F)
            ws.RH[ws.Hnum] = 100.0F;
          if (ws.RH[ws.Hnum] < 0.0F)
            ws.RH[ws.Hnum] = 0.0F;
        }
        else
        {
          werr (WERR_DEBUG0,
                "Error reading %s. Vad = %f, Vdd = %f",
                devices_list[devices_H1 + ws.Hnum].menu_entry,
                Vad, Vdd);

          return 0;
        }

        break ;
      }

      case HYGROCHRON_FAMILY:
      {
        if (hygrochron_read(
              devices_H1 + ws.Hnum,
              &(ws.RH[ws.Hnum]),
              &(ws.Trh[ws.Hnum])) < 0)
        {
          werr (WERR_DEBUG0,
                "Error reading %s. RH = %f, Trh = %f",
                devices_list[devices_H1 + ws.Hnum].menu_entry,
                ws.RH[ws.Hnum], ws.Trh[ws.Hnum]);

          return 0;
        }
        break ;
      }
    }
  }

  ++ws.Hnum;

  if (ws.Hnum == MAXHUMS)
  {
    /* All temps read */
    ws.Hnum = 0;

    /* Perhaps we have a barometer? */
    //if ((!do_first_barometer ()) && (!do_first_gpc ()))
    /* No - carry on to anemometer */
    //	state_new (state_readanemend);

    weather_next_device(state_read_humidity) ;
  }

  return 1;		/* OK */
}

static int
weather_read_ws_read_solar (void)
{
  /* Start a solar conversion */
  float current ;

  assert((ws.solnum>=0)&&(ws.solnum<MAXSOL));

  if (devices_have_solar_sensor (ws.solnum))
  {
    // Read Vsens

    if (0 == ad26_current_reading(devices_sol1 + ws.solnum, VOLT_READING_TRIES, &current))
    {
      /* Current reading Ok */
      if (current < -512) current = 1024 ; // >FSD goes very -ve
      else if (current < 0) current = 0 ;  // Clip noise at 0
      ws.solar[ws.solnum] = current *
                  devices_list[devices_sol1 + ws.solnum].calib[0] +
                  devices_list[devices_sol1 + ws.solnum].calib[1] ;
    }

    //msDelay(1) ;

  }

  ++ws.solnum;

  if (ws.solnum == MAXSOL)
  {
    /* All solar sensors read */
    ws.solnum = 0;

    weather_next_device(state_read_solar) ;
  }

  return 1;		/* OK */
}

static int
weather_read_ws_read_uv (void)
{
  /* Start a uv conversion */
  float uv, T ;

  assert((ws.uvnum>=0)&&(ws.uvnum<MAXUV));

  if (devices_have_uv_sensor (ws.uvnum))
  {
    // Read UVI value from Hobby Boards UV sensor

    if (0 == hbuvi_read_UVI(devices_uv1 + ws.uvnum, &uv))
    {
      /* UVI reading Ok */
      ws.uvi[ws.uvnum] = uv *
    	  devices_list[devices_uv1 + ws.uvnum].calib[0] +
          devices_list[devices_uv1 + ws.uvnum].calib[1] ;
    }

    // Read T value from Hobby Boards UV sensor

    if (0 == hbuvi_read_T(devices_uv1 + ws.uvnum, &uv))
    {
      /* T reading Ok */
      hbuvi_read_T(devices_uv1 + ws.uvnum, &T);
      ws.uviT[ws.uvnum] = T;
    }

    //msDelay(1) ;

  }

  ++ws.uvnum;

  if (ws.uvnum == MAXUV)
  {
    /* All UV sensors read */
    ws.uvnum = 0;

    weather_next_device(state_read_uv) ;
  }

  return 1;		/* OK */
}

static int
weather_read_ws_read_adc (void)
{
//  /* Start a solar conversion */
//  float current ;

  assert((ws.adcnum>=0)&&(ws.adcnum<MAXADC));

  if (devices_have_adc (ws.adcnum))
  {
    // Read ADC data

    ad30_read(devices_adc1 + ws.adcnum, &ws.adc[ws.adcnum]);

    // V is measured in mV but converted to V. So here we must convert offset from V to mV
    ws.adc[ws.adcnum].V =
      ws.adc[ws.adcnum].V * devices_list[devices_adc1 + ws.adcnum].calib[0] +
      devices_list[devices_adc1 + ws.adcnum].calib[1] * 1000.0F;

    ws.adc[ws.adcnum].I =
      ws.adc[ws.adcnum].I * devices_list[devices_adc1 + ws.adcnum].calib[2] +
      devices_list[devices_adc1 + ws.adcnum].calib[3];

    ws.adc[ws.adcnum].Q =
      ws.adc[ws.adcnum].Q * devices_list[devices_adc1 + ws.adcnum].calib[4];

//    printf("ADC%d read: %f, %f, %f, %f\n",
//      ws.adcnum+1,
//      ws.adc[ws.adcnum].V,
//      ws.adc[ws.adcnum].I,
//      ws.adc[ws.adcnum].Q,
//      ws.adc[ws.adcnum].T
//    );
  }

  ++ws.adcnum;

  if (ws.adcnum == MAXADC)
  {
    /* All ADC sensors read */
    ws.adcnum = 0;

    weather_next_device(state_read_adc) ;
  }

  return 1;		/* OK */
}

static int
weather_read_ws_read_thrmcpl (void)
{
  /* Start a thermocouple conversion */
  float current ;

  assert((ws.tcnum>=0)&&(ws.tcnum<MAXTC));

  if (devices_have_thrmcpl(ws.tcnum))
  {
    // Read ADC data

	adc_struct adc;

    ad30_read(devices_tc1 + ws.tcnum, &adc);

    ws.thrmcpl[ws.tcnum].type = (int) floor(0.5 + devices_list[devices_tc1 + ws.tcnum].calib[0]);

    ws.thrmcpl[ws.tcnum].V =
      adc.I - devices_list[devices_tc1 + ws.tcnum].calib[1] ;

    ws.thrmcpl[ws.tcnum].Tcj = adc.T;

    if (0 != getThermocoupleT(&(ws.thrmcpl[ws.tcnum])))
      werr(WERR_DEBUG0, "Failed to generate temperature for %s", devices_list[devices_tc1 + ws.tcnum].menu_entry);

//    printf("TC%d read: T=%f, Tcj=%f, V=%f, type=%d\n",
//      ws.tcnum+1,
//      ws.thrmcpl[ws.tcnum].T,
//      ws.thrmcpl[ws.tcnum].Tcj,
//      ws.thrmcpl[ws.tcnum].V,
//      ws.thrmcpl[ws.tcnum].type
//    );
  }

  ++ws.tcnum;

  if (ws.tcnum == MAXTC)
  {
    /* All thermocouples read */
    ws.tcnum = 0;

    weather_next_device(state_read_tc) ;
  }

  return 1;		/* OK */
}

static int
weather_read_ws_read_moist (void)
{
  /* Do a moisture read */
  moist_struct moist;
//  int types[4] = {HBMOIST_SOIL, HBMOIST_LEAF, HBMOIST_LEAF, HBMOIST_SOIL};
//  static int set=0;

  assert((ws.moistnum>=0)&&(ws.moistnum<MAXMOIST));

  if (devices_have_moisture_board(ws.moistnum))
  {
//	hbmoist_set_assignments(devices_moist1 + ws.moistnum, types);
    // Read 4 channels from Hobby Boards moisture board (also updates channel types)

    if (0 == hbmoist_read_sensors(devices_moist1 + ws.moistnum, &moist))
    {
      /* Moisture readings Ok */
      int i;

      memcpy(devices_list[devices_moist1 + ws.moistnum].local, &moist, sizeof(moist_struct));

      for (i=0; i<4; ++i) {
    	if (moist.type[i]==HBMOIST_SOIL)
    	  ws.soil_moist[i+4*ws.moistnum] = moist.sensor[i];
    	else
    	  ws.leaf_wet[i+4*ws.moistnum] = moist.sensor[i];
      }
    }

//    if (!set)
//    {
//      hbmoist_set_assignments(devices_moist1 + ws.moistnum, 2);
//      hbmoist_get_assignments(devices_moist1 + ws.moistnum);
//      set = 1;
//    }
  }

  ++ws.moistnum;

  if (ws.moistnum == MAXMOIST)
  {
    /* All UV sensors read */
    ws.moistnum = 0;

    weather_next_device(state_read_moist) ;
  }

  return 1;		/* OK */
}

static int weather_read_ws_read_ad26vane (int *index)
{
  /* Start a vane conversion */
  float reading;
  int i;
  float Vad;

  if (devices_have_vane ())
  {
        // Read Vsens

        /* Start the temperature conversion */
//      if (ad26_start_tempconv (devices_vane_adc) < 0)
//      {
//        werr (WERR_WARNING + WERR_AUTOCLOSE,
//                "%s: could not start temperature conversion",
//                devices_list[devices_vane_adc].menu_entry);
//        return -1;
//      }

        /* Get Vad */
        Vad = Volt_Reading (FALSE, devices_vane_adc,
               VOLT_READING_TRIES);

    /* Current reading Ok */
    for (i=0; i<16; i++)
    {
        if ((points_voltage_minimum[i]-0.02 <= Vad) && (points_voltage_minimum[i]+0.08 >= Vad))
        {
          return *index = i+1;
        }
    }
        //msDelay(1) ;

  }
  else return -1;

  return 1;             /* OK */
}

/* This function handles the HobbyBoards ADS Vane.  To use it, you need to change your devices
   file to use "vaneads" instead of "vaneadc".  It has one calibration item: compass_offset.
   This corrects a badly north-aligned hardware install.  */
static int weather_read_ws_read_adsvane (int *index)
{
  /* Start a vane conversion */
  float Vad;

  if (devices_have_vane ())
  {
	/* Get Vad */
	Vad = Volt_Reading (FALSE, devices_vane_ads,
		VOLT_READING_TRIES);
	werr (WERR_DEBUG1,
                "Vad: %f read in adsvane", Vad);

        // Corrected numbers from Hobby-Boards.
        if ((Vad >= 2.60) && (Vad <= 2.75)) return *index = 1;
        else if ((Vad >= 6.30) && (Vad <= 6.70)) return *index = 2;
        else if ((Vad >= 5.80) && (Vad <= 6.10)) return *index = 3;
        else if ((Vad >= 9.35) && (Vad <= 9.42)) return *index = 4;
        else if ((Vad >= 9.15) && (Vad <= 9.34)) return *index = 5;
        else if ((Vad >= 9.49) && (Vad <= 9.70)) return *index = 6;
        else if ((Vad >= 8.40) && (Vad <= 8.60)) return *index = 7;
        else if ((Vad >= 8.90) && (Vad <= 9.10)) return *index = 8;
        else if ((Vad >= 7.50) && (Vad <= 7.70)) return *index = 9;
        else if ((Vad >= 7.90) && (Vad <= 8.05)) return *index = 10;
        else if ((Vad >= 4.20) && (Vad <= 4.35)) return *index = 11;
        else if ((Vad >= 4.52) && (Vad <= 4.70)) return *index = 12;
        else if ((Vad >= 0.80) && (Vad <= 1.00)) return *index = 13;
        else if ((Vad >= 2.10) && (Vad <= 2.30)) return *index = 14;
        else if ((Vad >= 1.50) && (Vad <= 1.70)) return *index = 15;
        else if ((Vad >= 3.50) && (Vad <= 3.65)) return *index = 16;
        /* Current reading Ok */
       else return -1;

  }
  else return -1;

  return 1;             /* OK */
}

/* This function handles the HobbyBoards Inspeed Vane.  To use it, you need to change your 
   devices file to use "vaneins" instead of "vaneadc".  It has one calibration item: 
   compass_offset.  This corrects a badly north-aligned hardware install.  Original code 
   from Glenn McKechnie from the forum.  This does not return an exact degree, which the
   hardware can do.  I might fix this code later to return the exact point instead of a 
   cardinal, but will have to do some modifications to other sections to make it work. */
static int weather_read_ws_read_insvane (int *index)
{
  /* Start a vane conversion */
  float Vad;
  float Vdd;
  float degrees;
  float greyzone;
  int cardinal;

  if (devices_have_vane ())
  {
        /* Get Vad and Vdd */
        Vad = Volt_Reading (FALSE, devices_vane_ins, VOLT_READING_TRIES);
        Vdd = Volt_Reading (TRUE, devices_vane_ins, VOLT_READING_TRIES);
        werr (WERR_DEBUG1, "Vad: %f read in insvane", Vad);
        werr (WERR_DEBUG1, "Vdd: %f read in insvane", Vdd);

        // Directions in degrees
        // http://www.hobby-boards.com/catalog/links/wind-in/Inspeed%20Anemometer%20User%20Manual.pdf
        degrees = ((400*(Vad-(0.05*Vdd))/Vdd));
        if (degrees>=348.75) cardinal = 0;
        else
        {
           greyzone = (degrees+11.26);
           cardinal = (greyzone/22.5);
        }
        return *index = cardinal+1;
  }

  return -1;
}

static int
weather_read_ws_end_tempconv (void)
{
  /* Get temperature conversion result */

  int in_error = 0;
  uchar send_block[30], uc_cr, uc_cpc;
  int send_cnt = 0, tsht, i;
  float tmp, Tresult = 85.0F /*, cr, cpc */ ;

  static int errors_this_T = 0;


# ifndef max_T_errors
#define max_T_errors 3
# endif

  if (!weather_strong_off ())
  {
    werr (WERR_DEBUG0,
          "weather_read_ws_end_tempconv weather_strong_off failed");
    return 0;
  }

  /* access the device */
  if (!devices_access (devices_T1 + ws.Tnum))
  {
    in_error = 1;
  }
  else
  {
    int p ;
    ushort lastcrc8;
    p = devices_portnum(devices_T1 + ws.Tnum) ;
    setcrc8(p,0);

    /* create a block to send that reads the temperature */
    /* read scratchpad command */
    send_block[send_cnt++] = 0xBE;
    /* now add the read bytes for data bytes and crc8 */
    for (i = 0; i < 9; i++)
      send_block[send_cnt++] = 0xFF;

    /* now send the block */
    if (owBlock (p, FALSE, send_block, send_cnt))
    {
      /* perform the CRC8 on the last 8 bytes of packet */
      for (i = send_cnt - 9; i < send_cnt; i++)
        lastcrc8 = docrc8(p,send_block[i]);

      /* verify CRC8 is correct */
      if (lastcrc8 == 0x00)
      {
        /* Temperature calculation depends on family */

        switch (devices_list[devices_T1 + ws.Tnum].id[0])
        {
        case TEMP_FAMILY:	/* DS18S20 */
          /* calculate the high-res temperature */
          tsht = send_block[1];
          if (send_block[2] & 0x01)
            tsht |= -256;
          /*tmp = (float)(tsht/2); */
          /* Modified as per note by Philip Gladstone,
           * to weather list 29/9/0 */
          tmp = (float) (tsht - (tsht & 1)) / 2;
          uc_cr = send_block[7];
          uc_cpc = send_block[8];
          if (uc_cpc == 0)
          {
            werr (WERR_DEBUG0,
                  "weather_read_ws_end_tempconv: cpc == 0");
            ++errors_this_T;
            in_error = 1;
            Tresult = 85.0F;
          }
          else
          {
            /* Check for error condition */
            if ((send_block[1] == 0xaa) &&
                (send_block[2] == 0x00) &&
                (send_block[7] == 0x0c) &&
                (send_block[8] == 0x10))
            {
              werr (WERR_DEBUG0,
                    "weather_read_ws_end_tempconv: T%d = 85C",
                    ws.Tnum);
              ++errors_this_T;
              in_error = 1;
              Tresult = 85.0F;
            }
            else
            {
              Tresult =
                tmp - 0.25F + (float) (uc_cpc - uc_cr) / (float) uc_cpc ;

              ws.T[ws.Tnum] =
                devices_list[devices_T1 + ws.Tnum].calib[0] * Tresult +
                devices_list[devices_T1 + ws.Tnum].calib[1] ;
#ifdef HAVE_FINITEF
              if (!finitef
                  (ws.T[ws.Tnum]))
              {
                werr (WERR_WARNING, "tempconv read a nan");
                werr (WERR_WARNING, "cr = %d, cpc = %d, tsht = %d", uc_cr, uc_cpc, tsht);
                werr (WERR_WARNING, "send_block[1]: %u, [2]: %u, [7]: %u, [8]: %u", send_block[1], send_block[2], send_block[7], send_block[8]);
                werr (WERR_WARNING, "calib[0]: %f, calib[1]: %f", devices_list[devices_T1 + ws.Tnum].calib[0], devices_list[devices_T1 + ws.Tnum].calib[1]);
                ws.T[ws.
                     Tnum] =
                       85.0F;
                in_error = 1;
                ++errors_this_T;
              }
#endif

            }
          }
          break;

        case TEMP_FAMILY_DS1822:
        case TEMP_FAMILY_DS18B20:
          uc_cpc = send_block[8];
          if (uc_cpc == 0)
          {
            werr (WERR_DEBUG0,
                  "weather_read_ws_end_tempconv: cpc == 0");
            in_error = 1;
            ++errors_this_T;
          }
          else
          {
            tsht = send_block[1] +
                   send_block[2] * 256;
            if (send_block[2] & 8)
              tsht |= ~0x7ff;

            Tresult =
              (float) tsht *0.0625F;

            if (Tresult == 85.0)
            {
              werr (WERR_DEBUG0,
                    "weather_read_ws_end_tempconv: T%d = 85C",
                    ws.Tnum);
              ++errors_this_T;
            }
            else
            {
              ws.T[ws.Tnum] =
                devices_list
                [devices_T1 +
                 ws.Tnum].
                calib[0] *
                Tresult +
                devices_list
                [devices_T1 +
                 ws.Tnum].
                calib[1];
            }
          }
          break;
        }
      }
      else
      {
        werr (WERR_DEBUG0,
              "CRC error reading %s",
              devices_list[devices_T1 +
                           ws.Tnum].menu_entry);
        in_error = 1;
        ++errors_this_T;
      }
    }
    else
    {
      werr (WERR_DEBUG0, "Read scratchpad failed: %s",
            devices_list[devices_T1 + ws.Tnum].menu_entry);
    }
  }

  /* Do we need to try this one again? */
  if (in_error)
  {
    if (errors_this_T < max_T_errors)
    {
      /* Haven't had too many erros yet, so have another go */
      state_new (state_starttempconv);
      return 1;	/* WS readout will continue at restarted tempconv */
    }
    else
    {
      /* We've had too many errors */
      /* Go back to start condition - immediate bus reset */
      state_new (state_port_ok);
      /* This will call weather_init and we'll be back at square 1 */
      return 0;
    }
  }

  /* Got a good reading */

  /* On to next thermometer */
  do
  {
    ++ws.Tnum;
  }
  while ((ws.Tnum < MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS) && !devices_have_thermometer (ws.Tnum));

  if (ws.Tnum == MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS)
  {
    /* All temps read */
    ws.Tnum = 0;
    weather_next_device(state_endtempconv) ;
  }
  else
  {
    /* Ready to read next temp */
    state_new (state_starttempconv);
  }

  errors_this_T = 0;

  return 1;		/* Ok */
}

static int
bearing_num (int b)
{
  /* Add offset &c */

  if (setup_wv_reverse)
    return 1 + (16 - b + setup_wv_offet) % 16;
  else
    return 1 + (16 + b + setup_wv_offet) % 16;
}

//static int same_day(struct tm *tm1, struct tm *tm2)
//{
//  return ((tm1->tm_yday == tm2->tm_yday) && (tm1->tm_year == tm2->tm_year));
//}

static void weather_check_auto_reset (int reset_type)
{
  struct tm *nreset;
  time_t next_t;

  // Was reset_time set yet?

  if (reset_time == 0)
  {
	// Perhaps we have a rain or gpc reset registered?

	int i;

	if (devices_have_rain())
	{
	  for (i=0; i<8; ++i)
	  {
		if (ws.t_rain[i] > reset_time)
		{
		  reset_time = ws.t_rain[i];
		  werr(WERR_DEBUG0, "reset_time set by t_rain[%d] - %s", i, asctime(localtime(&reset_time)));
		}
	  }
	}

	for (i=0; i<MAXGPC; ++i)
	{
	  if (devices_have_gpc (i))
	  {
		if (ws.gpc[i].time_reset > reset_time)
		{
		  reset_time = ws.gpc[i].time_reset;
		  werr(WERR_DEBUG0, "reset_time set by gpc[%d] - %s\n", i, asctime(localtime(&reset_time)));
		}
	  }
	}
  }

  // Get the broken-down time of the previous reset
  // We will manipulate this to get the next reset
  nreset = localtime(&reset_time);

  // Resets come on the reset hour
  nreset->tm_hour = setup_rainreset_hour;
  nreset->tm_min = nreset->tm_sec = 0;

  // For non-daily resets, manipulate the date in the month
  if (reset_type == rain_reset_weekly)
  {
	// Set nominal month date according to reset week day
	nreset->tm_mday -= setup_rainreset_day - nreset->tm_wday;
  }
  else if (reset_type == rain_reset_monthly)
  {
	// Set nominal month date according to reset date
	nreset->tm_mday = setup_rainreset_date;
  }

  // Depending on when the previous reset occurred this could refer to
  // the previous reset or the next one. If previous, set next one.

  // Convert broken-down time back into unix time stamp
  next_t = mktime(nreset);

  if (next_t < reset_time)
  {
	// This was a late reset - Advance to next reset
	switch (reset_type)
	{
	case rain_reset_manual:
	  // Never called?
	  return;

	case rain_reset_daily:
	  // Add a day
	  next_t += 86400;
	  break;

	case rain_reset_weekly:
	  // Add 7 days
	  next_t += 604800;
	  break;

	case rain_reset_monthly:
	  // Add one month - time in seconds depends on month
	  ++nreset->tm_mon;
	  next_t = mktime(nreset);
	  break;
	}
  }

  // Now test current time against next reset time

  if (ws.t >= next_t)
  {
	werr (WERR_DEBUG0, "Time triggered automatic weather statistics reset");
//	printf("Time triggered automatic weather statistics reset\n");
	weather_reset_stats ();
  }

  return;
}

int
weather_read_ws_read_gpc (void)
{
  /*
   *
   * Now read GPC
   *
   */

  assert((ws.gpcnum>=0)&&(ws.gpcnum<MAXGPC));

  if (devices_have_gpc (ws.gpcnum))
  {
    werr (WERR_DEBUG0, "Reading GPC %d", ws.gpcnum);
    /* We claim to have this GPC. Read it. */

    /* Record the previous GPC count */
    ws.gpc[ws.gpcnum].count_old = ws.gpc[ws.gpcnum].count;

    /* Update the GPC count */
    if (ReadCounter_robust (devices_GPC1 + ws.gpcnum,
                            15,
                            &(ws.gpc[ws.gpcnum].count),
                            GPC_TRIES))
    {
      /* GPC counter read OK */

      /* Catch first run to avoid stupid GPC delta */
      if (ws.gpc[ws.gpcnum].count_old == 0) {	/* i.e. program startup */
        ws.gpc[ws.gpcnum].count_old =
          ws.gpc[ws.gpcnum].count;
      }

      /* As suggested by Steinar */
      ws.gpc[ws.gpcnum].count_delta =
        (ws.gpc[ws.gpcnum].count -
         ws.gpc[ws.gpcnum].count_old) & 0xffffffffUL;

      ws.gpc[ws.gpcnum].delta =
        ws.gpc[ws.gpcnum].count_delta *
        devices_list[devices_GPC1 +
                     ws.gpcnum].calib[0];

      ws.gpc[ws.gpcnum].time_delta =
        (ws.gpc[ws.gpcnum].time) ? ws.t -
        ws.gpc[ws.gpcnum].time : 0;
      ws.gpc[ws.gpcnum].time = ws.t;

      /* If we have a delta and a time_delta use these for rate */

      ws.gpc[ws.gpcnum].rate =
        ((ws.gpc[ws.gpcnum].delta > 0) &&
         (ws.gpc[ws.gpcnum].time_delta > 0)) ?
        (float) ws.gpc[ws.gpcnum].delta /
        (float) ws.gpc[ws.gpcnum].time_delta : 0.0F;

      /* Calculate increment since reset */
      ws.gpc[ws.gpcnum].gpc =
        devices_list[devices_GPC1 +
                     ws.gpcnum].calib[0] *
        (float) (ws.gpc[ws.gpcnum].count -
                 ws.gpc[ws.gpcnum].count_offset);

      // Use the final subcount delta scratch value for this update - will go in stats
      ws.gpc[ws.gpcnum].max_sub_delta = devices_list[devices_GPC1 + ws.gpcnum].calib[0] *
          ws.gpc[ws.gpcnum].max_subcount_delta_scratch ;

      // and reset it
      ws.gpc[ws.gpcnum].max_subcount_delta_scratch = 0L ;
    }

    /* Ignore failed readings */

  }
  else
  {
    ws.gpc[ws.gpcnum].gpc = -1.0F;
    ws.gpc[ws.gpcnum].rate = ws.gpc[ws.gpcnum].delta = 0.0F;
  }

  ++ws.gpcnum;
  if (ws.gpcnum == MAXGPC)
  {
    /* All GPCs read */
    ws.gpcnum = 0;

    /* Next do the anemometer */
    //state_new (state_readanemend);

    weather_next_device(state_read_gpc) ;
  } //else return weather_read_ws_read_gpc();

  return -1;
}

void
weather_sub_update_gpcs (void)
{
  /*
   *
   * Now read GPC
   *
   */

  int gpcnum ;

  for (gpcnum=0; gpcnum<MAXGPC; ++gpcnum)
  {
    if (devices_have_gpc (gpcnum))
    {
      werr (WERR_DEBUG0, "Reading GPC %d (sub-update)", gpcnum);
      /* We claim to have this GPC. Read it. */

      /* Record the previous GPC count */
      ws.gpc[gpcnum].subcount_old = ws.gpc[gpcnum].subcount;

      /* Update the GPC count */
      if (ReadCounter_robust (devices_GPC1 + gpcnum,
                              15,
                              &(ws.gpc[gpcnum].subcount),
                              GPC_TRIES))
      {
        /* GPC counter read OK */

        /* Catch first run to avoid stupid delta */
        if (ws.gpc[gpcnum].subcount_old == 0) {	/* i.e. program startup */
          ws.gpc[gpcnum].subcount_old = ws.gpc[gpcnum].subcount;
          ws.gpc[gpcnum].event_count = 0;
        }

        /* As suggested by Steinar */
        ws.gpc[gpcnum].subcount_delta =
          (ws.gpc[gpcnum].subcount -
          ws.gpc[gpcnum].subcount_old) & 0xffffffffUL;

        if (ws.gpc[gpcnum].subcount_delta != 0)
        {
          ++ws.gpc[gpcnum].event_count ;
          if (ws.gpc[gpcnum].subcount_delta > ws.gpc[gpcnum].max_subcount_delta_scratch)
          ws.gpc[gpcnum].max_subcount_delta_scratch = ws.gpc[gpcnum].subcount_delta ;
        }

      }
      else
      {
        ws.gpc[gpcnum].subcount_old = ws.gpc[gpcnum].subcount;
        ws.gpc[gpcnum].subcount_delta = 0L ;
      }

      applctn_quick_poll(0) ;
    }
  }
}

int
weather_read_wsgust (void)
{
  double revolution_sec;
  int p ;
  p = devices_portnum(devices_anem) ;

  //werr(WERR_DEBUG0, "weather_read_wsgust called");

  /* Get the 3-s updated anemometer count */
  /* read the counter again */

  /* Make sure we have an anemometer */
  if (devices_list[devices_anem].id[0] == COUNT_FAMILY)
  {
    if (!ReadCounter_robust (devices_anem, 15,
                             &(ws.gust_end_count), ANEM_TRIES))
    {
      werr (WERR_WARNING + WERR_AUTOCLOSE,
            "Error reading anemometer (end). Device present? : %s",
            owVerify (p, FALSE) ? "Yes" : "No");
      ws.gust_end_count = ws.gust_start_count;	/* Make safe */
      //state_new (state_wsdead);
      return 0;
    }
    /* get a time stamp (mS) */
    ws.gust_end_time = msGettick ();

    /* If the count went backwards, assume we lost power at the start */
    if (ws.gust_end_count < ws.gust_start_count)
      ws.gust_start_count = 0;

    if (ws.gust_end_time > ws.gust_start_time)
    {
      /* calculate the wind speed based on the revolutions per second */
      revolution_sec = (((double)
                         (ws.gust_end_count -
                          ws.gust_start_count) * 1000.0) /
                        (double) (ws.gust_end_time -
                                  ws.gust_start_time)) /
                       2.0;
      if (revolution_sec >= 0)
      {
        if ((revolution_sec >= 0)
            && (revolution_sec * 2.453 < 200.0))
        {
          ws.anem_speed =
            (float) revolution_sec
            *2.453F;

          /* Allow for calibration */
          ws.anem_speed = ws.anem_speed *
                          devices_list[devices_anem].
                          calib[0];

          if (ws.anem_speed > ws.anem_speed_max)
            ws.anem_speed_max =
              ws.anem_speed;
          if (ws.anem_speed > ws.anem_int_gust_scratch)
            ws.anem_int_gust_scratch =
              ws.anem_speed;
          //werr(WERR_DEBUG0, "%f", ws.anem_speed) ;
        }
        else
        {
          werr (WERR_DEBUG0,
                "Crazy anem speed");
        }
      }
    }
    /* Convert mph to m/s */
    ws.anem_mps = ws.anem_speed * 0.447040972F;

    /* Update gust list and ws.anem_gust */
    arne_new_wind_speed (&ws);

    ws.gust_start_time = ws.gust_end_time ;
    ws.gust_start_count = ws.gust_end_count ;
  }

  return 1;		/* Done - Ok */
}

static int
weather_read_ws_anem_end (void)
{
  double revolution_sec;
  int newdir;
  int p ;

  p = devices_portnum(devices_anem) ;

  /* Get the final anemometer count */
  /* read the counter again */

  /* Make sure we have an anemometer */
  if (devices_have_wsi603a())
  {
	// Handle all the anem stats here
  }
  else
  {
	if (devices_list[devices_anem].id[0] == COUNT_FAMILY)
	{
	  if (!ReadCounter_robust (devices_anem, 15,
		  &(ws.anem_end_count), ANEM_TRIES))
	  {
		werr (WERR_WARNING + WERR_AUTOCLOSE,
			"Error reading anemometer (end). Device present? : %s",
			owVerify (p, FALSE) ? "Yes" : "No");
		ws.anem_end_count = ws.anem_start_count;	/* Make safe */
		//state_new (state_wsdead);
		return 0;
	  }
	  /* get a time stamp (mS) */
	  ws.anem_end_time = msGettick ();

	  /* If the count went backwards, assume we lost power at the start */
	  if (ws.anem_end_count < ws.anem_start_count)
		ws.anem_start_count = 0;

	  if (ws.anem_end_time > ws.anem_start_time)
	  {
		/* calculate the wind speed based on the revolutions per second */
		revolution_sec = (((double)
			(ws.anem_end_count -
				ws.anem_start_count) * 1000.0) /
				(double) (ws.anem_end_time -
					ws.anem_start_time)) /
					2.0;
		if (revolution_sec >= 0)
		{
		  if ((revolution_sec >= 0)
			  && (revolution_sec * 2.453 < 200.0))
		  {
			// Reset gust values for fresh interval
			ws.gust_start_count = ws.anem_end_count ;
			ws.gust_start_time = ws.anem_end_time ;
			// Take the final scratch value of the interval gust readings
			ws.anem_int_gust = ws.anem_int_gust_scratch ;
			// Reset scratch value for future gust readings
			ws.anem_int_gust_scratch = 0 ;

			ws.anem_speed =
			  (float) revolution_sec
			  *2.453F;

			/* Allow for calibration */
			ws.anem_speed = ws.anem_speed *
			devices_list[devices_anem].
			calib[0];

			if (ws.anem_speed > ws.anem_int_gust)
			  ws.anem_int_gust =
				ws.anem_speed;
			if (ws.anem_speed > ws.anem_speed_max)
			  ws.anem_speed_max =
				ws.anem_speed;
			//werr(WERR_DEBUG0, "%f %f", ws.anem_speed, ws.anem_int_gust) ;
		  }
		  else
		  {
			werr (WERR_DEBUG0,
				"Crazy anem speed");
		  }
		}
	  }
	}

	/* read the wind direction if we have it */
	if (devices_have_vane ())
	{
	  /* Which type of vane? */
	  if (devices_have_wsi603a())
	  {
		// Results already read in
	  }
	  else if (devices_list[devices_vane_adc].id[0] == ATOD_FAMILY)
	  {
		/* B-type */
		if ((newdir =
		  ReadWindDirection_ADC (devices_vane_adc)) != -1)
		  ws.vane_bearing = newdir;
		else
		{
		  werr (WERR_DEBUG0,
			  "ReadWindDirection_ADC failed (1st go)");
		  // Try again
		  if ((newdir =
			ReadWindDirection_ADC (devices_vane_adc)) != -1)
			ws.vane_bearing = newdir;
		  else
		  {
			werr (WERR_WARNING,
				"ReadWindDirection_ADC failed (tried twice)");
		  }
		}
	  }
	  else if (devices_list[devices_vane_adc].id[0] == SBATTERY_FAMILY )
	  {
		/* ADS-DS2438-type */
		if (weather_read_ws_read_ad26vane(&newdir) != -1)
		{
		  ws.vane_bearing = newdir;
		}
		else
		{
		  werr (WERR_DEBUG0,
			  " weather_read_ws_read_vane failed (1st go)");
		  // Try again
		  if (weather_read_ws_read_ad26vane(&newdir) != -1)
		  {
			ws.vane_bearing = newdir;
		  }
		  else
		  {
			werr (WERR_WARNING,
				" weather_read_ws_read_vane failed (tried twice)");
		  }
		}
	  }
	  else if (devices_list[devices_vane_ads].id[0] == SBATTERY_FAMILY )
	  {
		/* HobbyBoards ADS Type Vane */
                /* Added a calibration for this..-15 - 15, changes the cardinal +/- for North. */
		if (weather_read_ws_read_adsvane(&newdir) != -1)
		{
		  ws.vane_bearing = (newdir+devices_list[devices_vane_ads].calib[0]);
		  if (ws.vane_bearing > 16) ws.vane_bearing -= 16 ;
                  if (ws.vane_bearing < 1) ws.vane_bearing += 16 ;
		}
		else
		{
		  werr (WERR_DEBUG0,
			  " weather_read_ws_read_vane (ADS) failed (1st go)");
		  // Try again
		  if (weather_read_ws_read_adsvane(&newdir) != -1)
		  {
		        ws.vane_bearing = (newdir+devices_list[devices_vane_ads].calib[0]);
		        if (ws.vane_bearing > 16) ws.vane_bearing -= 16 ;
                        if (ws.vane_bearing < 1) ws.vane_bearing += 16 ;
		  }
		  else
		  {
			werr (WERR_WARNING,
				" weather_read_ws_read_vane (ADS) failed (tried twice)");
		  }
		}
	  }
	  else if (devices_list[devices_vane_ins].id[0] == SBATTERY_FAMILY )
	  {
		/* HobbyBoards Inspeed Type Vane */
                /* Added a calibration for this..-15 - 15, changes the cardinal +/- for North. */
		if (weather_read_ws_read_insvane(&newdir) != -1)
		{
		  ws.vane_bearing = (newdir+devices_list[devices_vane_ins].calib[0]);
		  if (ws.vane_bearing > 16) ws.vane_bearing -= 16 ;
                  if (ws.vane_bearing < 1) ws.vane_bearing += 16 ;
		}
		else
		{
		  werr (WERR_DEBUG0,
			  " weather_read_ws_read_vane (INS) failed (1st go)");
		  // Try again
		  if (weather_read_ws_read_insvane(&newdir) != -1)
		  {
		        ws.vane_bearing = (newdir+devices_list[devices_vane_ins].calib[0]);
		        if (ws.vane_bearing > 16) ws.vane_bearing -= 16 ;
                        if (ws.vane_bearing < 1) ws.vane_bearing += 16 ;
		  }
		  else
		  {
			werr (WERR_WARNING,
				" weather_read_ws_read_vane (INS) failed (tried twice)");
		  }
		}
	  }
	  else
	  {
		/* A_type */
		if ((newdir = ReadWindDirection (devices_vane)) != -1)
		  ws.vane_bearing = newdir;
		else
		{
		  /* Try again */
		  if ((newdir =
			ReadWindDirection (devices_vane)) != -1)
			ws.vane_bearing = newdir;
		  else
			werr (WERR_DEBUG0,
				"Wind direction not found");
		}
	  }
	}
  }

  /*
   *
   * Now measure the rain fall
   *
   */

  /* Check for auto reset before measuring the rain
   * We have the anem speed and temperature for _now_
   * Must count any extra rainfall since last value in the next result;
   * so reset first */
  if (setup_rainreset != rain_reset_manual)
    weather_check_auto_reset (setup_rainreset);

  if (devices_have_rain ())
  {
    /* We claim to have a rain gauge. Read it. */

    int counts_since, counts_today, counts_per_sec;

    /* Record the previous monotonic rain count */
    ws.rain_count_old = ws.rain_count;

    /* Update the monotonic rain count */
    if (ReadCounter_robust
        (devices_rain, 15, &(ws.rain_count), RAIN_TRIES))
    {
      /* Rain counter read OK */
      int resetDay, nowDay;

      if (setup_rain_backrst
          && (ws.rain_offset[0] > ws.rain_count))
      {
        /* The counter has gone backwards! */
        werr(WERR_WARNING+WERR_AUTOCLOSE,
             "Rain gauge has gone backwards!") ;
        ws.rain = 0 ;
        weather_reset_dailyrain ();
        weather_reset_rain ();
        setup_save_stats ();
      }

      resetDay = localtime(&(ws.t_dailyrain))->tm_yday;
      nowDay = localtime(&(ws.t))->tm_yday;

      // If now is a different (local time) day from the reset day, reset now
      // If dailyrain_offset is zero, assume we didn't read anything from the stats file and do a reset now
      if ((ws.t == 0) || (resetDay != nowDay) || (ws.dailyrain_offset == 0L))
        weather_reset_dailyrain ();

      /* Calculate rainfall since reset */
      counts_since = ws.rain_count - ws.rain_offset[0];
      ws.rain = RAINCALIB * (float) counts_since;
      //werr(WERR_DEBUG0, "Rain count %d, offset %d, since, %d, value %f, calib %f", ws.rain_count, ws.rain_offset[0], counts_since, ws.rain, RAINCALIB);

      counts_today = ws.rain_count - ws.dailyrain_offset;
      ws.dailyrain = RAINCALIB * (float) counts_today;

      /* Catch first run to avoid stupid rain delta */
      if (ws.rain_count_old == COUNT_NOT_SET)	/* i.e. program startup */
        ws.rain_count_old = ws.rain_count;

      /* Catch old>new - wrap around? */
      if (ws.rain_count_old > ws.rain_count)
      {
        ws.rain_count_old = ws.rain_count;
        ws.dailyrain = ws.rain = 0.0F;
      }

      /* Catch crazy count rate */
  	counts_per_sec = (ws.delta_t > 0) ? (int) (ws.rain_count - ws.rain_count_old) / ws.delta_t : 0;
  	if (counts_per_sec > 2) // Unphysical condition
  	{
  	  werr(WERR_WARNING | WERR_AUTOCLOSE, "Rain gauge count rate too high!");
  	  ws.dailyrain = ws.rain = 0 ;
  	  ws.rain_count_old = ws.rain_count;
  	  weather_reset_dailyrain ();
  	  weather_reset_rain ();
  	  setup_save_stats ();
  	}

  	/* Fresh rainfall detected? */
  	if (ws.rain_count > ws.rain_count_old)
      {
  	  // Crazy rates will be treated as single tip
        rainint_new_rain (&rainlocal, &ws);
        rainint_new_rain (&rainlocal24, &ws);
      }

      ws.rainint1hr = rainint_readout(&rainlocal);
      ws.rainint24hr = rainint_readout(&rainlocal24);

      ws.rain_rate = rainint_rate (&rainlocal, &ws);
    }

    /* Ignore failed readings */

  }
  else
  {
    /* No rain gauge */
    /* Have we lost one? */
    if (ws.rain != -1.0F)
      werr (WERR_DEBUG0, "Lost rain gauge");

    ws.rain = -1.0F;
    ws.rain_rate = 0.0F;
  }

  state_new (state_read_ws_done);

  return 1;		/* OK */
}

/* How many thermometers do we have? */
/* Returns the number of thermometers */

static int
count_thermometers (void)
{
  int Ti = 0, TN = 0;

  for (Ti = 0; Ti < MAXTEMPS+MAXSOILTEMPS+MAXINDOORTEMPS; ++Ti)
    if (devices_have_thermometer (Ti))
      ++TN;

  return TN;
}

int
weather_read_ws (void)
{
  time_t old_time;
  static int NumTemps = 0, sensor = 0;
  int rv;

  /* Call WS read function for current state */

  switch (prog_state)
  {
  case state_readanemstart:
    NumTemps = count_thermometers ();
    sensor = 0;
    old_time = ws.t;

    time (&ws.t);	/* Note the Unix time */
    if (old_time)
    {
      /* Legal time recorded */
      ws.delta_t = ws.t - old_time;
    }
    else
    {
      /* Program start? Use rain reset values */
      ws.delta_t = ws.t - ws.t_rain[0];
      ws.rain_count_old = ws.rain_offset[0];
    }
    /* Possibly we just passed over midnight? */
    return weather_read_ws_anem_start ();
    break;

  case state_starttempconv:
    return weather_read_ws_start_tempconv ();

  case state_endtempconv:
    return weather_read_ws_end_tempconv ();

  case state_read_humidity:
    if (weather_read_ws_read_humidity () == 1)
      return 1;	/* Ok */
    /* Try again */
    return (weather_read_ws_read_humidity () == 1);

  case state_read_barometer:
    if (weather_read_ws_read_barom () == 1)
      return 1;	/* Ok */
    /* Try again */
    return (weather_read_ws_read_barom () == 1);

  case state_read_gpc:
    return weather_read_ws_read_gpc () ;

  case state_read_solar:
    return weather_read_ws_read_solar ();

  case state_read_uv:
    return weather_read_ws_read_uv ();

  case state_read_adc:
    return weather_read_ws_read_adc ();

  case state_read_tc:
    return weather_read_ws_read_thrmcpl ();

  case state_read_moist:
    return weather_read_ws_read_moist();

  case state_readanemend:
      return weather_read_ws_anem_end ();

  case state_read_ws_done:
  {
    float T;

    T = weather_primary_T (NULL);
          if(T > ws.Tmax)
              ws.Tmax = T;

          else
              if(T < ws.Tmin)
                  ws.Tmin = T;


          /* Update Arne statistics */
          if(!arne_wind_ready && (arne_init_wind() < 0))
              werr(0, "arne_init_wind failed");

          /* Update wind direction matrix */
          arne_new_wind_dir(ws.vane_bearing - 1, ws.t);
          /* Convert mph to m/s */
          ws.anem_mps = ws.anem_speed * 0.447040972F;
          /* Update gust list and ws.anem_gust */
          arne_new_wind_speed(&ws);
          /* Update ws.vane_mode */
          ws.vane_mode = arne_read_out_wind_dir();
          /* We are local, so copy lat/long */
          ws.latitude = setup_latitude;
          ws.longitude = setup_longitude;
          arne_tx(&ws);
          oww_trx_tx (&ws, OWW_TRX_MSG_WSDATA);
          txtserve_tx(&ws);
          return 1; // Ok
  }default:
      werr(0, "Program Error: Arrived at weather_read_ws in wrong state - %s", state_get_name(prog_state));
      break;
  }

  return 1;
}

/*----------------------------------------------------------------------
   Search for the devices required for a Weather Station:

    - DS1820
    - DS2423
    - DS2407 Vane switch or...
    - DS2450 ADC wind vane

  Optional:
    - DS2423 for rain gauge (SJM)
    - DS2438 Humidity sensor (SJM)
    - DS2438 barometer
    - DS2406 barometer (AAG TAI-8570)

   Returns: TRUE(1)  success, all required device types found
            FALSE(0) all required devices not found
*/
int
FindWeatherStation (void)
{
  int			/*first_switch=-1,
  				 * last_switch=-1,
  				 * first_adc=-1,
  				 * last_adc=-1, */
  i, have_vane_switch = 0, have_vane_adc = 0;

  /*devices_purge_search_list() ; */
  devices_clear_search_list ();

  /*weather_clear_wind_vanes() ; */

  /* Check for branches */
  devices_search_and_add (BRANCH_FAMILY, 1);

  /* Search for all devices */
  devices_search_and_add (0, 0);

  /* Add back in any known ROM IDs */
  devices_build_search_list_from_devices (0);

  /* Zap any IDs we found before, but which have now vanished */
  devices_purge_devices_list ();

  /* Loop over all the devices that were found */
  for (i = 0; i < devices_known; ++i)
  {
    werr (WERR_DEBUG1, "Alloc check %d", i);
    if (devices_search_list[i].alloc != -1)
      werr (WERR_DEBUG1, "Alloced to %s",
            devices_list[devices_search_list[i].alloc].
            menu_entry);
    /* Action depends on device type */
    switch (devices_search_list[i].id[0])
    {
    case SWITCH_FAMILY:
      if (!have_vane_switch && !have_vane_adc)
      {
        if ((devices_search_list[i].alloc != -1) &&
            (devices_search_list[i].alloc !=
             devices_vane))
        {
          /* Already allocated to something else - TAI8570? */
          devices_list[devices_search_list[i].
                       alloc].branch =
                         devices_search_list[i].branch;
          werr (WEATHER_DBLEV,
                "Search list entry %d is a switch, but it was allocated already",
                i);
        }
        else
        {
          /* This might be a vane switch */
          /* Let's assume it is */
          devices_allocate (devices_vane, i);
          if (-1 !=
              ReadWindDirection (devices_vane))
          {
            /* Found a vane ID chip */
            /* So this must be the right switch */
            /*devices_allocate(devices_vane, i) ; */
            werr (WEATHER_DBLEV,
                  "Search list entry %d seems to be the Vane switch",
                  i);
            have_vane_switch = 1;
          }
          else
          {
            /* No joy. Deallocate */
            werr (WEATHER_DBLEV,
                  "Search list entry %d is not the Vane switch",
                  i);
            devices_queue_realloc(i, NULL, 0, NULL);
            devices_auto_allocate (i);
          }
        }
      }
      else	/* Already have a wind vane of some sort */
      {
        if (-1 == devices_search_list[i].alloc)
          devices_auto_allocate (i);
        else
          /* Just update branch info */
          devices_list[devices_search_list[i].
                       alloc].branch =
                         devices_search_list[i].branch;
      }
      break;

    case ATOD_FAMILY:
      if (!have_vane_switch && !have_vane_adc)
      {
        if ((devices_search_list[i].alloc != -1) &&
            (devices_search_list[i].alloc !=
             devices_vane_adc))
        {
          /* Already allocated to something else? */
          devices_list[devices_search_list[i].
                       alloc].branch =
                         devices_search_list[i].branch;
          continue;
        }

        /* Let's assume this is the vane ADC */
        devices_allocate (devices_vane_adc, i);
        if (-1 !=
            ReadWindDirection_ADC (devices_vane_adc))
        {
          /*devices_allocate(devices_vane_adc, first_adc) ; */
          werr (WEATHER_DBLEV,
                "Allocating ADC device %d", i);
          have_vane_adc = 1;
        }
        else
        {
          /* No joy. Deallocate */
          werr (WEATHER_DBLEV,
                "Search list entry %d is not the Vane ADC",
                i);
          devices_queue_realloc(i, NULL, 0, NULL);
        }
      }
      break;

    case BRANCH_FAMILY:
      /* Already allocated */
      break;

    case TEMP_FAMILY:
    case TEMP_FAMILY_DS1822:
    case TEMP_FAMILY_DS18B20:
    case SBATTERY_FAMILY:
    case COUNT_FAMILY:
    default:
      if (-1 == devices_search_list[i].alloc)
        devices_auto_allocate (i);
      else
        /* Just update branch info */
        devices_list[devices_search_list[i].alloc].
        branch =
          devices_search_list[i].branch;
      break;
    }
  }

//  weather_check_required ();

  tai8570_check_alloc ();

  return devices_have_something();
}

void
weather_lcd_output()
{
  int i, len = 0 ;
  char *message = NULL ;

  if (devices_lcd1 == -1) return ;

  message = tai8590_message(&ws, &len) ;

  if (len <= 0) return ;

  for (i=0; i<MAXLCD; ++i)
    if (devices_have_lcd(i))
      tai8590_SendMessage(devices_lcd1+i, message, 0) ;
}

/*----------------------------------------------------------------------
   Read the counter on a specified page of a DS2423.

   'SerialNum'   - Serial Number of DS2423 that contains the counter
                   to be read
   'CounterPage' - page number that the counter is associated with
   'Count'       - pointer to variable where that count will be returned

   Returns: TRUE(1)  counter has been read and verified
            FALSE(0) could not read the counter, perhaps device is not
                     in contact
*/
static int
ReadCounter ( /*uchar SerialNum[8] */ int device,
                                      int CounterPage, uint32_t *Count)
{
  int rt = FALSE;
  uchar send_block[30];
  int send_cnt = 0, address, i;
  ushort lastcrc16;
  int p ;
  p = devices_portnum(device) ;
  setcrc16(p,0);

  /* Set up to access device */
  if (devices_access (device))
  {
    /* create a block to send that reads the counter */
    /* read memory and counter command */
    send_block[send_cnt++] = 0xA5;
    lastcrc16 = docrc16 (p, 0xA5);
    /* address of last data byte before counter */
    address = (CounterPage << 5) + 31;	/* (1.02) */
    send_block[send_cnt++] = (uchar) (address & 0xFF);
    lastcrc16 = docrc16 (p, address & 0xFF);
    send_block[send_cnt++] = (uchar) (address >> 8);
    lastcrc16 = docrc16 (p, address >> 8);
    /* now add the read bytes for data byte,counter,zero bits, crc16 */
    for (i = 0; i < 11; i++)
      send_block[send_cnt++] = 0xFF;

    /* now send the block */
    if (owBlock (p, FALSE, send_block, send_cnt))
    {
      /* perform the CRC16 on the last 11 bytes of packet */
      for (i = send_cnt - 11; i < send_cnt; i++)
        lastcrc16 = docrc16 (p, send_block[i]);

      /* verify CRC16 is correct */
      if (lastcrc16 == 0xB001)
      {
        /* success */
        rt = TRUE;
        /* extract the counter value */
        *Count = 0;
        for (i = send_cnt - 7; i >= send_cnt - 10;
             i--)
        {
          *Count <<= 8;
          *Count |= send_block[i];
        }
      }
    }
    else
    {
      werr (WERR_DEBUG0,
            "ReadCounter - owBlock failed: %s",
            devices_list[device].menu_entry);
    }
  }

  /* return the result flag rt */
  return rt;
}


static int
FindFromList (uchar SearchID[], devices_struct * list, int known)
{
  int i, j, ok;

  /* Search each list entry */
  for (j = 0; j < known; ++j)
  {
    ok = 1;
    /* Check each byte of ID */
    for (i = 0; i < 8; ++i)
    {
      if (SearchID[i] != list[j].id[i])	/* Mismatch? */
      {
        ok = 0;
        break;
      }
    }
    /* No mismatch found? */
    if (ok)
      return j;
  }

  /* If we get to here the ID is not on the list */
  return -1;
}

static int
find_adc_bearing (float val[])
{
  float m, a, b, c, d;

  /* Find maximum */
  m = val[0];
  if (val[1] > m)
    m = val[1];
  if (val[2] > m)
    m = val[2];
  if (val[3] > m)
    m = val[3];

  /* Normalize readings */
  a = val[0] / m;
  b = val[1] / m;
  c = val[2] / m;
  d = val[3] / m;

  if (a < 0.26F)
  {
    if (b < 0.505F)
    {
      return 11;
    }
    else
    {
      if (d < 0.755F)
        return 13;
      else
        return 12;
    }
  }
  else
  {
    if (b < 0.26F)
    {
      if (c < 0.505F)
        return 9;
      else
        return 10;
    }
    else
    {
      if (c < 0.26F)
      {
        if (d < 0.505F)
          return 7;
        else
          return 8;
      }
      else
      {
        if (d < 0.26F)
        {
          if (a < 0.755F)
            return 5;
          else
            return 6;
        }
        else
        {
          if (d < 0.84F)
          {
            if (c < 0.84F)
              return 15;
            else
              return 14;
          }
          else
          {
            if (a < 0.845F)
            {
              if (b < 0.845F)
                return 3;
              else
                return 4;
            }
            else
            {
              if (b > 0.84F)
              {
                return 0;
              }
              else
              {
                if (c >
                    0.845F)
                  return 2;
                else
                  return 1;
              }
            }
          }
        }
      }
    }
  }
}

static int
ReadWindDirection_ADC (int vane)
{
  uchar ctrl[16] = "";
  char message[64] ;
  int p ;
  p = devices_portnum(vane) ;

  if (!devices_access (vane))
    return -1;

  /* Set up ADC */
  if (SetupAtoDControl (p, devices_list[vane].id, &ctrl[0], message))
  {
    if (WriteAtoD (p, 0,
                   devices_list[vane].id, &ctrl[0], 0x08, 0x11))
    {
      /* Read ADC */
      if (DoAtoDConversion (p, 0, devices_list[vane].id))
      {
        float prslt[4] = { 0.0F, 0.0F, 0.0F, 0.0F };

        if (ReadAtoDResults (p, 0,
                             devices_list[vane].id,
                             &prslt[0], &ctrl[0]))
        {
          return bearing_num (find_adc_bearing (prslt));
        }
        else
        {
          werr (WERR_WARNING + WERR_AUTOCLOSE,
                "Error reading channel, verify device present: %d",
                owVerify (devices_portnum(vane), FALSE));
        }
      }
      else
      {
        werr (WERR_WARNING + WERR_AUTOCLOSE,
              "DoAtoDConversion failed");
      }
    }
    else
    {
      werr (WERR_WARNING + WERR_AUTOCLOSE,
            "WriteAtoD failed");
    }
  }
  else
  {
    werr (WERR_WARNING + WERR_AUTOCLOSE,
          "SetupAtoDControl failed");
  }

  /* Return BAD result */

  return -1;
}


static int
ReadWindDirection ( /*uchar SwitchID[8] */ int vane)
{
  int dira = -1, dirb = -1, i, found = 0;
  uchar DirSN[MAX_DEVS_SEARCH][8];
  int p ;
  p = devices_portnum(vane) ;

  memset ((void *) DirSN, 0, MAX_DEVS_SEARCH * 8 * sizeof (uchar));

  /* Access the Vane Switch - Vane IDs are behind this */

  if (!devices_access (vane))
  {
    werr (WERR_AUTOCLOSE, _("Could not access Vane Switch"));
    return -1;
  }

  /* connect channel B of DS2407 */
  /* There shouldn't be any trouble here, but may as well
   * use the robust function */
  if (!set_switch_robust (vane, SWITCH_AOFF_BON, SWITCH_TRIES))
  {
    werr (WERR_DEBUG0, "Connect channel B of DS2407 failed");
    return -1;
  }

  /* delay to allow presence pulse to proceed */
  msDelay (10);

  /* record all of DS2401's found */

  /* Trunk or branch? */
  if (devices_list[vane].branch == -1)
  {
    /* The vane switch is on the trunk, so perform a normal search */
    /* set the search to first find direction family codes DS2401 */
    owFamilySearchSetup (p, DIR_FAMILY);

    /* Conduct search */

    while (owNext (p, TRUE, FALSE))
    {
      /* verify the family code */
      owSerialNum (p, DirSN[found], TRUE);
      ++found;
    }
  }
  else
  {
    /* The vane switch is on a branch, so do a branch search */
    found = FindBranchDevice (p, devices_list
                              [DevicesBranchNum
                               (devices_list[vane].branch)].id,
                              DirSN, MAX_DEVS_SEARCH,
                              DevicesMainOrAux (devices_list
                                                [vane].branch));
  }

  /* We now have a list of devices, some of wich should be direction IDs */
  for (i = 0; i < found; ++i)
  {
    /* if custom DS2401 then skip */
    if (((DirSN[i][0] & 0x7F) == DIR_FAMILY)
        && ((DirSN[i][0] & 0x80) == 0x80))
      continue;

    /* check for correct DS2401 family code */
    if (DirSN[i][0] == DIR_FAMILY)
    {
      int listmem;

      listmem = FindFromList (DirSN[i],
                              &devices_list[devices_wv0],
                              wv_known);

      /* Is this a known vane ID? */
      if ((listmem == -1) && (wv_known < 8))
      {
        /* This ID is not on the list of wind vane IDs
         * but we are learning wind vane IDs */

        /* Learn this ID */
        /*AddToList(DirSN, &devices_list[devices_wv0], wv_known) ; */
        devices_new_search_entry (DirSN[i], -1, 1);

        /* Now we can treat this as a known ID */
        listmem = wv_known;

        ++wv_known;
      }

      if (listmem != -1)
      {
        /* A matching entry was found on the list */
        if (dira == -1)
        {
          /* This is the first ID we found from the list */
          dira = listmem;
        }
        else
        {
          /* This is the second we found from the list */
          dirb = listmem;

          /* Done with search */
          break;
        }
      }
    }
  }



  /* open channel B of DS2407 */
  if (!set_switch_robust (vane, SWITCH_AOFF_BOFF, SWITCH_TRIES))
  {
    werr (WERR_DEBUG0,
          "Failed to open the B channel of the DS2407");
    return FALSE;
  }

  if (dira == -1)
  {
    /* Didn't find any vane IDs - bad */
    return -1;
  }

  if (dirb == -1)
  {
    /* Just one ID */
    return bearing_num (dira * 2);
  }

  /* Two IDs */

  /* Check special cases */

  if ((dira == 0) && (dirb == 7))
    return bearing_num (15);

  if ((dirb == 0) && (dira == 7))
    return bearing_num (15);

  return bearing_num (dira + dirb);
}


/*----------------------------------------------------------------------
   Set the channel state of the specified DS2407

   'SerialNum'   - Serial Number of DS2407 to set the switch state
   'State'       - State value to be written to location 7 of the
                   status memory.
   'depth'       - Recursion depth for retries

   Returns: TRUE(1)  State of DS2407 set and verified
            FALSE(0) could not set the DS2407, perhaps device is not
                     in contact
*/
int
set_switch (int device, int State)
{
  uchar send_block[30];
  int send_cnt = 0;
  ushort lastcrc16;
  int p ;
  p = devices_portnum(device) ;

  setcrc16(p,0);

  if (devices_list[device].id[0] != SWITCH_FAMILY)
    return FALSE;

  /* access the device */
  if (devices_access (device))
  {
    /* write status command */
    send_block[send_cnt++] = 0x55;
    lastcrc16 = docrc16 (p, 0x55);
    /* address of switch state */
    send_block[send_cnt++] = 0x07;
    lastcrc16 = docrc16 (p, 0x07);
    send_block[send_cnt++] = 0x00;
    lastcrc16 = docrc16 (p, 0x00);
    /* write state */
    send_block[send_cnt++] = (uchar) State;
    lastcrc16 = docrc16 (p, State);
    /* read CRC16 */
    send_block[send_cnt++] = 0xFF;
    send_block[send_cnt++] = 0xFF;

    /* now send the block */
    if (owBlock (p, FALSE, send_block, send_cnt))
    {
      /* perform the CRC16 on the last 2 bytes of packet */
      lastcrc16 = docrc16 (p, send_block[send_cnt - 2]);
      lastcrc16 = docrc16 (p, send_block[send_cnt - 1]);

      /* verify CRC16 is correct */
      if (lastcrc16 == 0xB001)
        /* success */
        return (TRUE);

      werr (WERR_DEBUG0, "set_switch CRC error - %s",
            devices_list[device].menu_entry);
    }
    else
    {
      werr (WERR_DEBUG0,
            "set_switch - owBlock failed: %s",
            devices_list[device].menu_entry);
    }
  }
  return FALSE;
}

static int
set_switch_robust (int device, int State, int depth)
{
  while (depth >= 0)
  {
    if (set_switch (device, State) == TRUE)
      return TRUE;

    --depth;
    msDelay (10);
  }

  return FALSE;
}

/*----------------------------------------------------------------------
   Read and print the Serial Number.
*/
char *
PrintSerialNum (void)
{
  static char buff[24], *bp;
  uchar TempSerialNumber[8] = "";
  int i;

  bp = &buff[0];

  owSerialNum (devices_portnum_var, TempSerialNumber, TRUE);
  for (i = 7; i >= 0; i--)
    bp += sprintf (bp, "%02X", TempSerialNumber[i]);
  return buff;
}
