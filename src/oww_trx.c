/* oww_trx.c

   Data transfer using oww protocol

*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
# include <sys/types.h>
#endif

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif

#ifdef HAVE_MACHINE_ENDIAN_H
# include <machine/endian.h>
#endif

//#include <assert.h>
#ifdef WIN32
#  include <winsock.h>
#else
#include <sys/socket.h>
#ifdef RISCOS
#  include <sys/byteorder.h>
#endif // RISCOS
#include <netinet/in.h>
#endif // WIN32

/*#include <arpa/inet.h>*/
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
/*#include <unistd.h>*/
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "globaldef.h"
#include "werr.h"
#include "oww.h"
#include "wstypes.h"
#include "sendwx.h"
#include "stats.h"
#include "setup.h"
#include "server.h"
#include "weather.h"
#include "oww_trx.h"
#include "applctn.h"
#include "devices.h"
#include "intl.h"

//extern time_t the_time ;

#ifdef WIN32
   extern WSADATA WSAinfo;
#  ifdef errno
#    undef errno
#  endif
#  define errno h_errno
#endif

#define OWW_PROTO_VERSION 101L
#define OWWTRX_DEBUG WERR_DEBUG1

/* Pointer to memory for building message */
static uint32_t *oww_trx_buffer = NULL ;

static uint32_t oww_trx_remote_version ;

/* Length and allocation */
static int oww_trx_buflen = 0, oww_trx_bufalloc = 0 ;

static void
oww_trx_add_float(int i, float f)
{
  uint32_t intbuf;
  memcpy(&intbuf, &f, 4);
  oww_trx_buffer[i] = htonl(intbuf);
}

static void
oww_trx_bufexp(int words)
{
  /* Expand buffer size */
  oww_trx_buflen += words ;

  /* Need more memory now? */
  if (oww_trx_buflen <= oww_trx_bufalloc) return ;

  oww_trx_bufalloc += OWW_TRX_BUFINC ;

  oww_trx_buffer = (uint32_t *) realloc((void *) oww_trx_buffer,
    sizeof(uint32_t) * oww_trx_bufalloc) ;

  if (!oww_trx_buffer) exit(1) ; /* Failed */
}

static int
oww_trx_tx_build_head(wsstruct *wd, int bufnext, int msg_type)
{
  char ident[] = "Oww" ;
  int *identi ;

  identi = (int *) ident ;

  /* Construct header */

  oww_trx_bufexp(5) ;

  oww_trx_buffer[bufnext++] = htonl(OWW_TRX_HEAD + 4L) ;
  oww_trx_buffer[bufnext++] = *identi ;
  oww_trx_buffer[bufnext++] = 0 ; /* This will hold the size */
  oww_trx_buffer[bufnext++] = htonl(msg_type & 0xffL) ;
  oww_trx_buffer[bufnext++] = htonl(OWW_PROTO_VERSION) ;

  return bufnext ;
}

int
oww_trx_tx_build(wsstruct *wd, int msg_type)
{
  int i, bufnext = 0 ;

  oww_trx_buflen = 0 ;

  //assert(sizeof(float) == sizeof(long int)) ;

  /* Construct header */

  bufnext = oww_trx_tx_build_head(wd, bufnext, msg_type) ;

  switch (msg_type)
  {
    case OWW_TRX_MSG_ANNOUN:
      /* Initial announcement - send location */
      oww_trx_bufexp(3) ;
      oww_trx_buffer[bufnext++] = htonl(OWW_TRX_LOC + 2L) ;
      oww_trx_add_float(bufnext++, setup_latitude) ;
      oww_trx_add_float(bufnext++, setup_longitude) ;
     /* Fall through for normal WS data */

     /* no break */

    case OWW_TRX_MSG_WSDATA:
    {
      /* Normal data update */
      /* Times */

      oww_trx_bufexp(5) ;

      oww_trx_buffer[bufnext++] = htonl(OWW_TRX_TIME + 4L) ;
      oww_trx_buffer[bufnext++] = htonl(wd->t) ;
      oww_trx_buffer[bufnext++] = htonl(convert_time(wd->t)) ;
      oww_trx_buffer[bufnext++] = htonl(setup_interval) ;
      oww_trx_buffer[bufnext++] = 0L ; /* Not using microsecond slot */

      /* Wind */

      if (devices_have_vane())
      {
        oww_trx_bufexp(4) ;
        oww_trx_buffer[bufnext++] = htonl(OWW_TRX_WIND + 3L) ;
        if ((wd->vane_bearing>0)&&(wd->vane_bearing<=16))
          oww_trx_buffer[bufnext++] = htonl((int32_t) (wd->vane_bearing - 1) * 2250L) ;
        else
          oww_trx_buffer[bufnext++] = 0L ;
        oww_trx_add_float(bufnext++, wd->anem_mps) ;
        oww_trx_add_float(bufnext++, wd->anem_int_gust * 0.447040972F) ;
      }

      /* Rain */

      if (devices_have_rain())
      {
        oww_trx_bufexp(5) ;
        oww_trx_buffer[bufnext++] = htonl(OWW_TRX_RAIN + 4L) ;
        oww_trx_buffer[bufnext++] = htonl(wd->rain_count) ;
        /*if (wd->rain < (float) LONG_MAX * 3.9e-4)
          oww_trx_buffer[bufnext++] = htonl((int) (wd->rain * 2540.0F)) ;
        else
          oww_trx_buffer[bufnext++] = 0 ;*/
        oww_trx_add_float(bufnext++, wd->rain) ;
        oww_trx_buffer[bufnext++] = htonl((int) wd->t_rain[0]) ;
        /*oww_trx_buffer[bufnext++] =
          htonl((int) (wd->rain_rate * 2540.0F)) ;*/
        oww_trx_add_float(bufnext++, wd->rain_rate) ;
      }

      /* Temperatures */

      for (i = 0; i < MAXTEMPS; ++i)
        if (devices_have_temperature(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_T + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->T[i]) ;
          /*oww_trx_buffer[bufnext++] =
            htonl((int) (wd->T[i] * 1000.0F)) ;*/
        }

      /* Soil Temperatures */

      for (i = 0; i < MAXSOILTEMPS; ++i)
        if (devices_have_soil_temperature(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_T + OWW_TRX_SUB_TSOIL + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->soilT[i]) ;
          /*oww_trx_buffer[bufnext++] =
            htonl((int) (wd->T[i] * 1000.0F)) ;*/
        }

      /* Indoor Temperatures */

      for (i = 0; i < MAXINDOORTEMPS; ++i)
        if (devices_have_indoor_temperature(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_T + OWW_TRX_SUB_TINDOOR + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->indoorT[i]) ;
          /*oww_trx_buffer[bufnext++] =
            htonl((int) (wd->T[i] * 1000.0F)) ;*/
        }

      /* RH */

      for (i = 0; i < MAXHUMS; ++i)
        if (devices_have_hum(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_RH + OWW_TRX_SERIAL * i + 1L) ;
          /*oww_trx_buffer[bufnext++] =
            htonl((int) (wd->RH[i] * 1000.0F)) ;*/
          oww_trx_add_float(bufnext++, wd->RH[i]) ;
        }

      /* Trh */

      for (i = 0; i < MAXHUMS; ++i)
        if (devices_have_hum(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_T + OWW_TRX_SUB_TRH + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->Trh[i]) ;
          /*oww_trx_buffer[bufnext++] =
            htonl((int) (wd->Trh[i] * 1000.0F)) ;*/
        }

      /* Barometers */

      for (i = 0; i < MAXBAROM; ++i)
        if (devices_have_barom(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_BP + OWW_TRX_SERIAL * i + 1L) ;
          /*oww_trx_buffer[bufnext++] =
            htonl((int) (wd->barom[i] * 1000.0F)) ;*/
          oww_trx_add_float(bufnext++, wd->barom[i]) ;
        }

      /* Tb */

      for (i = 0; i < MAXBAROM; ++i)
        if (devices_have_barom(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_T + OWW_TRX_SUB_TB + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->Tb[i]) ;
        }

      /* General purpose counters */

      for (i = 0; i < MAXGPC; ++i)
        if (devices_have_gpc(i))
        {
          oww_trx_bufexp(6) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_GPC + OWW_TRX_SERIAL * i + 5L) ;
          /*oww_trx_buffer[bufnext++] =
            htonl(*((long *) &(wd->gpc[i].gpc))) ;*/
          oww_trx_add_float(bufnext++, wd->gpc[i].gpc) ;
            /*htonl((long) (wd->gpc[i].gpc * 1000.0F)) ;*/
          /*oww_trx_buffer[bufnext++] =
            htonl((long) (wd->gpc[i].delta * 1000.0F)) ;*/
          oww_trx_add_float(bufnext++, wd->gpc[i].delta) ;
          oww_trx_buffer[bufnext++] =
            htonl(wd->gpc[i].count) ;
          /*oww_trx_buffer[bufnext++] =
            htonl((long) (wd->gpc[i].rate * 1000.0F)) ;*/
          oww_trx_add_float(bufnext++, wd->gpc[i].rate) ;
          oww_trx_buffer[bufnext++] =
            htonl(wd->gpc[i].time_reset) ;
        }

      /* Solar sensors */

      for (i = 0; i < MAXSOL; ++i)
        if (devices_have_solar_data(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_SOLAR + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->solar[i]) ;
        }

      /* UVI sensors */

      for (i = 0; i < MAXUV; ++i)
        if (devices_have_uv_data(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_UVI + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->uvi[i]) ;
        }
//
//      /* Tuv */
//
//      for (i = 0; i < MAXUV; ++i)
//        if (devices_have_uv_data(i))
//        {
//          oww_trx_bufexp(2) ;
//          oww_trx_buffer[bufnext++] =
//            htonl(OWW_TRX_T + OWW_TRX_SUB_TUV + OWW_TRX_SERIAL * i + 1L) ;
//          oww_trx_add_float(bufnext++, wd->uviT[i]) ;
//        }

      /* ADC channels */
      for (i=0; i<MAXADC; ++i)
    	if (devices_have_adc(i))
    	{
          oww_trx_bufexp(4) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_ADC + OWW_TRX_SERIAL * i + 3L) ;
          oww_trx_add_float(bufnext++, wd->adc[i].V) ;
          oww_trx_add_float(bufnext++, wd->adc[i].I) ;
          oww_trx_add_float(bufnext++, wd->adc[i].Q) ;
    	}

      /* ADC temperatures */
      for (i=0; i<MAXADC; ++i)
    	if (devices_have_adc(i))
    	{
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_T + OWW_TRX_SUB_TADC + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->adc[i].T) ;
    	}

      /* Thermocouple temperatures */
      for (i=0; i<MAXTC; ++i)
    	if (devices_have_thrmcpl(i))
    	{
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_T + OWW_TRX_SUB_TTC + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->thrmcpl[i].T) ;
    	}

      /* Thermocouple cold junction temperatures */
      for (i=0; i<MAXTC; ++i)
    	if (devices_have_thrmcpl(i))
    	{
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_T + OWW_TRX_SUB_TCJ + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->thrmcpl[i].Tcj) ;
    	}


      /* Soil moisture sensors */

      for (i = 0; i < MAXMOIST*4; ++i)
        if (devices_have_soil_moist(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_MOIST + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, 10.0F * wd->soil_moist[i]) ;
        }

      /* Leaf wetness sensors */

      for (i = 0; i < MAXMOIST*4; ++i)
        if (devices_have_leaf_wet(i))
        {
          oww_trx_bufexp(2) ;
          oww_trx_buffer[bufnext++] =
            htonl(OWW_TRX_LEAF + OWW_TRX_SERIAL * i + 1L) ;
          oww_trx_add_float(bufnext++, wd->leaf_wet[i]) ;
        }

      break ;
    }

    case OWW_TRX_MSG_UPDT:
      /* Change in update interval */
      /* Times */

      oww_trx_bufexp(2) ;

      oww_trx_buffer[bufnext++] = htonl(OWW_TRX_UPDT + 1L) ;
      oww_trx_buffer[bufnext++] = htonl(setup_interval) ;
      break ;

    case OWW_TRX_MSG_MORT:
      /* Server is dying - no data */
      break ;

  }

  /* End */

  oww_trx_bufexp(1) ;

  oww_trx_buffer[bufnext++] = htonl(OWW_TRX_END) ;

  /* Set size in oww_trx_buffer[1] */

  oww_trx_buffer[2] = htonl(bufnext * (int32_t) sizeof(uint32_t)) ;

  return bufnext ;
}

int
oww_trx_tx(wsstruct *wd, int msg_type)
{
  int buflen ;

  buflen = oww_trx_tx_build(wd, msg_type) ;

  if (0 >= buflen) return -1 ; /* Failed */

  /* Check client status */

  server_poll_clients(&server_oww) ;
  server_poll_clients(&server_oww_un) ;

  server_send_bin_to_clients((void *) oww_trx_buffer,
    buflen * sizeof(uint32_t),
    &server_oww) ;

  server_send_bin_to_clients((void *) oww_trx_buffer,
    buflen * sizeof(uint32_t),
    &server_oww_un) ;

  return 0 ;
}

/* Build an announce message, returning a malloced copy */

void *
oww_trx_build_announce(wsstruct *wd, int *len)
{
  *len = sizeof(int) * oww_trx_tx_build(wd, OWW_TRX_MSG_ANNOUN) ;

  return oww_trx_buffer ;
}

/* oww_trx_rx_decode
 * Decode one item from the remote data source
 *
 * Return 0 if this is not the last item
 * Return 1 if it is the last item
 * Return -1 on failure
*/

static int
oww_trx_rx_decode(
  uint32_t head_word,
  uint32_t data_word[],
  uint32_t data_raw[],
  wsstruct *wsp,
  int *msg_type)
{
  int32_t i ;
  int32_t *data_long ;
  float *data_float ;
  int32_t length ;

  length = 0xffL & head_word ;

  i = (head_word & 0xff00) >> 8 ;

  data_long = (int32_t *) data_word ;
  data_float = (float *) data_word ;

  werr(OWWTRX_DEBUG, "oww_trx_rx_decode(0x%x, [], *)", head_word) ;
  switch (head_word & 0xff000000L)
  {
    case OWW_TRX_END:
      /* We are done */
      return 1 ;

    case OWW_TRX_HEAD:
    {
      /* Check that the "Oww" identifier is present */
      if (strcmp((char *) data_raw, "Oww") != 0)
      {
        werr(0, _("The remote data source is not using Oww protocol!")) ;
        return -1 ; /* Failed */
      }

      /* Get message type */
      *msg_type = (int) data_word[2] ;

      /* Get the protocol version, if given */
      if (length >= 4L)
      {
        oww_trx_remote_version = data_long[3] ;
      }
      else
      {
        oww_trx_remote_version = 0 ;
      }
      werr(OWWTRX_DEBUG,
           "oww_trx_remote_version = %d",
           oww_trx_remote_version) ;

      break ;
    }

    case OWW_TRX_LOC:
      /* Do we have latitude and longitude? */
      werr(OWWTRX_DEBUG, "Received location record") ;
      if (length >= 2L)
      {
        ws.latitude  = data_float[0] ;
        ws.longitude = data_float[1] ;
        werr(OWWTRX_DEBUG, "Latitude = %f, Longitude = %f",
          ws.latitude, ws.longitude) ;
      }
      break ;

    case OWW_TRX_UPDT:
      wsp->interval = (int) data_word[2] ;
      werr(OWWTRX_DEBUG, "Received time change (%d)", wsp->interval) ;
      break ;

    case OWW_TRX_TIME:
    {
      //time_t last_t = wsp->t;
      wsp->interval = (int) data_word[2] ;
      wsp->t = (time_t) data_word[0] ;
      //wsp->delta_t = (last_t==0)? wsp->interval: wsp->t-wsp->delta_t;
      /* If we have times we have data */
      werr(OWWTRX_DEBUG,
           "Received header data. interval = %d",
           wsp->interval) ;
      break ;
    }

    case OWW_TRX_WIND:
      wsp->vane_bearing = (int) data_long[0] / 2250 + 1 ;
      if (oww_trx_remote_version >= 100L)
      {
        wsp->anem_mps = data_float[1] ;
        wsp->anem_int_gust = wsp->anem_gust = data_float[2] * 2.2369314F ;
      }
      else
      {
        wsp->anem_mps = 0.001F * (float) data_long[1] ;
        wsp->anem_int_gust = wsp->anem_gust = (float) data_long[2] * 0.0022369314F ;
      }
      wsp->anem_speed = ws.anem_mps * 2.2369314F ;
      devices_remote_vane = 1 ;
      werr(OWWTRX_DEBUG, "Received wind data: %d, %f, %f",
        wsp->vane_bearing, wsp->anem_mps, wsp->anem_gust) ;
      break ;

    case OWW_TRX_RAIN:
      wsp->rain_count_old = wsp->rain_count; /* Will pick up COUNT_NOT_SET initially */
      wsp->rain_count = data_word[0] ;
      wsp->t_rain[0] = (time_t) data_word[2] ;
      if (oww_trx_remote_version >= 100L)
      {
      	// We assume rain == dailyrain i.e. reset at local midnight
        wsp->rain = wsp->dailyrain = data_float[1] ;
        wsp->rain_rate = data_float[3] ;
      }
      else
      {
        wsp->rain = wsp->dailyrain = (float) data_long[1] / 2540.0F ;
        /* Fix me - what about monthly rain stats? */
        wsp->rain_rate = (float) data_long[3] / 2540.0F ;
      }
      devices_remote_rain = 1 ;
      werr(OWWTRX_DEBUG, "Received rain data: %f (%f) t=%d",
        wsp->rain, wsp->rain_rate, wsp->t_rain[0]) ;
      break ;

    case OWW_TRX_T:
      if (oww_trx_remote_version >= 100L)
      {
        werr(OWWTRX_DEBUG, "Received T data: %f",
          data_float[0]) ;
        switch (head_word & 0xff0000)
        {
          case OWW_TRX_SUB_TRH:
            wsp->Trh[i] = data_float[0] ;
            devices_remote_RH[i] = 1 ;
            werr(OWWTRX_DEBUG, "Sub type Trh") ;
            break ;

          case OWW_TRX_SUB_TB:
            wsp->Tb[i] = data_float[0] ;
            devices_remote_barom[i] = 1 ;
            werr(OWWTRX_DEBUG, "Sub type Tb") ;
            break ;

          case OWW_TRX_SUB_TUV:
            wsp->uviT[i] = data_float[0] ;
            devices_remote_uv[i] = 1 ;
            werr(OWWTRX_DEBUG, "Sub type Tuv") ;
            break ;

          case OWW_TRX_SUB_TTC:
            wsp->thrmcpl[i].T = data_float[0] ;
            devices_remote_tc[i] = 1 ;
            werr(OWWTRX_DEBUG, "Sub type Ttc") ;
            break ;

          case OWW_TRX_SUB_TCJ:
            wsp->thrmcpl[i].Tcj = data_float[0] ;
            devices_remote_tc[i] = 1 ;
            werr(OWWTRX_DEBUG, "Sub type Tcj") ;
            break ;

          case OWW_TRX_SUB_TSOIL:
        	wsp->soilT[i] = data_float[0] ;
            devices_remote_soilT[i] = 1 ;
            werr(OWWTRX_DEBUG, "Sub type Tsoil") ;
            break ;

          case OWW_TRX_SUB_TINDOOR:
        	wsp->indoorT[i] = data_float[0] ;
            devices_remote_indoorT[i] = 1 ;
            werr(OWWTRX_DEBUG, "Sub type Tindoor") ;
            break ;

          default:
            wsp->T[i] = data_float[0] ;
            devices_remote_T[i] = 1 ;
            break ;
        }
      }
      else
      {
        werr(OWWTRX_DEBUG, "Received T data: %f",
          0.001 * (double) data_long[0]) ;
        switch (head_word & 0xff0000)
        {
        case OWW_TRX_SUB_TRH:
          wsp->Trh[i] = (float) data_long[0] * 0.001F ;
          devices_remote_RH[i] = 1 ;
          werr(OWWTRX_DEBUG, "Sub type Trh") ;
          break ;

        case OWW_TRX_SUB_TB:
          wsp->Tb[i] = (float) data_long[0] * 0.001F ;
          devices_remote_barom[i] = 1 ;
          werr(OWWTRX_DEBUG, "Sub type Tb") ;
          break ;

        case OWW_TRX_SUB_TADC:
          wsp->adc[i].T = (float) data_long[0] * 0.001F ;
          devices_remote_adc[i] = 1 ;
          werr(OWWTRX_DEBUG, "Sub type Tadc") ;
          break ;

          default:
            wsp->T[i] = (float) data_long[0] * 0.001F ;
            devices_remote_T[i] = 1 ;
            break ;
        }
      }
      break ;

    case OWW_TRX_RH:
      if (oww_trx_remote_version >= 100L)
      {
        wsp->RH[i] = data_float[0] ;
      }
      else
      {
        wsp->RH[i] = (float) data_long[0] * 0.001F ;
      }
      devices_remote_RH[i] = 1 ;
      werr(OWWTRX_DEBUG, "Received RH data: %f",
        wsp->RH[i]) ;
      break ;

    case OWW_TRX_BP:
      if (oww_trx_remote_version >= 100L)
      {
        wsp->barom[i] = data_float[0] ;
      }
      else
      {
        wsp->barom[i] = (float) data_long[0] * 0.001F ;
      }
      devices_remote_barom[i] = 1 ;
      werr(OWWTRX_DEBUG, "Received BP data: %f",
        wsp->barom[i]) ;
      break ;

    case OWW_TRX_GPC:
      wsp->gpc[i].gpc = data_float[0] ;
      /*wsp->gpc[i].gpc = (float) data_long[0] * 0.001F ;*/
      /*wsp->gpc[i].delta = (float) data_long[1] * 0.001F ;*/
      wsp->gpc[i].delta = data_float[1] ;
      wsp->gpc[i].count = data_word[2] ;
      /*wsp->gpc[i].rate = (float) data_long[3] * 0.001F ;*/
      wsp->gpc[i].rate = data_float[3] ;
      wsp->gpc[i].time_reset = (time_t) data_word[4] ;
      devices_remote_gpc[i] = 1 ;
      werr(OWWTRX_DEBUG, "Received GPC data: %f",
        wsp->gpc[i].gpc) ;
      break ;

    case OWW_TRX_SOLAR:
      wsp->solar[i] = data_float[0] ;
      devices_remote_solar[i] = 1 ;
      werr(OWWTRX_DEBUG, "Received Solar data: %f",
        wsp->solar[i]) ;
      break ;

    case OWW_TRX_UVI:
      wsp->uvi[i] = data_float[0] ;
      devices_remote_uv[i] = 1 ;
      werr(OWWTRX_DEBUG, "Received UV data: %f",
        wsp->uvi[i]) ;
      break ;

    case OWW_TRX_ADC:
      wsp->adc[i].V = data_float[0] ;
      wsp->adc[i].I = data_float[1] ;
      wsp->adc[i].Q = data_float[2] ;
      wsp->adc[i].T = data_float[3] ;
      devices_remote_adc[i] = 1 ;
      werr(OWWTRX_DEBUG, "Received ADC data: %f, %f, %f",
        wsp->adc[i].V,
        wsp->adc[i].I,
        wsp->adc[i].Q) ;
      break ;

    case OWW_TRX_MOIST:
      wsp->soil_moist[i] = 0.1F * data_float[0] ;
      devices_remote_soilmoisture[i] = 1 ;
      werr(OWWTRX_DEBUG, "Received Soil Moisture data: %f",
        wsp->soil_moist[i]) ;
      break ;

    case OWW_TRX_LEAF:
      wsp->leaf_wet[i] = data_float[0] ;
      devices_remote_leafwetness[i] = 1 ;
      werr(OWWTRX_DEBUG, "Received Leaf Wetness data: %f",
        wsp->leaf_wet[i]) ;
      break ;
  }

  return 0 ; /* Not done yet */
}

int
oww_trx_rx(int socket, wsstruct *wsp, int *msg_type)
{
  uint32_t head_word ;
  uint32_t data_word[16], data_word_n[16], word_n ;
  int data_size, data_to_get, done = 0, size, i ;

  //assert(sizeof(float) == sizeof(long int)) ;

  /* Try to read data from the server */

  if (socket <= 0)
  {
    werr(1, _("Bad socket: %d"), socket) ;
  }

  /* Ready to read remote data, but assume all remote devices gone */

  devices_clear_remote() ;

  /* Read the header word */

  /*werr(WERR_WARNING, "About to call recv()") ;*/

  /* Socket has been marked as non-blocking, so just try to read */

  while (!done)
  {
    size = recv(socket,
      (char *) &word_n,
      sizeof(uint32_t),
      0) ;

    /*werr(OWWTRX_DEBUG, "recv() [head] returned %d", size) ;*/

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
      werr(OWWTRX_DEBUG, "0 == recv() [head]") ;
      return -1 ;
    }

    head_word = ntohl(word_n) ;

    werr(OWWTRX_DEBUG, "Head word 0x%x", head_word);

    /* Now get the data */

    data_size = (int) (head_word & 0xffL) ;
    //assert (data_size <= 16) ;
    werr(WERR_DEBUG1, "data_size = %d", data_size) ;

    data_to_get = data_size ;

    while(data_to_get)
    {
      size = recv(socket,
        (char *) &data_word_n[data_size-data_to_get],
        sizeof(uint32_t) * data_to_get,
        0) ;

      if (size > 0)
      {
        data_to_get -= size / sizeof(uint32_t) ;
        break ;
      }

      if (size == -1)
      {
        /*if (errno != EWOULDBLOCK)
        { */
          werr(0, "recv() failed: %d \"%s\"", errno, strerror(errno)) ;
          return -1 ; /* Connexion down */
        /*}*/
      }

      if (size == 0)
      {
        werr(OWWTRX_DEBUG, "0 == recv()") ;
        return -1 ;
      }
    }

    for (i=0; i<data_size; ++i)
      data_word[i] = ntohl(data_word_n[i]) ;

    done = oww_trx_rx_decode(
      head_word, data_word, data_word_n, wsp, msg_type) ;

    if (-1 == done) return -1 ; /* Failure */
  }

  /* Flag that now we have remote data */
  devices_remote_used = 1 ;

  return 0 ; /* Ok - done */
}
