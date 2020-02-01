/* utility.c */

/*
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * 1999 - 2000
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/* various functions */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifdef RISCOS
#include "kernel.h"
#include "swis.h"
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int setenv(const char *name, const char *value, int overwrite)
{
    return SetEnvironmentVariable(name,value);
}

// Get the current centisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
unsigned long int csGettick(void)
{
    return GetTickCount()/10;
}
#endif


#ifndef HAVE_STRDUP
char *strdup(const char *s)
{
  char *n ;

  n = (char *) malloc(sizeof(char) * (1+strlen(s))) ;
  if (n) strcpy(n, s) ;
  return n ;
}
#endif // ifndef HAVE_STRDUP

#ifdef RISCOS
int setenv(const char *name, const char *value, int overwrite)
{
  /* Acorn clib does not have this function */

  _kernel_setenv(name, value) ;

  return 0 ;
}
#endif

// #ifndef HAVE_STRTOF
// float strtof (const char *nptr, char **endptr)
// {
//   return (float) strtod(nptr, endptr) ;
// }
// #endif
