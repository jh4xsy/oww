/* auxwin.h */

/* Dr. Simon J. Melhuish
   Oww project

   Maintainance of auxillary readout window

   GUI functions provided by auxwin_ro / auxwin_un
*/

#ifndef AUXWIN_H
#define AUXWIN_H 1

int
auxwin_init(void) ;

int
auxwin_update(wsstruct *wd) ;

int
auxwin_new_datasource(int ds) ;

#endif
