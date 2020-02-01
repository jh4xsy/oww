/* globaldef.h */

#ifndef GLOBALDEF_H
#define GLOBALDEF_H 1

#define temp_string_len 512

#define HTTP_TIMEOUT 60
#define WEATHER_TUNE 1
#define WEATHER_TUNE_FRAC 0.25F

#define REMOTE_RETRY 500L /* Remote retry delay = 500 cs = 5 s */
#define LOCAL_RETRY  500L /* Local retry delay = 500 cs = 5 s */

#define ANEM_RATE 2.0F
#define DEFAULT_INTERVAL 10
#define GUST_TIME 300 /* 300 cs = 3 seconds */
/*#define DEFAULT_LOGDATAF " , %7.1f,�%s   , %7.2f,%5s,%10s   ,%5.1f  ,%3d    ,%10s"
#define DEFAULT_LOGTIMEF "%d/%m/%y , %H:%M:%S"*/
#define DEFAULT_LOGF "$localtime%d/%m/%y , %H:%M:%S$ , $t1%7.1$,$tunit$   , $wsp%7.2$,$wspunit%5$,$wdrname%10$   ,$wdrdeg%5.1$  ,$wdrpoint%3$    ,$rain%5.2$, $rainunit$"
#define DEFAULT_LOGHEADF "Date          Time      Temperature    Speed           Direction                      RainGauge"

#define DEFAULT_RAINDATE "Rain: %%s since %d/%m/%y"
#define DEFAULT_RAINTIME "Rain: %%s since %H:%M"
#define DEFAULT_USEPROXY 0
#define DEFAULT_PROXY ""
#define DEFAULT_HTTPDALLAS "http://www.aag.com.mx/servlet/weather/"
//#define DEFAULT_HTTPDALLAS "http://205.153.100.122:8080/servlet/WeatherServlet"
/* Old value - */ /* #define DEFAULT_HTTPDALLAS http://198.3.123.122:8080/servlet/WeatherServlet" */
/*#define DEFAULT_MAP_URL "http://www.mapblast.com/mymapblast/map.mb?CMD=LFILL&IC=%f:%f:8:"*/
#define DEFAULT_MAP_URL "http://www.mapquest.com/maps/map.adp?latlongtype=decimal&latitude=%f&longitude=%f"
#define DEFAULT_LONGITUDE 0.0F
#define DEFAULT_LATITUDE  0.0F
#define DEFAULT_HTTPWUND "http://www.wunderground.com/weatherstation/updateweatherstation.php"
#define DEFAULT_HTTPWUND_USER "NO_USER"
#define DEFAULT_HTTPWUND_PASS ""
#define DEFAULT_HTTPWOW "http://wow.metoffice.gov.uk/automaticreading"
#define DEFAULT_HTTPWOW_ID "NO_USER"
#define DEFAULT_HTTPWOW_PIN ""
#define DEFAULT_HTTPW4YOU "http://www.hamweather.net/weatherstations/vwsupdate.php"
#define DEFAULT_HTTPW4YOU_USER "NO_USER"
#define DEFAULT_HTTPW4YOU_PASS ""
#define DEFAULT_CWOP_PORT 14580
#define DEFAULT_CWOP_USER "NO_USER"
#define DEFAULT_CWOP_PASS "-1"
#define DEFAULT_CWOP_SERVER "rotate.aprs.net"
#define CWOP_TIMEOUT 120
#define MAXFILENAME 128
#define MAXPARSELINE 256
/*#define RAINCALIB 0.01F*/
/* Use slope value instead of RAINCALIB */
#define RAINCALIB devices_list[devices_rain].calib[0]
#define RAINSLOPE 0.01F
#define ARNE_UDP_PORT 8890
#define ARNE_TCP_PORT 8888
#define OWW_TCP_PORT 8899
#define DEFAULT_TXTSERVE_TCP_PORT 8891
#define DEFAULT_TXTSERVE_FORMAT "$localtime%d/%m/%y , %H:%M:%S$ , $t1%7.1$,$tunit$   , $wsp%7.2$,$wspunit%5$,$wdrname%10$   ,$wdrdeg%5.1$  ,$wdrpoint%3$    ,$rain%5.2$, $rainunit$"
#define DEFAULT_LCD_FORMAT "$localtime%H:%M:%S$ $t1%4.1$$tunit$ $wsp$$wspunit%5$ $wdrname$ $rain%5.2$$rainunit$"/*#define ARNE_TCP_MAX_CON 20*/
#define SERVER_MAX_SOCK 20
#define DEFAULT_OWWREMOTE_HOST "localhost"
#define DEFAULT_OWWREMOTE_PORT 8899
#define DEFAULT_ARNEREMOTE_HOST "localhost"
#define DEFAULT_ARNEREMOTE_PORT 8888
#define DEFAULT_DALLASREMOTE_STN ""
#define DEFAULT_DATASOURCE 1
#define DEFAULT_URL_LAUNCHER 1
#define SOLAR_SLOPE 1.0F
#define SOLAR_OFFSET 0.0F
#define SOLAR_MAX 1024.0
#define UV_SLOPE 1.0F
#define UV_OFFSET 0.0F
#define ADC_VSLOPE 1.0F
#define ADC_VOFFSET 0.0F
#define ADC_ISLOPE 1.0F
#define ADC_IOFFSET 0.0F
#define ADC_ACCSLOPE 1.0F
#define TC_TYPE 3.0F
#define TC_VOFFSET 0.0F

#ifdef RISCOS
  #define DEGSYMB "\xb0"
  #define WpSqM "W/m\xb2"
  #define DEFAULT_DISPLAYDATE "%d/%m/%y %H:%M:"
  #define DEFAULT_LOGFILE "logfile"
  #define DEFAULT_LOGDIR  "logdir"
  #define DEFAULT_LOGFILEF ".%y%m.%y%m%d"
  #define DEFAULT_LOGDIRF ".%y%m"
  /*#define OWW_SETUP "<Oww$Dir>.Setup"
  #define OWW_DEVICES "<Oww$Dir>.Devices"*/
  #define OWW_MESSAGES "<Oww$Dir>.Messages"
  #define OWW_LOGGED "Oww$Logged"
  #define OWW_UPDATE "Oww$Update"
  #define FONT_NAME   "Homerton.Medium"
  #define FONT_WIDTH  352
  #define FONT_HEIGHT 352
  #define TSTEM_HEIGHT 56
  #define WimpVersion    310
  #define ANIMATION_INTERVAL 25
  #define DEFAULT_SERIAL "Internal"
  #define DEFAULT_OWW_UN_NAME ""
  #define DEFAULT_TXTSERVE_UN_NAME ""
#else
  #define DEGSYMB "\xc2\xb0"
  #define WpSqM "W/m\xc2\xb2"
  #define DEFAULT_DISPLAYDATE "%d/%m/%y %H:%M:%S"
  #define DEFAULT_LOGFILE "logfile.csv"
  #define DEFAULT_LOGDIR  "logdir"
  #define DEFAULT_LOGFILEF "/%y%m/%y%m%d.csv"
  #define DEFAULT_LOGDIRF "/%y%m"
  /*#define OWW_SETUP "$HOME/.oww_setup"
  #define OWW_DEVICES "$HOME/.oww_devices"*/
  /*#define OWW_LOGGED "OWWLOGGED"
  #define OWW_UPDATE "OWWUPDATE"*/
  #define OWW_LOGGED ""
  #define OWW_UPDATE ""
  #define ANIMATION_INTERVAL 250
  #define DEFAULT_OWW_UN_NAME "/tmp/oww_trx_un"
  #define DEFAULT_TXTSERVE_UN_NAME "/tmp/oww_txtserve"
#ifdef WIN32
  #define DEFAULT_SERIAL "COM2"
#else
# define DEFAULT_SERIAL "/dev/ttyS0" // Default should suit Linux / Posix
# ifdef __FreeBSD__
#   undef DEFAULT_SERIAL
#   define DEFAULT_SERIAL "/dev/cuaa0"
# endif
# ifdef	__NetBSD__
#   undef DEFAULT_SERIAL
#   define DEFAULT_SERIAL "/dev/cuaa0" // Is this a good choice?
# endif
#endif
#endif
#endif /* GLOBALDEF_H */
