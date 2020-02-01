/*
  setup.h

  This defines the setup variables */

#ifndef WSTYPES_H
  #include "wstypes.h"
#endif

extern int setup_loaded_setup   ; /* Has setup file been read? */
extern int setup_loaded_stats   ; /* Has stats file been read? */
extern int setup_loaded_devices ; /* Has devices file been read? */

//extern int setup_mph ;            /* Wind speed mph not kph */
extern int setup_unit_speed ;     /* Wind speed unit: 0=>kph, 1=>mph, 2=>m/s, 3=>knot */
extern int setup_anim ;           /* Display animation */
/*extern int setup_raind ;*/          /* Daily rain gauge reset */
extern int setup_rainreset ;      /* Daily / Weekly / Monthly rain gauge reset */
extern int setup_rainreset_hour ; /* The hour when we should reset */
extern int setup_rainreset_day ;  /* The day of the week when we should reset */
extern int setup_rainreset_date ; /* The date in the month when we should reset */
extern int setup_rain_backrst ;   /* Reset the rain gauge if it goes back */
extern int setup_interval ;       /* Update interval (s) */
extern int setup_gust_time ;      /* Interval between wind gust readings */
extern int setup_log_interval ;   /* Logging interval (s) */
extern int setup_log_snap ;       /* Do we snap the log times? */
extern int setup_mm ;             /* Rainfall in mm not not inches */
extern int setup_f ;              /* Temperature in F not C */
extern int setup_unit_bp ;        /* Barom pressure inches Hg not mBar */
extern int setup_wctype ;         /* Windchill type */
extern int setup_hitype ;         /* HI type */
extern int setup_popup ;          /* Automatic popup of messages window */
extern int setup_wv_offet ;       /* Wind vane offset */
extern int setup_wv_reverse ;     /* Wind vane - reverse sense */
extern int setup_ring_colours[] ; /* Colours for wind dispersion ring */
extern char setup_map_url[] ;     /* URL format string for maps */
extern int setup_port ;           /* Communications port number */
extern int setup_ds2480_write ;   /* Default PARMSET_Write12us */
extern int setup_ds2480_samp ;    /* Default PARMSET_SampOff10us */
extern int setup_ds2490_slew ;    /* DS2490 pull down slew rate */
extern int setup_ds2490_write ;   /* DS2490 write 1 low time */
extern int setup_ds2490_samp ;    /* DS2490 DSOW0 recovery time */
extern int setup_recharge ;       /* Do we do ds2438 recharge on failuer? */
extern int setup_autoalloc ;      /* Do we do automatic device allocation? */
//extern int setup_barvdd ;         /* Do we read Vdd from ds2438 barometers? */

extern float setup_tmin ;         /* Minimum temperature */
extern float setup_tmax ;         /* Maximum temperature */
extern float setup_smax ;         /* Maximum solar */

extern char setup_driver[] ;      /* Communications driver name */

extern char setup_logfile_name[], setup_logdir_name[] ;
extern char *setup_postupdate, *setup_postlog ;
extern char setup_updatevar[] ;   /* Name of log environment variable */
extern char setup_logvar[] ;      /* Name of log environment variable */
extern char *setup_format_logline ;
/*extern char setup_format_logdata[] ;
extern char setup_format_logtime[] ;*/
extern char setup_format_logfile[] ;
extern char setup_format_logdir[] ;
extern char setup_format_loghead[] ;
extern char setup_format_raindate[] ;
extern char setup_format_raintime[] ;
extern char setup_font[] ;
extern int setup_httpuseproxy ;
extern char setup_httpproxy[] ;
extern char setup_httpdallas[] ;
extern char setup_httpwund[] ;
extern char setup_httpwund_user[] ;
extern char setup_httpwund_pass[] ;
extern char setup_httpwow[] ;
extern char setup_httpwow_id[] ;
extern char setup_httpwow_pin[] ;
extern char setup_httpham[] ;
extern char setup_httpham_user[] ;
extern char setup_httpham_pass[] ;
extern int setup_cwop_enable ;
extern int setup_cwop_port ;
extern char setup_cwop_user[] ;
extern char setup_cwop_pass[] ;
extern char setup_cwop_server[] ;
extern char *setup_httpprecom ;
extern char *setup_httppostcom ;
extern int setup_http_interval ;
extern int setup_http_interval_snap ;
extern int setup_arne_udp_port ; /* Port for Arne-style UDP broadcasts */
extern int setup_arne_tcp_port ; /* Port for Arne-style TCP connexions */
extern int setup_oww_tcp_port ; /* Port for oww-style TCP connexions */
extern char setup_oww_un_name[] ; /* Socket name for oww-style LOCAL connexions */
extern int setup_txt_tcp_port ; /* Port for parsed output */
extern char setup_txt_un_name[] ; /* Socket name for parsed output */
extern char *setup_txt_format ; /* Parser format string for txt server */
extern char *setup_lcd_format ; /* Parser format string for LCD output */
extern int setup_url_launcher ;
extern int setup_httpdallas_enable ;
extern int setup_httpwund_type ;
extern int setup_httpwow_type ;
extern int setup_httpham_enable ;
extern float setup_longitude ;
extern float setup_latitude ;
extern char setup_displaydate[] ;
extern int setup_report_Trh1 ; /* Use Trh1 as primary temperature reading */
extern char setup_owwremote_host[] ;
extern int setup_owwremote_port ;
extern char setup_arneremote_host[] ;
extern int setup_arneremote_port ;
extern char setup_dallasremote_stn[] ;
extern int setup_datasource ;

enum data_sources {
  data_source_none = 0,
  data_source_local,
  data_source_remote_oww,
  data_source_arne,
  data_source_dallas
} ;

enum rain_reset_types {
  rain_reset_manual = 0,
  rain_reset_daily,
  rain_reset_weekly,
  rain_reset_monthly
} ;

extern int setup_logtype ; /* Log type */

extern int setup_use_gui ;        /* Use the GUI */

extern char *setup_setup, *setup_devices ;

extern wsstruct ws ;

int setup_id_to_string(char *, unsigned char *) ;
int setup_string_to_id(char *, unsigned char *) ;
int setup_save_stats(void) ;
int setup_load_stats(void) ;
int setup_save_setup(int ng) ;
int setup_load_setup(void) ;
int setup_save_devices(void) ;
int setup_load_devices(void) ;
/*void setup_set_devices(char **) ;*/
