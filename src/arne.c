/* arne.c

   Broadcasting weather station data by UDP
   according to protocol of Arne Henriksens

   http://weather.henriksens.net/
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef WIN32
#  include <winsock.h>
#else
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
#include <sys/socket.h>
#ifdef RISCOS
#  include <sys/byteorder.h>
#endif
#include <netinet/in.h>
#endif
/*#include <arpa/inet.h>*/
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
/*#include <unistd.h>*/
#include <stdlib.h>
#include <string.h>

#include "globaldef.h"
#include "werr.h"
#include "oww.h"
#include "wstypes.h"
#include "stats.h"
#include "setup.h"
#include "server.h"
#include "weather.h"
#include "devices.h"
#include "arne.h"
#include "intl.h"

#define WINDROWS 20
#define WINDROW_UPDATE 30
#define GUSTSAMPLES 20
#define GUSTUPDATE 30

static struct sockaddr_in to_addr ;
static int ss_in ;
static int arne_sock_created = 0 ;
int arne_wind_ready   = 0 ;
static int arne_wind_dir[WINDROWS][16] ;
static int arne_wind_old[16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
} ;
static int arne_wind_row = 0 ;
static time_t arne_wind_row_time = 0 ;
static time_t arne_gust_time = 0 ;
static float arne_gust_list[GUSTSAMPLES] ;
/*float arne_gust = 0.0F ;*/
static int arne_gust_current = 0 ;

#ifdef WIN32
   extern WSADATA WSAinfo;
#  ifdef errno
#    undef errno
#  endif
#  define errno h_errno
#endif

static void
arne_clear_wind_row(int row)
{
  int j ;

  for (j=0; j<16; ++j)
    arne_wind_dir[row][j] = 0 ;

}

int
arne_init_wind(void)
{
  int i ;

  /* Clear wind direction matrix */

  for (i=0; i<WINDROWS; ++i)
    arne_clear_wind_row(i) ;

  /* Clear gust array */
  for (i=0; i<GUSTSAMPLES; ++i)
    arne_gust_list[i] = 0.0F ;

  arne_wind_ready = 1 ;

  return 0 ; /* OK */
}

static void
arne_get_old(void)
{
  /* Generate column totals excluding the current row */

  int i, j, coltot ;

  /* Calculate column totals */

  /* Loop over columns */
  for (j=0; j<16; ++j)
  {
    coltot = 0 ;

    /* Loop over rows (excluding current row) */
    for (i=0; i<WINDROWS; ++i)
    {
      if (i == arne_wind_row) continue ;

      coltot += arne_wind_dir[i][j] ;
    }

    arne_wind_old[j] = coltot ;
  }
}

int
arne_new_wind_dir(int dir, time_t t)
{
  /*if (!arne_wind_ready && (arne_init_wind() < 0)) return -1 ;*/

  /* Just received new wind direction - add it to the matrix */

  /* Is it time to step to the new row? */

  if (t - arne_wind_row_time >= WINDROW_UPDATE)
  {
    /* Time to change row */
    arne_wind_row_time = t ;
    arne_wind_row = (arne_wind_row + 1) % WINDROWS ;

    /* Clear the row */
    arne_clear_wind_row(arne_wind_row) ;
    arne_get_old() ;
  }

  /* Just increment the corresponding cell */

  ++arne_wind_dir[arne_wind_row][dir] ;

  return 0 ; /* OK */
}

int
arne_read_out_wind_dir(void)
{
  /* Which wind column has the highest total? */

  int j, wind_max = -1, wind_mode = 0, total ;

  /* Check column totals - which is biggest? */
  /* We already have the totals excluding the current row */

  for (j=0; j<16; ++j)
  {
    total = arne_wind_dir[arne_wind_row][j] + arne_wind_old[j] ;
    if (total > wind_max)
    {
      wind_max = total ;
      wind_mode = j ;
    }
  }

  return wind_mode ;
}

int
arne_new_wind_speed(wsstruct *wd)
{
  /*if (!arne_wind_ready && (arne_init_wind() < 0)) return -1 ;*/

  /* Just received new wind direction - add it to the matrix */

  /* Is it time to step to the new row? */

  if (wd->t - arne_gust_time >= GUSTUPDATE)
  {
    int i ;

    /* Time to change sample number */
    arne_gust_time = wd->t ;
    arne_gust_current = (arne_gust_current + 1) % GUSTSAMPLES ;

    /* Reset current gust value */
    arne_gust_list[arne_gust_current] = 0.0F ;

    /* Find highest gust so far */
    wd->anem_gust = 0.0F ;
    for (i=0; i<GUSTSAMPLES; ++i)
      if (arne_gust_list[i] > wd->anem_gust)
        wd->anem_gust = arne_gust_list[i] ;
  }

  /* Check for new gust in this period */
  if (wd->anem_mps > arne_gust_list[arne_gust_current])
    arne_gust_list[arne_gust_current] = wd->anem_mps ;

  /* Check for highest gust */
  if (wd->anem_mps > wd->anem_gust)
    wd->anem_gust = wd->anem_mps ;

  return 0 ; /* OK */
}

static int
arne_create_socket(void)
{
  /* Try to create a UDP socket for broadcasts */

  int i = 1 ;

  if (arne_sock_created) return 0 ;

#ifdef WIN32
  // ??? where should we do WSACleanup();
  if (!WSAinfo.wVersion && WSAStartup(MAKEWORD(1, 1), &WSAinfo) != 0) {
    werr(0, "WSAStartup() failed: %d",h_errno);
    return -1;
  }
  ss_in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ;

  if (ss_in == INVALID_SOCKET)
#else
  ss_in = socket(AF_INET, SOCK_DGRAM, 0) ;

  if (ss_in == -1)
#endif
  {
    werr(0,
         "UDP Socket call failed [errno %d]. Perhaps UDP is not configured?",
         errno);
    setup_arne_udp_port = 0 ;
    return -1 ;
  }

  setsockopt(ss_in,
             SOL_SOCKET,
             SO_REUSEADDR,
             (char *) &i,
             sizeof(i)) ;

  setsockopt(ss_in,
             SOL_SOCKET,
             SO_BROADCAST,
             (char *) &i,
             sizeof(i)) ;

  /* Set up the address */
  to_addr.sin_family = AF_INET ;
  to_addr.sin_port   = htons(setup_arne_udp_port) ;
  to_addr.sin_addr.s_addr = INADDR_BROADCAST ;

  /* Bind the socket to the address */
  /*if (bind (ss_in, (struct sockaddr *) &addr_in, addrlen) == -1)
  {
    werr(0, "Error binding UDP socket") ;
    return -1 ;
  }*/

  arne_sock_created = 1 ;
  return 0 ;
}

int
arne_tx(wsstruct *wd)
{
  /* Broadcast WS data by UDP, accorning to Arne's protocol */

  int rv ;
  /*float current_speedMS ;*/
  char buffer[128] = "" ;

  /* Check that user wants Arne-style broadcasts? */
  /* Using the port number as a flag */
  if (!setup_arne_udp_port && !server_arne.port) return 0 ;

  sprintf(buffer,
    "%2.1f %2.1f %2.1f %2.1f %2.1f %2.1f %d %d %3.3f %3.3f %3.3f %3.3f\r\n",
    weather_primary_T(NULL),
    wd->Tmax /*max_tempC*/,
    wd->Tmin /*min_tempC*/,
    wd->anem_mps,
    wd->anem_gust /*peak_speedMS*/,
    wd->anem_speed_max * 0.447040972 /*max_speedMS*/,
    wd->vane_bearing - 1 /*current_dir*/,
    wd->vane_mode /*max_dir*/,
    wd->rain_rate /*rain_rateI*/,
    (wd->rain >= 0.0F) ? wd->rain : 0.0,
    (wd->rain >= 0.0F) ? 0.01F * (float)
                                 (wd->rain_count - wd->rain_offset[6]) :
                         0.0F,
    (wd->rain >= 0.0F) ? 0.01F * (float)
                                 (wd->rain_count - wd->rain_offset[7]) :
                         0.0F);


  /* Now broadcast the latest values */

  if (server_arne.port)
  {
    /* Check connexion status */
    server_poll_clients(&server_arne) ;

    /* Send data */
    server_send_to_clients(buffer, &server_arne) ;
  }

  /* Has socket been created? */
  /* Try to create it if not */
  if (setup_arne_udp_port) {
    if (!arne_sock_created && (arne_create_socket() < 0))
      return -1;
    rv = sendto(ss_in,
         buffer,
         strlen(buffer)+1,
         0,
         (struct sockaddr *) &to_addr,
         sizeof(struct sockaddr_in)) ;
  }

  return 0 ;
}

/* Receive WS data by UDP, accorning to Arne's protocol */

static int
arne_rx_decode(char *buffer,  wsstruct *wd, int *msg_type)
{
  float rain6, rain7 ;
  unsigned int nread ; /* Offset into buffer */

  if (12 == sscanf(buffer,
    "%f %f %f %f %f %f %d %d %f %f %f %f%n",
    &wd->T[0],
    &wd->Tmax           /*max_tempC*/,
    &wd->Tmin           /*min_tempC*/,
    &wd->anem_mps,
    &wd->anem_gust      /*peak_speedMS*/,
    &wd->anem_speed_max /*max_speedMS*/,
    &wd->vane_bearing   /*current_dir*/,
    &wd->vane_mode      /*max_dir*/,
    &wd->rain_rate      /*rain_rateI*/,
    &wd->rain,
    &rain6,
    &rain7,
    &nread))
  {
    float RH_max, RH_min, barom_max, barom_min, barom_rate ;
    int check ;

    devices_remote_vane = 1 ;
    devices_remote_T[0] = 1 ;
    devices_remote_rain =
      ((wd->rain > 0.0F) || (rain6>0.0F) || (rain7>0.0F)) ;

    wd->anem_speed = wd->anem_mps * 2.2369314F ;
    ++wd->vane_bearing ;
    wd->anem_speed_max /= 0.447040972F ;

    /*
    wd->rain_offset[6] = wd->rain_count - (unsigned int) (rain6 * 100.0F) ;
    wd->rain_offset[7] = wd->rain_count - (unsigned int) (rain7 * 100.0F) ;
    */

    wd->rain_count_old = wd->rain_count ;
    wd->rain_count = wd->rain_offset[0] + 100.0F * (float) wd->rain ;

    *msg_type = ARNE_RX_WSDATA ;

    /* Perhaps we have more data? */

    if ((strlen(buffer) > nread) &&
        (9 == sscanf(&buffer[nread],
         "%f %f %f %f %f %f %f %f %d",
         &wd->RH[0],
         &RH_max,
         &RH_min,
         &wd->barom[0],
         &barom_max,
         &barom_min,
         &barom_rate,
         &wd->rain_rate,
         &check)))
    {
      if (check != wd->vane_bearing + wd->vane_mode - 1)
      {
        werr(WERR_WARNING, _("CRC error reading from WServer")) ;
        werr(WERR_WARNING, _("check = %d, vane_bearing = %d, vane_mode = %d, sum = %d"),
          check, wd->vane_bearing, wd->vane_mode, wd->vane_bearing + wd->vane_mode) ;
        return -1 ;
      }

      if ((wd->RH[0] != 0.0F) || (RH_max != 0.0F) || (RH_min != 0.0F))
        devices_remote_RH[0] = 1 ;

      if ((wd->barom[0] != 0.0F) || (barom_max != 0.0F) || (barom_min != 0.0F))
        devices_remote_barom[0] = 1 ;

      if (wd->rain_rate != 0.0F)
        devices_remote_rain = 1 ;
    }

    return 0 ; /* Ok */
  }

  /* Not a valid data line - but perhaps this is an announcement? */
  if ((strstr(buffer, "ready:") != NULL) || (strstr(buffer, "Henriksen") != NULL))
  {
    *msg_type = ARNE_RX_ANNOUNCE ;
    return 1 ; /* No data yet */
  }

  return -1 ; /* Failed */
}

int
arne_rx(int socket, wsstruct *wsp, int *msg_type)
{
  char buffer[256] ;

  int size ;

  werr(WERR_DEBUG0, "arne_rx") ;

  /* Try to read data from the server */

  if (socket <= 0)
  {
    werr(1, _("Bad socket: %d"), socket) ;
  }

  /* Ready to read remote data, but assume all remote devices gone */

  devices_clear_remote() ;

  /* Socket has been marked as non-blocking, so just try to read */

  size = recv(socket,
    buffer,
    sizeof(buffer)-1,
    0) ;

  /* Terminate string */
  buffer[size] = '\0' ;

  werr(WERR_DEBUG0, "arne_rx: \"%s\"", buffer) ;

  if (size == -1)
  {
    /* Not ready or error */
    /*if (errno == EWOULDBLOCK)
    { */
      /* This is Ok - not ready yet - just return */
      /*return 1 ;
    }   */

      werr(0, "recv() [head] failed: %d \"%s\"",
        errno, strerror(errno)) ;
      return -1 ; /* Connexion down */
  }

  if (size == 0)
  {
    werr(WERR_DEBUG0, "0 == recv() [head]") ;
    return -2 ;
  }

  /* *msg_type = ARNE_RX_WSDATA ;*/

  return arne_rx_decode(buffer, wsp, msg_type) ;
}
