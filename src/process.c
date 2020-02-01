/*
  process.c

  S. J. Melhuish
  Sun 07th January 2001

  Crude type of non-blocking system() calls for Oww
*/

#include <config.h>

#include <unistd.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
# ifdef TIME_WITH_SYS_TIME
#  include <time.h>
# endif
#else
# include <time.h>
#endif

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stdio.h>

#include "process.h"
#include "werr.h"

/*
  Check status of any child processes to clear zombies
*/

int
process_poll(void)
{
  int status ;
  struct rusage resusage ;

  return wait3(&status,
        WNOHANG,
        &resusage) ;
}

/*
  process_new

  Run a new shell command (non-blocking)
*/
int
process_new(char *command)
{
  pid_t newproc ;

  /* Ready to allocate new process */

  newproc = fork() ;

  if (newproc < 0)
  {
    /* Error from fork() */
    werr(0, "Failed to fork for command execution") ;
    return -1 ;
  }

  if (newproc > 0)
  {
    /* We are in the parent */

    /* Check status immediately */
    process_poll() ;
  }
  else
  {
    /* We are in the child */
    /* Execute the command */
    char *argv[4] ;
    argv[0] = "sh";
    argv[1] = "-c";
    argv[2] = command ;
    argv[3] = 0;
    execve("/bin/sh", argv, NULL);

    /* We never reach here */
  }

  return 0 ;
}

