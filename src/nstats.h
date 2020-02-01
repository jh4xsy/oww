/*
 * nstats.h
 *
 *  Created on: 31-Mar-2009
 *      Author: sjm
 */

#ifndef NSTATS_H_
#define NSTATS_H_

#include "sllist.h"

typedef struct {
  sllist *list;
  int period; /** update interval in seconds */
} nstats;

#endif /* NSTATS_H_ */
