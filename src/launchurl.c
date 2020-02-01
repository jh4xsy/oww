#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "setup.h"


void
launchurl(char *url)
{
  pid_t pid;

  pid = fork();

  if (pid == 0) 
  {
	char *args[4] ;
	char *command=NULL ;

	switch (setup_url_launcher)
	{
	case 0: /* mozilla -remote */
	{
	  char format[] = "openURL(%s, new-window)" ;

	  command = malloc(strlen(url) + sizeof(format)) ;
	  sprintf(command, format, url) ;

	  args[0] = "mozilla" ;
	  args[1] = "-remote" ;
	  args[2] = command ;
	  args[3] = NULL ;
	  break ;
	}

	case 1: /* netscape -remote */
	{
	  char format[] = "openURL(%s, new-window)" ;

	  command = malloc(strlen(url) + sizeof(format)) ;
	  sprintf(command, format, url) ;

	  args[0] = "netscape" ;
	  args[1] = "-remote" ;
	  args[2] = command ;
	  args[3] = NULL ;
	  break ;
	}

	case 2: /* kfm */
	  args[0] = "kfmclient" ;
	  args[1] = "openURL" ;
	  args[2] = url ;
	  args[3] = NULL ;
	  break ;

	case 3: /* Opera */
	  args[0] = "opera" ;
	  args[1] = "-newwindow" ;
	  args[2] = url ;
	  args[3] = NULL ;
	  break ;

	case 4: /* Galeon */
	  args[0] = "galeon" ;
	  args[1] = url ;
	  args[2] = NULL ;
	  break ;

	case 5: /* firefox url */
	  args[0] = "firefox" ;
	  args[1] = url ;
	  args[2] = NULL ;
	  args[3] = NULL ;
	  break ;
	}

	execvp(args[0], args);

	if (command!=NULL) free(command) ;

	_exit(0);
  }
}
