/* rainint.c

   Rain fall over a one-hour interval

   Oww project
   Dr. Simon J. Melhuish

   Fri 01st December 2000
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "rainint.h"

#include "werr.h"
#include "setup.h"

#include "devices.h"

#include "globaldef.h"

extern time_t the_time ;

///* The list of rain events in the preceding hour */
//static rainevent *rainint_list = NULL ;
//
///* The number of events recorded */
//static int rainint_N = 0 ;
//
///* The number of events for which memory has been allocated */
//static int rainint_Nalloc = 0 ;

rainlist rainlocal = {NULL, 0, 0, RAININT_TIMEOUT_1HR} ;
rainlist rainlocal24 = {NULL, 0, 0, RAININT_TIMEOUT_24HR} ;
rainlist gpclist[8] = {
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0},
  {NULL, 0, 0}
} ;

#define RAININT_CHUNK_SIZE 10

static int rainint_ensure_memory(rainlist *list)
{
  /* Ensure that enough memory is available */

  int size ;

  size = sizeof(rainevent) * RAININT_CHUNK_SIZE *
    (1 + list->N / RAININT_CHUNK_SIZE) ;

  if (size != list->N_alloc)
  {
    if (list->events)
    {
      /* Wrong size allocated - call realloc */
        list->events = (rainevent *) realloc(list->events, size) ;
    }
    else
    {
      /* No memory allocated yet */
      list->events = (rainevent *) malloc(size) ;
    }
  }

  list->N_alloc = size ;

  if (!list->events)
    werr(0, "Out of memory for rain data") ;

  return (list->events != NULL) ;
}

void
rainint_wipe(rainlist *list)
{
  if (!list) return ;

  if (list->events)
  {
    free(list->events) ;
    list->events = NULL ;
  }

  list->N = list->N_alloc = 0 ;
}

static void rainint_check_list(rainlist *list, time_t outdate)
{
  /* Look for out-of-date list entries and remove them */

  while (list->N > 0)
  {
    /* The last element (list->N-1) is the oldest */
    /* Exit loop as soon as we find an entry younger than outdate */

    if (list->events[list->N-1].t > outdate) break ;

    /* OK - So this entry is out-of-date */
    /* Drop the last entry */

    --list->N ;
  }
}

void rainint_new_rain(rainlist *list, wsstruct *vals)
{
  /* Latest WS poll has detected increment of monotonic rain counter */
  /* Check list and push the new value onto it */

  rainint_check_list(list, vals->t - list->timeout) ; /* Zap expired values */
  ++list->N ; /* New entry coming */

  /* Check that a suitable amount of memory was allocated */
  if (!rainint_ensure_memory(list))
  {
    werr(WERR_WARNING, "No memory for rain");
    return ;
  }

  /* Might need to shift old data */
  if (list->N > 1)
  {
    /* There were already entries on the list */
    int i ;

    for (i=list->N-1; i>0; --i)
      list->events[i] = list->events[i-1] ; /* Copy up one */
  }
//  list->events[0].rain = vals->rain_count ;
  list->events[0].delta_rain =
    (int) (vals->rain_count - vals->rain_count_old) ;
  list->events[0].t = vals->t ;
  list->events[0].delta_t = vals->delta_t ;

  // Check for crazy tip rate (> 2 Hz)
  if (list->events[0].delta_rain / list->events[0].delta_t > 2)
	list->events[0].delta_rain = 1;
}

float rainint_readout(rainlist *list)
{
  int i;
  int sum_delta = 0;

  /* Expire list and read out */

  rainint_check_list(list, the_time - list->timeout) ; /* Zap expired values */
  if (!rainint_ensure_memory(list)) return 0.0F ;

  /* The amount of rain that fell over the integration period is the sum of the deltas */
  for (i=0; i<list->N; ++i) {
	sum_delta += list->events[i].delta_rain;
  }

  return sum_delta * RAINCALIB;

//  /* The amount that fell is the latest reading - oldest reading
//     + the oldest delta (to get back the the reading before) */
//
//  switch (list->N) /* Check for special cases */
//  {
//    case 0: /* Special case - no rain events recorded */
//      return 0.0F ;
//      break ;
//
//    case 1: /* Special case - only one rain event recorded */
//      return (list->events[0].delta_rain * RAINCALIB) ;
//      break ;
//  }
//
//  /* General case */
//  return ((float) (list->events[0].rain - list->events[list->N-1].rain
//    + (unsigned long) list->events[list->N-1].delta_rain) * RAINCALIB) ;
}

float rainint_rate(rainlist *list, wsstruct *vals)
{
  /* Calculate the current rate of rainfall - or best guess */
  /* Call after rainint_new_rain(), so that list is up-to-date */
  /* Returns inches per hour */

  int rain_delta ;
  float old_rate, now_rate ;

  rain_delta = (int) (vals->rain_count - vals->rain_count_old) ;
  now_rate = (vals->delta_t>0)? rain_delta / vals->delta_t : 0 ;
  if (now_rate > 2) // > 2 Hz is a crazy rate for rain tips
	rain_delta = 1; // Assume this was just one real tip

  switch (rain_delta)
  {
    case 0:
      /* No tip this poll */
      /* What's on the list? */
      switch (list->N)
      {
        case 0:
          /* No rain this poll and no history of rain */
          /* So it's not raining! */
          return 0.0F ;
          break ;

        case 1:
          /* No rain this poll, but one event on list */
          old_rate = 3600.0F * RAINCALIB *
            (float) list->events[0].delta_rain /
            ((list->events[0].delta_rain > 1) ?
            (float) list->events[0].delta_t /* Multi-tip poll */
            : 3600.0F /* one-tip poll - maybe took an hour */) ;

          /* Should we expect another tip by now at this rate? */
          if ((float) (vals->t - list->events[0].t) * old_rate
            > RAINCALIB * 3600.0F)
            return (3600.0F * RAINCALIB / (float) (vals->t - list->events[0].t)) ;
          return (old_rate) ;
          break ;

        default:
          /* Multiple events on list */
          /* Was most recent event a multi-tip ? */
          if (list->events[0].delta_rain > 1)
          {
            /* Multiple tips in one integration */
            old_rate = 3600.0F * RAINCALIB *
              (float) list->events[0].delta_rain /
              (float) list->events[0].delta_t ;
          }
          else
          {
            /* Single tip over a period */
            old_rate = 3600.0F * RAINCALIB
//              * (float) (list->events[0].rain - list->events[1].rain)
              / (float) (list->events[0].t - list->events[1].t) ;
          }

          /* Should we expect another tip by now at this rate? */
          if ((float) (vals->t - list->events[0].t) * old_rate
            > RAINCALIB * 3600.0F)
            return (3600.0F * RAINCALIB / (float) (vals->t - list->events[0].t)) ;
          return (old_rate) ;
          break ;
      }
      break ;

    case 1:
      /* One tip this poll */
      /* So one measure fell since the last tip */
      if (list->N > 1)
      {
        /* Another recent tip recorded */
        /* Check time of last-but-one tip */
        return (3600.0F * RAINCALIB /
          (float) (list->events[0].t - list->events[1].t)) ;
      }
      else
      {
        /* No recent tip - assume it took an hour to collect this bucket */
        return (RAINCALIB) ; /* 0.01" per hour */
      }
      break ;

    default:
      /* Multiple tips this poll */
      if (0 == vals->delta_t) return 0.0F ;
      return (3600.0F * RAINCALIB *
        (float) rain_delta / (float) vals->delta_t) ;
      break ;
  }

  return 0.0F ;
}
