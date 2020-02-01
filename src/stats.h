/* stats.h

   Collect data for mean, max, min, &c

   Oww project
   Dr. Simon J. Melhuish

   Fri 01st December 2000
*/

#ifndef STATS_H
#define STATS_H 1

#ifndef WSTYPES_H
#  include "wstypes.h"
#endif
#ifndef __time_h
#  include <time.h>
#endif

#include "rainint.h"

#ifndef MAXEXTREMES
#define MAXEXTREMES 8
#endif

typedef struct {
  int N ;                 /* Number of contributing readings */
  time_t timeBase ;       /* time of first contributing readings */
  time_t sumTime ;        /* sum of (time - timeBase) */
  time_t last_t ;         /* Time of the last readout before this integration */
  float sumT[MAXTEMPS] ;  /* Temperature sums */
  int   T_N[MAXTEMPS] ;   /* Temperature N */
  float sumWx ;           /* Sum of wind x components */
  float sumWy ;           /* Sum of wind y components */
  float sumWs ;           /* Sum of wind speeds */
  float minT[MAXTEMPS] ;  /* Minimum temperatures */
  float minWs ;           /* Minimum wind speed */
  float maxWs ;           /* Maximum wind speed */
  float maxT[MAXTEMPS] ;  /* Maximum temperatures */
  float sumSoilT[MAXSOILTEMPS] ;  /* Soil Temperature sums */
  int   soilT_N[MAXSOILTEMPS] ;   /* Soil Temperature N */
  float minSoilT[MAXSOILTEMPS] ;  /* Soil Minimum temperatures */
  float maxSoilT[MAXSOILTEMPS] ;  /* Soil Maximum temperatures */
  float sumIndoorT[MAXINDOORTEMPS] ;  /* Indoor Temperature sums */
  int   indoorT_N[MAXINDOORTEMPS] ;   /* Indoor Temperature N */
  float minIndoorT[MAXINDOORTEMPS] ;  /* Indoor Minimum temperatures */
  float maxIndoorT[MAXINDOORTEMPS] ;  /* Indoor Maximum temperatures */
  int   sumMoisture[MAXMOIST*4] ; /* Soil moisture sums */
  int   moisture_N[MAXMOIST*4] ; /* Soil moisture N */
  int   minMoisture[MAXMOIST*4] ; /* Soil moisture minima */
  int   maxMoisture[MAXMOIST*4] ; /* Soil moisture maxima */
  int   sumLeaf[MAXMOIST*4] ; /* Leaf wetness sums */
  int   leaf_N[MAXMOIST*4] ; /* Leaf wetness N */
  int   minLeaf[MAXMOIST*4] ; /* Leaf wetness minima */
  int   maxLeaf[MAXMOIST*4] ; /* Leaf wetness maxima */
  int bearing ;           /* Latest wind bearing */
  float rain ;            /* Rainfall (not monotonic) */
  float rainint1hr ;      /* Rainfall in the last hour */
  float rainint24hr ;     /* Rainfall in the last 24 hours */
  float dailyrain ;       /* Rainfall today */
  int rainN ;             /* Number of rainfall readings taken */
  unsigned long rainNow ; /* Latest rain reading (monotonic) */
  unsigned long rainOld ; /* Rain reading from previous integration */
  float gpc[MAXGPC] ;            /* GPC values (not monotonic) */
  uint32_t gpcNow[MAXGPC] ; /* Latest GPC readings (monotonic) */
  uint32_t gpcOld[MAXGPC] ; /* GPC readings from previous integration */
  uint32_t gpcEvents[MAXGPC] ; /* Latest GPC event */
  float gpceventmax[MAXGPC] ; /* Maximum GPC event delta */
  int gpcN[MAXGPC] ;      /* GPC readings */
  float sumTrh[MAXHUMS] ; /* Temperature sums from RH sensors */
  float minTrh[MAXHUMS] ; /* Minimum temperatures from RH sensors */
  float maxTrh[MAXHUMS] ; /* Maximum temperatures from RH sensors */
  float  sumRH[MAXHUMS] ; /* RH sums */
  int     RH_N[MAXHUMS] ; /* RH N from RH sensors */
  float  minRH[MAXHUMS] ; /* Minimum RH from RH sensors */
  float  maxRH[MAXHUMS] ; /* Maximum RH from RH sensors */
  float  sumbarom[MAXBAROM] ; /* barom sums */
  int     barom_N[MAXBAROM] ; /* barom N from barom sensors */
  float  minbarom[MAXBAROM][MAXEXTREMES] ; /* Minimum barom from barom sensors */
  float  maxbarom[MAXBAROM][MAXEXTREMES] ; /* Maximum barom from barom sensors */
  float sumTb[MAXBAROM] ; /* Temperature sums from bp sensors */
  float minTb[MAXBAROM][MAXEXTREMES] ; /* Minimum temperatures from bp sensors */
  float maxTb[MAXBAROM][MAXEXTREMES] ; /* Maximum temperatures from bp sensors */
  int   sol_N[MAXSOL] ;   /* Solar radiation No. of values */
  float sol_sum[MAXSOL] ; /* Solar radiation summation */
  float sol_min[MAXSOL] ; /* Solar radiation minimum */
  float sol_max[MAXSOL] ; /* Solar radiation  */
  int   uvi_N[MAXSOL] ;   /* UVI No. of values */
  float uvi_sum[MAXSOL] ; /* UVI summation */
  float uvi_min[MAXSOL] ; /* UVI minimum */
  float uvi_max[MAXSOL] ; /* UVI maximum */
  float uviT_sum[MAXSOL] ; /* UVI T summation */
  float uviT_min[MAXSOL] ; /* UVI T minimum */
  float uviT_max[MAXSOL] ; /* UVI T maximum */
  int adc_N[MAXADC] ; /* ADC No. of values */
  adc_struct adc_sum[MAXADC] ; /* ADC summation */
  adc_struct adc_min[MAXADC] ; /* ADC minimum */
  adc_struct adc_max[MAXADC] ; /* ADC maximum */
  int tc_N[MAXTC] ; /* TC No. of values */
  tc_struct tc_sum[MAXTC] ; /* TC summation */
  tc_struct tc_min[MAXTC] ; /* TC minimum */
  tc_struct tc_max[MAXTC] ; /* TC maximum */
} statsstruct ;

typedef struct {
  time_t meanTime ;       /* Mean time for this integration */
  struct tm meanTime_tm ; /* Mean time as struct tm - localtime */
  float rain ;            /* Rainfall (not monotonic) */
  float deltaRain ;       /* Rainfall this integration */
  float rain_rate ;       /* Rainfall rate */
  float rainint1hr ;      /* Rainfall in the last hour */
  float rainint24hr ;     /* Rainfall in the last 24 hours */
  float dailyrain ;       /* Rainfall today */
  float meanWd ;          /* Mean wind direction */
  float meanWs ;          /* Mean wind speed */
  float maxWs ;           /* Maximum wind speed */
  int point ;             /* Closest compass point */
  const char *pointName ; /* Compass point as a name */
  int   haveT[MAXTEMPS] ; /* Valid meanT[i] value? */
  float meanT[MAXTEMPS] ; /* Temperature means */
  int   haveSoilT[MAXSOILTEMPS] ; /* Valid meanT[i] value? */
  float meanSoilT[MAXSOILTEMPS] ; /* Temperature means */
  int   haveIndoorT[MAXINDOORTEMPS] ; /* Valid meanT[i] value? */
  float meanIndoorT[MAXINDOORTEMPS] ; /* Temperature means */
  int   haveMoisture[MAXMOIST*4] ; /* Valid meanMoisture[i] value? */
  float meanMoisture[MAXMOIST*4] ; /* Soil moisture means */
  int   haveLeaf[MAXMOIST*4] ; /* Valid meanLeaf[i] value? */
  float meanLeaf[MAXMOIST*4] ; /* Soil leaf wetness means */
  int   haveRH[MAXHUMS] ; /* Valid meanRH[i] value? */
  float meanTrh[MAXHUMS] ;/* Temperature means from RH sensors */
  float meanRH[MAXHUMS] ; /* RH means from RH sensors */
  int   havebarom[MAXBAROM] ; /* Valid meanbarom[i] value? */
  float meanbarom[MAXBAROM] ; /* barometer means from barometer sensors */
  float meanTb[MAXBAROM] ; /* T means from barometer sensors */
  int havesol[MAXSOL] ; /* Valid meansol[i] value? */
  float meansol[MAXSOL] ; /* Mean solar radiation */
  int haveuvi[MAXUV] ; /* Valid meanuv[i] value? */
  float meanuvi[MAXUV] ; /* Mean UV index */
  float meanuviT[MAXUV] ; /* Mean UV T */
  int haveADC[MAXADC] ; /* Valid ADC[i] values? */
  adc_struct meanADC[MAXADC] ; /* ADC means */
  int haveTC[MAXTC] ; /* Valid TC[i] values? */
  tc_struct meanTC[MAXTC] ; /* Mean thermocouple values */
  int haveGPC[MAXGPC] ;   /* Valid GPC value? */
  float gpc[MAXGPC] ;     /* Absolute gpc */
  float deltagpc[MAXGPC] ; /* GPC increment this integration */
  uint32_t gpcmono[MAXGPC] ; /* Monotonic gpc count */
  uint32_t gpcEvents[MAXGPC] ; /* Gpc event counter */
  float gpceventmax[MAXGPC] ; /* Gpc maximum event delta */
  float gpcrate[MAXGPC] ; /* gpc rate */
} statsmean ;

void stats_new_anem(wsstruct vals, statsstruct *stats) ;
void stats_new_gpc(wsstruct vals, statsstruct *stats) ;
void stats_new_data(wsstruct vals, statsstruct *stats) ;
void stats_reset(statsstruct *stats) ;
int  stats_do_means(statsstruct *stats, statsmean *means) ;
int  stats_do_ws_means(wsstruct *vals, statsmean *means) ;
struct tm *stats_mean_tm(statsstruct *stats) ;
time_t stats_mean_t(statsstruct *stats) ;

#endif

