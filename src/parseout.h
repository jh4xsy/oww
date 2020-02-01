/*

  parseout.h

  Parse strings to formatted stats values for output

  Oww project
  Dr. Simon J. Melhuish

  Wed 03rd January 2001

*/

#ifndef PARESOUT_H
#define PARESOUT_H 1
#include "stats.h"

/*int
parseout_parse(statsmean *stats,
               char *parseline,
               char *buffer,
               int buffsupplied) ;*/

int
parseout_parse_and_realloc(char **buffptr,
                           int *bufflen,
                           statsmean *stats,
                           char *parseline,
                           char *suffix) ;
#endif

