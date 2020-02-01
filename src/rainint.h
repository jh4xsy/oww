/* rainint.h

   Rain fall over a one-hour interval

   Oww project
   Dr. Simon J. Melhuish

   Fri 01st December 2000
*/

#ifndef RAININT_H
#define RAININT_H 1

#ifndef WSTYPES_H
#  include "wstypes.h"
#endif

#ifndef __time_h
#include <time.h>
#endif

#define RAININT_TIMEOUT_1HR 3600
#define RAININT_TIMEOUT_24HR 86400

typedef struct {
//  unsigned long rain       ; /* monotonic rain count */
  int           delta_rain ; /* new count - old count */
  time_t        t          ; /* time of measurement */
  int           delta_t    ; /* time since previous ws poll */
} rainevent ;

typedef struct {
  rainevent *events ; /* Pointer to array of events */
  int N ;             /* Number of events in list */
  int N_alloc ;       /* Allocation */
  int timeout ;
} rainlist ;

extern rainlist rainlocal ;
extern rainlist rainlocal24 ;
//extern rainlist gpclist[] ;

void
rainint_wipe(rainlist *list) ;

/* An increment of the rain guage was detected */
void  rainint_new_rain(rainlist *list, wsstruct *vals) ;

///* An increment of a GPC was detected */
//void rainint_new_gpc(rainlist *list, wsstruct *vals, int i) ;

/* Check the list to get latest hourly/24hr value */
float rainint_readout(rainlist *list) ;

/* Calculate (or best guess) rainfall rate (inches per hour) */
float rainint_rate(rainlist *list, wsstruct *vals) ;

#endif /* ifndef RAININT_H */
