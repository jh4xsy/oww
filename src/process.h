/*
  process.h

  S. J. Melhuish
  Sun 07th January 2001

  Crude type of non-blocking system() calls for Oww
*/

/*
  Check status of any child processes to clear zombies
*/

int
process_poll(void) ;

/*
  process_new

  Run a new shell command (non-blocking)
*/
int
process_new(char *command) ;

// OK, so this is blocking...  quick and dirty but it works!
#ifdef WIN32
#  define process_new(command) system(command)
#  define process_poll() (0)
#endif
