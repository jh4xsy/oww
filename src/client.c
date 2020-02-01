/* client.c for Oww

Simon J. Melhuish
2001

*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#include <netdb.h>
#ifdef WIN32
#  include <winsock.h>
#else
#include <sys/socket.h>
#ifdef RISCOS
#  include <sys/byteorder.h>
#endif
#include <netinet/in.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif // HAVE_SYS_FILIO_H

#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#else
# ifdef HAVE_ERRNO_H
#  include <errno.h>
# endif
#endif

#endif
#ifndef HAVE_UNISTD_H
int fcntl(int fd, int cmd, long arg);
#endif
/*#include <arpa/inet.h>*/
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <sys/filio.h>*/

/*#ifdef WIN32
#  include "config-w32gcc.h"
#else
#include "config.h"
#endif*/

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#else

#include "intl.h"

extern int close(int fd) ;
#endif

/*#include "globaldef.h"*/
#include "werr.h"
#include "setup.h"
#include "client.h"
#include "intl.h"

#ifdef WIN32
#  define ioctl(x,y,z) ioctlsocket(x,y,z)
#  define EWOULDBLOCK WSAEWOULDBLOCK
   WSADATA WSAinfo={0};
#  ifdef errno
#    undef errno
#  endif
#  define errno h_errno
#endif

#ifndef AF_LOCAL
# define AF_LOCAL AF_UNIX
#endif // AF_LOCAL


int
client_connect(char *hostname, int port)
{
  /* Try to connect to the client */

  int socket_client, addr_len ;
  struct hostent *host ;
  struct sockaddr *address ;
# ifdef HAVE_SYS_UN_H
  struct sockaddr_un addr_un ;
# endif
  struct sockaddr_in addr_in ;

  /* What kind of connexion? */

  if (hostname[0] == '/') /* AF_LOCAL */
  {
# ifdef HAVE_SYS_UN_H
	  memset(&addr_un, 0, sizeof(addr_un)) ;

	  addr_un.sun_family = AF_LOCAL ;
	  strncpy(addr_un.sun_path, hostname, sizeof(addr_un.sun_path)-1) ;
    addr_un.sun_path[sizeof(addr_un.sun_path)-1] = '\0' ;
	  addr_len = sizeof(addr_un.sun_family) + strlen(addr_un.sun_path) ;
	  address = (struct sockaddr *) &addr_un ;
	  werr(WERR_DEBUG0, "Connecting to LOCAL (UNIX) port: %s", addr_un.sun_path) ;
# else
         return 0 ;
# endif
  }
  else
  {
	  /* Just use the ordinary (blocking) dns lookup for now */

	  host = gethostbyname(hostname) ;

	  if (!host)
	  {
		werr(0, _("Unable to resolve host name \"%s\""), hostname) ;
		return -1 ;
	  }

	  addr_in.sin_family = AF_INET ;
	  addr_in.sin_port   = htons(port) ;

	  /* Take the first ip address */

	  memcpy(&addr_in.sin_addr,
		host->h_addr_list[0],
		sizeof(addr_in.sin_addr)) ;
	  address = (struct sockaddr *) &addr_in ;
	  addr_len = sizeof(addr_in) ;
	  werr(WERR_DEBUG0, "Connecting by IP to server %s:%d", hostname, port) ;
  }


  socket_client = socket(address->sa_family, SOCK_STREAM, 0) ;

  if (socket_client < 0)
  {
    werr(0, _("Unable to open socket for communication with \"%s\""),
    hostname) ;
    return -1 ;
  }

  /* Now connect to the server */

  if (connect(socket_client,
      address, addr_len))
  {
    werr(WERR_WARNING, _("Unable to connect to \"%s\""),
    hostname) ;
    close(socket_client);
    return -1 ;
  }

  /* Connected Ok */

  /* Mark the socket as non-blocking */

  /*ioctl(socket_client, FIONBIO, &data) ;*/

  return socket_client ;
}

/* Kill off the client connexion */

int
client_kill(int socket_client)
{
  if (socket_client > 0)
  {
    //shutdown(socket_client, 2) ;
    close(socket_client) ;
  }

  return -1 ;
}

/* Check that the client connexion is still up */

int
client_check_conn(int socket_client)
{
  /* Try to send a few bytes */
  int test = 0 ;

  if (sizeof(int) == send(socket_client, (char *) &test, sizeof(int), 0))
    return 0 ;

  return -1 ; /* Failed */
}
