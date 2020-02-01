/*
  mypopt.h

  Cut-down popt-type thing

*/

typedef struct {
  char opt ;  /* Letter for this option */
  char *longopt ;
  char *parhelp ; /* Help text */
  int  pars ; /* What type of parameter follows? */
  int *arg ;  /* > 0 if found at argv[arg] */
} mypopt_opt_type ;

void mypopt_readall(int argc, char *argv[], mypopt_opt_type opts[]) ;

