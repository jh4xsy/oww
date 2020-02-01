/* server.c */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef WIN32
#  include <winsock.h>
#else
#include <sys/socket.h>
#ifdef RISCOS
typedef int socklen_t;
#  include <sys/byteorder.h>
#endif
#include <netinet/in.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif // HAVE_SYS_FILIO_H

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif

#endif
/*#include <arpa/inet.h>*/
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#  include "config-w32gcc.h"
#else
#include "config.h"
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#else
extern int close (int fd);
#endif

#include "globaldef.h"
#include "werr.h"
#include "setup.h"
#include "server.h"
#include "intl.h"

#ifdef WIN32
#  define ioctl(x,y,z) ioctlsocket(x,y,z)
#  define EWOULDBLOCK WSAEWOULDBLOCK
WSADATA WSAinfo = { 0 };
#  ifdef errno
#    undef errno
#  endif
#  define errno h_errno
#endif

#ifndef AF_LOCAL
# define AF_LOCAL AF_UNIX
#endif // AF_LOCAL

extern wsstruct ws ;

server_struct server_arne, server_oww, server_oww_un,
              server_txt_tcp, server_txt_un ;

/*static int arne_tcp_listen = 0 ;*//* For listening socket */
/*static int arne_tcp_sockets[ARNE_TCP_MAX_CON] ;
static int sockN = 0, socksInUse = 0 ;*/

void
close_sock (server_struct * ss)
{
  if (ss->socket_listen)
  {
    //shutdown(ss->socket_listen, 2) ;
    close (ss->socket_listen);
    
#   ifdef HAVE_SYS_UN_H
    // Remove Unix socket node
    if (ss->family == AF_LOCAL)
    {
      unlink(ss->name);
    }
#   endif
  }
  ss->socket_listen = 0;
# ifdef WIN32
  WSACleanup ();
  WSAinfo.wVersion = 0;
# endif
}

void
server_init (void)
{
  server_arne.socket_listen = server_arne.socket_last =
  server_arne.socket_used = 0;
  server_arne.family = AF_INET ;
  server_arne.port = setup_arne_tcp_port;

  server_oww.socket_listen = server_oww.socket_last =
  server_oww.socket_used = 0;
  server_oww.family = AF_INET ;
  server_oww.port = setup_oww_tcp_port;

  server_txt_tcp.socket_listen = server_txt_tcp.socket_last =
  server_txt_tcp.socket_used = 0;
  server_txt_tcp.family = AF_INET ;
  server_txt_tcp.port = setup_txt_tcp_port ;

# ifdef HAVE_UNISTD_H
  server_oww_un.socket_listen = server_oww_un.socket_last =
  server_oww_un.socket_used = 0;
  server_oww_un.family = AF_LOCAL ;
  server_oww_un.name = setup_oww_un_name ;

  server_txt_un.socket_listen = server_txt_un.socket_last =
  server_txt_un.socket_used = 0;
  server_txt_un.family = AF_LOCAL ;
  server_txt_un.name = setup_txt_un_name ;
# endif

//  atexit (server_close);
}

/* Close a connexion from a client */
static void
server_kill_client(server_struct * ss, int i)
{
  if (ss->socket_conn[i] <= 0) return ;

  close (ss->socket_conn[i]) ;

  ss->socket_conn[i] = 0 ;

  if (i == ss->socket_last - 1)
    --(ss->socket_last) ;

  --(ss->socket_used) ;
}

/* Send text to a client */
static void
server_send_to_a_client (char *text, int len, server_struct * ss, int i)
{
  int /*len, */ sent ;

  if (ss->socket_conn[i] > 0)
  {
    /* This should be a valid socket */
    sent = send (ss->socket_conn[i], text, len, 0);
    if (len != sent)
    {
      /* Just ignore calls that would block */
      if ((sent < 0) && (errno == EWOULDBLOCK))
      return;

      /* Some kind of error - client disconnected probably */
      werr (WERR_DEBUG0,
        "Send to socket %d failed. Closing connexion.",
        i) ;
      server_kill_client(ss, i) ;
    }
  }
}

/* Kill all clients */
int
server_kill_clients(server_struct * ss)
{
  int i ;

  if (ss->socket_used) /* Any clients? */
  {
    /* Loop over client sockets */
    for (i = 0; i < ss->socket_last; ++i)
    {
      if (ss->socket_conn[i] > 0)
      {
        /* This one is in use */
        server_kill_client(ss, i) ;
      }
    }
  }

  return 0 ; /* Ok */
}

/* Shut down a server */
void
server_shutdown(server_struct * ss)
{
  /* First kill any client connexions */
  server_kill_clients(ss);

  /* Now close the socket */
  close_sock(ss);
}

/* Poll clients of server to flush rx data and collect pending close */
int
server_poll_clients(server_struct * ss)
{
  int i, received ;
  char buffer[64];

  if (ss->socket_used) /* Any clients? */
  {
    /* Loop over client sockets */
    for (i = 0; i < ss->socket_last; ++i)
    {
      if (ss->socket_conn[i] > 0)
      {
        /* This one is in use */

        /* Read from the socket to flush any junk sent to us */
        do
        {
          received = recv (ss->socket_conn[i], buffer, 64, 0) ;
          if (received > 0)
          {
            int j ;
            for (j=0;j<received;++j)
            {
              char c, d ;
              c = buffer[j] ;
              d = c & 0x7f ;
              if (d<32) d+=32 ;
              printf("%c - 0x%x\n", d, c) ;
            }
          }
        }
        while (received > 0) ;

        if (received == 0) /* Client is closing connexion */
        {
          werr(WERR_DEBUG0, "Closing client %d", i) ;
          server_kill_client(ss, i) ;
        }
      }
    }
  }

  return 0 ; /* Ok */
}

void
server_poll_accept (server_struct * ss,
  void *(*announce)(wsstruct *, int *))
{
	/* Listen for incoming calls */
	int conn, i;
	struct sockaddr *addr = NULL ;
	struct sockaddr_in addr_in ;
# ifdef HAVE_SYS_UN_H
	struct sockaddr_un addr_un ;
# endif
	socklen_t addrLength;
	unsigned long a = 1L;

#ifdef WIN32
	if (ss->family == AF_LOCAL)
		return;
#endif

	/* Check that port number is valid */
	switch (ss->family)
	{
	case AF_INET:
		/* IP socket needs a port number */
		if (ss->port == 0)
		{
			//werr(WERR_DEBUG0, "IP server disabled") ;
			return;
		}
		addr = (struct sockaddr *) &addr_in ;
		break;

#ifdef HAVE_SYS_UN_H
	case AF_LOCAL:
		/* Unix socket needs a name */
		if (!ss->name || !ss->name[0])
		{
			//werr(WERR_DEBUG0, "LOCAL (UNIX) server disabled") ;
			return;
		}
		addr = (struct sockaddr *) &addr_un ;
		break;
#endif /* HAVE_SYS_UN_H */

	default:
		return;
	}

	/* Check that the listening socket has been set up */

	if (ss->socket_listen == 0)
	{
#ifdef WIN32
    if (!WSAinfo.wVersion && WSAStartup(MAKEWORD(1, 1), &WSAinfo) != 0) {
      werr(0, "WSAStartup() failed: %d",h_errno);
      return;
    }
    if ((ss->socket_listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
#else
		if ((ss->socket_listen =
		     socket (ss->family /*PF_INET */ , SOCK_STREAM, 0)) < 0)
#endif
		{
			werr (0,
			      "Socket call failed [errno %d]. Perhaps TCP/IP is not configured?",
			      errno);
			ss->socket_listen = 0;	// nothing to close
			ss->port = 0;	// Flag off
			ss->name = NULL;
			close_sock (ss);	// but do other cleanup
			return;
		}

		/*atexit(close_sock) ; */
#   ifndef RISCOS
#   ifndef WIN32
		signal (SIGPIPE, SIG_IGN);
#   endif
#   endif

		i = 1;
		setsockopt (ss->socket_listen,
			    SOL_SOCKET,
			    SO_REUSEADDR, (char *) &i, sizeof (i));

		ioctl (ss->socket_listen, FIONBIO, &a);

		//address.sin_family = ss->family ;

		switch (ss->family)
		{
		case AF_INET: /* IP */
		{
			addr_in.sin_port = htons (ss->port);
		    addr_in.sin_family = AF_INET ;
			memset (&addr_in.sin_addr,
				0, sizeof (addr_in.sin_addr));
			addrLength = sizeof (addr_in);

			if (bind (ss->socket_listen,
				  addr,
				  sizeof (addr_in)))
			{
				werr (0,
				      _("Unable to claim TCP port %d. Perhaps another task claimed it already?"),
				      ss->port);
				close_sock (ss);
				ss->port = 0;
				return;
			}

			if (listen (ss->socket_listen, 5))
			{
				werr (0, "listen() failed, TCP port %d",
				      ss->port);
				close_sock (ss);
				return;
			}
			break ;
		}
#ifdef HAVE_SYS_UN_H
		case AF_LOCAL: /* Unix socket */
		{
		    addr_un.sun_family = AF_LOCAL ;
			unlink(ss->name) ;
			strncpy(addr_un.sun_path, ss->name, sizeof(addr_un.sun_path) - 1) ;
      addr_un.sun_path[sizeof(addr_un.sun_path)-1] = '\0' ;
			addrLength = sizeof(addr_un.sun_family) + strlen(addr_un.sun_path) ;

			if (bind (ss->socket_listen,
				  addr,
				  addrLength))
			{
				werr (0,
				      _("Unable to claim LOCAL port %s. Perhaps another task claimed it already?"),
				      ss->name);
				close_sock (ss);
				ss->name = NULL ;
				return;
			}
			werr(WERR_DEBUG0, "bound Unix socket") ;

			if (listen (ss->socket_listen, 5))
			{
				werr (0, "listen() failed, LOCAL port %s",
				      ss->name);
				close_sock (ss);
				return;
			}
			break;
		}
#endif /* HAVE_SYS_UN_H */
	  }
    }

	if (ss->socket_used >= SERVER_MAX_SOCK)
	{
		werr(WERR_DEBUG0, "SERVER_MAX_SOCK reached") ;
		return;
	}

	/* Have we an address of some sort? */
	if (!addr)
	{
		werr(WERR_DEBUG0, "No address") ;
		return ;
	}

        /* Set available address length depending on family */
        switch(ss->family)
        {
          case AF_INET:
            addrLength = sizeof(struct sockaddr_in) ;
            break ;

#ifdef HAVE_SYS_UN_H
	  case AF_LOCAL:
            addrLength = sizeof(struct sockaddr_un) ;
            break ;
#endif /* HAVE_SYS_UN_H */
	}

	conn = accept (ss->socket_listen, addr, &addrLength);
#ifdef WIN32
	if (conn == INVALID_SOCKET)
#else
	if (conn < 0)
#endif
	{
		if (errno == EWOULDBLOCK)
		{
			//werr(WERR_DEBUG0, "EWOULDBLOCK") ;
			return;
		}
		werr (0, "accept() failed: %d - %s", errno, strerror(errno));
		return;
	}
	++(ss->socket_used);

	a = 1L;
	ioctl (conn, FIONBIO, &a);

	werr (WERR_DEBUG0,
	      "Accepted\n%d of %d sockets in use",
	      ss->socket_used, SERVER_MAX_SOCK);

	/* Find a free entry for the socket */
	for (i = 0; i < ss->socket_last; ++i)
	{
		if (ss->socket_conn[i] == 0)
		{
			werr (WERR_DEBUG0, "Allocating to entry %d", i);
			ss->socket_conn[i] = conn;
			return;
		}
	}

	ss->socket_conn[ss->socket_last] = conn;

	if (announce)
	{
	  void *message ;
	  int len ;

	  message = announce(&ws, &len) ;

	  if (message)
		server_send_to_a_client (message,
					 len,
					 ss, ss->socket_last);
	}

	++(ss->socket_last);
}

/* Send text to all clients */
void
server_send_to_clients (char *text, server_struct * ss)
{
  int i;

  for (i = 0; i < ss->socket_last; ++i)
  {
    server_send_to_a_client (text, 1 + strlen (text), ss, i);
  }
}


/* Send binary data to all clients */
void
server_send_bin_to_clients (void *data, int len, server_struct * ss)
{
  int i;

  for (i = 0; i < ss->socket_last; ++i)
  {
    server_send_to_a_client ((char *) data, len, ss, i);
  }
}
