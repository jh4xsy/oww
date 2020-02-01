/*

  parseout.c

  Parse strings to formatted stats values for output

  Oww project
  Dr. Simon J. Melhuish

  Wed 03rd January 2001

*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stddef.h>
//#include <assert.h>

#include "wstypes.h"
#include "werr.h"
#include "stats.h"
#include "setup.h"
#include "globaldef.h"
#include "devices.h"
#include "convert.h"
#include "rainint.h"
#include "meteo.h"
#include "parseout.h"
#include "weather.h"
#include "intl.h"

#define MAXTAG 12

//extern time_t the_time ;
extern wsstruct ws ;

static statsmean parseout_means ;

/*static int parseout_true = 1, parseout_false = 0 ;*/

static int tag_suffix ;

struct parseout_struct {
  char tag[MAXTAG] ;
  char format_letter ;
  int maxsuf ;
  void *data ;
  int  *data2 ;
  int base_unit ;
  int (*available)(statsmean *means, int i) ;
  int (*parse)(struct parseout_struct *entry,
               char *format,
               char buffer[],
               int bufflen,
               statsmean *means,
               int suf) ;
} ;

#define NOAVAIL (int (*)(statsmean *, int)) NULL
#define NOPARSE (int (*)(struct parseout_struct *, char *, char *, int, statsmean *, int)) NULL


static char *unit_names_T[] = {
  "�C", "�F"} ;

static char *unit_names_bp[] = {
  "mBar", "\"Hg", "kPa", "ATM"} ;

static char *unit_names_wsp[] = {
  "KPH", "MPH", "m/s", "knots"} ;

static char *unit_names_rain[] = {
  "inches", "mm"} ;

#define PARSE_ADC_V 0
#define PARSE_ADC_I 1
#define PARSE_ADC_Q 2
#define PARSE_ADC_T 3

#define PARSE_TC_T   0
#define PARSE_TC_TCJ 1
#define PARSE_TC_V   2

static int ParseADC_V = PARSE_ADC_V;
static int ParseADC_I = PARSE_ADC_I;
static int ParseADC_Q = PARSE_ADC_Q;
static int ParseADC_T = PARSE_ADC_T;

static int ParseTC_T   = PARSE_TC_T;
static int ParseTC_Tcj = PARSE_TC_TCJ;
static int ParseTC_V   = PARSE_TC_V;

/* Prototypes for availability functions */

//static int parseout_have_gpc(statsmean *means, int i) ;

static int parseout_have_T(statsmean *means, int i) ;

static int parseout_have_Tindoor(statsmean *means, int i) ;

static int parseout_have_Tsoil(statsmean *means, int i) ;

static int parseout_have_gpc(statsmean *means, int i) ;

static int parseout_have_RH(statsmean *means, int i) ;

static int parseout_have_solar(statsmean *means, int i) ;

static int parseout_have_uvi(statsmean *means, int i) ;

static int parseout_have_adc(statsmean *means, int i) ;

static int parseout_have_tc(statsmean *means, int i) ;

static int parseout_have_barom(statsmean *means, int i) ;

static int parseout_have_moist(statsmean *means, int i) ;

static int parseout_have_leaf(statsmean *means, int i) ;

/* Prototypes for parsing functions - return number of chars written */

static int
parseout_id_to_string(struct parseout_struct *entry,
                      char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_int(struct parseout_struct *entry,
             char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_uint(struct parseout_struct *entry,
             char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

//static int
//parseout_uint_array(struct parseout_struct *entry,
//             char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_string(struct parseout_struct *entry,
                char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_indstring(struct parseout_struct *entry,
                char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_float(struct parseout_struct *entry,
               char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_gmtime(struct parseout_struct *entry,
                char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_mysqltime(struct parseout_struct *entry,
                char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_localtime(struct parseout_struct *entry,
                   char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

/*static int
parseout_T(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;*/

/*static int
parseout_T_array(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;
*/

static int
parseout_float_array(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_ulong_array(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;
/*
static int
parseout_RH_array(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_barom_array(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;
*/

static int
parseout_dew_point(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_adc(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf);
static int
parseout_thrmcpl(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf);
/*static int
parseout_mph(struct parseout_struct *entry,
             char *format, char buffer[], int bufflen, statsmean *means, int suf) ;*/

static int
parseout_rain(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_rainint(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

/*static int
parseout_rainrate(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;*/

//static int
//parseout_rainint_array(struct parseout_struct *entry,
//              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_unit(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_windchill(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;

static int
parseout_heat_index(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf) ;


static struct parseout_struct parseout[] = {
  {"gmtime",
    '\0', 0, (void *) &(parseout_means.meanTime), (int *) NULL, 0, NOAVAIL, parseout_gmtime},
  {"mysqltime",
    's', 0, (void *) &parseout_means.meanTime, (int *) NULL, 0, NOAVAIL, parseout_mysqltime},
  {"localtime",
    '\0', 0, (void *) &parseout_means.meanTime, (int *) NULL, 0, NOAVAIL, parseout_localtime},
  {"gpc",      'f', MAXGPC, (void *) parseout_means.gpc,      (int *) NULL, 0, parseout_have_gpc, parseout_float_array},
  {"gpcdelta", 'f', MAXGPC, (void *) parseout_means.deltagpc, (int *) NULL, 0, parseout_have_gpc, parseout_float_array},
  {"gpcmono",  'u', MAXGPC, (void *) parseout_means.gpcmono,  (int *) NULL, 0, parseout_have_gpc, parseout_ulong_array},
  {"gpcevents",    'u', MAXGPC, (void *) parseout_means.gpcEvents,(int *)NULL, 0, parseout_have_gpc, parseout_ulong_array},
  {"gpceventmax", 'f', MAXGPC, (void *) parseout_means.gpceventmax,(int *)NULL, 0, parseout_have_gpc, parseout_float_array},
  {"gpcrate",  'f', MAXGPC, (void *) parseout_means.gpcrate,  (int *) NULL, 0, parseout_have_gpc, parseout_float_array},
  /*{"gpc",
    'f', MAXGPC, (void *) &ws.gpc, (int *) NULL, 0, NOAVAIL, parseout_gpc},
  {"gpcdelta",
    'f', MAXGPC, (void *) &ws.gpc, (int *) NULL, 0, NOAVAIL, parseout_gpcdelta},
  {"gpcmono",
    'u', MAXGPC, (void *) &ws.gpc, (int *) NULL, 0, NOAVAIL, parseout_gpcmono},
  {"gpcrate",
    'f', MAXGPC, (void *) ws.gpc, (int *) NULL, 0, NOAVAIL, parseout_gpcrate},*/
  {"rain",
    'f', 0, (void *) &parseout_means.rain, &setup_mm, convert_unit_inches, NOAVAIL, parseout_rain},
  {"rainin",
    'f', 0, (void *) &parseout_means.rain, (int *) NULL, convert_unit_inches, NOAVAIL, parseout_rain},
  {"rainmm",
    'f', 0, (void *) &parseout_means.rain, (int *) NULL, convert_unit_mm, NOAVAIL, parseout_rain},
  {"dailyrain",
  'f', 0, (void *) &parseout_means.dailyrain, &setup_mm, convert_unit_inches, NOAVAIL, parseout_rain},
  {"dailyrainin",
  'f', 0, (void *) &parseout_means.dailyrain, (int *) NULL, convert_unit_inches, NOAVAIL, parseout_rain},
  {"dailyrainmm",
  'f', 0, (void *) &parseout_means.dailyrain, (int *) NULL, convert_unit_mm, NOAVAIL, parseout_rain},
  {"rainint",
  'f', 0, (void *) &parseout_means.rain, &setup_mm, convert_unit_inches, NOAVAIL, parseout_rainint},
  {"rainintin",
    'f', 0, (void *) &parseout_means.rain, (int *) NULL, convert_unit_inches, NOAVAIL, parseout_rainint},
  {"rainintmm",
    'f', 0, (void *) &parseout_means.rain, (int *) NULL, convert_unit_mm, NOAVAIL, parseout_rainint},
  {"rainrate",
    'f', 0, (void *) &parseout_means.rain_rate, &setup_mm, convert_unit_inches, NOAVAIL, parseout_rain},
  {"rainratein",
    'f', 0, (void *) &parseout_means.rain_rate, (int *) NULL, convert_unit_inches, NOAVAIL, parseout_rain},
  {"rainratemm",
    'f', 0, (void *) &parseout_means.rain_rate, (int *) NULL, convert_unit_mm, NOAVAIL, parseout_rain},
  {"rainunit",
    's', 0, (void *) &unit_names_rain, &setup_mm, 0, NOAVAIL, parseout_unit},
  {"rainmono",
    'u', 0, (void *) &ws.rain_count, (int *) NULL, 0, NOAVAIL, parseout_uint},
  {"t",
    'f', MAXTEMPS, (void *) &parseout_means.meanT[0], &setup_f,
    convert_unit_c, parseout_have_T, parseout_float_array},
  {"tc",
    'f', MAXTEMPS, (void *) &parseout_means.meanT[0], (int *) NULL,
    convert_unit_c, parseout_have_T, parseout_float_array},
  {"tf",
    'f', MAXTEMPS, (void *) &parseout_means.meanT[0], (int *) NULL,
    convert_unit_f, parseout_have_T, parseout_float_array},
  {"tsoil",
      'f', MAXSOILTEMPS, (void *) &parseout_means.meanSoilT[0], &setup_f,
    convert_unit_c, parseout_have_Tsoil, parseout_float_array},
  {"tsoilc",
      'f', MAXSOILTEMPS, (void *) &parseout_means.meanSoilT[0], (int *) NULL,
    convert_unit_c, parseout_have_Tsoil, parseout_float_array},
  {"tsoilf",
    'f', MAXSOILTEMPS, (void *) &parseout_means.meanSoilT[0], (int *) NULL,
    convert_unit_f, parseout_have_Tsoil, parseout_float_array},
  {"tindoor",
    'f', MAXINDOORTEMPS, (void *) &parseout_means.meanIndoorT[0], &setup_f,
    convert_unit_c, parseout_have_Tindoor, parseout_float_array},
  {"tindoorc",
    'f', MAXINDOORTEMPS, (void *) &parseout_means.meanIndoorT[0], (int *) NULL,
    convert_unit_c, parseout_have_Tindoor, parseout_float_array},
  {"tindoorf",
      'f', MAXINDOORTEMPS, (void *) &parseout_means.meanIndoorT[0], (int *) NULL,
    convert_unit_f, parseout_have_Tindoor, parseout_float_array},
  {"moisture",
  	'f', MAXMOIST*4, (void *) &parseout_means.meanMoisture[0], (int *) NULL,
  	convert_unit_none, parseout_have_moist, parseout_float_array},
  {"leaf",
    'f', MAXMOIST*4, (void *) &parseout_means.meanLeaf[0], (int *) NULL,
    convert_unit_none, parseout_have_leaf, parseout_float_array},
  {"rh",
    'f', MAXHUMS, (void *) &parseout_means.meanRH[0], (int *) NULL,
    convert_unit_none, parseout_have_RH, parseout_float_array},
  {"trh",
    'f', MAXHUMS, (void *) &parseout_means.meanTrh[0], &setup_f,
    convert_unit_c, parseout_have_RH, parseout_float_array},
  {"trhc",
    'f', MAXHUMS, (void *) &parseout_means.meanTrh[0], (int *) NULL,
    convert_unit_c, parseout_have_RH, parseout_float_array},
  {"trhf",
    'f', MAXHUMS, (void *) &parseout_means.meanTrh[0], (int *) NULL,
    convert_unit_f, parseout_have_RH, parseout_float_array},
  {"dp", 'f', MAXHUMS, (void *) NULL, &setup_f, convert_unit_c, parseout_have_RH, parseout_dew_point},
  {"dpc", 'f', MAXHUMS, (void *) NULL, (int *) NULL, convert_unit_c, parseout_have_RH, parseout_dew_point},
  {"dpf", 'f', MAXHUMS, (void *) NULL, (int *) NULL, convert_unit_f, parseout_have_RH, parseout_dew_point},
  {"tunit",
    's', 0, (void *) &unit_names_T, &setup_f, 0, NOAVAIL, parseout_unit},
  {"bar",
    'f', MAXBAROM, (void *) &parseout_means.meanbarom[0], &setup_unit_bp,
    convert_unit_millibar, parseout_have_barom, parseout_float_array},
  {"solar",
    'f', MAXSOL, (void *) &parseout_means.meansol[0], (int *) NULL,
    0, parseout_have_solar, parseout_float_array},
  {"uvi",
      'f', MAXUV, (void *) &parseout_means.meanuvi[0], (int *) NULL,
      0, parseout_have_uvi, parseout_float_array},
  {"tuvi",
        'f', MAXUV, (void *) &parseout_means.meanuviT[0], &setup_f,
        convert_unit_c, parseout_have_uvi, parseout_float_array},
  {"tuvic",
          'f', MAXHUMS, (void *) &parseout_means.meanuviT[0], (int *) NULL,
          convert_unit_c, parseout_have_uvi, parseout_float_array},
  {"tuvif",
          'f', MAXHUMS, (void *) &parseout_means.meanuviT[0], (int *) NULL,
          convert_unit_f, parseout_have_uvi, parseout_float_array},
  {"vadc",
    'f', MAXADC, (void *) &parseout_means.meanADC[0], &ParseADC_V,
    0, parseout_have_adc, parseout_adc},
  {"iadc",
    'f', MAXADC, (void *) &parseout_means.meanADC[0], &ParseADC_I,
    0, parseout_have_adc, parseout_adc},
  {"vsadc",
   'f', MAXADC, (void *) &parseout_means.meanADC[0], &ParseADC_I,
    0, parseout_have_adc, parseout_adc},
  {"accadc",
   'f', MAXADC, (void *) &parseout_means.meanADC[0], &ParseADC_Q,
    0, parseout_have_adc, parseout_adc},
  {"tadc",
    'f', MAXADC, (void *) &parseout_means.meanADC[0], &ParseADC_T,
    0, parseout_have_adc, parseout_adc},
  {"ttc",
    'f', MAXADC, (void *) &parseout_means.meanTC[0], &ParseTC_T,
    0, parseout_have_tc, parseout_thrmcpl},
  {"tcjtc",
    'f', MAXADC, (void *) &parseout_means.meanTC[0], &ParseTC_Tcj,
    0, parseout_have_tc, parseout_thrmcpl},
  {"vtc",
   'f', MAXADC, (void *) &parseout_means.meanTC[0], &ParseTC_V,
    0, parseout_have_tc, parseout_thrmcpl},
  {"barinhg",
    'f', MAXBAROM, (void *) &parseout_means.meanbarom[0], (int *) NULL,
    convert_unit_inches_hg, parseout_have_barom, parseout_float_array},
  {"barmbar",
    'f', MAXBAROM, (void *) &parseout_means.meanbarom[0], (int *) NULL,
    convert_unit_millibar, parseout_have_barom, parseout_float_array},
  {"barkpa",
    'f', MAXBAROM, (void *) &parseout_means.meanbarom[0], (int *) NULL,
    convert_unit_kpa, parseout_have_barom, parseout_float_array},
  {"barunit",
    's', 0, (void *) &unit_names_bp, &setup_unit_bp, 0, NOAVAIL, parseout_unit},
  {"tb",
    'f', MAXBAROM, (void *) &parseout_means.meanTb[0], &setup_f,
    convert_unit_c, parseout_have_barom, parseout_float_array},
  {"tbc",
    'f', MAXBAROM, (void *) &parseout_means.meanTb[0], (int *) NULL,
    convert_unit_c, parseout_have_barom, parseout_float_array},
  {"tbf",
    'f', MAXBAROM, (void *) &parseout_means.meanTb[0], (int *) NULL,
    convert_unit_f, parseout_have_barom, parseout_float_array},
  {"unixtime",
    'd', 0, (void *) &parseout_means.meanTime, (int *) NULL, 0, NOAVAIL, parseout_int},
  {"wid",
    '\0', 0, (void *) NULL, (int *) NULL, 0, NOAVAIL, parseout_id_to_string},
  {"wchill",
    'f', 0, (void *) NULL, &setup_f, convert_unit_c, NOAVAIL, parseout_windchill},
  {"wchillc",
    'f', 0, (void *) NULL, (int *) NULL, convert_unit_c, NOAVAIL, parseout_windchill},
  {"wchillf",
    'f', 0, (void *) NULL, (int *) NULL, convert_unit_f, NOAVAIL, parseout_windchill},
  {"heatindex",
    'f', 0, (void *) NULL, &setup_f, convert_unit_c, NOAVAIL, parseout_heat_index},
  {"heatindexc",
    'f', 0, (void *) NULL, (int *) NULL, convert_unit_c, NOAVAIL, parseout_heat_index},
  {"heatindexf",
    'f', 0, (void *) NULL, (int *) NULL, convert_unit_f, NOAVAIL, parseout_heat_index},
  {"wdrdeg",
    'f', 0, (void *) &parseout_means.meanWd, (int *) NULL, 0, NOAVAIL, parseout_float},
  {"wdrpoint",
    'd', 0, (void *) &parseout_means.point, (int *) NULL, 0, NOAVAIL, parseout_int},
  {"wdrname",
    's', 0, (void *) &parseout_means.pointName, (int *) NULL, 0, NOAVAIL, parseout_indstring},
  /*{"windmono",
    'u', 0, (void *) &ws.anem_end_count, NULL, parseout_uint},*/
  {"wsp",
    'f', 0, (void *) &parseout_means.meanWs,
    &setup_unit_speed, convert_unit_kph, NOAVAIL, parseout_float},
  {"wspmph",
    'f', 0, (void *) &parseout_means.meanWs,
    (int *) NULL, convert_unit_mph, NOAVAIL, parseout_float},
  {"wspknots",
    'f', 0, (void *) &parseout_means.meanWs,
    (int *) NULL, convert_unit_knot, NOAVAIL, parseout_float},
  {"wspmps",
    'f', 0, (void *) &parseout_means.meanWs,
    (int *) NULL, convert_unit_mps, NOAVAIL, parseout_float},
  {"wspkph",
    'f', 0, (void *) &parseout_means.meanWs,
    (int *) NULL, convert_unit_kph, NOAVAIL, parseout_float},
  /* Maximum wind speed */
  {"wspmax",
    'f', 0, (void *) &parseout_means.maxWs,
    &setup_unit_speed, convert_unit_kph, NOAVAIL, parseout_float},
  {"wspmaxmph",
    'f', 0, (void *) &parseout_means.maxWs,
    (int *) NULL, convert_unit_mph, NOAVAIL, parseout_float},
  {"wspmaxkph",
    'f', 0, (void *) &parseout_means.maxWs,
    (int *) NULL, convert_unit_kph, NOAVAIL, parseout_float},
  {"wspmaxmps",
    'f', 0, (void *) &parseout_means.maxWs,
    (int *) NULL, convert_unit_mps, NOAVAIL, parseout_float},
  {"wspmaxknots",
    'f', 0, (void *) &parseout_means.maxWs,
    (int *) NULL, convert_unit_knot, NOAVAIL, parseout_float},
  {"wspunit",
    's', 0, (void *) &unit_names_wsp, &setup_unit_speed, 0, NOAVAIL, parseout_unit},
  {"wunduser",
    's', 0, (void *) setup_httpwund_user, (int *) NULL, 0, NOAVAIL, parseout_string},
  {"wundpass",
    's', 0, (void *) setup_httpwund_pass, (int *) NULL, 0, NOAVAIL, parseout_string},
  {"",
    '\0', 0, (void *) NULL, (int *) NULL, 0, NOAVAIL, NOPARSE} /* Marker for end of list */
} ;

/*static int
parseout_have_gpc(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXGPC) return 0 ;
  return (means->haveGPC[i-1]) ;
}*/

static int
parseout_have_T(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXTEMPS) return 0 ;
  return (means->haveT[i-1]) ;
}

static int
parseout_have_Tindoor(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXINDOORTEMPS) return 0 ;
  return (means->haveIndoorT[i-1]) ;
}

static int
parseout_have_Tsoil(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXSOILTEMPS) return 0 ;
  return (means->haveSoilT[i-1]) ;
}

static int
parseout_have_moist(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXMOIST*4) return 0 ;
  return (means->haveMoisture[i-1]) ;
}

static int
parseout_have_leaf(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXMOIST*4) return 0 ;
  return (means->haveLeaf[i-1]) ;
}

static int
parseout_have_gpc(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXGPC) return 0 ;
  return (means->haveGPC[i-1]) ;
}

static int
parseout_have_RH(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXHUMS) return 0 ;
  return (means->haveRH[i-1]) ;
}

static int
parseout_have_solar(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXSOL) return 0 ;
  return (means->havesol[i-1]) ;
}

static int
parseout_have_uvi(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXUV) return 0 ;
  return (means->haveuvi[i-1]) ;
}

static int
parseout_have_adc(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXADC) return 0 ;
  return (means->haveADC[i-1]) ;
}

static int
parseout_have_tc(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXTC) return 0 ;
  return (means->haveTC[i-1]) ;
}

static int
parseout_have_barom(statsmean *means, int i)
{
  if (i < 1) return 0 ;
  if (i > MAXBAROM) return 0 ;
  return (means->havebarom[i-1]) ;
}

static float
parseout_convert_unit(struct parseout_struct *entry, float val)
{
  int u ;

  /* Do unit conversion */

  u = entry->base_unit ;

  if (u == convert_unit_none) return val ;

  /* Pointer to setup variable is optional */
  if (entry->data2) u += *entry->data2 ;

  return convert_unit(val, u) ;
}

/* Wrapper for setup_id_to_string */

static int
parseout_id_to_string(/*@unused@*/ struct parseout_struct *entry,
                      char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  //assert(bufflen > 16) ;

  (void) setup_id_to_string(buffer, devices_list[devices_vane].id) ;

  return 16 ;
}


/* Print an integer */

static int
parseout_int(struct parseout_struct *entry,
             char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
# ifdef HAVE_SNPRINTF
  return snprintf(buffer, bufflen, format, *((int *) entry->data)) ;
# else
  return sprintf(buffer, format, *((int *) entry->data)) ;
# endif // ifdef HAVE_SNPRINTF
  /*return sprintf(buffer, format, *((int *) entry->data)) ;*/
}

/* Print an unsinged integer */

static int
parseout_uint(struct parseout_struct *entry,
             char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
# ifdef HAVE_SNPRINTF
  return snprintf(buffer, bufflen, format, *((unsigned int *) entry->data)) ;
# else
  return sprintf(buffer, format, *((unsigned int *) entry->data)) ;
# endif // ifdef HAVE_SNPRINTF
  /*return sprintf(buffer, format, *((unsigned int *) entry->data)) ;*/
}



/* Print a string */

static int
parseout_string(struct parseout_struct *entry,
                char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
# ifdef HAVE_SNPRINTF
  return snprintf(buffer, bufflen, format, (char *) entry->data) ;
# else
  return sprintf(buffer, format, (char *) entry->data) ;
# endif // ifdef HAVE_SNPRINTF
  /*return sprintf(buffer, format, (char *) entry->data) ;*/
}

/* Print an indirected string */

static int
parseout_indstring(struct parseout_struct *entry,
                char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
# ifdef HAVE_SNPRINTF
  return snprintf(buffer, bufflen, format, *((char **) entry->data)) ;
# else
  return sprintf(buffer, format, *((char **) entry->data)) ;
# endif // ifdef HAVE_SNPRINTF
  /*return sprintf(buffer, format, (char *) entry->data) ;*/
}

/* Print a time (UT) */

static int
parseout_gmtime(struct parseout_struct *entry,
                char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  if ((format != NULL ) && (format[0] != '\0'))
    return strftime(buffer, bufflen, format, gmtime((time_t *) entry->data)) ;
  else
    return strftime(buffer, bufflen, "%H:%M:%S %x", gmtime((time_t *) entry->data)) ;
}

/* Print a time (UT) in MySQL format */

static int
parseout_mysqltime(struct parseout_struct *entry,
                char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  char tbuf[22] ;

  strftime(tbuf, 22,
           "%Y-%m-%d+%H:%M:%S",
           gmtime((time_t *) entry->data)) ;
  if (strlen(tbuf) < (size_t) bufflen)
    return sprintf(buffer, format, tbuf) ;

  werr(WERR_WARNING, "parseout_mysqltime buffer too short") ;
  buffer[0] = '\0' ;
  return 0 ;
}

/* Print a time (local) */

static int
parseout_localtime(struct parseout_struct *entry,
                   char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  if ((format != NULL ) && (format[0] != '\0'))
    return strftime(buffer, bufflen, format, localtime((time_t *) entry->data)) ;
  else
    return strftime(buffer, bufflen, "%H:%M:%S %x", localtime((time_t *) entry->data)) ;
}

/* Print a float */

static int
parseout_float(struct parseout_struct *entry,
               char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
# ifdef HAVE_SNPRINTF
  return snprintf(
    buffer,
    bufflen,
    format,
    parseout_convert_unit(
      entry,
      *((float *) entry->data)
    )
  ) ;
# else
  return sprintf(
    buffer,
    format,
    parseout_convert_unit(
      entry,
      *((float *) entry->data)
    )
  )  ;
# endif // ifdef HAVE_SNPRINTF
  /*return sprintf(
    buffer,
    format,
    parseout_convert_unit(
      entry,
      *((float *) entry->data)
    )
  ) ;*/
}

static void dump_means(statsmean *means)
{
  int i ;
  werr(WERR_DEBUG0, "Dump of main values...") ;

  for (i=0; i<MAXTEMPS; ++i)
  {
    if (means->haveT[i])
      werr(WERR_DEBUG0, "T%d: last %f, mean %f", i+1, ws.T[i], means->meanT[i]) ;
    else
      werr(WERR_DEBUG0, "T%d not available", i+1) ;
  }

  for (i=0; i<MAXHUMS; ++i)
  {
    if (means->haveRH[i])
    {
      werr(WERR_DEBUG0, "RH%d: last %f, mean %f", i+1, ws.RH[i], means->meanRH[i]) ;
      werr(WERR_DEBUG0, "Trh%d: last %f, mean %f", i+1, ws.Trh[i], means->meanTrh[i]) ;
    }
    else
    {
      werr(WERR_DEBUG0, "RH%d not available", i+1) ;
    }
  }
}

static int
parseout_print_float(char *buffer, int bufflen, char *format, float val,
  struct parseout_struct *entry, statsmean *means, int suf)
{
  int rv ;

# ifdef HAVE_SNPRINTF
  rv = snprintf(buffer, bufflen, format, val) ;
# else
  rv = sprintf(buffer, format, val) ;
  if (rv > bufflen)
    werr(0, "parseout_print_float buffer overflow!") ;
# endif // ifdef HAVE_SNPRINTF

  if (werr_will_output(WERR_DEBUG0) && (strstr(buffer, "nan") != NULL))
  {
    werr(WERR_DEBUG0, "\"nan\" detected in parseout_print_float") ;
    werr(WERR_DEBUG0, "result:        \"%s\"", buffer) ;
    werr(WERR_DEBUG0, "tag:           \"%s\"", entry->tag) ;
    werr(WERR_DEBUG0, "suffix:         %d", suf) ;
    werr(WERR_DEBUG0, "val:            %f", val) ;
    werr(WERR_DEBUG0, "format string: \"%s\"", format) ;
    werr(WERR_DEBUG0, "bufflen:        %d", bufflen) ;

    dump_means(means) ;

    return -1 ; /* Signal an error condition */
  }

  return rv ;
}

/* Print a float from the array */

static int
parseout_float_array(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  /* Suffix range already checked */

  float val ;
  int u ;

  /* Do unit conversion */

  u = entry->base_unit ;

  /* Pointer to setup variable is optional */
  if (entry->data2) u += *entry->data2 ;

  val = convert_unit(((float *) entry->data)[suf-1], u) ;

  return parseout_print_float(
    buffer,
    bufflen,
    format,
    val,
    entry,
    means,
    suf) ;

  //return sprintf(buffer, format, val) ;
}

/* Print an long from the array */

static int
parseout_ulong_array(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  /* Suffix range already checked */

  unsigned long val ;
  int rv ;
  
  val = *((unsigned long *) entry->data) ;

# ifdef HAVE_SNPRINTF
  rv = snprintf(buffer, bufflen, format, val) ;
# else
  rv = sprintf(buffer, format, val) ;
  if (rv > bufflen)
    werr(0, "parseout_ulong_array buffer overflow!") ;
# endif // ifdef HAVE_SNPRINTF

  return rv ;
}


/* Print a dew point temperature from the array */

static int
parseout_dew_point(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  int rv ;
  float val ;

  /*if (!means->haveRH[suf-1])
  {
    werr(WERR_DEBUG0, "Dew point %d not available", suf) ;
    buffer[0] = '\0' ;
    return 0 ;
  }*/

  val = parseout_convert_unit(entry,
                   meteo_dew_point(means->meanTrh[suf-1],
                                   means->meanRH[suf-1])) ;

  rv = parseout_print_float(
    buffer,
    bufflen,
    format,
    val,
    entry,
    means,
    suf) ;

  /*rv =  sprintf(buffer, format, val) ;*/

  if (werr_will_output(WERR_DEBUG0) && strstr(buffer, "nan"))
  {
    val = parseout_convert_unit(entry,
                   meteo_dew_point(means->meanTrh[suf-1],
                                   means->meanRH[suf-1])) ;

    rv = parseout_print_float(
    buffer,
    bufflen,
    format,
    val,
    entry,
    means,
    suf) ;

    werr(WERR_DEBUG0, "2nd go: %s", buffer) ;

    /*werr(WERR_DEBUG0, "parseout_dew_point returned nan") ;
    werr(WERR_DEBUG0, "meanTrh = %g", means->meanTrh[suf-1]) ;
    werr(WERR_DEBUG0, "meanRH = %g", means->meanRH[suf-1]) ;
    werr(WERR_DEBUG0, "conv flag = %d", entry->data2) ;
    rv =  sprintf(buffer, format,
                   parseout_convert_unit(entry,
                     meteo_dew_point(means->meanTrh[suf-1],
                                     means->meanRH[suf-1]))) ;
    werr(WERR_DEBUG0, "2nd go: %s", buffer) ;*/
  }

  return rv ;
}

/* Print an ADC value from the array of ADC values */

static int
parseout_adc(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  int rv ;
  float val ;
  adc_struct *adc;

  if (!means->haveADC[suf-1])
  {
    werr(WERR_DEBUG0, "ADC %d not available", suf) ;
    buffer[0] = '\0' ;
    return 0 ;
  }

  adc = &((adc_struct *) entry->data)[suf-1];

  switch(*(entry->data2))
  {
    case PARSE_ADC_V:
      val = adc->V;
      break;

    case PARSE_ADC_I:
      val = adc->I;
      break;

    case PARSE_ADC_Q:
      val = adc->Q;
      break;

    case PARSE_ADC_T:
      val = adc->T;
      break;
  }

  rv = parseout_print_float(
    buffer,
    bufflen,
    format,
    val,
    entry,
    means,
    suf) ;

  return rv ;
}

/* Print a thermocouple value from the array of thermocouple values */

static int
parseout_thrmcpl(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  int rv ;
  float val ;
  tc_struct *tc;

  if (!means->haveTC[suf-1])
  {
    werr(WERR_DEBUG0, "Thermocouple %d not available", suf) ;
    buffer[0] = '\0' ;
    return 0 ;
  }

  tc = &((tc_struct *) entry->data)[suf-1];

  switch(*(entry->data2))
  {
    case PARSE_TC_T:
      val = tc->T;
      break;

    case PARSE_TC_TCJ:
      val = tc->Tcj;
      break;

    case PARSE_TC_V:
      val = tc->V;
      break;
  }

  rv = parseout_print_float(
    buffer,
    bufflen,
    format,
    val,
    entry,
    means,
    suf) ;

  return rv ;
}

/* Print rain (in mm if data2 true) */

static int
parseout_rain(struct parseout_struct *entry,
              char *format, char buffer[],
              int bufflen, statsmean *means, int suf)
{
  if (*((float *) entry->data) < 0.0F)
    return sprintf(buffer, "%s", _("None")) ;
# ifdef HAVE_SNPRINTF
  return snprintf(buffer, bufflen, format,
    parseout_convert_unit(entry, *((float *) entry->data))) ;
# else
  return sprintf(buffer, format,
    parseout_convert_unit(entry, *((float *) entry->data))) ;
# endif // ifdef HAVE_SNPRINTF
}

/* Print interval rain (in mm if data2 true) */

static int
parseout_rainint(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  if (*((float *) entry->data) < 0.0F)
    return sprintf(buffer, "%s", _("None")) ;
# ifdef HAVE_SNPRINTF
  return snprintf(buffer, bufflen, format,
    parseout_convert_unit(entry, means->rainint1hr)) ;
# else
  return sprintf(buffer, format,
    parseout_convert_unit(entry, means->rainint1hr)) ;
# endif // ifdef HAVE_SNPRINTF
}

static int
parseout_unit(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  /* Fill in the name of a unit */

  char **unit_name ;
  unit_name = (char **) entry->data ;

# ifdef HAVE_SNPRINTF
  return snprintf(buffer, bufflen, format, unit_name[*entry->data2]) ;
# else
  return sprintf(buffer, format, unit_name[*entry->data2]) ;
# endif // ifdef HAVE_SNPRINTF
  /*return sprintf(buffer, format, unit_name[*entry->data2]) ;*/
}

static int
parseout_windchill(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  /* Wind chill from primary T and wind speed */

  int rv ;
  float val ;

  val = parseout_convert_unit(entry,
    meteo_wind_chill(weather_primary_T(means),
      means->meanWs, setup_wctype)
  ) ;

  rv = parseout_print_float(
    buffer,
    bufflen,
    format,
    val,
    entry,
    means,
    suf) ;

  if (werr_will_output(WERR_DEBUG0) && (strstr(buffer, "nan") != NULL))
  {
    werr(WERR_DEBUG0, "parseout_windchill returned nan") ;
    werr(WERR_DEBUG0, "means->meanT[0] = %g", means->meanT[0]) ;
    werr(WERR_DEBUG0, "means->meanWs = %g", means->meanWs) ;
    werr(WERR_DEBUG0, "meteo_wind_chill() -> %g", val) ;
  }

  return rv ;
}

static int
parseout_heat_index(struct parseout_struct *entry,
              char *format, char buffer[], int bufflen, statsmean *means, int suf)
{
  /* Heat index from primary T and primary RH */

  int rv, rh ;
  float val ;

  rh = weather_primary_rh(means) ;

  if (rh == -1)
  {
    werr(WERR_DEBUG0, "Heat index not available - no RH sensor") ;
    val = parseout_convert_unit(entry, weather_primary_T(means)) ;
    return 0 ;
  }
  else
  {
    val = parseout_convert_unit(entry,
            meteo_hi(
              (double) weather_primary_T(means),
              (double) means->meanRH[rh],
              setup_hitype
            )
          ) ;
  }

  rv = parseout_print_float(
    buffer,
    bufflen,
    format,
    val,
    entry,
    means,
    suf) ;

  if (werr_will_output(WERR_DEBUG0) && (strstr(buffer, "nan") != NULL))
  {
    werr(WERR_DEBUG0, "parseout_heat_index returned nan") ;
    werr(WERR_DEBUG0, "means->meanT[0] = %g", means->meanT[0]) ;
    werr(WERR_DEBUG0, "means->meanRH[rh] = %g", means->meanRH[rh]) ;
    werr(WERR_DEBUG0, "meteo_heat_index() -> %g", val) ;
  }

  return rv ;
}

/*
   get_num_suffix

   Look for a (positive) number on the end.
   Chop it off.
   Return integer value > 0, 0 if not found, -1 on error
*/

static int
get_num_suffix(char *s)
{
  int i = 0, val ;

  /* Check for silly cases */
  if ((s == NULL) || (*s == '\0')) return -1 ;

  /* Get to end of string */
  while (s[i] != '\0') ++i ;

  /* Step back to last char */
  --i ;

  /* Not a digit? */
  if (!isdigit(s[i])) return 0 ;

  /* Step back over any more digits */
  while ((i > 0) && isdigit(s[i-1])) --i ;

  /* Convert suffix value */
  val = atoi(&s[i]) ;

  /* Curtail string at suffix */
  s[i] = '\0' ;

  return val ;
}

/*
   put_char

   Copy characters to output, solong as there is space

   s_out           output buffer
   i_out           index to start copying to
   N_out           total size allocated to s_out
   s_in            input string from which to copy
   N_in            number of characters to copy

   Returns index to next character in s_out

   Note that s_out will not be terminated, but copying will
   stop with space left for termination.
*/

static int
put_char(char *s_out, int i_out, int N_out, char *s_in, int N_in)
{
  int i ;

  /* Loop over characters in input string */
  for (i=0; i<N_in; ++i)
  {
    /* Is there space in s_out for this character? */
    /* We may write up to s_out[N_out - 2] */
    /* s_out[N_out - 1] is reserved for termination */

    if (i_out < N_out - 1)
    {
      /* OK - there is space */
      s_out[i_out] = s_in[i] ;
      ++i_out ;
    }
  }

  /* i_out is left indexing the next slot in s_out */
  /* As i_out <= N_out - 1 we may terminate here */
  return i_out ;
}

/*
   parse_tag

   Parse a tag, placing result in resultbuff, and return length
*/

static int
parse_tag(char *tag,
          char *formmod,
          statsmean *means,
          char resultbuff[],
          int resultbufflen)
{
  /*return sprintf(resultbuff, "Tag: \"%s\", Modifier: \"%s\"",
    tag, formmod) ;*/

  int i ;

  /* Strip any numerical suffix */
  tag_suffix = get_num_suffix(tag) ;

  if (tag_suffix < 0)
  {
    werr(0, _("Silly suffix (%d) in parser string"), tag_suffix) ;
    resultbuff[0] = '\0' ;
    return 0 ;
  }

  /* Look at parse entries for matching tag */
  for (i=0; (parseout[i].parse != NULL); ++i)
  {
    if (0 == strcmp(tag, parseout[i].tag))
    {
      /* Matching tag found */

      char format[64] ;

      /* Check suffix range */

      if (tag_suffix > parseout[i].maxsuf)
      {
        werr(0, _("Suffix (%d) greater than maximum allowed (%d) for tag \"%s\""),
          tag_suffix,
          parseout[i].maxsuf,
          parseout[i].tag) ;
        resultbuff[0] = '\0' ;
        return 0 ;
      }

      /* Call availability function if we have one */
      if (parseout[i].available)
      {
        if (!parseout[i].available(means, tag_suffix))
        {
          werr(WERR_DEBUG0,
               "%s%d is not available",
               tag, tag_suffix) ;
          resultbuff[0] = '\0' ;
          return 0 ;
        }
      }

      /* Build a format string for sprintf */
      sprintf(format, "%%%s%c", formmod, parseout[i].format_letter) ;

      /* Return the result of the parse function */
      return (parseout[i].parse(&parseout[i],
                                format,
                                resultbuff,
                                resultbufflen,
                                means,
                                tag_suffix)) ;
    }
  }

  /* Ran out of tags to try */
  werr(0, _("Unknown tag: \"%s\""), tag) ;

  return 0 ;
}

/*
   parseout_parse

   Parse string parseline, replacing tokens with values from stats

   Returns space needed for buffer or -1 for error
*/

static int
parseout_parse(statsmean *stats,
               char *parseline,
               char *buffer,
               int buffsupplied,
               int *buffneeded)
{
  int i_out = 0 ; /* Index into buffer for output */
  int i_in  = 0 ; /* Index into parseline for input */
  /* int buffneeded = 0 ; *//* Space needed */
  int i_tag ; /* Index into tagbuff */
  char tagbuff[64] ; /* To hold tags */
  char *formmod ; /* Format modifier */
  char resultbuff[64] = "" ; /* To hold result of parsing one tag */
  int resultlen ; /* Length of result string */

  *buffneeded = 0 ;

  /* Take a copy of the means  */
  //parseout_means = *stats ;
  memcpy((void *) &parseout_means, (void *) stats, sizeof(statsmean)) ;

  /* Loop over characters in parseline until \0 */

  while (parseline[i_in] != '\0')
  {
    formmod = NULL ;

    /* Copy up to $ */
    if (parseline[i_in] != '$')
    {
      i_out = put_char(buffer, i_out, buffsupplied, &parseline[i_in], 1) ;
      ++i_in ;
      ++*buffneeded ;
      continue ;
    }

    /* Is this a '$$' ? => insert $ symbol in output */
    ++i_in ;
    if (parseline[i_in] == '$')
    {
      /* Yes - insert $ */
      i_out = put_char(buffer, i_out, buffsupplied, &parseline[i_in], 1) ;
      ++i_in ;
      ++*buffneeded ;
      continue ;
    }

    /* This must be a tag */
    /* Copy to tag buffer, making lower case */

    for (i_tag=0; parseline[i_in+i_tag] != '$'; ++i_tag)
    {
      if (parseline[i_in+i_tag] == '\0')
      {
        werr(0, _("Unmatched $ symbol in \"%s\""), parseline) ;
        return -1 ;
      }

      /* Copy tags lower case */
      /* Copy format modifier as-is */
      tagbuff[i_tag] = (formmod) ?
                       parseline[i_in+i_tag] :
                       (char) tolower(parseline[i_in+i_tag]) ;

      /* Is this the start of a format modifier? */
      if (!formmod && (tagbuff[i_tag] == '%'))
      {
        /* Terminate tag here */
        tagbuff[i_tag] = '\0' ;

        /* Point formmod to next slot */
        formmod = &tagbuff[i_tag+1] ;
      }
    }

    /* So we have copied i_tag characters to tagbuff */
    /* Increment i_in to after the closing $ */
    i_in += i_tag + 1 ;

    /* Terminate tagbuff */
    tagbuff[i_tag] = '\0' ;

    // if (!formmod) formmod = "" ;

    /* Now parse the tag, passing any format modifier */
    /*werr(0, "parse_tag passed \"%s\" \"%s\"", tagbuff, formmod);*/
    resultlen = parse_tag(tagbuff,
                          (formmod != NULL) ? formmod : "",
                          &parseout_means,
                          resultbuff,
                          64) ;

    /* Check for error condition (-ve resultlen) */
    if (resultlen < 0) return -1 ; /* Error */

    /*werr(0, "parse_tag returned \"%s\"", resultbuff);*/

    /* Copy the result to the output buffer */
    i_out =
      put_char(buffer, i_out, buffsupplied, resultbuff, resultlen) ;
    *buffneeded += resultlen ;
  }

  /* Terminate output string */
  if (i_out < buffsupplied) buffer[i_out] = '\0' ;

  ++*buffneeded ;
  return 0 ; /* Ok */
}

/* Call parseout_parse, but handle memory allocation also

   buffptr       -      pointer to buffer address
   bufflen       -      pointer to int giving bytes allocated to buffer
   stats         -      pointer to mean values to use
   parseline     -      the string to parse
   suffix        -      string to add to end e.g. \n

   returns 0 on success, -1 on error or quits on memory exhaustion

*/

int
parseout_parse_and_realloc(char **buffptr,
                           int *bufflen,
                           statsmean *stats,
                           char *parseline,
                           char *suffix)
{
  int buffneeded = 0 ;
  int sufflen = 0 ;
  char *buff ;

  buff = *buffptr ;

  /* Need room for suffix? */
  if (suffix != NULL) sufflen = strlen(suffix) ;

  do
  {
    int status ;

    status = parseout_parse(stats,
                            parseline,
                            *buffptr,
                            *bufflen,
                            &buffneeded) ;

    if (status < 0) return status ; /* Pass back error condition */

    buffneeded += sufflen ;

    if (*bufflen < buffneeded * sizeof(char))
    {
      /*werr(WERR_DEBUG0, "realloc to %d", buffneeded);*/
      buff = (char *)
        realloc((void *) buff, buffneeded * sizeof(char)) ;

      *buffptr = buff ;

      if (buff)
      {
        *bufflen = buffneeded * sizeof(char) ;
      }
      else
      {
        *buffptr = (char *) NULL ;
        *bufflen = 0 ;
        werr(1, "parseout_parse_and_realloc: Out of memory") ;
      }
    }
    else
    {
      /* Add any suffix */
      if (suffix != NULL) strcat (*buffptr, suffix) ;
      return 0 ;
    }

  } while (1) ;

  free((void *) buff) ;
  *buffptr = (char *) NULL ;

  return -1 ;
}
