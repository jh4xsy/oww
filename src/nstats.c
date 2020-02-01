/*
 * nstats.c
 *
 *  Created on: 31-Mar-2009
 *      Author: sjm
 */

#include <stdlib.h>

#include "sllist.h"
#include "nstats.h"
#include "devices.h"

/**
 * List of nstats structures
 */
sllist *nstats_list = NULL;

static void nstats_add(nstats *stats) {
  sllist_append(&nstats_list, stats);
}

/**
 * Create a new nstats list, and add it to the list (of lists)
 */
nstats *nstats_new(int period){
  nstats *stats;

  stats = calloc(1, sizeof(nstats));
  stats->list = NULL;
  stats->period = period;

  nstats_add(stats);
}

/**
 * Have all devices add their entries to this stats list
 */
void nstats_populate(nstats *stats){
  int i;

  // For each device on the devices list...
  for (i=0; i<DEVICES_TOTAL; ++i){
	if (devices_list[i].stats_maker != NULL)
	  // Have a stats_maker - use it to make stats entry
	  sllist_append(&nstats_list, devices_list[i].stats_maker(&devices_list[i]));
  }
}
