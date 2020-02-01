/* wstypes.h */

#ifndef WSTYPES_H
#define WSTYPES_H 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <time.h>

#include "ad30.h"
#include "thermocouple.h"

#include "hobbyboards_moist.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */

#ifndef MAXTEMPS
#define MAXTEMPS           8
#endif

#ifndef MAXSOILTEMPS
#define MAXSOILTEMPS       2
#endif

#ifndef MAXINDOORTEMPS
#define MAXINDOORTEMPS     2
#endif

#ifndef MAXMOIST
#define MAXMOIST           2
#endif

#ifndef MAXHUMS
#define MAXHUMS            8
#endif

#ifndef MAXBAROM
#define MAXBAROM           8
#endif

#ifndef MAXGPC
#define MAXGPC             8
#endif

#ifndef MAXSOL
#define MAXSOL             4
#endif

#ifndef MAXUV
#define MAXUV             4
#endif

#ifndef MAXADC
#define MAXADC             4
#endif

/*#ifndef MAXTC
#define MAXTC             4
#endif*/

#ifndef MAXLCD
#define MAXLCD             4
#endif

#ifndef MAXBRANCHES
#define MAXBRANCHES        9
#endif

// family codes of devices
#define TEMP_FAMILY        0x10
#define TEMP_FAMILY_DS1822 0x22
#define TEMP_FAMILY_DS18B20 0x28
#define SBATTERY_FAMILY    0x26
#define ADC_DS2760_FAMILY  0x30
#define SWITCH_FAMILY      0x12
#define COUNT_FAMILY       0x1D
#define DIR_FAMILY         0x01
#define BRANCH_FAMILY      0x1F
#define NULL_FAMILY        0xFF
#define ATOD_FAMILY        0X20
#define PIO_FAMILY         0x29
#define HYGROCHRON_FAMILY  0x41
#define HOBBYBOARDS_FAMILY 0xEE
#define HOBBYBOARDS_NO_T_FAMILY 0xEF
// switch states
#define SWITCH_AON_BON     0x1F
#define SWITCH_AOFF_BON    0x3F
#define SWITCH_AON_BOFF    0x4F
#define SWITCH_AOFF_BOFF   0x7F

#ifndef WSTYPES_H
  #define WSTYPES_H 1
#endif

#ifndef HAVE_UINT32_T
typedef unsigned long uint32_t ;
#endif

#ifndef HAVE_INT32_T
typedef long int32_t ;
#endif

#ifndef HAVE_INT64_T
typedef long long int		int64_t;
#endif

#define COUNT_NOT_SET 0xffffffffU

typedef struct {
  uint32_t count ;         /* Latest count value */
  uint32_t count_old ;     /* Previous count value */
  uint32_t count_delta ;   /* Change in counts */
  uint32_t count_offset ;  /* Value to subtract */
  uint32_t subcount ;      /* Count at latest sub-update */
  uint32_t subcount_old ;  /* Count at previous sub-update */
  uint32_t subcount_delta ;/* Difference between last two sub-update counts */
  float max_sub_delta ;         /* Maximum event size */
  uint32_t max_subcount_delta_scratch ;/* Maximum event delta in counts */
  uint32_t event_count ;   /* Change counter (e.g. lightning counter) */
  float         sub_delta ;     /* sub-integration calibrated delta */
  time_t        time_reset ;    /* Time corresponding to count_offset */
  time_t        time ;          /* Time of last read */
  time_t        time_delta ;    /* Time since last read */
  time_t        time_last_inc ; /* Time since last increment */
  float         gpc ;           /* calib[0] * (count - count_offset) */
  float         delta ;         /* count_delta * calib[0] */
  float         rate ;          /* 3600 * delta / time_delta */
} gpc_struct ;

typedef struct {
  uint32_t anem_start_count /* Anemometer counter reading at start */,
           anem_end_count   /* Anemometer counter reading at end */,
           gust_start_count,
           gust_end_count,
           rain_count       /* Rain gauge counter reading */,
           rain_offset[8]   /* Rain gauge offset values */,
           dailyrain_offset /* Rain gauge offset for Wunderground midnight reset */,
           rain_count_old ;
  float rainint1hr ;      /* Rainfall in the last hour */
  float rainint24hr ;     /* Rainfall in the last 24 hours */
  float dailyrain ; /* Rainfall today */
  char rain_date[128] ;
  float rain_rate ; /* Inches per hour */
  time_t t_rain[8] ; /* Unix time each rain reset */
  time_t t_dailyrain ; /* Unix time of last daily rain reset */
  float Tmin, Tmax, anem_speed_max ; /* "Daily" stats values */
  float anem_int_gust ; /* Max gust during an update interval */
  float anem_int_gust_scratch ; /* Working value for int gust - copied to anem_int_gust at end */
  time_t t ; /* The time at which these values were recorded */
  int delta_t ; /* The change in t */
  int interval ; /* How long till the next results are due */
  float latitude, longitude ; /* Station location */
  int64_t anem_start_time, anem_end_time ;
  int64_t gust_start_time, gust_end_time ;
  float anem_speed, anem_mps, anem_gust, rain ;
  int vane_bearing, vane_mode ;
  unsigned char vane1_sn[12], vane2_sn[12]/*, anem_sn[12], rain_sn[12]
    T_sn[MAXTEMPS][12], switch_sn[12]*/ ;
  int Tnum;
  float T[MAXTEMPS] ;     /* Celsius */
//  int soilTnum;
  float soilT[MAXSOILTEMPS] ; /* Celsius */
//  int indoorTnum;
  float indoorT[MAXINDOORTEMPS] ; /* Celsius */
  int Hnum;
  float RH[MAXHUMS] ;     /* per cent */
  float Trh[MAXHUMS] ;    /* Celsius */
  int Bnum;
  float barom[MAXBAROM] ; /* mBar */
  float Tb[MAXBAROM] ;    /* Celsius */
  int gpcnum;
  gpc_struct gpc[MAXGPC] ; /* General Purpose Counters */
  int solnum;
  float solar[MAXSOL] ;   /* Solar radiation values */
  int adcnum;
  adc_struct adc[MAXADC] ; /* General ADC sensors */
  int uvnum;
  float uvi[MAXUV] ;   /* UVI values */
  float uviT[MAXUV] ;   /* UVI T values */
  int tcnum;
  tc_struct thrmcpl[MAXTC] ; /* Thermocouples */
  int moistnum;
  int soil_moist[MAXMOIST*4];
  int leaf_wet[MAXMOIST*4];
//  moist_struct moist[MAXMOIST] ; /* Moisture channels */
} wsstruct ;
#endif
