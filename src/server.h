/* server.h */

/* Serving incoming tcp connexions */

#ifndef SERVER_H
#define SERVER_H

#ifndef GLOBALDEF_H
#include "globaldef.h"
#endif

/* Structure for connexions */

typedef struct {
  int family ; /* Address family */
  int port ;   /* Port number for IP */
  char *name ; /* Name for LOCAL */
  int socket_listen ;
  int socket_conn[SERVER_MAX_SOCK] ;
  int socket_last ;
  int socket_used ;
} server_struct ;

extern server_struct server_arne, server_oww, server_oww_un,
                     server_txt_tcp, server_txt_un ;

/* Check for incoming connexion requests */
/* Send message to connecting client */
/* optional announce function generates inital message */
void
server_poll_accept (server_struct * ss,
  void *(*announce)(wsstruct *, int *)) ;

/* Poll clients of server to flush rx data and collect pending close */
int
server_poll_clients(server_struct * ss) ;

/* Squirt text string to all clients */
void
server_send_to_clients(char *text, server_struct *ss) ;

/* Squirt binary data to all clients */
void
server_send_bin_to_clients(void *data, int len, server_struct *ss) ;

/* Shut down a server */
void
server_shutdown(server_struct * ss) ;

void
server_init(void) ;

#endif
