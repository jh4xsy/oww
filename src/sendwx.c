/* sendwx.c */

/*
   Dallas weather servlet protocol
   TX and RX

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#ifdef WIN32
#include "config-w32gcc.h"
#else
#include "../config.h"
#endif
#endif

#include <locale.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#include <string.h>
#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#include <time.h>

#include "werr.h"
#include "wstypes.h"
#include "setup.h"
#include "convert.h"
#include "url_get.h"
#include "stats.h"
#include "rainint.h"
#include "devices.h"
#include "globaldef.h"
#ifdef WIN32
#include "config-w32gcc.h"
#else
#include "config.h"
#endif
#include "parseout.h"
#include "weather.h"
#include "meteo.h"
#include "sendwx.h"
#include "cwop.h"
#include "intl.h"

#ifndef RISCOS
#  include "process.h"
/*  pid_t fork(void) ;
  int execl( const char *path, const char *arg, ...);*/
#else
#include "swis.h" /* RISC OS operating system calls */

#endif

extern wsstruct ws ;

#define DALLAS_OK " Data is received"
#define WUND_OK "success"
#define WOW_OK "200"
#define HAM_OK "Data Logged and posted"
/*#define WUND_OK "query=insert"*/
/* HTTP_TIMEOUT defined in globaldef.h */

static int test_strstr(char *, void*) ;
int sendwx_parse_dallas_rx(char *body, void *test_data) ;
/*static int sendwx_test_mode2(char *body, void *test_data) ;*/

/*static time_t dallas_time = 0 ; Time of last Dallas update */
/*static time_t wund_time = 0 ; Time of last Weather Underground update */
static time_t http_time ; /* Time of last http uploads */
static url_session dallas_session =
  {"",  test_strstr, DALLAS_OK, -1, NULL, -1, NULL, 0, 0, 0} ;
static url_session wund_session =
  {"",  test_strstr, WUND_OK, -1, NULL, -1, NULL, 0, 0, 0} ;
static url_session wow_session =
  {"",  test_strstr, WOW_OK, -1, NULL, -1, NULL, 0, 0, 0} ;
static url_session ham_session =
  {"",  test_strstr, HAM_OK, -1, NULL, -1, NULL, 0, 0, 0} ;
static url_session dallas_mode1_session =
  {"",  sendwx_parse_dallas_rx, (void *) &ws, -1, NULL, -1, NULL, 0, 0, 0} ;
static url_session dallas_mode2_session =
  {"",  NULL, NULL, -1, NULL, -1, NULL, 0, 0, 0} ;

statsstruct sendwxstats ;

extern time_t the_time ;

/* Variables for parser */
static char *httppostcom_buffer = NULL ;
static int httppostcom_alloced = 0 ;

sendwx_stn_info *sendwx_stn_list = NULL ; /* List of stations */
int sendwx_stn_list_count = 0 ; /* Number of stations in list */
char sendwx_dallasremote_name[STN_NAME_MAX] = "" ; /* Station name */
char sendwx_dallasremote_id[18] = "" ; /* Station id */

enum session_enum {
  session_failed = -1,
  session_open   = 0,
  session_none,
  session_finished } ;

enum wund_subtype {
  wund_normal,
  wund_rapid,
  wund_wow
};

char *old_locale = NULL;

/* Take copy of current locale and change to POSIX */
static void set_posix(void)
{
#ifndef RISCOS
  old_locale = strdup(setlocale(LC_NUMERIC, NULL));

  /* If that worked change to POSIX */
  if (old_locale != NULL)
  {
    setlocale (LC_NUMERIC, "POSIX");
  }
#endif
}

/* Go back to the old locale */
static void set_old_locale(void)
{
#ifndef RISCOS
  if (old_locale != NULL)
  {
    setlocale (LC_NUMERIC, old_locale);
    free(old_locale);
    old_locale = NULL;
  }
#endif
}

static int test_strstr(char *body, void *test_data)
{
  /* Simply check for text in body - return 1 if found */
  if (strstr(body, (char *) test_data)) return 1 ;

  return 0 ;
}

/* Add a station to the list */

static int
sendwx_add_stn(char *desc)
{
  char *name_start, *name_end = NULL ;


  werr(WERR_DEBUG1,
       "Parsing \"%s\"",
       desc
       ) ;

  /* Make space on list */

  sendwx_stn_list = (sendwx_stn_info *)
    realloc(sendwx_stn_list,
            (1 + sendwx_stn_list_count) * sizeof(sendwx_stn_info)) ;

  if (!sendwx_stn_list)
  {
    sendwx_stn_list_count = 0 ;
    return -1 ; /* Failure */
  }

  strncpy(sendwx_stn_list[sendwx_stn_list_count].id, desc, 16) ;
  sendwx_stn_list[sendwx_stn_list_count].id[16] = '\0' ;

  sendwx_stn_list[sendwx_stn_list_count].name[0] = '\0' ;

  name_start = strchr(&desc[16], '\"') ;

  if (name_start)
  {
    ++name_start ;

    name_end = strchr(name_start, '\"') ;

    if (name_end)
    {
      *name_end = '\0' ;
      strncpy(sendwx_stn_list[sendwx_stn_list_count].name,
              name_start,
              STN_NAME_MAX - 1) ;
      sendwx_stn_list[sendwx_stn_list_count].name[STN_NAME_MAX-1] = '\0' ;
    }
  }

  if (!name_start || !name_end)
  {
    strncpy(sendwx_stn_list[sendwx_stn_list_count].name,
            desc,
            16) ;
    sendwx_stn_list[sendwx_stn_list_count].name[16] = '\0' ;
  }

  werr(WERR_DEBUG1,
       "Added \"%s\"",
       sendwx_stn_list[sendwx_stn_list_count].name) ;

  ++sendwx_stn_list_count ;

  return 0 ; /* Ok */
}

/* Try to parse mode 2 response */

int
sendwx_parse_dallas_list(char *body, void *test_data)
{
  char *sc, *desc_start ;

  sendwx_stn_list_count = 0 ;

  desc_start = body ;

  while (desc_start)
  {
    sc = strchr(desc_start, ';') ;

    if (sc)
    {
      *sc = '\0' ;
      ++sc ;

      if (-1 == sendwx_add_stn(desc_start))
        return 0 ; /* Failed */
    }

    desc_start = sc ;
  }

  return 1 ;
}


/* Try to parse mode 1 response */

int
sendwx_parse_dallas_rx(char *body, void *test_data)
{
  static wsstruct remote_ws ;
  static rainlist remote_list = {NULL, 0, 0, RAININT_TIMEOUT_1HR} ;
  static rainlist remote_list24 = {NULL, 0, 0, RAININT_TIMEOUT_24HR} ;
  static int remote_rain = 0 ;

  char *start = NULL, *end = NULL ;
  int i ;

  //werr(WERR_DEBUG0, "RX: %s", body) ;

  set_posix();

  start = body ;

  devices_clear_remote() ;

  for (i=0; i<11; ++i)
  {
    if (!start)
    {
      set_old_locale();
      return 0 ; /* failed */
    }

    if (i<10) /* Find ';' and terminate there */
    {
      end = strchr(start, ';') ;
      if (end)
      {
        *end = '\0' ;
      }
      else
      {
        werr(WERR_WARNING, "Dallas data fetch failed") ;
        set_old_locale();
        return 0 ; /* Failed */
      }
    }

    /* Which value is this? */
    switch (i)
    {
      case 0: /* Station name */
        //werr(WERR_DEBUG0, "Station name: \"%s\"", start) ;
        if (0 == strcmp(start, "null"))
        {
          /* No name on Dallas database */
          strcpy(sendwx_dallasremote_name, setup_dallasremote_stn) ;
        }
        else
        {
          char *s1, *s2 ;

          s1 = strchr(start, '\"') ;
          if (s1)
          {
            ++s1 ;
            s2 = strchr(s1, '\"') ;
          }
          if (s1 && s2)
          {
            *s2 = '\0' ;
            strcpy(sendwx_dallasremote_name, s1) ;
          }
          else
          {
            //sendwx_dallasremote_name[0] = '\0' ;
             strcpy(sendwx_dallasremote_name, setup_dallasremote_stn) ;
          }
        }
        //werr(WERR_DEBUG0, "Parsed station name: \"%s\"", sendwx_dallasremote_name) ;
        break ;

      case 1: /* ID */
        //werr(WERR_DEBUG0, "ID: %s", start) ;
        /* Is this a different ID than last time? */
        if (strcmp(sendwx_dallasremote_id, start))
        {
          /* ID changed - reset rain stuff */
          memset(&remote_ws, 0, sizeof(wsstruct)) ;
          /*remote_ws.t_rain[0] = 0 ;
          remote_ws.rain = 0.0F ;
          remote_ws.rain_count = remote_ws.rain_offset[0] = 0UL ;*/
          rainint_wipe(&remote_list) ;
          rainint_wipe(&remote_list24) ;

          /* Remember this ID */
          strcpy(sendwx_dallasremote_id, start) ;
        }
        break ;

      case 2: /* Time */
        //werr(WERR_DEBUG0, "Time: %s", start) ;
        the_time = ws.t = (time_t) strtoul(start, NULL, 10) ;
        if (remote_ws.t == 0)
        {
          remote_ws.t = ws.t ;
          remote_ws.delta_t = 0 ;
        }
        else if (ws.t > remote_ws.t)
        {
          remote_ws.delta_t = ws.t - remote_ws.t ;
          remote_ws.t = ws.t ;
        }
        else
        {
          remote_ws.t = ws.t ;
          remote_ws.delta_t = 0 ;
        }

        break ;

      case 3: /* Temperature */
        //werr(WERR_DEBUG0, "Temperature: %s", start) ;
        ws.T[0] = (float) atof(start) ;
        devices_remote_T[0] = 1 ;
        break ;

      case 4: /* Wind speed */
        //werr(WERR_DEBUG0, "Wind speed: %s", start) ;
        ws.anem_speed = (float) atof(start) ;
        break ;

      case 5: /* Wind direction */
        //werr(WERR_DEBUG0, "Wind direction: %s", start) ;
        ws.vane_bearing = 1 + atoi(start) ;
        devices_remote_vane = 1 ;
        break ;

      case 6: /* Rain */
      {

        //werr(WERR_DEBUG0, "Rain: %s", start) ;
        if (0 != strcmp(start, "null"))
        {
          remote_ws.rain = remote_ws.dailyrain = (float) atof(start) ;
          remote_rain = 1 ;
        }
        else
        {
          /* ws.rain = -1.0F ;*/
          ws.rain = remote_ws.rain = remote_ws.dailyrain = -1.0F ;
          remote_rain = 0 ;
        }
      }
      break ;

      case 7: /* Rain reset time */
        //werr(WERR_DEBUG0, "Rain reset time: %s", start) ;
        if (remote_rain && (0 != strcmp(start, "null")))
        {
          ws.t_rain[0] = (time_t) strtoul(start, NULL, 10) ;
          remote_rain = 1 ;
        }
        else
        {
          remote_rain = 0 ;
        }
        break ;

      case 8: /* English or Metric */
        //werr(WERR_DEBUG0, "English or Metric: %s", start) ;
        if (0 == strcmp(start, "Metric"))
        {
          /* Units are: deg C, km/hr, mm */
          /* T Ok as is */
          /* Convert from km/hr to mph */
          ws.anem_speed *= 0.6213698F ;

          /* Convert from mm to inches */
          if (remote_rain)
          {
            remote_ws.rain /= 25.4F ;
            /*ws.rain_count = ws.rain_offset[0] +
              (unsigned int) (100.0F * ws.rain) ;*/
          }
        }
        else
        {
          /* Units are deg F, mph, inches */
          /* Convert from deg F to deg C */
          ws.T[0] = (ws.T[0] - 32.0F) * 5.0F / 9.0F ;

          /* Wind speed Ok as is */
          /* Rain Ok as is */
        }

        if (remote_rain)
        {
          remote_ws.rain_count_old = remote_ws.rain_count ;

          /* Check for new rain reset time */
          if (remote_ws.t_rain[0] != ws.t_rain[0])
          {
            /* New reset */
            /* Assume reset was at previous value */
            remote_ws.rain_offset[0] = remote_ws.rain_count ;
            remote_ws.rain_count = remote_ws.rain_offset[0] +
              (unsigned long) (100.0F * remote_ws.rain) ;
            ws.rain = remote_ws.rain ;
            remote_ws.t_rain[0] = ws.t_rain[0] ;
          }

          /* Any new rain? */
          if (remote_ws.delta_t &&
             (remote_ws.rain_count > remote_ws.rain_count_old))
          {
            /* There was some rain */
            /* Accumulate hourly and daily rain data */
            rainint_new_rain(&remote_list, &remote_ws) ;
            rainint_new_rain(&remote_list24, &remote_ws) ;
          }

          remote_ws.rain_rate = ws.rain_rate =
            rainint_rate(&remote_list, &remote_ws) ;

          ws.rainint1hr  = rainint_readout(&remote_list);
          ws.rainint24hr = rainint_readout(&remote_list24);
        }

        devices_remote_rain = remote_rain ;
        break ;

      case 9: /* Longitude */
        //werr(WERR_DEBUG0, "Longitude: %s", start) ;
        ws.longitude = (float) atof(start) ;
        break ;

      case 10: /* Latitude */
        //werr(WERR_DEBUG0, "Latitude: %s", start) ;
        ws.latitude = (float) atof(start) ;
        break ;

      default:
        werr(WERR_DEBUG0, "?? %s", start) ;
        werr(WERR_WARNING, "Read too many values from Dallas data reply") ;
        set_old_locale();
        return 0 ;
    }

    start = ++end ;
  }

  devices_remote_used = 1 ;

  set_old_locale();

  return 1 ;
}


static int
http_poll(url_session *session)
{
  if (!url_get_any_session(session)) return session_none ; /* No session */

  if (session->idle && (session->idle + HTTP_TIMEOUT < time(NULL)))
  {
    /* Timeout */
    werr(WERR_DEBUG0 /*WERR_WARNING + WERR_AUTOCLOSE*/,
         "http upload timeout - %s", session->url) ;

    /* Should abort now */

    url_stop(session) ;
    url_failure(session) ;
    url_init_session(session) ;

    return session_failed ; /* Error - Session aborted */
  }
  url_get_body_part(session) ;

  if (!url_finished(session)) return session_open ; /* Session open */
  /* Errors count as a finished session, but body will be cleared */

  if (session->body)
  {
    /* Terminate the body */
    session->body[session->body_alloc] = '\0' ;
    /* Curtail very long bodies */
    /* [Bad idea - broke Dallas list!] */
    /*if (session->body_alloc > 400) session->body[400] = '\0' ;*/
    if (werr_will_output(WERR_DEBUG1))
      werr(WERR_DEBUG1, session->body) ;
    if ((session->test) &&
        (0 == session->test(session->body, session->test_data)))
    /*if (!strstr(session->body, session->reply))*/
    {
      /* Not the expected reply */
      werr(WERR_WARNING, _("URL fetch failed: %s"), session->body) ;
    }
    else
    {
      werr(WERR_DEBUG0, "http upload successful") ;
    }
    url_get_reinit(session);
  }

  /* Deregister this URL module session */

  werr(WERR_DEBUG1, "End URL session") ;

  url_failure(session) ;
  url_init_session(session) ;

  /* If all sessions done with, execute httppostcom */
  /* Don't count rapid wunderground updates */
  if (setup_httppostcom[0] &&
      !url_get_any_session(&dallas_session) &&
      !(url_get_any_session(&wund_session) && (setup_httpwund_type == WUND_NORMAL)) &&
      !url_get_any_session(&ham_session))
  {
    /* Time to execute command */

    werr(WERR_DEBUG0, "httppostcom: %s", setup_httppostcom) ;

    /* It was already parsed by sendwx_send */
    if (httppostcom_buffer && httppostcom_buffer[0])
#     ifdef RISCOS
      system(httppostcom_buffer) ;
#     else
      if (0 > process_new(httppostcom_buffer))
        werr(WERR_WARNING, "Failed to run posthttp command") ;
      /*if (0 == fork())
      {
        if (execl("/bin/sh", httppostcom_buffer, NULL) < 0)
          exit(0) ;
      }*/
#     endif
  }

  return session_finished ; /* Session ended normally */
}

int
sendwx_poll(void)
{
  int running ;

  running  = (session_open == http_poll(&dallas_session)) ;
  running |= (session_open == http_poll(&wund_session)) ;
  running |= (session_open == http_poll(&wow_session)) ;
  running |= (session_open == http_poll(&ham_session)) ;
  running |= (session_open == http_poll(&dallas_mode1_session)) ;
  running |= (session_open == http_poll(&dallas_mode2_session)) ;
  running |= (cwop_poll() != 0) ;

  return running ;
}

/* Convert Unix time to local calendar time */
time_t
convert_time(time_t t)
{
# ifdef RISCOS
  int offset ;
  char *zonename ;

  /* Use teritory module calls */
    _swi(Territory_ReadCurrentTimeZone,
      _OUT(0) | _OUT(1),
      &zonename,
      &offset) ;

  return t + offset / 100 ;

# else
  struct tm *blt ;
  time_t ct ;

  /* Get broken-down UT */
  blt = gmtime(&t) ;

  /* Get Unix time from blt - but this presumes it is a local time
     so the correction is in the wrong direction */
  ct = mktime(blt) ;

  if (blt->tm_isdst)
    ct -= 3600 ;

  return (ct > t) ?
    t - (ct - t)  :
    t + (t - ct)  ;
# endif
}

void
construct_dallas_upload_url(char *urlstring, statsmean *means)
{
  /* Build upload URL string for Dallas servlet interface */

  /* Note that if setup_f is true we use 'English' units for everything */

  char Wid[20] ;

  urlstring[0] = '\0' ;

  set_posix();

  /* Which type of vane? */
  if (devices_list[devices_vane_adc].id[0] == ATOD_FAMILY)
  {
    setup_id_to_string(Wid, devices_list[devices_vane_adc].id) ;
  }
  else
  {
    setup_id_to_string(Wid, devices_list[devices_vane].id) ;
  }


  if (means->rain >= 0)
  {
    sprintf(urlstring, "%s?Mode=0&WID=%s&Dat=%d"
      "&Tem=%.1f&WSp=%.0f&WDr=%d"
      "&RSp=%.2f&Dar=%d"
      "&Cod=%s&Lon=%.6f&Lat=%.6f",
      setup_httpdallas,
      Wid,
      (int) convert_time(means->meanTime),
      convert_temp(weather_primary_T(means), setup_f),
      convert_speed(means->meanWs, setup_f),
      means->point,
      convert_mm(means->rain, !setup_f),
      (int) convert_time(ws.t_rain[0]),
      (setup_f)?"English":"Metric",
      setup_longitude,
      setup_latitude
      ) ;
  }
  else
  {
    sprintf(urlstring, "%s?Mode=0&WID=%s&Dat=%d"
      "&Tem=%.1f&WSp=%.0f&WDr=%d&Cod=%s"
      "&Lon=%.6f&Lat=%.6f",
      setup_httpdallas,
      Wid,
      (int) convert_time(means->meanTime),
      convert_temp(weather_primary_T(means), setup_f),
      convert_speed(means->meanWs, setup_f),
      means->point,
      (setup_f)?"English":"Metric",
      setup_longitude,
      setup_latitude
      ) ;
  }

  set_old_locale();
}

static void
construct_wund_upload_url(char *urlstring, statsmean *means, char *urlbase,
	char *user, char *pass, int subtype)
{
  /* Build upload URL string for The Weather Underground interface */

  /* We use 'English' units for everything */

  char sqltime[30],
    winddir[30] = "",
    rhs[30] = "",
    T[30] = "",
    soilT[30] = "",
    indoorT[30] = "",
    Td[30] = "",
    solar[30] = "",
    uvi[30] = "",
    rain[30] = "",
    dailyrain[30] = "",
    barom[30] = "",
    soilmoist[30] = "",
    leafwet[30] = "";
  int which_rh, which_barom, which_solar, which_uvi, which_soilT, which_indoorT, which_soilmoist, which_leafwet ;
  char *userparam, *passparam;

  switch (subtype) {
  case wund_normal:
  case wund_rapid:
	userparam = "ID";
	passparam = "PASSWORD";
	sprintf(winddir, "&winddir=%g", means->meanWd);
	break;

  case wund_wow:
	userparam = "siteid";
	passparam = "siteAuthenticationKey";
	sprintf(winddir, "&winddir=%d", (int) floor(0.5+means->meanWd));
	break;
  }

  char rtstring[30] = "";

  if (subtype == wund_rapid)
  	sprintf(rtstring, "&realtime=1&rtfreq=%d", setup_interval);

  set_posix();

  strftime(sqltime, 29, "%Y-%m-%d+%H%%3A%M%%3A%S",
    gmtime(&the_time /*means->meanTime*/)) ;

  which_rh = weather_primary_rh(means) ;

  urlstring[0] = '\0' ;

  if (which_rh >= 0)
  {
    sprintf(rhs,
            "%g",
            means->meanRH[which_rh]) ;

    if (strstr(rhs, "nan"))
    {
      urlstring[0] = '\0' ;
      set_old_locale();
      return ;
    }

    sprintf(Td,
            "%g",
            convert_temp(meteo_dew_point(means->meanTrh[which_rh],
                                         means->meanRH[which_rh]),
                                         CONVERT_FAHRENHEIT)) ;
    if (strstr(Td, "nan"))
    {
      urlstring[0] = '\0' ;
      set_old_locale();
      return ;
    }
  }
  else
  {
    rhs[0] = '\0' ;
    Td[0] = '\0' ;
  }

  which_barom = weather_primary_barom(means) ;

  if (which_barom >= 0)
  {
    sprintf(barom,
            "%g",
            convert_barom(means->meanbarom[which_barom],
            CONVERT_INCHES_HG)) ;

    if (strstr(barom, "nan"))
    {
      urlstring[0] = '\0' ;
      set_old_locale();
      return ;
    }
  }
  else
  {
    barom[0] = '\0' ;
  }

  if (means->rain >= 0)
  {
    sprintf(rain,
            "%g",
            convert_mm(means->rainint1hr, CONVERT_INCHES)) ;
    sprintf(dailyrain,
            "%g",
            convert_mm(means->dailyrain, CONVERT_INCHES)) ;
  }
  else
  {
    rain[0] = '\0' ;
    dailyrain[0] = '\0' ;
  }

  sprintf(T, "%g", convert_temp(weather_primary_T(means), CONVERT_FAHRENHEIT)) ;
  if (strstr(T, "nan"))
  {
    urlstring[0] = '\0' ;
    set_old_locale();
    return ;
  }

  which_soilT = weather_primary_soilT(means);
  if (which_soilT != -1)
  {
	sprintf(soilT, "&soiltempf=%g",
		convert_temp(means->meanSoilT[which_soilT], CONVERT_FAHRENHEIT)) ;
	if (strstr(soilT, "nan"))
	{
	  soilT[0]='\0';
	}
  } else {
	soilT[0]='\0';
  }

  which_indoorT = weather_primary_indoorT(means);
  if (which_indoorT != -1)
  {
	sprintf(indoorT, "&indoortempf=%g",
		convert_temp(means->meanIndoorT[which_indoorT], CONVERT_FAHRENHEIT)) ;
	if (strstr(indoorT, "nan"))
	{
	  indoorT[0]='\0';
	}
  } else {
	indoorT[0]='\0';
  }

  which_solar = weather_primary_solar(means);
  if (which_solar!=-1) {
	sprintf(solar, "&solarradiation=%g", means->meansol[which_solar]) ;
	if (strstr(solar, "nan"))
	{
	  solar[0]='\0';
	}
  } else {
	solar[0]='\0';
  }

  which_uvi = weather_primary_uv(means);
  if (which_uvi!=-1) {
	sprintf(uvi, "&UV=%g", means->meanuvi[which_uvi]) ;
	if (strstr(uvi, "nan"))
	{
	  uvi[0]='\0';
	}
  } else {
	uvi[0]='\0';
  }

  which_soilmoist = weather_primary_soilmoisture(means);
  if (which_soilmoist!=-1) {
	sprintf(soilmoist, "&soilmoisture=%g", means->meanMoisture[which_soilmoist]) ;
	if (strstr(soilmoist, "nan"))
	{
	  soilmoist[0]='\0';
	}
  } else {
	soilmoist[0]='\0';
  }

  which_leafwet = weather_primary_leafwetness(means);
  if (which_leafwet!=-1) {
	sprintf(leafwet, "&leafwetness=%g", means->meanLeaf[which_leafwet]) ;
	if (strstr(leafwet, "nan"))
	{
	  leafwet[0]='\0';
	}
  } else {
	leafwet[0]='\0';
  }

  sprintf(urlstring, "%s?action=updateraw"
    "&%s=%s&%s=%s&dateutc=%s"
    "%s&windspeedmph=%g&windgustmph=%g&humidity=%s"
    "&tempf=%s&rainin=%s&dailyrainin=%s&baromin=%s"
    "%s%s&dewptf=%s&weather=&clouds=%s%s%s%s"
    "&softwaretype=%s%s",
    urlbase,
    userparam,
    user,
    passparam,
    pass,
    sqltime,
    /*(float) (ws.vane_bearing-1) * 22.5F,*/
    /*means->meanWd,*/
    winddir,
    convert_speed(means->meanWs, CONVERT_MPH),
    convert_speed(means->maxWs,  CONVERT_MPH),
    rhs,
    T,
    rain,
    dailyrain,
    barom,
    solar,
    uvi,
    Td,
    soilT,
    indoorT,
    soilmoist,
    leafwet,
    "Oww/" VERSION
#   ifdef RISCOS
    "-RO"
#   else
    "-L"
#   endif
     , rtstring
  ) ;

  set_old_locale();
}

//static void
//construct_wow_upload_url(char *urlstring, statsmean *means, char *urlbase, char *user, char *pass)
//{
//  /* Build upload URL string for The Weather Underground interface */
//
//  /* We use 'English' units for everything */
//
////  winddir 	Instantaneous Wind Direction 	Degrees (0-360)
////  windspeedmph 	Instantaneous Wind Speed 	Miles per Hour
////  windgustdir 	Current Wind Gust Direction (using software specific time period) 	0-360 degrees
////  windgustmph 	Current Wind Gust (using software specific time period) 	Miles per Hour
////  humidity 	Outdoor Humidity 	0-100 %
////  dewptf 	Outdoor Dewpoint 	Fahrenheit
////  tempf 	Outdoor Temperature 	Fahrenheit
////  rainin 	Accumulated rainfall in the past 60 minutes 	Inches
////  dailyrainin 	Inches of rain so far today 	Inches
////  baromin 	Barometric Pressure (see note) 	Inches
////  soiltempf 	Soil Temperature 	Fahrenheit
////  soilmoisture 	% Moisture 	0-100 %
////  visibility 	Visibility 	Nautical Miles
//
////  The date must be in the following format: YYYY-mm-DD HH:mm:ss,
////  where ':' is encoded as %3A, and the space is encoded as either '+' or %20.
//
//  char sqltime[30],
//    rhs[30] = "",
//    T[30] = "",
//    Td[30] = "",
//    solar[30] = "",
//    rain[30] = "",
//    dailyrain[30] = "",
//    barom[30] = "" ;
//  int which_rh, which_barom, which_solar ;
//
//  char rtstring[30] = "";
//
//  set_posix();
//
//  strftime(sqltime, 29, "%Y-%m-%d+%H%%3A%M%%3A%S",
//    gmtime(&the_time /*means->meanTime*/)) ;
//
//  which_rh = weather_primary_rh(means) ;
//
//  urlstring[0] = '\0' ;
//
//  if (which_rh >= 0)
//  {
//    sprintf(rhs,
//            "%g",
//            means->meanRH[which_rh]) ;
//
//    if (strstr(rhs, "nan"))
//    {
//      urlstring[0] = '\0' ;
//      set_old_locale();
//      return ;
//    }
//
//    sprintf(Td,
//            "%g",
//            convert_temp(meteo_dew_point(means->meanTrh[which_rh],
//                                         means->meanRH[which_rh]),
//                                         CONVERT_FAHRENHEIT)) ;
//    if (strstr(Td, "nan"))
//    {
//      urlstring[0] = '\0' ;
//      set_old_locale();
//      return ;
//    }
//  }
//  else
//  {
//    rhs[0] = '\0' ;
//    Td[0] = '\0' ;
//  }
//
//  which_barom = weather_primary_barom(means) ;
//
//  if (which_barom >= 0)
//  {
//    sprintf(barom,
//            "%g",
//            convert_barom(means->meanbarom[which_barom],
//            CONVERT_INCHES_HG)) ;
//
//    if (strstr(barom, "nan"))
//    {
//      urlstring[0] = '\0' ;
//      set_old_locale();
//      return ;
//    }
//  }
//  else
//  {
//    barom[0] = '\0' ;
//  }
//
//  if (means->rain >= 0)
//  {
//    sprintf(rain,
//            "%g",
//            convert_mm(means->rainint1hr, CONVERT_INCHES)) ;
//    sprintf(dailyrain,
//            "%g",
//            convert_mm(means->dailyrain, CONVERT_INCHES)) ;
//  }
//  else
//  {
//    rain[0] = '\0' ;
//    dailyrain[0] = '\0' ;
//  }
//
//  sprintf(T, "%g", convert_temp(weather_primary_T(means), CONVERT_FAHRENHEIT)) ;
//  if (strstr(T, "nan"))
//  {
//    urlstring[0] = '\0' ;
//    set_old_locale();
//    return ;
//  }
//
////  which_solar = weather_primary_solar(means);
////  if (which_solar!=-1) {
////	sprintf(solar, "&solarradiation=%g", means->meansol[which_solar]) ;
////	if (strstr(solar, "nan"))
////	{
////	  solar[0]='\0';
////	}
////  } else {
////	solar[0]='\0';
////  }
//
//  sprintf(urlstring, "%s?action=updateraw"
//    "&siteid=%s&siteAuthenticationKey=%s&dateutc=%s"
//    "&winddir=%g&windspeedmph=%g&windgustmph=%g&humidity=%s"
//    "&tempf=%s&rainin=%s&dailyrainin=%s&baromin=%s"
//    "%s&dewptf=%s&"
//    "softwaretype=%s%s",
//    urlbase,
//    user,
//    pass,
//    sqltime,
//    /*(float) (ws.vane_bearing-1) * 22.5F,*/
//    means->meanWd,
//    convert_speed(means->meanWs, CONVERT_MPH),
//    convert_speed(means->maxWs,  CONVERT_MPH),
//    rhs,
//    T,
//    rain,
//    dailyrain,
//    barom,
//    solar,
//    Td,
//    "Oww/" VERSION
//#   ifdef RISCOS
//    "-RO"
//#   else
//    "-L"
//#   endif
//     , rtstring
//  ) ;
//
//  set_old_locale();
//}

static int
time_for_update(int t)
{
  int nt ;

  // Check for one-shot setting

  if (0 == setup_http_interval) return ws.t ;

  // Normal increments

  nt = t + setup_http_interval ;

  // Are we getting behind?

  if (nt + setup_http_interval < ws.t) nt = ws.t ;

  // Snap mode?

  if (setup_http_interval_snap)
  {
    int remainder ;

    // Is the next time a multiple of the interval?

    remainder = nt % setup_http_interval ;

    // Wait for next multiple if it is not

    if (remainder > 0) nt += setup_http_interval - remainder ;
  }

  return nt ;
}

void
sendwx_send(void)
{
  statsmean means ;

  /* Is http upload enabled or setup_httppre/postcom defined? */

  if (!setup_httpdallas_enable &&
      (setup_httpwund_type == WUND_OFF) &&
      (setup_httpwow_type  == WOW_OFF) &&
      !setup_httpham_enable &&
      !setup_cwop_enable &&
      !*setup_httpprecom) return ;

  /* Is this the first time of calling? */

  if (http_time == 0)
  {
    /* Start the clock */
    //time(&http_time) ;
    http_time = ws.t ;

    /* Initialize http sessions */
    url_init_session(&dallas_session) ;
    url_init_session(&wund_session) ;
    url_init_session(&wow_session) ;
    url_init_session(&ham_session) ;

    /*return ;*/
  }

  /* Is it time for an update? */

  if ((ws.t >= time_for_update(http_time)) ||
      (0 == setup_http_interval))
  {
    /* Get the stats */

    if (!stats_do_means(&sendwxstats, &means))
    {
      werr(0, "stats_do_means failed") ;
      return ;
    }

    // Abort this upload if means are not valid

    if (means.meanTime == 0) return ;


    /* Call user-defined http pre-upload command if supplied */
    if (*setup_httpprecom)
    {
      /* Variables for parser */
      static char *httpprecom_buffer = NULL ;
      static int httpprecom_alloced = 0 ;

      werr(WERR_DEBUG0, "httpprecom: %s", setup_httpprecom) ;

      parseout_parse_and_realloc(&httpprecom_buffer,
                                 &httpprecom_alloced,
                                 &means,
                                 setup_httpprecom,
                                 "") ;
      system(httpprecom_buffer) ;
    }

    /* Parse httppostcom for execution later */
    if (*setup_httppostcom)
    {
      parseout_parse_and_realloc(&httppostcom_buffer,
                                 &httppostcom_alloced,
                                 &means,
                                 setup_httppostcom,
                                 "") ;
    }

    /* Start Dallas session if enabled */

    /* Don't start at upload if one is in-progress already */

    if (setup_httpdallas_enable &&
      !url_get_any_session(&dallas_session)) /* No current session? */
    {
      construct_dallas_upload_url(dallas_session.url, &means) ;

      werr(WERR_DEBUG1, "Dallas upload: %s", dallas_session.url) ;

      if (-1 == url_request(&dallas_session, dallas_session.url,
        setup_httpuseproxy ? setup_httpproxy : NULL))
      {
        werr(WERR_WARNING, _("Dallas upload failed"));
        url_init_session(&dallas_session) ;
      }
    }

    /* Start Weather Underground session if enabled */

    /* Don't start at upload if one is in-progress already */

    if ((setup_httpwund_type == WUND_NORMAL) &&
      !url_get_any_session(&wund_session)) /* No current session? */
    {
      construct_wund_upload_url(wund_session.url, &means, setup_httpwund,
    	  setup_httpwund_user, setup_httpwund_pass, wund_normal) ;

      werr(WERR_DEBUG0, "WUnd upload: %s", wund_session.url) ;

      if (-1 == url_request(&wund_session, wund_session.url,
        setup_httpuseproxy ? setup_httpproxy : NULL))
      {
        werr(WERR_WARNING, _("WUnd upload failed"));
        url_init_session(&wund_session) ;
      }
    }

    /* Start Met Office WOW session if enabled */

    /* Don't start at upload if one is in-progress already */

    if ((setup_httpwow_type == WOW_NORMAL) &&
      !url_get_any_session(&wow_session)) /* No current session? */
    {
      construct_wund_upload_url(wow_session.url, &means, setup_httpwow,
    	  setup_httpwow_id, setup_httpwow_pin, wund_wow) ;

      werr(WERR_DEBUG0, "wow upload: %s", wow_session.url) ;

      if (-1 == url_request(&wow_session, wow_session.url,
        setup_httpuseproxy ? setup_httpproxy : NULL))
      {
        werr(WERR_WARNING, _("wow upload failed"));
        url_init_session(&wow_session) ;
      }
    }

    /* Start weather4you session if enabled */

    /* Don't start at upload if one is in-progress already */

    if (setup_httpham_enable &&
      !url_get_any_session(&ham_session)) /* No current session? */
    {
      construct_wund_upload_url(ham_session.url, &means, setup_httpham,
    	  setup_httpham_user, setup_httpham_pass, wund_normal) ;

      werr(WERR_DEBUG1, "ham upload: %s", ham_session.url) ;

      if (-1 == url_request(&ham_session, ham_session.url,
        setup_httpuseproxy ? setup_httpproxy : NULL))
      {
        werr(WERR_WARNING, _("ham upload failed"));
        url_init_session(&ham_session) ;
      }
    }

    /* Start CWOP session if enabled */
    if (setup_cwop_enable)
      cwop_send(&means) ;

    /* Set the time for next uploads */

    http_time = time_for_update(http_time) ;

    /* Re-initialize stats for next http upload */

    stats_reset(&sendwxstats) ;
  }

  // Fire a wund rapid update if set
  if ((setup_httpwund_type == WUND_RAPID) && !url_get_any_session(&wund_session)) /* No current session? */
  {
  	// Upload values are just the last update
  	stats_do_ws_means(&ws, &means);
    construct_wund_upload_url(wund_session.url, &means, setup_httpwund,
    	setup_httpwund_user, setup_httpwund_pass, wund_rapid) ;

    werr(WERR_DEBUG0, "WUnd upload (fast): %s", wund_session.url) ;

    if (-1 == url_request(&wund_session, wund_session.url,
      setup_httpuseproxy ? setup_httpproxy : NULL))
    {
      werr(WERR_WARNING, _("WUnd upload (fast) failed"));
      url_init_session(&wund_session) ;
    }
  }

  // Fire a wow rapid update if set
  if ((setup_httpwow_type == WOW_RAPID) && !url_get_any_session(&wow_session)) /* No current session? */
  {
  	// Upload values are just the last update
  	stats_do_ws_means(&ws, &means);
    construct_wund_upload_url(wow_session.url, &means, setup_httpwow,
    	setup_httpwow_id, setup_httpwow_pin, wund_wow) ;

    werr(WERR_DEBUG0, "wow upload (fast): %s", wow_session.url) ;

    if (-1 == url_request(&wow_session, wow_session.url,
      setup_httpuseproxy ? setup_httpproxy : NULL))
    {
      werr(WERR_WARNING, _("wow upload (fast) failed"));
      url_init_session(&wow_session) ;
    }
  }

}

void
sendwx_recv_list(int (*test_func)(char *, void*))
{
  /* Initialize http sessions */

  dallas_mode2_session.test = test_func ;

  url_init_session(&dallas_mode2_session) ;

  /* Don't start a fetch if one is in-progress already */

  if (!url_get_any_session(
      &dallas_mode2_session)) /* No current session? */
  {
    strcpy(dallas_mode2_session.url, setup_httpdallas) ;
    strcat(dallas_mode2_session.url, "?Mode=2") ;

    werr(WERR_DEBUG0, "Dallas list fetch: %s", dallas_mode2_session.url) ;

    if (-1 == url_request(&dallas_mode2_session, dallas_mode2_session.url,
      setup_httpuseproxy ? setup_httpproxy : NULL))
    {
      werr(WERR_WARNING, _("Dallas list fetch failed"));
      url_init_session(&dallas_mode2_session) ;
    }
  }
}

/* Kill Dallas RX session */

void
sendwx_kill_dallas_rx(void)
{
  if (url_get_any_session(
      &dallas_mode1_session)) /* No current session? */
  {
    url_stop(&dallas_mode1_session) ;
    url_failure(&dallas_mode1_session) ;
  }

  return ;
}

/* Start a fetch to receive data from Dallas */

void
sendwx_recv_data(int (*test_func)(char *, void*))
{
  /* Initialize http sessions */

  dallas_mode1_session.test = test_func ;

  url_init_session(&dallas_mode1_session) ;

  /* Don't start a fetch if one is in-progress already */

  if (!url_get_any_session(
      &dallas_mode1_session)) /* No current session? */
  {
    sprintf(dallas_mode1_session.url,
      "%s?Mode=1&UID=%s",
      setup_httpdallas,
      setup_dallasremote_stn) ;

    werr(WERR_DEBUG0, "Dallas data fetch: %s", dallas_mode1_session.url) ;

    if (-1 == url_request(&dallas_mode1_session, dallas_mode1_session.url,
      setup_httpuseproxy ? setup_httpproxy : NULL))
    {
      werr(WERR_WARNING, _("Dallas data fetch failed"));
      url_init_session(&dallas_mode1_session) ;
    }
  }
}
