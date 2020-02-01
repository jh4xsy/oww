/*int fill_in_logging(void) ;*/
/*int drag_end(int event_code, ToolboxEvent *event,
  IdBlock *id_block,void *handle) ;*/

#ifndef WSTYPES_H
#include "wstypes.h"
#endif

#define LOGTYPE_NONE  0
#define LOGTYPE_SINGL 1
#define LOGTYPE_DAILY 2

int log_stop_log_event(int event_code, void *, void *,void *) ;
int log_start_log_event(int event_code, void *, void *, void *) ;
/*int log_setup_averaging(void) ;*/
/*char *log_line(wsstruct *vals) ;*/
/*void log_accrue_values(wsstruct vals) ;*/
int log_poll(void) ; /* To be called from state_machine */

extern int logtype ;
