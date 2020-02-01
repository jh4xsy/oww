#ifndef WERR_H
#define WERR_H 1

/* Bits in the werr flags */
#define WERR_FATAL     1
#define WERR_WARNING   2
#define WERR_AUTOCLOSE 4
#define WERR_DEBUGONLY 8
#define WERR_DBLEVBIT0 0x10
#define WERR_DBLEVBIT1 0x20

/* flags for main debug levels */
#define WERR_DEBUG0 0x08
#define WERR_DEBUG1 0x18
#define WERR_DEBUG2 0x28
#define WERR_DEBUG3 0x38

enum werr_level {
  werr_level_none = 0,
  werr_level_fatal_only,
  werr_level_errors,
  werr_level_errors_and_warnings,
  werr_level_debug_0,
  werr_level_debug_1,
  werr_level_debug_2,
  werr_level_debug_3,
  werr_level_number
} ;

#define WERR_AUTOCLOSE_DELAY 60UL

#ifdef RISCOS
  #ifndef __kernel_h
    #include "kernel.h"
  #endif
  extern _kernel_oserror *e ;
#else
typedef struct {
   int errnum;           /* error number */
   char errmess[252];    /* error message (zero terminated) */
} _kernel_oserror;
#endif

extern int debug_level ;
extern int werr_message_level ;
extern char debug_file[] ;

extern void werr(int fatal, char* format, ...) ;
/*extern int debug_level ;*/

/* Will a call to werr() generate any output? */

int
werr_will_output(int flags) ;

/* Report any ownet errors */
void werr_report_owerr(void) ;


#ifndef NOGUI
extern void werr_poll(void) ;
#endif

#endif // ifndef WERR_H
