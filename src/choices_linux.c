/*
  choices.c

  Find out where to read / write setup and devices files

*/

/* Linux version */

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#ifdef WIN32
#include "config-w32gcc.h"
#else
#include "../config.h"
#endif
#endif

#include <stdlib.h>
#include <stdio.h>

//#include <string.h>
#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif
#include <time.h>
#include <unistd.h>

#include "file.h"
#include "werr.h"

#include "choices.h"
#include "mypopt.h"

#define DEFSETUP ".oww_setup"
#define DEFDEVICES ".oww_devices"
#define DEFSTATS ".oww_stats"

#ifndef OWWCONFDIR
# define OWWCONFDIR SYSCONFDIR
#endif // ifndef OWWCONFDIR

char *choices_stats_read = NULL,
     *choices_stats_write = NULL,
     *choices_setup_read = NULL,
     *choices_setup_write = NULL,
     *choices_setupng_read = NULL,
     *choices_setupng_write = NULL,
     *choices_values_read = NULL,
     *choices_values_write = NULL,
     *choices_devices_read = NULL,
     *choices_devices_write = NULL ;

static int arg_setup=0, arg_stats=0, arg_devices=0, arg_values=0 ;
int arg_daemon=0, arg_interactive=0 ;


static mypopt_opt_type opts[] = {
  {'s', "setup", "setup_file", 1, &arg_setup /* Setup */},
  {'S', "stats", "stats_file", 1, &arg_stats /* Stats */},
  {'D', "devices", "devices_file", 1, &arg_devices /* Devices */},
  {'v', "values", "values_file", 1, &arg_values /* Values - for sensible resets in single-shot mode */},
  {'d', "daemon", "\t", 0, &arg_daemon /* Daemonize */},
#ifdef ENABLE_INTERACTIVE
  {'i', "interactive", "\t", 0, &arg_interactive /* Go to interactive mode */},
#endif // ENABLE_INTERACTIVE
  {'\0', "", "", 0, NULL /* Terminate */}
} ;

char *path_and_leaf(const char *s1, const char *s2)
{
  char *new ;

  new = (char *) malloc(sizeof(char) * (2 + strlen(s1) + strlen(s2))) ;
  if (!new) return NULL ;
  sprintf(new, "%s/%s", s1, s2) ;
  return new ;
}

static int read_test(const char *filename)
{
  /* Can we read from the file? */

  return (access(filename, F_OK | R_OK) == 0) ;
}

static int write_test(const char *filename)
{
  /* Can we write to the file, if it exists, or create it if not? */

  if (access(filename, F_OK) == 0) {
    /* File exists - can we write to it? */
    return (access(filename, W_OK) == 0) ;
  } else {
    /* File does not exist - could we create it? */
    /* Do we have write access to the directory? */
    char *t ;
    int a ;

    /* Terminate name before filename */
    t = strrchr(filename, '/') ;
    if (!t) return 0 ;
    *t = '\0' ;
    a = access(filename, W_OK) ;

    /* Replace '/' */
    *t = '/' ;
    return (a == 0) ;
  }
  return 0 ;
}

void choices_find_files(int argc, char *argv[])
{
  mypopt_readall(argc, argv, opts) ;

  if (arg_setup) /* setup given */
  {
	//werr(0, "arg_setup %d -> \"%s\"", arg_setup, argv[arg_setup+1]) ;
    choices_setupng_read = choices_setup_read = strdup(argv[arg_setup+1]) ;
    choices_setupng_write = choices_setup_write = strdup(argv[arg_setup+1]) ;
  }

  if (arg_devices) /* devices given */
  {
	//werr(0, "arg_devices %d -> \"%s\"",arg_devices , argv[arg_devices+1]) ;
    choices_devices_read = strdup(argv[arg_devices+1]) ;
    choices_devices_write = strdup(argv[arg_devices+1]) ;

    //return ;
  }

  if (arg_stats) /* stats given */
  {
	//werr(0, "arg_stats %d -> \"%s\"",arg_stats , argv[arg_stats+1]) ;
    choices_stats_read = strdup(argv[arg_stats+1]) ;
    choices_stats_write = strdup(argv[arg_stats+1]) ;

    //return ;
  }

  if (arg_values) /* values given */
  {
    choices_values_read = strdup(argv[arg_values+1]) ;
    choices_values_write = strdup(argv[arg_values+1]) ;

    //return ;
  }

  /* Some options might not have been set */

  /* Look in OWWCONFDIR or cwd */

  if (!choices_setup_read) {
    /* Check cwd for read */
    if (read_test(DEFSETUP)) {
      /* Yes - it's there */
      choices_setup_read = DEFSETUP ;
      /* We'll write here too */
      choices_setup_write = DEFSETUP ;
    } else {
      choices_setup_read = path_and_leaf(OWWCONFDIR, "setup") ;
      choices_setup_write = path_and_leaf(OWWCONFDIR, "setup") ;

      /* Does the directory exist? We might need to create it... */
      if (!file_check_dir(OWWCONFDIR, 1) ||
      /* Can we write? */
      !write_test(choices_setup_write)) {
        /* No - fall back on cwd */
        free(choices_setup_write) ;
        choices_setup_write = DEFSETUP ;
      }
    }
  }

  /*werr(0, "choices_setup_read = \"%s\"", choices_setup_read) ;*/

  if (!choices_devices_read) {
    /* Check cwd for read */
    if (read_test(DEFDEVICES)) {
      /* Yes - it's there */
      choices_devices_read = DEFDEVICES ;
      /* We'll write here too */
      choices_devices_write = DEFDEVICES ;
    } else {
      choices_devices_read = path_and_leaf(OWWCONFDIR, "devices") ;
      choices_devices_write = path_and_leaf(OWWCONFDIR, "devices") ;

      /* Does the directory exist? We might need to create it... */
      if (!file_check_dir(OWWCONFDIR, 1) ||
      /* Can we write? */
      !write_test(choices_devices_write)) {
        /* No - fall back on cwd */
        free(choices_devices_write) ;
        choices_devices_write = DEFDEVICES ;
      }
    }
  }

  if (!choices_stats_read) {
    /* Check cwd for read */
    if (read_test(DEFSTATS)) {
      /* Yes - it's there */
      choices_stats_read = DEFSTATS ;
      /* We'll write here too */
      choices_stats_write = DEFSTATS ;
    } else {
      choices_stats_read = path_and_leaf(OWWCONFDIR, "stats") ;
      choices_stats_write = path_and_leaf(OWWCONFDIR, "stats") ;

      /* Does the directory exist? We might need to create it... */
      if (!file_check_dir(OWWCONFDIR, 1) ||
      /* Can we write? */
      !write_test(choices_stats_write)) {
        /* No - fall back on cwd */
        free(choices_stats_write) ;
        choices_stats_write = DEFSTATS ;
      }
    }
  }

  if (!choices_setupng_read)
  {
	  /* Save / load setupng from same dir as setup, so just concatenate */
	
	  choices_setupng_read = malloc(3 + strlen(choices_setup_read)) ;
	
	  if (choices_setupng_read)
	  {
		strcpy(choices_setupng_read, choices_setup_read) ;
		strcat(choices_setupng_read, "NG") ;
	
		/* Can we actually read this file? */
		if (!read_test(choices_setupng_read))
		{
		  /* No we can't - fall back on normal setup file */
		  free(choices_setupng_read) ;
		  choices_setupng_read = choices_setup_read ;
		}
	  }
  }

  if (!choices_setupng_write)
  {
	  choices_setupng_write = malloc(3 + strlen(choices_setup_write)) ;
	
	  if (choices_setupng_write)
	  {
		strcpy(choices_setupng_write, choices_setup_write) ;
		strcat(choices_setupng_write, "NG") ;
	  }
  }

  /*werr(0, "choices_devices_read = \"%s\"", choices_devices_read) ;*/
}
