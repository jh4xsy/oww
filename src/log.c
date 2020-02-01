/* log.c */

/*
 * log.c
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * August - December 1999
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/* Log-keeping functions */
/* Platform-independent */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>

//#include "message.h"
#include "werr.h"
#include "convert.h"
#include "wstypes.h"
#include "setup.h"
#include "stats.h"

#include "events.h"
#include "file.h"
#include "oww.h"
#include "progstate.h"
#include "utility.h"
#include "globaldef.h"
#include "log.h"
#include "parseout.h"
#include "intl.h"

#ifndef RISCOS
#  include "process.h"
#endif

time_t log_time = 0 ;
extern time_t the_time ;


statsstruct logstats ;

/* Means calculated by log_line from most recent logstats */
static statsmean logmeans ;

/* Means copied by log_line from last update */
static statsmean updatemeans ;

//int therm_bcst(wsstruct *ws) ;

/* What is the directory to log to right now? */

static char *
dir_for_today(void)
{
  static char todays_dir[MAXFILENAME] ;

  strncpy(todays_dir, setup_logdir_name, MAXFILENAME-1) ;
  todays_dir[MAXFILENAME-1] = '\0' ;

  strftime(&todays_dir[strlen(setup_logdir_name)],
    MAXFILENAME - strlen(setup_logdir_name) - 1,
    setup_format_logdir,
    stats_mean_tm(&logstats)) ;

  return todays_dir ;
}

/* What is the file to log to right now? */

static char *
file_for_today(void)
{
  static char todays_file[MAXFILENAME] ;

  strncpy(todays_file, setup_logdir_name, MAXFILENAME-1) ;
  todays_file[MAXFILENAME-1] = '\0' ;

  strftime(&todays_file[strlen(setup_logdir_name)],
    MAXFILENAME - strlen(setup_logdir_name) - 1,
    setup_format_logfile,
    stats_mean_tm(&logstats)) ;

  return todays_file ;
}

/* Write a log line to the log file */

static int
add_log_entry(char *log_name, char *log_entry, char mode[])
{
  FILE *fp ;

  if (log_entry[0] == '\0')
  {
    werr(WERR_DEBUG0, "Zero-length log line") ;
    return 1 ;
  }

  if (!log_name)
  {
    werr(0, "Tried to add entry to log file with NULL name") ;
    return 0 ;
  }

  if (log_name[0] == '\0')
  {
    werr(0, "Tried to add entry to log file with no name") ;
    return 0 ;
  }

  fp = fopen(log_name, mode) ;

  if (!fp)
  {
    werr(WERR_WARNING, _("add_log_entry(\"%s\", \"%s\") - could not open file"),
         log_name,
         log_entry) ;
    return 0 ;
  }

  if  (fputs(log_entry, fp) == EOF)
  {
    werr(WERR_WARNING, _("Error writing to \"%s\""), log_name) ;
    fclose(fp) ;
    return 0 ;
  }

  fclose(fp) ;
  return 1 ;
}

/* Generate a line for the log */

static char *
log_line(wsstruct *vals)
{
  /* Update mean values from logstats or last update */

  static char *logline = NULL ;
  static int logline_alloced = 0 ;
  statsmean *means ;

  if (vals)
  {
    stats_do_ws_means(vals, &updatemeans) ;
    means = &updatemeans ;
  }
  else
  {
    if (!stats_do_means(&logstats, &logmeans))
    {
      werr(WERR_DEBUG0, "stats_do_means failed") ;
      return "\n" ;
    }
    means = &logmeans ;
  }


  if (parseout_parse_and_realloc(&logline,
                             &logline_alloced,
                             means,
                             setup_format_logline,
                             "\n") < 0)
  {
    /* An error was returned */
    return NULL ;
  }

  return logline ;
}

/* Set and check the log time */
static void
set_log_time(time_t suggested_time)
{
  log_time = suggested_time ;

  if ((setup_log_snap) && (log_time % setup_log_interval != 0))
    log_time -= log_time % setup_log_interval ;

  if (log_time + setup_log_interval < the_time)
  {
    /* Need to catch up */
    log_time += setup_log_interval *
      ((the_time - log_time) / setup_log_interval) ;
  }
}

/* Log event - start a log / write a log line */
/* Returns 1 on success */

int
log_start_log_event(int event_code,
                    void *event,
                    void *id_block,
                    void *handle)
{
  char *logline = NULL ;

  /* Response to filer save operation,  return char in setup or log event */
  /* Create file / directory - write data for log event */


  logline = (handle == NULL) ? "" : (char *) handle ;

  if (event_code != USER_ADD_LOG)
  {
    /* Just writing headers - no log line */
    stats_reset(&logstats) ;

    /* Set log time */

    /* The mean time of the previous (non-existant) integration
       would have been half a log interval before the time now */
    set_log_time(the_time - setup_log_interval / 2) ;
  }
  else
  {
    /* Update log time */
    set_log_time(log_time + setup_log_interval) ;

    if (logline[0] != '\0')
    {
#     ifdef RISCOS
      /* Update system variable for any log type */
      if (setup_logvar[0])
        setenv(setup_logvar, logline, 1) ;
#     else
      /* Update single-line log file */
      if (setup_logvar[0])
        add_log_entry(setup_logvar, logline, "w") ;
#     endif

      /* Run any external postlog progam */
      if (setup_postlog && setup_postlog[0])
      {
        static char *postlog_buffer = NULL ;
        static int postlog_alloced = 0 ;

        if ((0 != parseout_parse_and_realloc(&postlog_buffer,
                                   &postlog_alloced,
                                   &logmeans,
                                   setup_postlog,
                                   "")) || (!postlog_buffer))
        {
          werr(WERR_WARNING, _("postlog command line did not parse correctly")) ;
        }
        else
        {

#         ifdef RISCOS
          system(postlog_buffer) ;
#         else
          if (0 > process_new(postlog_buffer))
            werr(WERR_WARNING, _("Failed to run postlog command")) ;
#         endif
        }
      }
    }
  }

  switch(setup_logtype)
  {
    case LOGTYPE_NONE:
      werr(WERR_WARNING, _("Trying to write no log!")) ;
      break ;

    case LOGTYPE_SINGL:
    {
        if (!file_check_file(setup_logfile_name, 1)) return 0 ; /* Failed */

        if (event_code == USER_ADD_LOG)
        {
          if (!add_log_entry(setup_logfile_name, logline, "a"))
          {
            werr(WERR_WARNING, "Attempt to update log (\"%s\") failed [calling add_log_entry called log_start_log_event", setup_logfile_name) ;
            return 0 ; /* Failed */
          }
        }
        else
        {
          /* Complete the save protocol */

          #ifndef NOGUI
          if (handle) file_complete_save(handle) ;
          #endif
        }
      break ;
    }

    case LOGTYPE_DAILY:
    {
      /*char *dot ;*/

      if (!file_check_dir(setup_logdir_name, 1)) return 0 ; /* Failed */

        /* OK - main directory exists */
        if (!file_check_dir(dir_for_today(), 1)) return 0 ; /* Failed */

        if (!file_check_file(file_for_today(), 1)) return 0 ; /* Failed */

        if (event_code == USER_ADD_LOG)
        {
          if (!add_log_entry(file_for_today(), logline, "a"))
          {
            werr(WERR_WARNING, "Attempt to update log (\"%s\") failed [calling add_log_entry from log_start_log_event]", file_for_today()) ;
            return 0 ; /* Failed */
          }
        }
        else
        {
          /* Complete the save protocol */

          #ifndef NOGUI
          if (handle) file_complete_save(handle) ;
          #endif
         }
      break ;
    }
  }
  return 1 ; /* OK */
}

int
log_stop_log_event(int event_code,
                   void *event,
                   void *id_block,
                   void *handle)
{
  /*int size, sn=0 ;*/

  log_time = 0 ; /* This stops the logging */
  /* Restore normal default action to setup */

  return 1 ; /* OK */
}

int
log_poll(void)
{
  char *log_line_result ;

  switch (prog_state) {
    case state_restart: /* Restart after trouble */
      /* Do nothing */
      return 1 ;

    case state_dallas_startrx: // About to read Dallas Web data
    case state_remote_read:    // About to read Arne or owwl data
    case state_ready:          // About to read local data
      if (setup_logtype != LOGTYPE_NONE)
        /* We should start logging straight away */
      {
        if (!log_start_log_event(USER_START_LOG, NULL, NULL, NULL))
          werr(WERR_WARNING, "Unable to start logging. log_start_log_event failed") ;
      }
      break ;

    case state_read_ws_done:
    case state_remote_done:
      /* WS update (local or remote) has just completed */
      /* Generate log-style string for environment variable */
      log_line_result = log_line(&ws) ;

      /* Make sure we got a valid log line */
      if ((log_line_result != NULL) && (log_line_result[0] != '\0'))
      {
#       ifdef RISCOS
        if (setup_updatevar[0])
          setenv(setup_updatevar, log_line_result, 1) ;
#       else
        if (setup_updatevar[0])
          add_log_entry(setup_updatevar, log_line_result, "w") ;
#       endif

        /* Run any external postupdate progam */
        if (setup_postupdate && setup_postupdate[0])
        {
          static char *postupdate_buffer = NULL ;
          static int postupdate_alloced = 0 ;

          if ((0 != parseout_parse_and_realloc(&postupdate_buffer,
                                     &postupdate_alloced,
                                     &updatemeans,
                                     setup_postupdate,
                                     "")) || (postupdate_buffer == NULL))
          {
            werr(WERR_WARNING, _("postupdate did not parse correctly")) ;
          }
          else
          {

#           ifdef RISCOS
            system(postupdate_buffer) ;
#           else
            if (0 > process_new(postupdate_buffer))
              werr(WERR_WARNING, _("Failed to run postupdate command")) ;
#           endif
          }
        }

        /* Broadcast values as ThermIIC message */
        #ifndef NOGUI
        #ifdef RISCOS
        therm_bcst(&ws) ;
        #endif
        #else
          printf("%s\n", log_line_result);
        #endif
      }

      /* Check to see if it is time for a new log line */

      if 
        (
          (log_time != 0) &&
          (
            // Trigger if mean time incremented by setup_log_interval
            (stats_mean_t(&logstats) >= log_time + setup_log_interval) ||
            // or if time now half an interval later than mean trigger time
            (ws.t >= log_time + (3*setup_log_interval)/2)
          )
        )
      {
        /* Generate log string from averages */
        log_line_result = log_line(NULL) ;
        if (log_line_result == NULL)
        {
          werr(WERR_WARNING, _("Generation of log entry failed")) ;
        }
        else if (log_line_result[0] == '\0')
          werr(WERR_DEBUG0, _("Zero-length log line")) ;

        if (!log_start_log_event(USER_ADD_LOG, NULL, NULL,
          (void *) log_line_result))
          werr(WERR_WARNING,
            _("Unable to update log. log_start_log_event failed")) ;

        stats_reset(&logstats) ;
      }

      break ;
  }

  return 1 ; /* OK */
}
