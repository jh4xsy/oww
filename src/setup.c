/*  setup.c */

/*
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * 1999 - 2000
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/* This defines the setup variables and initializes them */

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#ifdef WIN32
#include "config-w32gcc.h"
#else
#include "../config.h"
#endif
#endif

/*#define SETUP_SETUP   "<OwwS$dir>.setup"
#define SETUP_DEVICES "<OwwS$dir>.devices"*/

#include <stdlib.h>
#include <stdio.h>

//#include <string.h>
#ifdef STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (const char *s, int c);
char *strrchr (const char *s, int c);
# ifdef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif
#include <time.h>

#include "werr.h"
//#include "message.h"
#include "wstypes.h"
#include "setupp.h"
#include "globaldef.h"
#include "choices.h"
#include "ownet.h"
#include "devices.h"
#include "meteo.h"
#include "progstate.h"
#include "intl.h"

#include "utility.h"

/*#include "gdefs.h"*/ /* For logtype */

#define Update_Ok 1

extern int wv_known, debug_level, werr_message_level ;

/*char *comp_list[8] = {"                ", "                ",
 "                ", "                ", "                ",
 "                ", "                ", "                "} ;*/

int setup_loaded_setup   = 0 ; /* Has setup file been read? */
int setup_loaded_stats   = 0 ; /* Has stats file been read? */
int setup_loaded_devices = 0 ; /* Has devices file been read? */

//int setup_mph = 1 ;            /* Wind speed mph not kph */
int setup_unit_speed = 1 ;     /* Wind speed unit: 0=>kph, 1=>mph, 2=>m/s, 3=>knot */
int setup_anim = 0 ;           /* Display animation */
/*int setup_raind = 0 ;*/          /* Daily rain gauge reset */
int setup_rainreset = 0 ;      /* Manual / Daily / Weekly / Monthly rain gauge reset */
int setup_rainreset_hour = 0 ; /* The hour when we should reset */
int setup_rainreset_day = 0 ;  /* The day of the week when we should reset */
int setup_rainreset_date = 1 ; /* The date in the month when we should reset */
int setup_rain_backrst = 1 ;   /* Reset the rain gauge if it goes back */
int setup_interval = DEFAULT_INTERVAL ; /* Update interval (s) */
int setup_gust_time = GUST_TIME ; /* 3-s default */
int setup_log_interval = 100 ; /* Logging interval (s) */
int setup_log_snap = 0 ;       /* Do we snap the log times? */
int setup_mm = 0 ;             /* Rainfall in mm not not inches */
int setup_f = 0 ;              /* Temperature in F not C */
int setup_unit_bp = 0 ;        /* Barometric pressure inches Hg not mBar */
int setup_wctype = METEO_WINDCHILL_STEADMAN ; /* Default windchill type */
int setup_hitype = METEO_HI_HEATIND ; /* Default HI type */
int setup_popup = 1 ;          /* Automatic popup of messages window */
int setup_wv_offet = 0 ;       /* Wind vane offset */
int setup_wv_reverse = 0 ;     /* Wind vane - reverse sense */
int setup_ring_colours[16] =   /* Colours for wind dispersion ring */
  {
    0xffffff00,
    0xeeeeee00,
    0xdddddd00,
    0xcccccc00,
    0xbbbbbb00,
    0xaaaaaa00,
    0x99999900,
    0x88888800,
    0x77777700,
    0x66666600,
    0x55555500,
    0x44444400,
    0x33333300,
    0x22222200,
    0x11111100,
    0x00000000
  } ;

int setup_port = 0 ;           /* Communications port number */

int setup_logtype = 0 ;        /* Log type NONE */

char setup_logfile_name[MAXFILENAME] = DEFAULT_LOGFILE,
     setup_logdir_name[MAXFILENAME] = DEFAULT_LOGDIR ;
//char setup_postupdate[MAXPARSELINE] = "", setup_postlog[MAXPARSELINE] = "" ;
char *setup_postupdate = NULL, *setup_postlog = NULL ;
char setup_logvar[128] = OWW_LOGGED ;
char setup_updatevar[128] = OWW_UPDATE ;
//char setup_format_logline[MAXPARSELINE] = DEFAULT_LOGF ;
char *setup_format_logline = NULL ;
/*char setup_format_logdata[128] = DEFAULT_LOGDATAF ;
char setup_format_logtime[128] = DEFAULT_LOGTIMEF ;*/
char setup_format_logfile[128] = DEFAULT_LOGFILEF ;
char setup_format_logdir[128] = DEFAULT_LOGDIRF ;
char setup_format_loghead[128] = DEFAULT_LOGHEADF ;
char setup_format_raindate[128] = DEFAULT_RAINDATE ;
char setup_format_raintime[128] = DEFAULT_RAINTIME ;
char setup_font[128] ;
int setup_httpuseproxy = DEFAULT_USEPROXY ;
char setup_httpproxy[128] = DEFAULT_PROXY ;
char setup_httpdallas[128] = DEFAULT_HTTPDALLAS ;
char setup_httpwund[128] = DEFAULT_HTTPWUND ;
char setup_httpwund_user[40] = DEFAULT_HTTPWUND_USER ;
char setup_httpwund_pass[40] = DEFAULT_HTTPWUND_PASS ;
char setup_httpwow[128] = DEFAULT_HTTPWOW ;
char setup_httpwow_id[40] = DEFAULT_HTTPWOW_ID ;
char setup_httpwow_pin[40] = DEFAULT_HTTPWOW_PIN ;
char setup_httpham[128] = DEFAULT_HTTPW4YOU ;
char setup_httpham_user[40] = DEFAULT_HTTPW4YOU_USER ;
char setup_httpham_pass[40] = DEFAULT_HTTPW4YOU_PASS ;
//char setup_httpprecom[MAXPARSELINE] = "" ;
//char setup_httppostcom[MAXPARSELINE] = "" ;
char *setup_httpprecom = NULL ;
char *setup_httppostcom = NULL ;
int setup_httpdallas_enable = 0 ;
int setup_httpwund_type = 0 ; /* 0 =>Off, 1 =>On (normal), 2 =>Fast */
int setup_httpwow_type  = 0 ; /* 0 =>Off, 1 =>On (normal), 2 =>Fast */
int setup_httpham_enable = 0 ;
int setup_cwop_enable = 0 ;
int setup_cwop_port = DEFAULT_CWOP_PORT ;
char setup_cwop_user[40] = DEFAULT_CWOP_USER ;
char setup_cwop_pass[40] = DEFAULT_CWOP_PASS ;
char setup_cwop_server[128] = DEFAULT_CWOP_SERVER ;
int setup_http_interval = 0 ;
float setup_longitude = DEFAULT_LONGITUDE ;
float setup_latitude  = DEFAULT_LATITUDE ;
int setup_http_interval_snap = 0 ;
/* Port for Arne-style UDP broadcasts */
int setup_arne_udp_port = ARNE_UDP_PORT ;
/* Port for Arne-style TCP connexions */
int setup_arne_tcp_port = ARNE_TCP_PORT ;
/* Port for oww-style TCP connexions */
int setup_oww_tcp_port = OWW_TCP_PORT ;
/* Port for parsed output */
int setup_txt_tcp_port = DEFAULT_TXTSERVE_TCP_PORT ;
/* Socket name for parsed output */
char setup_txt_un_name[108] = DEFAULT_TXTSERVE_UN_NAME ;
/* Parser format string for txt server */
//char setup_txt_format[MAXPARSELINE] = DEFAULT_TXTSERVE_FORMAT ;
char *setup_txt_format = NULL ;
char *setup_lcd_format = NULL ;
/* Name for LOCAL connexions */
char setup_owwremote_host[128] = DEFAULT_OWWREMOTE_HOST ;
int setup_owwremote_port = DEFAULT_OWWREMOTE_PORT ;
char setup_arneremote_host[128] = DEFAULT_ARNEREMOTE_HOST ;
int setup_arneremote_port = DEFAULT_ARNEREMOTE_PORT ;
char setup_dallasremote_stn[18] = DEFAULT_DALLASREMOTE_STN ;
int setup_datasource = DEFAULT_DATASOURCE ;
char setup_oww_un_name[108] = DEFAULT_OWW_UN_NAME ;
int setup_url_launcher = DEFAULT_URL_LAUNCHER ;
char setup_map_url[128] = DEFAULT_MAP_URL ;
int setup_ds2480_write = 12 ; /* Default PARMSET_Write12us */
int setup_ds2480_samp = 10 ; /* Default PARMSET_SampOff10us */
int setup_ds2490_slew = -1 ; /* Default - don't set */
int setup_ds2490_write = -1 ; /* Default - don't set */
int setup_ds2490_samp = -1 ; /* Default - don't set */
int setup_recharge = 1 ; /* Do we do ds2438 recharge on failure? */
int setup_autoalloc = 1 ; /* Do we do automatic device allocation? */
//int setup_barvdd = 0 ; /* Do we read Vdd from ds2438 barometers? */

int setup_report_Trh1 = 0 ; /* Use Trh1 as primary temperature reading */

/* Format string for display date / time */
char setup_displaydate[128] = DEFAULT_DISPLAYDATE ;

float setup_tmin =  300.0F ;     /* Minimum primary temperature */
float setup_tmax = -300.0F ;     /* Maximum primary temperature */
float setup_smax = 1024.0F ;     /* Maximum primary solar */

/*int setup_use_gui = 1 ;*/        /* Use the GUI */

char setup_driver[128] = DEFAULT_SERIAL ; /* Communications driver name */

wsstruct ws ;

extern char debug_file[] ; /* Declared in werr */
//extern char error_log[]  ; /* Declared in werr */

/* To write out dispersion ring colours */

static int setup_16_ints_to_hex_string(char *string, setupp_liststr *member, int index)
{
  int i, n = 0 ;
  int *ints ;

  ints = (int *) member->data ;

  for (i=0; i<16; ++i)
  {
    n += sprintf(&string[n], "%x ", 0xffffff & (ints[i] >> 8)) ;
  }

  return 1 ;
}

/* To read in dispersion ring colours */

static int setup_hex_string_to_16_ints(char *string, setupp_liststr *member, int index)
{
  int i ;
  char *s ;

  /* data points to array of longs */

  int *ints = (int *) member->data ;

  s = string ;

  for (i=0; i<16; ++i)
  {
    if (!s) return 0 ;

    ints[i] = (int) strtoul(s, &s, 16) << 8 ;
    //werr(WERR_WARNING, "%i %x", i, ints[i]) ;
  }

  return 1 ;
}

int setup_id_to_string(char *string, unsigned char *s)
{
  int i, string_cnt = 0 ;

  string[0] = '\0' ;

  for (i=7; i>=0; --i)
    string_cnt += sprintf(&string[string_cnt],"%02X",
      0xFF & s[i]);

  return (s[0] != '\0') ;
}

int setup_string_to_id(char *string, unsigned char *s)
{
  int i, string_cnt = 0, val ;

  for (i=7; i>=0; --i) {
    if (sscanf(&string[string_cnt],"%02X", &val)) {
      string_cnt += 2 ;
      s[i] = (char) val ;
    } else {
      //werr(0, get_message("BADID:Error reading id string", NULL)) ;
		werr(0, "Error reading id string: \"%s\"", string);
		return 0 ;
    }
  }

  return 1 ;
}

int setup_load_sensor(char *string, setupp_liststr *member, int index)
{
  devices_struct *device ;
  char /*id[12],*/ *ws, *copy ;
  //int nr ;
  int devnum ;
  devnum = *((int *)(member->data)) ;
  if (devnum < 0)
    werr(1, "setup_load_sensor with -ve devnum") ;
  //device = (devices_struct *) data ;
  device = &devices_list[devnum+index] ;
  copy = strdup(string) ;

  /* Find any space following id string */
  ws = strchr(copy, ' ') ;

  /* Terminate id string */
  if (ws)
  {
    int i ;

    *ws = '\0' ;

    /* Calibration args follow */
    ++ws ;

    for (i=0; i<device->ncalib; ++i)
    {
      float val ;
      int consumed ;

      if (sscanf(ws, "%f%n", &val, &consumed) != 1) break ; // No conversion
      ws += consumed ;
      device->calib[i] = val ;
    }

    if (device->ncalib)
      werr(WERR_DEBUG1, "%s: %f %f %f %f",
        device->menu_entry,
        device->calib[0],
        device->calib[1],
        device->calib[2],
        device->calib[3]) ;
  }

  setup_string_to_id(copy, device->id) ;

  free(copy) ;

  return 1 ; /* Ok */
}

int
setup_save_sensor(char *string, setupp_liststr *member, int index)
{
  devices_struct *device ;
  /*char id_string[32] ;*/
  int i ;

  int devnum ;
  devnum = *((int *)(member->data)) ;
  if (devnum < 0)
    werr(1, "setup_save_sensor with -ve devnum") ;

  device = &devices_list[devnum+index] ;
  //device = (devices_struct *) data ;

  werr(WERR_DEBUG1, "setup_save_sensor %s", device->menu_entry) ;

  if (!setup_id_to_string(string, (void *) device->id))
    return 0 ;

  for (i=0; i<device->ncalib; ++i)
  {
    sprintf(&string[strlen(string)], " %f",
      (double) device->calib[i]) ;
  }

//   werr(WERR_DEBUG0, "%s - slope/offset", id_string) ;
//
//   sprintf(string, "%s %f %f", id_string, device->slope, device->offset) ;

  return 1 ;
}

static int
setup_save_gpc(char *string, setupp_liststr *member, int index)
{
  int i ;
  string[0] = '\0' ;

  for (i=0; i<MAXGPC; ++i)
    sprintf(&string[strlen(string)],
      " %lu %lu",
      ((gpc_struct *)member->data)[i].time_reset,
      ((gpc_struct *)member->data)[i].count_offset) ;

  return 1 ;
}

static int
setup_load_gpc(char *string, setupp_liststr *member, int index)
{
  int i, n=0 ;

  for (i=0; i<MAXGPC; ++i)
  {
    sscanf(&string[n],
      " %lu %lu%n",
      (unsigned long *)&((gpc_struct *)member->data)[i].time_reset,
      (unsigned long *)&((gpc_struct *)member->data)[i].count_offset,
      &n) ;
    werr(WERR_DEBUG1, "GPC%d %lu %lu",
      i,
      ((gpc_struct *)member->data)[i].time_reset,
      ((gpc_struct *)member->data)[i].count_offset) ;
  }

  return 1 ;
}

setupp_liststr stats_list[] = {
  {"raintimes", SETUPP_SUFFIX_NONE, 0, SETUPP_ULONGARR, (void *) ws.t_rain, 8, NULL, NULL},
  {"raincounts", SETUPP_SUFFIX_NONE, 0, SETUPP_ULONGARR, (void *) ws.rain_offset, 8, NULL, NULL},
  {"dailyraintime", SETUPP_SUFFIX_NONE, 0, SETUPP_ULONG, (void *) &(ws.t_dailyrain), 0, NULL, NULL},
  {"dailyraincount", SETUPP_SUFFIX_NONE, 0, SETUPP_ULONG, (void *) &(ws.dailyrain_offset), 0, NULL, NULL},
  {"gpc", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL,
    (void *) ws.gpc, MAXGPC, setup_save_gpc, setup_load_gpc},
  {"tmin", SETUPP_SUFFIX_NONE, 0, SETUPP_FLOAT, (void *) &ws.Tmin, 0, NULL, NULL},
  {"tmax", SETUPP_SUFFIX_NONE, 0, SETUPP_FLOAT, (void *) &ws.Tmax, 0, NULL, NULL},
  {"anemmax", SETUPP_SUFFIX_NONE, 0, SETUPP_FLOAT, (void *) &ws.anem_speed_max, 0, NULL, NULL},
  {"", SETUPP_SUFFIX_NONE, 0, 0,  NULL, 0, NULL, NULL}
} ;

setupp_liststr setup_list[] = {
  {"#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Miscellaneous", 0, NULL, NULL},
  {"debugfile", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, debug_file, 256, NULL, NULL},
  {"debuglevel", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &debug_level, 0, NULL, NULL},
  {"messagelevel", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &werr_message_level, 0, NULL, NULL},
//  {"errorlog", SETUPP_STRING, error_log, 256, NULL, NULL},
  {"interval", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_interval, 0, NULL, NULL},
  {"updatevar", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_updatevar, 128, NULL, NULL},
  {"gusttime", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_gust_time, 0, NULL, NULL},
  /*{"driver", SETUPP_STRING, setup_driver, 128, NULL, NULL},
  {"port", SETUPP_INT, &setup_port, 0, NULL, NULL},*/
  {"popup", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_popup, 0, NULL, NULL},
  {"raindaily", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_rainreset, 0, NULL, NULL},
  {"rainhour", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_rainreset_hour, 0, NULL, NULL},
  {"raindate", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_rainreset_date, 0, NULL, NULL},
  {"rainday", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_rainreset_day, 0, NULL, NULL},
  {"rainbackreset", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_rain_backrst, 0, NULL, NULL},
  /*{"raintime", SETUPP_INT, (void *) &(ws.t_rain), 0, NULL, NULL},
  {"raincount", SETUPP_INT, (void *) &(ws.rain_offset), 0, NULL, NULL},*/
  {"urllauncher", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_url_launcher, 0, NULL, NULL},
  {"mapurl", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_map_url, 128, NULL, NULL},
  {"ringcol", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL, (void *) setup_ring_colours, 0,
    setup_16_ints_to_hex_string, setup_hex_string_to_16_ints},


  {"#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Server ports and unix socket names", 0, NULL, NULL},
  {"arneudpport", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_arne_udp_port, 0, NULL, NULL},
  {"arnetcpport", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_arne_tcp_port, 0, NULL, NULL},
  {"owwtcpport", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_oww_tcp_port, 0, NULL, NULL},
  {"owwlocalname", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_oww_un_name, 107, NULL, NULL},
  {"txttcpport", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_txt_tcp_port, 0, NULL, NULL},
  {"txtname", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_txt_un_name, 107, NULL, NULL},
  {"txtform", SETUPP_SUFFIX_NONE, 0, SETUPP_ASTRING, &setup_txt_format, 0, NULL, NULL},


  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Format strings", 0, NULL, NULL},
  {"displaydate", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_displaydate, 128, NULL, NULL},
  {"logfilef", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_format_logfile, 128, NULL, NULL},
  {"logdirf", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_format_logdir, 128, NULL, NULL},
  {"logform", SETUPP_SUFFIX_NONE, 0, SETUPP_ASTRING, &setup_format_logline, 0, NULL, NULL},
  {"logheadf", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_format_loghead, 128, NULL, NULL},
  {"raintform", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_format_raintime, 128, NULL, NULL},
  {"raindform", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_format_raindate, 128, NULL, NULL},
  {"lcdform", SETUPP_SUFFIX_NONE, 0, SETUPP_ASTRING, &setup_lcd_format, 0, NULL, NULL},


  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Data source", 0, NULL, NULL},
  {"datasource", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_datasource, 0, NULL, NULL},
  {"owwremote_host", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_owwremote_host, 128, NULL, NULL},
  {"owwremote_port", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_owwremote_port, 0, NULL, NULL},
  {"arneremote_host", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_arneremote_host, 128, NULL, NULL},
  {"arneremote_port", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_arneremote_port, 0, NULL, NULL},
  {"dallasremote", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_dallasremote_stn, 17, NULL, NULL},

  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Command strings", 0, NULL, NULL},
  {"postupdate", SETUPP_SUFFIX_NONE, 0, SETUPP_ASTRING, &setup_postupdate, 0, NULL, NULL},
  {"postlog", SETUPP_SUFFIX_NONE, 0, SETUPP_ASTRING, &setup_postlog, 0, NULL, NULL},
  {"httpprecom", SETUPP_SUFFIX_NONE, 0, SETUPP_ASTRING, &setup_httpprecom, 0, NULL, NULL},
  {"httppostcom", SETUPP_SUFFIX_NONE, 0,
    SETUPP_ASTRING, &setup_httppostcom, 0, NULL, NULL},


  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "HTTP setup", 0, NULL, NULL},
  {"httpuseproxy", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_httpuseproxy, 0, NULL, NULL},
  {"httpproxy", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpproxy, 128, NULL, NULL},
  {"httpdallas", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpdallas, 128, NULL, NULL},
  {"httpdallason", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void*)&setup_httpdallas_enable,0,NULL,NULL},
  {"httpwund", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpwund, 128, NULL, NULL},
  {"httpwunduser", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpwund_user, 40, NULL, NULL},
  {"httpwundpass", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpwund_pass, 40, NULL, NULL},
  {"httpwundon", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void*)&setup_httpwund_type,0,NULL,NULL},
  {"httpwow", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpwow, 128, NULL, NULL},
  {"httpwowid", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpwow_id, 40, NULL, NULL},
  {"httpwowpin", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpwow_pin, 40, NULL, NULL},
  {"httpwowon", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void*)&setup_httpwow_type,0,NULL,NULL},
  {"httpham", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpham, 128, NULL, NULL},
  {"httphamuser", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpham_user, 40, NULL, NULL},
  {"httphampass", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_httpham_pass, 40, NULL, NULL},
  {"httphamon", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void*)&setup_httpham_enable,0,NULL,NULL},
  {"httptime", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_http_interval, 0, NULL, NULL},
  {"httptsnap", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_http_interval_snap,0,NULL,NULL},
  {"longitude", SETUPP_SUFFIX_NONE, 0, SETUPP_FLOAT, (void *) &setup_longitude, 0, NULL, NULL},
  {"latitude", SETUPP_SUFFIX_NONE, 0, SETUPP_FLOAT, (void *) &setup_latitude, 0, NULL, NULL},
  {"cwopon", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void*)&setup_cwop_enable,0,NULL,NULL},
  {"cwopport", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void*)&setup_cwop_port,0,NULL,NULL},
  {"cwopuser", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_cwop_user, 40, NULL, NULL},
  {"cwoppass", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_cwop_pass, 40, NULL, NULL},
  {"cwopserver", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_cwop_server, 128, NULL, NULL},


  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Display setup", 0, NULL, NULL},
  {"tmin", SETUPP_SUFFIX_NONE, 0, SETUPP_FLOAT, (void *) &setup_tmin, 0, NULL, NULL},
  {"tmax", SETUPP_SUFFIX_NONE, 0, SETUPP_FLOAT, (void *) &setup_tmax, 0, NULL, NULL},
  {"smax", SETUPP_SUFFIX_NONE, 0, SETUPP_FLOAT, (void *) &setup_smax, 0, NULL, NULL},
  {"mph", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_unit_speed, 0, NULL, NULL},
  {"fahr", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_f, 0, NULL, NULL},
  {"bpunit", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_unit_bp, 0, NULL, NULL},
  {"mm", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_mm, 0, NULL, NULL},
  {"anim", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_anim, 0, NULL, NULL},
  {"reporttrh", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_report_Trh1, 0, NULL, NULL},
  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING,
    "Wind chill: (1) Steadman, (2) NWS 1992b, (3) New WCT", 0, NULL, NULL},
  {"wctype", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_wctype, 0, NULL, NULL},
  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING,
    "Heat index: (1) U.S. Heat Index, (2) Canadian Humidex", 0, NULL, NULL},
  {"hitype", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_hitype, 0, NULL, NULL},


  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Log setup", 0, NULL, NULL},
  {"loginterval", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_log_interval, 0, NULL, NULL},
  {"logsnap", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_log_snap, 0, NULL, NULL},
  {"logtype", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, (void *) &setup_logtype, 0, NULL, NULL},
  {"logfile_name", SETUPP_SUFFIX_NONE, 0,
    SETUPP_STRING, &setup_logfile_name, MAXFILENAME, NULL, NULL},
  {"logdir_name", SETUPP_SUFFIX_NONE, 0,
    SETUPP_STRING, &setup_logdir_name, MAXFILENAME, NULL, NULL},
  {"logvar", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_logvar, 128, NULL, NULL},
  {"font", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_font, 128, NULL, NULL},

  {"", SETUPP_SUFFIX_NONE, 0, 0, NULL, 0, NULL, NULL}
} ;

setupp_liststr device_list[] = {
  {"wv", SETUPP_SUFFIX_0, 8, SETUPP_SPECIAL, &devices_wv0, 0, setup_save_sensor, setup_load_sensor},
  {"offset", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_wv_offet, 0, NULL, NULL},
  {"reverse", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_wv_reverse, 0, NULL, NULL},
  {"T", SETUPP_SUFFIX_1, MAXTEMPS, SETUPP_SPECIAL, &devices_T1, 0, setup_save_sensor, setup_load_sensor},
  {"SOILT", SETUPP_SUFFIX_1, MAXSOILTEMPS, SETUPP_SPECIAL, &devices_soilT1, 0, setup_save_sensor, setup_load_sensor},
  {"INDOORT", SETUPP_SUFFIX_1, MAXINDOORTEMPS, SETUPP_SPECIAL, &devices_indoorT1, 0, setup_save_sensor, setup_load_sensor},
  {"H", SETUPP_SUFFIX_1, MAXHUMS, SETUPP_SPECIAL, &devices_H1, 0, setup_save_sensor, setup_load_sensor},
  {"BAR", SETUPP_SUFFIX_1, MAXBAROM, SETUPP_SPECIAL, &devices_BAR1, 0, setup_save_sensor, setup_load_sensor},
  {"tai8570w", SETUPP_SUFFIX_1, MAXBAROM, SETUPP_SPECIAL, &devices_tai8570w1, 0, setup_save_sensor, setup_load_sensor},
  {"SOL", SETUPP_SUFFIX_1, MAXSOL, SETUPP_SPECIAL, &devices_sol1, 0, setup_save_sensor, setup_load_sensor},
  {"ADC", SETUPP_SUFFIX_1, MAXADC, SETUPP_SPECIAL, &devices_adc1, 0, setup_save_sensor, setup_load_sensor},
  {"TC", SETUPP_SUFFIX_1, MAXTC, SETUPP_SPECIAL, &devices_tc1, 0, setup_save_sensor, setup_load_sensor},
  {"UV", SETUPP_SUFFIX_1, MAXUV, SETUPP_SPECIAL, &devices_uv1, 0, setup_save_sensor, setup_load_sensor},
  {"BR", SETUPP_SUFFIX_A, MAXBRANCHES, SETUPP_SPECIAL, &devices_brA, 0, setup_save_sensor, setup_load_sensor},
  {"vane", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL, &devices_vane, 0,
     setup_save_sensor, setup_load_sensor},
  {"vaneadc", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL, &devices_vane_adc, 0,
     setup_save_sensor, setup_load_sensor},
  {"vaneads", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL, &devices_vane_ads, 0,
     setup_save_sensor, setup_load_sensor},
  {"vaneins", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL, &devices_vane_ins, 0,
     setup_save_sensor, setup_load_sensor},
  {"anem", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL, &devices_anem, 0,
     setup_save_sensor, setup_load_sensor},
  {"wsi603a", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL, &devices_wsi603a, 0, setup_save_sensor, setup_load_sensor},
  {"moisture", SETUPP_SUFFIX_1, MAXMOIST, SETUPP_SPECIAL, &devices_moist1, 0, setup_save_sensor, setup_load_sensor},
  {"rain", SETUPP_SUFFIX_NONE, 0, SETUPP_SPECIAL, &devices_rain, 0,
     setup_save_sensor, setup_load_sensor},
  {"GPC", SETUPP_SUFFIX_1, MAXGPC, SETUPP_SPECIAL, &devices_GPC1, 0, setup_save_sensor, setup_load_sensor},
  {"lcd", SETUPP_SUFFIX_1, MAXLCD, SETUPP_SPECIAL, &devices_lcd1, 0, setup_save_sensor, setup_load_sensor},
  {"driver", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, setup_driver, 128, NULL, NULL},
  {"port", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_port, 0, NULL, NULL},
  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Data sample offset: 3-10us", 0, NULL, NULL},
  {"ds2480samp", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_ds2480_samp, 0, NULL, NULL},
  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Write 1 low time: 5-15us", 0, NULL, NULL},
  {"ds2480write", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_ds2480_write, 0, NULL, NULL},
  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Parameters for DS2490.", 0, NULL, NULL},
  {"#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Use -1 for defaults, or 0-7 according to data sheet tables 9-11", 0, NULL, NULL},
  {"ds2490slew", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_ds2490_slew, 0, NULL, NULL},
  {"ds2490write", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_ds2490_write, 0, NULL, NULL},
  {"ds2490samp", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_ds2490_samp, 0, NULL, NULL},
  {"\n#", SETUPP_SUFFIX_NONE, 0, SETUPP_STRING, "Test setup", 0, NULL, NULL},
  {"recharge", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_recharge, 0, NULL, NULL},
  {"autoalloc", SETUPP_SUFFIX_NONE, 0, SETUPP_INT, &setup_autoalloc, 0, NULL, NULL},
  {"", SETUPP_SUFFIX_NONE, 0, 0,  NULL, 0, NULL, NULL}
} ;

int setup_save_stats(void)
{
  if (!setupp_write(choices_stats_write, &(stats_list[0]))) {
    werr(WERR_WARNING+WERR_AUTOCLOSE,
      _("stats write failed")) ;
    return 0 ;
  }
  return 1 ;
}

int setup_load_stats(void)
{
  if (!setupp_read(choices_stats_read, &(stats_list[0]))) {
    werr(WERR_WARNING+WERR_AUTOCLOSE,
      _("No stats file found - Using defaults")) ;
    return 0 ;
  }
  return 1 ;
}

/* setup_save_setup
  ng - 1 -> save to owwnogui setup file
*/

int setup_save_setup(int ng)
{
  if (!setupp_write(
    (ng) ? choices_setupng_write : choices_setup_write,
    &(setup_list[0]))) {
    werr(WERR_WARNING+WERR_AUTOCLOSE,
      _("Setup write failed")) ;
    return 0 ;
  }
  return 1 ;
}

/* Load either the normal setup or the nogui setup */

int setup_load_setup(void)
{
  if (!setupp_read(
#   ifndef NOGUI
    choices_setup_read,
#   else
    choices_setupng_read,
#   endif
    &(setup_list[0])))
  {
    werr(WERR_WARNING+WERR_AUTOCLOSE,
      _("No setup file found - Using defaults")) ;
    return 0 ;
  }
  werr(WERR_DEBUG0, "setup_oww_un_name: %s", setup_oww_un_name) ;
  return 1 ;
}

int setup_save_devices(void)
{
  if (!setupp_write(choices_devices_write, &(device_list[0]))) {
    werr(WERR_WARNING+WERR_AUTOCLOSE,
      _("Could not save device list.")) ;
    return 0 ;
  }
  return 1 ;
}

int setup_load_devices(void)
{
  /*int i ;*/

  wv_known = 0 ;

  /* First, set devices */

  if (!setupp_read(choices_devices_read, &(device_list[0]))) {
    werr(0, _("No devices file found. You must use the devices dialogue.")) ;
    return 0 ;
  }

  /* Check for valid vane IDs */
  devices_check_vane_ids() ;

  return 1 ;
}
