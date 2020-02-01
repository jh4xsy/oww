/* mypopt.c - cut-down popt-type things */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mypopt.h"
#include "werr.h"
#include "intl.h"

static void
mypopt_usage(char *argv[], mypopt_opt_type opts[])
{
  int j ;

  printf(_("Usage: %s [options]\n\n"), argv[0]) ;

  printf(_("Options:\n")) ;

  for (j=0; opts[j].opt; ++j) /* Loop over all possible opts */
    printf("\t -%c | --%-10s %s\n",
      opts[j].opt, opts[j].longopt, opts[j].parhelp) ;

  printf("\n") ;
}

/* Search for a single-character option, followed by its argument */

void
mypopt_readall(int argc, char *argv[], mypopt_opt_type opts[])
{
  int i, j=0 ;

  /* Scan all argv entries */

  for (i=1; i<argc; ++i)
  {
    if (argv[i][0] == '-')
    {
      /* Found an option - which one is it? */

      /* Help? */
      if ((argv[i][1] == 'h') || (argv[i][1] == '?') ||
          (0 == strcmp(&(argv[i][1]), "-help")))
      {
        mypopt_usage(argv, opts) ;
        exit(0) ;
      }

	  j = 0 ;

      while (opts[j].opt) /* Loop over all possible opts */
      {
        if ((argv[i][1] == opts[j].opt) ||
            ((argv[i][1] == '-') &&
             (0 == strcmp(opts[j].longopt, &(argv[i][2]))))) /* Found opt */
        {
          /* Do we have enough args? */
          if (argc > i+opts[j].pars)
          {
            *(opts[j].arg) = i ;
            i += opts[j].pars ;
          }
          else
          {
            fprintf(stderr,
              _("\"-%c\" takes %d parameters\n"),
              opts[j].opt,
              opts[j].pars) ;
          }
		  break ;
        }
		else
		{
			//if (*(opts[j].arg)) break ;

			++j ;

			if (!opts[j].opt)
			{
			  mypopt_usage(argv, opts) ;
			  exit(0) ;
			}
	    }
      }
      //++i ; /* Skip the agument */
    }
  }
}
