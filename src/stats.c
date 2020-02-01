/* stats.c

   Collect data for mean, max, min, &c

   Oww project
   Dr. Simon J. Melhuish

   Fri 01st December 2000
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
//#include <assert.h>

#include "wstypes.h"
#include "werr.h"
#include "stats.h"
#include "setup.h"
#include "globaldef.h"
#include "devices.h"

extern time_t the_time;

const char *stats_point_names[] = {
	"North", "NNE", "NE", "ENE",
	"East", "ESE", "SE", "SSE",
	"South", "SSW", "SW", "WSW",
	"West", "WNW", "NW", "NNW"
};

static double bearing_cos[] = {
	-0.382683432 /* Bearing is 1 - 16, so this value never used! */ ,
	0.0,
	0.382683432,
	0.707106781,
	0.923879532,
	1.0,
	0.923879532,
	0.707106781,
	0.382683432,
	0.0,
	-0.382683432,
	-0.707106781,
	-0.923879532,
	-1.0,
	-0.923879532,
	-0.707106781,
	-0.382683432
};

static double bearing_sin[] = {
	0.923879532 /* Bearing is 1 - 16, so this value never used! */ ,
	1.0,
	0.923879532,
	0.707106781,
	0.382683432,
	0.0,
	-0.382683432,
	-0.707106781,
	-0.923879532,
	-1.0,
	-0.923879532,
	-0.707106781,
	-0.382683432,
	0.0,
	0.382683432,
	0.707106781,
	0.923879532,
};

void
stats_new_anem (wsstruct vals, statsstruct * stats)
{
	/* Wind speed max/min */
	if (vals.anem_speed < stats->minWs)
		stats->minWs = vals.anem_speed;
	if (vals.anem_speed > stats->maxWs)
		stats->maxWs = vals.anem_speed;
}

void
stats_new_gpc (wsstruct vals, statsstruct * stats)
{
	int i;

	/* Maximum counter changes */
	for (i = 0; i < MAXGPC; ++i)
	{
		if (vals.gpc[i].max_sub_delta > stats->gpceventmax[i])
                  stats->gpceventmax[i] = vals.gpc[i].max_sub_delta;
	}
}

/* void stats_new_data(wsstruct vals, statsstruct *stats)

   vals - new data for integration
   stats - pointer to struct holding accrued values
*/

void
stats_new_data (wsstruct vals, statsstruct * stats)
{
	int i;

	++stats->N;
	stats->sumTime += the_time - stats->timeBase;

	/* Sum temperature readings and do max/min */
	for (i = 0; i < MAXTEMPS; ++i)
	{
		if (devices_have_temperature(i))
		{
			++stats->T_N[i];
			stats->sumT[i] += vals.T[i];
			if (vals.T[i] < stats->minT[i])
				stats->minT[i] = vals.T[i];
			if (vals.T[i] > stats->maxT[i])
				stats->maxT[i] = vals.T[i];
		}
	}

	/* Sum soil temperature readings and do max/min */
	for (i = 0; i < MAXSOILTEMPS; ++i)
	{
		if (devices_have_soil_temperature(i))
		{
			++stats->soilT_N[i];
			stats->sumSoilT[i] += vals.soilT[i];
			if (vals.soilT[i] < stats->minSoilT[i])
				stats->minSoilT[i] = vals.soilT[i];
			if (vals.soilT[i] > stats->maxSoilT[i])
				stats->maxSoilT[i] = vals.soilT[i];
		}
	}

	/* Sum indoor temperature readings and do max/min */
	for (i = 0; i < MAXINDOORTEMPS; ++i)
	{
		if (devices_have_indoor_temperature(i))
		{
			++stats->indoorT_N[i];
			stats->sumIndoorT[i] += vals.indoorT[i];
			if (vals.indoorT[i] < stats->minIndoorT[i])
				stats->minIndoorT[i] = vals.indoorT[i];
			if (vals.indoorT[i] > stats->maxIndoorT[i])
				stats->maxIndoorT[i] = vals.indoorT[i];
		}
	}

	/* Sum moisture readings and do max/min */
	for (i = 0; i < MAXMOIST*4; ++i)
	{
		if (devices_have_soil_moist(i))
		{
			++stats->moisture_N[i];
			stats->sumMoisture[i] += vals.soil_moist[i];
			if (vals.soil_moist[i] < stats->minMoisture[i])
				stats->minMoisture[i] = vals.soil_moist[i];
			if (vals.soil_moist[i] > stats->maxMoisture[i])
				stats->maxMoisture[i] = vals.soil_moist[i];
		}
	}

	/* Sum leaf wetness readings and do max/min */
	for (i = 0; i < MAXMOIST*4; ++i)
	{
		if (devices_have_leaf_wet(i))
		{
			++stats->leaf_N[i];
			stats->sumLeaf[i] += vals.leaf_wet[i];
			if (vals.leaf_wet[i] < stats->minLeaf[i])
				stats->minLeaf[i] = vals.leaf_wet[i];
			if (vals.leaf_wet[i] > stats->maxLeaf[i])
				stats->maxLeaf[i] = vals.leaf_wet[i];
		}
	}

	/* Humidity stats */
	for (i = 0; i < MAXHUMS; ++i)
	{
		if (devices_have_hum (i))
		{
			++stats->RH_N[i];
			stats->sumTrh[i] += vals.Trh[i];
			if (vals.Trh[i] < stats->minTrh[i])
				stats->minTrh[i] = vals.Trh[i];
			if (vals.Trh[i] > stats->maxTrh[i])
				stats->maxTrh[i] = vals.Trh[i];
			stats->sumRH[i] += vals.RH[i];
			if (vals.RH[i] < stats->minRH[i])
				stats->minRH[i] = vals.RH[i];
			if (vals.RH[i] > stats->maxRH[i])
				stats->maxRH[i] = vals.RH[i];
		}
	}

	/* Barometer stats */
	for (i = 0; i < MAXBAROM; ++i)
	{
		if (devices_have_barom (i))
		{
			int j;

			++stats->barom_N[i];
			stats->sumTb[i] += vals.Tb[i];

			/* Among min values? */
			for (j = 0; j < sizeof(stats->minTb[i]) / sizeof(*stats->minTb[i]); j++)
				if (vals.Tb[i] < stats->minTb[i][j]) {
					int k = sizeof(stats->minTb[i]) / sizeof(*stats->minTb[i]);
					while (--k > j)
						stats->minTb[i][k] = stats->minTb[i][k-1];
					stats->minTb[i][j] = vals.Tb[i];
					break;
				}

			/* Among max values? */
			for (j = 0; j < sizeof(stats->maxTb[i]) / sizeof(*stats->maxTb[i]); j++)
				if (vals.Tb[i] > stats->maxTb[i][j]) {
					int k = sizeof(stats->maxTb[i]) / sizeof(*stats->maxTb[i]);
					while (--k > j)
						stats->maxTb[i][k] = stats->maxTb[i][k-1];
					stats->maxTb[i][j] = vals.Tb[i];
					break;
				}

			stats->sumbarom[i] += vals.barom[i];

			/* Among min values? */
			for (j = 0; j < sizeof(stats->minbarom[i]) / sizeof(*stats->minbarom[i]); j++)
				if (vals.barom[i] < stats->minbarom[i][j]) {
					int k = sizeof(stats->minbarom[i]) / sizeof(*stats->minbarom[i]);
					while (--k > j)
						stats->minbarom[i][k] = stats->minbarom[i][k-1];
					stats->minbarom[i][j] = vals.barom[i];
					break;
				}

			/* Among max values? */
			for (j = 0; j < sizeof(stats->maxbarom[i]) / sizeof(*stats->maxbarom[i]); j++)
				if (vals.barom[i] > stats->maxbarom[i][j]) {
					int k = sizeof(stats->maxbarom[i]) / sizeof(*stats->maxbarom[i]);
					while (--k > j)
						stats->maxbarom[i][k] = stats->maxbarom[i][k-1];
					stats->maxbarom[i][j] = vals.barom[i];
					break;
				}
		}
	}

	if (devices_have_vane())
	{
	  /* Sum wind x and y components, and speed */

	  /* vane_bearing range is 1 to 16 i.e. 22.5 deg steps */
	  /* cos and sin values are pre-calculated */

	  if ((vals.vane_bearing >=0 ) && (vals.vane_bearing < 17 ))
	  {
		stats->sumWx +=
			vals.anem_speed * (float) bearing_cos[vals.vane_bearing];
		stats->sumWy +=
			vals.anem_speed * (float) bearing_sin[vals.vane_bearing];
	  }
	  stats->sumWs += vals.anem_speed;

	  /* Record bearing too, in case of zero wind speed */
	  stats->bearing = vals.vane_bearing;

	  /* Wind speed max/min */
	  if (vals.anem_speed < stats->minWs)
		stats->minWs = vals.anem_speed;
	  if (vals.anem_speed > stats->maxWs)
		stats->maxWs = vals.anem_speed;
	}

	/* Record monotonic rain value (tips) */
  if (devices_have_rain())
  {
    //if (stats->rainN == 0) // First rain reading this integration
	  //  stats->rainOld = stats->rainNow ;

    // rainOld collected by stats_reset()

	  stats->rainNow = vals.rain_count;
    ++stats->rainN ;

    stats->rainint1hr = vals.rainint1hr;
    stats->rainint24hr = vals.rainint24hr;
    stats->dailyrain = vals.dailyrain;
  }

	/* Record rain since last reset (inches) */
	stats->rain = vals.rain;

	for (i = 0; i < MAXGPC; ++i)
	{
		if (devices_have_gpc (i))
		{
      if (stats->gpcN[i] == 0) /* First read this integration? */
        stats->gpcOld[i] = vals.gpc[i].count_old ;

			/* Record monotonic gpc value (counts) */
			//stats->gpcOld[i] = stats->gpcNow[i] ;
			stats->gpcNow[i] = vals.gpc[i].count;
			stats->gpcEvents[i] = vals.gpc[i].event_count;

			/* Record gpc since last reset (arbitrary units) */
			stats->gpc[i] = vals.gpc[i].gpc;
      ++stats->gpcN[i] ;
		}
	}

	  // Solar sensors

	  for (i = 0; i < MAXSOL; ++i)
	  {
	    if (devices_have_solar_data(i))
	    {
	      if (stats->sol_N[i] == 0) // First value this int?
	        stats->sol_min[i] = stats->sol_max[i] = vals.solar[i] ;

	      ++stats->sol_N[i] ;
	      stats->sol_sum[i] +=  vals.solar[i] ;
	    }
	  }

	  // UV sensors

	  for (i = 0; i < MAXUV; ++i)
	  {
	    if (devices_have_uv_data(i))
	    {
	      if (stats->uvi_N[i] == 0) // First value this int?
	      {
	        stats->uvi_min[i] = stats->uvi_max[i] = vals.uvi[i] ;
	        stats->uviT_min[i] = stats->uviT_max[i] = vals.uviT[i] ;
	      }

	      ++stats->uvi_N[i] ;
	      stats->uvi_sum[i] +=  vals.uvi[i] ;
	      stats->uviT_sum[i] +=  vals.uviT[i] ;
	      if (vals.uvi[i]<stats->uvi_min[i])  stats->uvi_min[i]=vals.uvi[i];
	      if (vals.uvi[i]>stats->uvi_max[i])  stats->uvi_max[i]=vals.uvi[i];
	      if (vals.uviT[i]<stats->uviT_min[i])  stats->uviT_min[i]=vals.uviT[i];
	      if (vals.uviT[i]>stats->uviT_max[i])  stats->uviT_max[i]=vals.uviT[i];
	    }
	  }

	  // ADC sensors

	  for (i = 0; i < MAXADC; ++i)
	  {
	    if (devices_have_adc(i))
	    {
	      if (stats->adc_N[i] == 0) // First value this int?
	      {
	        stats->adc_min[i].V = stats->adc_max[i].V = vals.adc[i].V ;
	        stats->adc_min[i].I = stats->adc_max[i].I = vals.adc[i].I ;
	        stats->adc_min[i].Q = stats->adc_max[i].Q = vals.adc[i].Q ;
	        stats->adc_min[i].T = stats->adc_max[i].T = vals.adc[i].T ;
	      }

	      ++stats->adc_N[i] ;
	      stats->adc_sum[i].V +=  vals.adc[i].V ;
	      stats->adc_sum[i].I +=  vals.adc[i].I ;
	      stats->adc_sum[i].Q +=  vals.adc[i].Q ;
	      stats->adc_sum[i].T +=  vals.adc[i].T ;
	    }
	  }

	  // Thermocouples

	  for (i = 0; i < MAXTC; ++i)
	  {
	    if (devices_have_thrmcpl(i))
	    {
	      if (stats->tc_N[i] == 0) // First value this int?
	      {
	        stats->tc_min[i].T   = stats->tc_max[i].T   = vals.thrmcpl[i].T ;
	        stats->tc_min[i].Tcj = stats->tc_max[i].Tcj = vals.thrmcpl[i].Tcj ;
	        stats->tc_min[i].V   = stats->tc_max[i].V   = vals.thrmcpl[i].V ;
	      }

	      ++stats->tc_N[i] ;
	      stats->tc_sum[i].T   +=  vals.thrmcpl[i].T ;
	      stats->tc_sum[i].Tcj +=  vals.thrmcpl[i].Tcj ;
	      stats->tc_sum[i].V   +=  vals.thrmcpl[i].V ;
	    }
	  }
}

void
stats_reset (statsstruct * stats)
{
	/* Initialize the values in stats */

	unsigned long rainOld, gpcOld[MAXGPC];
	int i;
	time_t last_t = 0;

	rainOld = stats->rainNow;

	for (i = 0; i < MAXGPC; ++i)
		gpcOld[i] = stats->gpcNow[i];

	/* If this isn't the first reset, calculate last_t */
	if ((stats->N > 0) && (stats->timeBase != 0))
		last_t = stats->timeBase + stats->sumTime / stats->N;

	memset ((void *) stats, 0, sizeof (statsstruct));

	stats->rainOld = rainOld;

	stats->timeBase = the_time;
	stats->last_t = last_t;

	for (i = 0; i < MAXGPC; ++i)
	{
	  stats->gpcOld[i] = gpcOld[i];
	}

	/* Set min/max outside of reasonable range */

	stats->minWs = 1e6F;
	stats->maxWs = -1e6F;

	for (i = 0; i < MAXTEMPS; ++i)
	{
		stats->minT[i] = 1e6F;
		stats->maxT[i] = -1e6F;
	}

	for (i = 0; i < MAXINDOORTEMPS; ++i)
	{
		stats->minIndoorT[i] = 1e6F;
		stats->maxIndoorT[i] = -1e6F;
	}

	for (i = 0; i < MAXSOILTEMPS; ++i)
	{
		stats->minSoilT[i] = 1e6F;
		stats->maxSoilT[i] = -1e6F;
	}

	for (i = 0; i < MAXMOIST*4; ++i)
	{
		stats->minMoisture[i] = 1e6F;
		stats->maxMoisture[i] = -1e6F;
		stats->minLeaf[i] = 1e6F;
		stats->maxLeaf[i] = -1e6F;
	}

	for (i = 0; i < MAXGPC; ++i)
	{
		stats->gpceventmax[i] = 0.0F;
	}

	for (i = 0; i < MAXHUMS; ++i)
	{
		stats->minTrh[i] = stats->minRH[i] = 1e6F;
		stats->maxTrh[i] = stats->maxRH[i] = -1e6F;
	}

	for (i = 0; i < MAXBAROM; ++i)
	{
		int j;

		for (j = 0; j < sizeof(stats->minbarom[i]) / sizeof(*stats->minbarom[i]); j++)
			stats->minbarom[i][j] = 1e6F;
		for (j = 0; j < sizeof(stats->maxbarom[i]) / sizeof(*stats->maxbarom[i]); j++)
			stats->maxbarom[i][j] = -1e6F;
		for (j = 0; j < sizeof(stats->minTb[i]) / sizeof(*stats->minTb[i]); j++)
			stats->minTb[i][j] = 1e6F;
		for (j = 0; j < sizeof(stats->maxTb[i]) / sizeof(*stats->maxTb[i]); j++)
			stats->maxTb[i][j] = -1e6F;
	}
}

time_t
stats_mean_t (statsstruct * stats)
{
	if (stats->N > 0)
		return stats->timeBase + (time_t) (stats->sumTime / stats->N);
	else
		return (time_t) 0;
}

struct tm *
stats_mean_tm (statsstruct * stats)
{
	static time_t mean_time;
	mean_time = stats_mean_t (stats);
	if (mean_time == 0)
		mean_time = the_time + setup_log_interval / 2;	/* Best guess */

	return localtime (&mean_time);
}

static void
stats_closest_point (statsmean * means)
{
	/* Closest compass point */
	means->point = (int) floor (0.5 + (double) means->meanWd / 22.5);
	if (means->point < 0)
		means->point += 16;
	means->point = means->point % 16;
}

static void
stats_named_point (statsmean * means)
{
	/* Fill in the name of the compass point (in English) */

	if ((means->point < 0) || (means->point >= 16))
		means->point = 0;

	means->pointName = stats_point_names[means->point];
}

int
stats_do_means (statsstruct * stats, statsmean * means)
{
	/* Calculate the means */

	int i;

	if (stats->N < 1)
	{
		werr (0, "stats_do_means N = %d", stats->N);
		return 0;	/* Bad stats - expected N > 0 */
	}

	/* Mean time */
	means->meanTime = stats->timeBase + stats->sumTime / stats->N;
	means->meanTime_tm = *localtime (&means->meanTime);

	/* Mean temperatures */
	for (i = 0; i < MAXTEMPS; ++i)
	{
		if (stats->T_N[i] > 0)
		{
			float old_T;
			old_T = means->meanT[i];
			means->meanT[i] =
				stats->sumT[i] / (float) stats->T_N[i];
			means->haveT[i] = 1;
			stats->T_N[i] = 0;

#     ifdef HAVE_FINITEF
			if (!finitef (means->meanT[i]))
			{
				werr (WERR_WARNING,
				      "Non-finite T%d value generated in stats",
				      i + 1);
				werr (WERR_WARNING, "sumT = %f",
				      stats->sumT[i]);
				werr (WERR_WARNING, "T_N = %d",
				      stats->T_N[i]);

				means->meanT[i] = old_T;
			}
#     endif
		}
		else
		{
			means->meanT[i] = 0.0F;
			means->haveT[i] = 0;
		}
	}

	/* Mean soil temperatures */
	for (i = 0; i < MAXSOILTEMPS; ++i)
	{
		if (stats->soilT_N[i] > 0)
		{
			float old_T;
			old_T = means->meanSoilT[i];
			means->meanSoilT[i] =
				stats->sumSoilT[i] / (float) stats->soilT_N[i];
			means->haveSoilT[i] = 1;
			stats->soilT_N[i] = 0;

#     ifdef HAVE_FINITEF
			if (!finitef (means->meanSoilT[i]))
			{
				werr (WERR_WARNING,
				      "Non-finite Tsoil%d value generated in stats",
				      i + 1);
				werr (WERR_WARNING, "sumSoilT = %f",
				      stats->sumSoilT[i]);
				werr (WERR_WARNING, "soilT_N = %d",
				      stats->soilT_N[i]);

				means->meanSoilT[i] = old_T;
			}
#     endif
		}
		else
		{
			means->meanSoilT[i] = 0.0F;
			means->haveSoilT[i] = 0;
		}
	}

	/* Mean indoor temperatures */
	for (i = 0; i < MAXINDOORTEMPS; ++i)
	{
		if (stats->indoorT_N[i] > 0)
		{
			float old_T;
			old_T = means->meanIndoorT[i];
			means->meanIndoorT[i] =
				stats->sumIndoorT[i] / (float) stats->indoorT_N[i];
			means->haveIndoorT[i] = 1;
			stats->indoorT_N[i] = 0;

#     ifdef HAVE_FINITEF
			if (!finitef (means->meanIndoorT[i]))
			{
				werr (WERR_WARNING,
				      "Non-finite Tindoor%d value generated in stats",
				      i + 1);
				werr (WERR_WARNING, "sumIndoorT = %f",
				      stats->sumIndoorT[i]);
				werr (WERR_WARNING, "indoorT_N = %d",
				      stats->indoorT_N[i]);

				means->meanIndoorT[i] = old_T;
			}
#     endif
		}
		else
		{
			means->meanIndoorT[i] = 0.0F;
			means->haveIndoorT[i] = 0;
		}
	}

	/* Mean soil moisture */
	for (i = 0; i < MAXMOIST*4; ++i)
	{
		if (stats->moisture_N[i] > 0)
		{
			means->meanMoisture[i] =
				(float) stats->sumMoisture[i] / (float) stats->moisture_N[i];
			means->haveMoisture[i] = 1;
			stats->moisture_N[i] = 0;
		}
		else
		{
			means->meanMoisture[i] = 0.0F;
			means->haveMoisture[i] = 0;
		}
	}

	/* Mean leaf wetness */
	for (i = 0; i < MAXMOIST*4; ++i)
	{
		if (stats->leaf_N[i] > 0)
		{
			means->meanLeaf[i] =
				(float) stats->sumLeaf[i] / (float) stats->leaf_N[i];
			means->haveLeaf[i] = 1;
			stats->leaf_N[i] = 0;
		}
		else
		{
			means->meanLeaf[i] = 0.0F;
			means->haveLeaf[i] = 0;
		}
	}

	/* Mean temperatures and RH from hygrometers */
	for (i = 0; i < MAXHUMS; ++i)
	{
		if (stats->RH_N[i] > 0)
		{
			means->meanTrh[i] =
				stats->sumTrh[i] / (float) stats->RH_N[i];
			means->meanRH[i] =
				stats->sumRH[i] / (float) stats->RH_N[i];
			means->haveRH[i] = 1;
			stats->RH_N[i] = 0;
		}
		else
		{
			means->meanTrh[i] = 0.0F;
			means->meanRH[i] = 0.0F;
			means->haveRH[i] = 0;
		}
	}

	/* Mean barometer readings */
	for (i = 0; i < MAXBAROM; ++i)
	{
		if (stats->barom_N[i] > 0)
		{
		  int j, k;
			int percentage = 10; /* Filter 10% of the extremes and average the rest */
			int lim = floor(stats->barom_N[i] * percentage / 100);

			means->meanTb[i] =
				stats->sumTb[i] / (float) stats->barom_N[i];

			means->meanTb[i] = stats->sumTb[i];
			for (j = 0; j < (sizeof(stats->minTb[i]) / sizeof(*stats->minTb[i]) < lim ? sizeof(stats->minTb[i]) / sizeof(*stats->minTb[i]) : lim); j++)
			       means->meanTb[i] -= stats->minTb[i][j];
			for (k = 0; k < (sizeof(stats->maxTb[i]) / sizeof(*stats->maxTb[i]) < lim ? sizeof(stats->maxTb[i]) / sizeof(*stats->maxTb[i]) : lim); k++)
			       means->meanTb[i] -= stats->maxTb[i][k];
			means->meanTb[i] /= (float)(stats->barom_N[i] - j - k);

			means->meanbarom[i] = stats->sumbarom[i];
			for (j = 0; j < (sizeof(stats->minbarom[i]) / sizeof(*stats->minbarom[i]) < lim ? sizeof(stats->minbarom[i]) / sizeof(*stats->minbarom[i]) : lim); j++)
			       means->meanbarom[i] -= stats->minbarom[i][j];
			for (k = 0; k < (sizeof(stats->maxbarom[i]) / sizeof(*stats->maxbarom[i]) < lim ? sizeof(stats->maxbarom[i]) / sizeof(*stats->maxbarom[i]) : lim); k++)
			       means->meanbarom[i] -= stats->maxbarom[i][k];
			means->meanbarom[i] /= (float)(stats->barom_N[i] - j - k);

			means->havebarom[i] = 1;
			stats->barom_N[i] = 0;
		}
		else
		{
			means->meanTb[i] = 0.0F;
			means->meanbarom[i] = 0.0F;
			means->havebarom[i] = 0;
		}
	}

	/* Mean wind direction */
	if (stats->sumWx != 0.0)
	{
		means->meanWd = 90.0F - 57.29578F * (float)
			atan2 ((double) stats->sumWy, (double) stats->sumWx);
	}
	else
	{
		/* Zero wind speed - just use last value of vane */
		means->meanWd = (float) (stats->bearing - 1) * 22.5F;
	}

	/* Wind gust */
	means->maxWs = stats->maxWs;

	/* Confine to range 0 to 360 degrees */
	if (means->meanWd < 0)
		means->meanWd += 360.0F;

	/* Closest compass point */
	stats_closest_point (means);

	/* Named compass point */
	stats_named_point (means);

	/* Mean wind speed */
	means->meanWs = stats->sumWs / (float) stats->N;

	/* Rainfall since last reset */
	means->rain = stats->rain;

	means->rainint1hr = stats->rainint1hr;
	means->rainint24hr = stats->rainint24hr;
        means->dailyrain = stats->dailyrain;

	/* Rainfall this integration */
	if ((stats->rainN > 0) && (stats->rainNow > stats->rainOld))
	{
		means->deltaRain =
			RAINCALIB * (float) (stats->rainNow - stats->rainOld);
		if (stats->last_t > 0)
			means->rain_rate = 3600.0F * means->deltaRain /
				(float) (the_time - stats->last_t);
		else
			means->rain_rate = 0.0F;
	}
	else
	{
		means->deltaRain = means->rain_rate = 0.0F;
	}

	/* GPCs */
	for (i = 0; i < MAXGPC; ++i)
	{
		if (stats->gpcN[i] > 0)
		{
      int delta ;

			means->haveGPC[i] = 1;
			means->gpc[i] = stats->gpc[i];
      means->gpcmono[i] = stats->gpcNow[i] ;
			means->gpcEvents[i] = stats->gpcEvents[i] ;
                        means->gpceventmax[i] = stats->gpceventmax[i] ;

			if (stats->gpcNow[i] > stats->gpcOld[i])
      {
        delta = stats->gpcNow[i] - stats->gpcOld[i] ;
	means->deltagpc[i] =
	  devices_list[devices_GPC1 + i].calib[0] * (float) delta ;
        means->gpcrate[i] = 3600.0F * means->deltagpc[i] /
				  (float) (the_time - stats->timeBase);
      }
			else
      {
				means->deltagpc[i] = means->gpcrate[i] = 0.0F;
      }
		}
		else
		{
			means->haveGPC[i] = 0;
      means->gpc[i] = 0 ;
      means->gpcmono[i] = 0 ;
      means->deltagpc[i] = means->gpcrate[i] = 0.0F;
		}
	}

  // Solar sensors

  for (i = 0; i < MAXSOL; ++i)
  {
    if (stats->sol_N[i] > 0)
    {
      means->meansol[i] = stats->sol_sum[i] / (float) stats->sol_N[i] ;
      means->havesol[i] = 1 ;
    }
    else
    {
      means->meansol[i] = 0.0F ;
      means->havesol[i] = 0 ;
    }
  }

  // UV sensors

  for (i = 0; i < MAXUV; ++i)
  {
    if (stats->uvi_N[i] > 0)
    {
      means->meanuvi[i] = stats->uvi_sum[i] / (float) stats->uvi_N[i] ;
      means->meanuviT[i] = stats->uviT_sum[i] / (float) stats->uvi_N[i] ;
      means->haveuvi[i] = 1 ;
    }
    else
    {
      means->meanuvi[i] = means->meanuviT[i] = 0.0F ;
      means->haveuvi[i] = 0 ;
    }
  }

  // ADC sensors

  for (i = 0; i < MAXADC; ++i)
  {
    if (stats->adc_N[i] > 0)
    {
      means->meanADC[i].V = stats->adc_sum[i].V / (float) stats->adc_N[i] ;
      means->meanADC[i].I = stats->adc_sum[i].I / (float) stats->adc_N[i] ;
      means->meanADC[i].Q = stats->adc_sum[i].Q / (float) stats->adc_N[i] ;
      means->meanADC[i].T = stats->adc_sum[i].T / (float) stats->adc_N[i] ;
      means->haveADC[i] = 1 ;
    }
    else
    {
      means->meanADC[i].V =
      means->meanADC[i].I =
      means->meanADC[i].Q =
      means->meanADC[i].T = 0.0F ;
      means->haveADC[i] = 0 ;
    }
  }

  // Thermocouples

  for (i = 0; i < MAXTC; ++i)
  {
    if (stats->tc_N[i] > 0)
    {
      means->meanTC[i].T   = stats->tc_sum[i].T   / (float) stats->tc_N[i] ;
      means->meanTC[i].Tcj = stats->tc_sum[i].Tcj / (float) stats->tc_N[i] ;
      means->meanTC[i].V   = stats->tc_sum[i].V   / (float) stats->tc_N[i] ;
      means->haveTC[i]     = 1 ;
    }
    else
    {
      means->meanTC[i].T   =
      means->meanTC[i].Tcj =
      means->meanTC[i].V   = 0.0F ;
      means->haveTC[i]     = 0 ;
    }
  }

  return 1;		/* OK */
}

int
stats_do_ws_means (wsstruct * vals, statsmean * means)
{
	/* Use ws vals in place of means the means */

	int i;

	/* Mean time */
	means->meanTime = the_time;
	means->meanTime_tm = *localtime (&means->meanTime);

	/* Mean temperatures */
	for (i = 0; i < MAXTEMPS; ++i)
	{
		if (devices_have_temperature(i))
		{
			means->meanT[i] = vals->T[i];
			means->haveT[i] = 1;
		}
		else
		{
			means->meanT[i] = 0.0F;
			means->haveT[i] = 0;
		}
	}

	/* Mean soil temperatures */
	for (i = 0; i < MAXSOILTEMPS; ++i)
	{
		if (devices_have_soil_temperature(i))
		{
			means->meanSoilT[i] = vals->soilT[i];
			means->haveSoilT[i] = 1;
		}
		else
		{
			means->meanSoilT[i] = 0.0F;
			means->haveSoilT[i] = 0;
		}
	}

	/* Mean indoor temperatures */
	for (i = 0; i < MAXINDOORTEMPS; ++i)
	{
		if (devices_have_indoor_temperature(i))
		{
			means->meanIndoorT[i] = vals->indoorT[i];
			means->haveIndoorT[i] = 1;
		}
		else
		{
			means->meanIndoorT[i] = 0.0F;
			means->haveIndoorT[i] = 0;
		}
	}

	/* Mean soil moisture */
	for (i = 0; i < MAXMOIST*4; ++i)
	{
		if (devices_have_soil_moist(i))
		{
			means->meanMoisture[i] = (float) vals->soil_moist[i];
			means->haveMoisture[i] = 1;
		}
		else
		{
			means->meanMoisture[i] = 0.0F;
			means->haveMoisture[i] = 0;
		}
	}

	/* Mean leaf wetness */
	for (i = 0; i < MAXMOIST*4; ++i)
	{
		if (devices_have_leaf_wet(i))
		{
			means->meanLeaf[i] = (float) vals->leaf_wet[i];
			means->haveLeaf[i] = 1;
		}
		else
		{
			means->meanLeaf[i] = 0.0F;
			means->haveLeaf[i] = 0;
		}
	}

	/* Mean humidities */
	for (i = 0; i < MAXHUMS; ++i)
	{
		if (devices_have_hum (i))
		{
			means->meanTrh[i] = vals->Trh[i];
			means->meanRH[i] = vals->RH[i];
			means->haveRH[i] = 1;
		}
		else
		{
			means->meanTrh[i] = means->meanRH[i] = 0.0F;
			means->haveRH[i] = 0;
		}
	}

	/* Mean barometer readings */
	for (i = 0; i < MAXBAROM; ++i)
	{
		if (devices_have_barom (i))
		{
			means->meanTb[i] = vals->Tb[i];
			means->meanbarom[i] = vals->barom[i];
			means->havebarom[i] = 1;
		}
		else
		{
			means->meanbarom[i] = means->meanTb[i] = 0.0F;
			means->havebarom[i] = 0;
		}
	}

	/* Wind bearing and point */
	if (devices_have_vane())
	{
	  means->point = vals->vane_bearing - 1;
	  if ((means->point < 0) || (means->point > 15))
	  {
		werr (WERR_DEBUG0, "stats_do_ws_means() Wind point = %d",
			means->point);
		means->point = 0;
	  }
	}
	else
	{
	  means->point = 1;
	}
	means->meanWd = (float) means->point * 22.5F;

	/* Named compass point */
	stats_named_point (means);

	/* Mean wind speed */
	means->meanWs = vals->anem_speed;

	/* Wind gust - biggest value found during update interval */
	means->maxWs = vals->anem_int_gust;

	/* Rainfall since last reset */
  if (devices_have_rain())
  {
    means->rain = vals->rain;

    means->rainint1hr  = vals->rainint1hr;
    means->rainint24hr = vals->rainint24hr;
    means->dailyrain = vals->dailyrain;

    /* Rainfall rate */
    means->rain_rate = vals->rain_rate;

    /* Rainfall this integration */
    if (vals->rain_count > vals->rain_count_old)
      means->deltaRain = RAINCALIB *
        (float) (vals->rain_count - vals->rain_count_old);
    else
      means->deltaRain = 0.0F;
  }
  else
  {
    means->rain = means->rain_rate = means->deltaRain = 0.0F ;
  }

	for (i = 0; i < MAXGPC; ++i)
	{
		if (devices_have_gpc (i))
		{
			/* GPC since last reset */
			means->gpc[i] = vals->gpc[i].gpc;
			means->haveGPC[i] = 1;
      means->gpcmono[i] = vals->gpc[i].count ;
      means->gpcrate[i] = vals->gpc[i].rate ;
			means->gpcEvents[i] = vals->gpc[i].event_count;

			/* GPC increment this integration */
			if (vals->gpc[i].count > vals->gpc[i].count_old)
                        {
				means->deltagpc[i] =
					devices_list[devices_GPC1 +
						     i].calib[0] *
					(float) (vals->gpc[i].count -
						 vals->gpc[i].count_old);
                                means->gpceventmax[i] = vals->gpc[i].max_sub_delta ;
                        }
			else
                          means->deltagpc[i] = means->gpceventmax[i] = 0.0F;
		}
		else
		{
			means->gpc[i] = means->deltagpc[i] = 0.0F;
			means->haveGPC[i] = 0;
		}
	}

	  // Solar sensors

	  for (i = 0; i < MAXSOL; ++i)
	  {
	    if (devices_have_solar_data(i))
	    {
	      means->meansol[i] = vals->solar[i] ;
	      means->havesol[i] = 1 ;
	    }
	    else
	    {
	       means->meansol[i] = 0.0F ;
	       means->havesol[i] = 0 ;
	    }
	  }

	  // UV sensors

	  for (i = 0; i < MAXUV; ++i)
	  {
	    if (devices_have_uv_data(i))
	    {
	      means->meanuvi[i] = vals->uvi[i] ;
	      means->meanuviT[i] = vals->uviT[i] ;
	      means->haveuvi[i] = 1 ;
	    }
	    else
	    {
	       means->meanuvi[i] = means->meanuviT[i] = 0.0F ;
	       means->haveuvi[i] = 0 ;
	    }
	  }

	  // ADC sensors

	  for (i = 0; i < MAXADC; ++i)
	  {
	    if (devices_have_adc(i))
	    {
	      means->meanADC[i].V = vals->adc[i].V ;
	      means->meanADC[i].I = vals->adc[i].I ;
	      means->meanADC[i].Q = vals->adc[i].Q ;
	      means->meanADC[i].T = vals->adc[i].T ;
	      means->haveADC[i] = 1 ;
	    }
	    else
	    {
	      means->meanADC[i].V =
	      means->meanADC[i].I =
	      means->meanADC[i].Q =
	      means->meanADC[i].T = 0.0F ;
	      means->haveADC[i] = 0 ;
	    }
	  }

	  // Thermocouples

	  for (i = 0; i < MAXTC; ++i)
	  {
	    if (devices_have_thrmcpl(i))
	    {
	      means->meanTC[i].T = vals->thrmcpl[i].T ;
	      means->meanTC[i].Tcj = vals->thrmcpl[i].Tcj ;
	      means->meanTC[i].V = vals->thrmcpl[i].V ;
	      means->haveTC[i] = 1 ;
	    }
	    else
	    {
	      means->meanTC[i].T   = 0.0F ;
	      means->meanTC[i].Tcj = 0.0F ;
	      means->meanTC[i].V   = 0.0F ;
	      means->haveTC[i] = 0 ;
	    }
	  }

  return 1;		/* OK */
}

