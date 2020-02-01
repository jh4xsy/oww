/* progstate.c */

/*
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * 1999 - 2000
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/*
   program state machine
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <string.h>
#include "werr.h"
#include "progstate.h"

//#include "wstypes.h"
#include "applctn.h"
#include "weather.h"
#include "log.h"
#include "utility.h"
#include "stats.h"
#include "client.h"

#include "setup.h"
//#include "message.h"
#include "oww.h"
#include "events.h"
#include "globaldef.h"
#include "wstypes.h"

#include "sendwx.h"
#include "oww_trx.h"
#include "arne.h"
#include "devices.h"
#include "intl.h"
#include "ownet.h"

#ifndef NOGUI
#include "mainwin.h"
#include "auxwin.h"
#endif

#ifdef RISCOS
#include "devclaim.h"

unsigned long int next_animate=0UL ;
#endif

int client_sock = -1 ;

const char *prog_states[] = {"startup",
                       "ready",
                       "waiting",
                       "readanemstart",
                       "starttempconv",
                       "waiting_tempconv",
                       "endtempconv",
                       "read_humidity",
                       "read_barometer",
                       "read_gpc",
                       "read_solar",
                       "read_uv",
                       "read_adc",
                       "read_tc",
                       "read_moist",
                       "readanemend",
                       "read_ws_done",
                       "idle",
                       "port_ok",
                       "wsdead",
                       "learn",
                       "newcom",
                       "zombie",
                       "devclaim",
                       "noport",
                       "remote_connect",
                       "remote_read",
                       "remote_done",
                       "datasource_none",
                       "dallas_startrx",
                       "dallas_rx",
                       "remote_zombie",
                       "restart",
                       "unknown state"
} ;

int prog_state=state_startup, prog_state_old=state_startup ;

int die_next = 0;
int local_shutdown_pending = 0;

/*
static int prog_state_count[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ;*/


extern statsstruct logstats, sendwxstats ;
//extern wsstruct ws ;

static rainlist remote_list1hr = {NULL, 0, 0, RAININT_TIMEOUT_1HR} ;
static rainlist remote_list24hr = {NULL, 0, 0, RAININT_TIMEOUT_24HR} ;


/*extern int end_temp_time ;*/
int64_t end_temp_time ;

time_t the_time ;
int64_t next_update=0UL ;
int64_t last_gust=0UL ;

int64_t csGettick(void) ;

const char *state_get_name(int i)
{
  if (i > STATE_STATES) i = STATE_STATES ;

  return prog_states[i] ;
}

void state_new(int new_state)
{
  static int state_pending = -1 ;

  if ((state_pending != -1) && (new_state == state_readanemend)) {
    /* Temp conv finished - continue with pending state change */
    new_state = state_pending ;
    state_pending = -1 ;
    werr(WERR_DEBUG0, "Changing to pending state - %s",
      prog_states[new_state]) ;
  } else {
    if ((prog_state == state_waiting_tempconv) &&
        (new_state != state_endtempconv)) {
      /* Have been asked to change state other than state_endtempconv
         during a temp conv.
         Wait until it has finished */
      werr(WERR_DEBUG0, "State %s requested during tempconv - waiting",
        prog_states[new_state]) ;
      state_pending = new_state ;
      return ;
    }
  }

  prog_state_old = prog_state ;
  prog_state = new_state ;

  werr(WERR_DEBUG1, "prog_state %s -> %s",
    prog_states[prog_state_old], prog_states[prog_state]) ;
}

#ifndef NOGUI
/* Source name for mainwin */
char *
progstate_sourcename(void)
{
  switch (setup_datasource)
  {
    case data_source_local:
      return "Local" ;

    case data_source_remote_oww:
      return setup_owwremote_host ;

    case data_source_arne:
      return setup_arneremote_host ;

    case data_source_dallas:
      return sendwx_dallasremote_name ;
  }

  return "" ;
}
#endif

/* Change the data source */

void
state_change_datasource(int new_source)
{
  werr(WERR_DEBUG0, "state_change_datasource %d -> %d, state = %s",
    setup_datasource, new_source, state_get_name(prog_state)) ;

  switch (setup_datasource)
  {
    case data_source_none:
      break ;

    case data_source_local:
//      weather_shutdown() ;
      local_shutdown_pending = 1;
      /* Bus released - force immediate state change */
      //prog_state = state_idle ;
      break ;

    case data_source_remote_oww:
    case data_source_arne:
      client_sock = client_kill(client_sock) ;
      break ;

    case data_source_dallas:
      sendwx_kill_dallas_rx() ;
      break ;
  }

  setup_datasource = new_source ;

  /* Local needs immediate update */
  next_update=last_gust=0UL ;

  state_new(state_startup) ;

  return ;
}

/* Read out state counts for debugging and reset */

/*static void state_readout(void)
{
  int i ;

  werr(WERR_DEBUG0,
       "SU:%d RDY:%d WT:%d RAS:%d STT:%d WTC:%d ETC:%d RH:%d RAE:%d DN:%d IDL:%d POK:%d DEAD:%d LRN:%d NC:%d Z:%d DC:%d NP:%d",
       prog_state_count[state_startup],
       prog_state_count[state_ready],
       prog_state_count[state_waiting],
       prog_state_count[state_readanemstart],
       prog_state_count[state_starttempconv],
       prog_state_count[state_waiting_tempconv],
       prog_state_count[state_endtempconv],
       prog_state_count[state_read_humidity],
       prog_state_count[state_readanemend],
       prog_state_count[state_read_ws_done],
       prog_state_count[state_idle],
       prog_state_count[state_port_ok],
       prog_state_count[state_wsdead],
       prog_state_count[state_learn],
       prog_state_count[state_newcom],
       prog_state_count[state_zombie],
       prog_state_count[state_devclaim],
       prog_state_count[state_noport]) ;

  for (i=0; i<30; prog_state_count[i++]=0) ;
}*/

/* Get the data parsed and update mainwin */

static int
state_update_mainwin(char *body, void *test_data)
{
  if (!sendwx_parse_dallas_rx(body, &ws)) return 0 ;

  #ifndef NOGUI
  mainwin_update(1 /* New data */) ;
  auxwin_update(&ws) ;
  #endif

  return 1 ;
}

//void checkvane(char *msg)
//{
//  static int vane=0;
//
//  if (ws.vane_bearing != vane) {
//	printf("Vane changed from %d to %d at state %s - %s\n", vane, ws.vane_bearing, prog_states[prog_state], msg);
//	vane = ws.vane_bearing;
//  }
//
//}

int state_machine(void)
{
  int64_t cs ;
  static int64_t resurrect = 0L ;

//  checkvane("start of state_machine()");

  werr(WERR_DEBUG2, "state_machine: %s", prog_states[prog_state]) ;

  /*++prog_state_count[prog_state] ;*/ /* Increment count for debug */

  switch (prog_state) {
    case state_startup:
      if (local_shutdown_pending)
      {
        local_shutdown_pending = 0;
        weather_shutdown() ;
      }

      if (setup_datasource == data_source_none)
    	oww_main_quit();

#     ifdef NOGUI
      /* owwnogui should save a default setup file */
      if (!setup_loaded_setup)
      {
        werr(0, _("Saving default setup")) ;
        setup_save_setup(1) ;
        setup_loaded_setup =  1  ;
      }
#     endif

      time(&the_time) ;
      stats_reset(&logstats) ;    /* Initialize stats for log */
      stats_reset(&sendwxstats) ; /* Initialize stats for http upload */
      ws.rain_count = COUNT_NOT_SET;

      if (applctn_startup_finished())
      {
        switch (setup_datasource)
        {
          case data_source_none:
            state_new(state_datasource_none) ;
            break ;

          case data_source_local:
            /* Local data */
#ifdef RISCOS
#ifdef NOGUI
            state_new(state_port_ok) ;
#else // ifdef NOGUI
            state_new(state_devclaim) ;
#endif // ifdef NOGUI
#else // ifdef RISCOS
            state_new(state_port_ok) ;
#endif // ifdef RISCOS
            break ;

          case data_source_remote_oww:
            rainint_wipe(&remote_list1hr) ;
            rainint_wipe(&remote_list24hr) ;
            state_new(state_remote_connect) ;
            /* Remote Oww */
            break ;

          case data_source_arne:
            rainint_wipe(&remote_list1hr) ;
            rainint_wipe(&remote_list24hr) ;
            state_new(state_remote_connect) ;
            /* Remote Arne server (could be Oww) */
            break ;

          case data_source_dallas:
            state_new(state_dallas_startrx) ;
            break ;
        }
      }
      applctn_smtest_at(SMTEST_AT_ASAP) ; /* re-enter immediately  */
      break ;

    /* Idling with no data source */
    case state_datasource_none:
      if (setup_datasource != data_source_none)
        /* Want some data now - restart */
        state_new(state_startup) ;

      /* Still idling */
     applctn_smtest_at(csGettick() + 25L) ;
     break ;


    case state_remote_connect:
      switch (setup_datasource)
      {
        case data_source_remote_oww:
          client_sock = client_connect(
            setup_owwremote_host,
            setup_owwremote_port) ;

          if (-1 == client_sock)
          {
            werr(WERR_WARNING + WERR_AUTOCLOSE, _("Unable to connect to Oww server \"%s:%d\""),
              setup_owwremote_host, setup_owwremote_port) ;
            /* Try again in a bit */
            werr(WERR_WARNING + WERR_AUTOCLOSE,
              _("Will try again in %us"),
              REMOTE_RETRY/100UL) ;
            state_new(state_remote_zombie) ;
            resurrect = csGettick() + REMOTE_RETRY ;
            applctn_smtest_at(resurrect) ;
            break ;
          }
          break ;

        case data_source_arne:
          werr(WERR_DEBUG0, "connect to Arne server \"%s:%d\"",
              setup_arneremote_host, setup_arneremote_port) ;
          client_sock = client_connect(
            setup_arneremote_host,
            setup_arneremote_port) ;

          if (-1 == client_sock)
          {
            werr(WERR_WARNING + WERR_AUTOCLOSE,
              _("Unable to connect to Arne server \"%s:%d\""),
              setup_arneremote_host, setup_arneremote_port) ;
            /* Try again in a bit */
            werr(WERR_WARNING + WERR_AUTOCLOSE,
              _("Will try again in %us"),
              REMOTE_RETRY/100UL) ;
            state_new(state_remote_zombie) ;
            resurrect = csGettick() + REMOTE_RETRY ;
            applctn_smtest_at(resurrect) ;
            break ;
          }
          break ;

        default:
          werr(0, _("Unexpected datasource type")) ;
          state_change_datasource(data_source_none) ;
          break ;
      }

      if (client_sock > 0)
      {
        /* We have connected to the remote Oww */
        werr(WERR_DEBUG0, "We have connected to the remote Oww");
        state_new(state_remote_read) ;
      }
      break ;

    case state_remote_zombie:
      if (resurrect <= csGettick())
      {
        state_new(state_remote_connect) ;
        applctn_smtest_at(SMTEST_AT_ASAP) ;
      }
      else
      {
        applctn_smtest_at(csGettick() + 25L) ;
      }
      break ;

    case state_remote_read:
      {
        static struct timeval select_time = {0, 10} ;
        static fd_set fds, ers ;
        static time_t remote_timeout = 0 ;
        int msg_type ;

        if (client_sock < 0)
        {
          werr(0, "Illegal client socket") ;
          break ;
        }

        /* Time to try a read from the remote Oww */

        FD_ZERO(&fds) ;
        FD_SET(client_sock, &fds) ;

        FD_ZERO(&ers) ;
        FD_SET(client_sock, &ers) ;

        select(client_sock+1, &fds, NULL, &ers, &select_time) ;

        /* Error condition? */
        if (FD_ISSET(client_sock, &ers))
          werr(WERR_WARNING, _("Remote host error")) ;

        /* Data waiting? */

        if (FD_ISSET(client_sock, &fds))
        {
          int rx_return ;

          log_poll() ; /* Might want to start log now */
          if (setup_datasource == data_source_arne)
            rx_return = arne_rx(client_sock, &ws, &msg_type) ;
          else
            rx_return = oww_trx_rx(client_sock, &ws, &msg_type) ;

          switch (rx_return)
          {
            case 0: /* Good read - go to done state for mainwin_update */
              /*What did we get? */
              switch (msg_type)
              {
                case OWW_TRX_MSG_ANNOUN:
                  /* Initial announcement from server */
                  /*werr(WERR_WARNING, "Announced. %f, %f",
                    ws.longitude, ws.latitude) ;*/
                case ARNE_RX_WSDATA:
                case OWW_TRX_MSG_WSDATA:
                  /* We received a weather station update - so we're done */

                  /* Handle rain events */
                  if (devices_have_rain())
                  {
                    if (ws.rain_count > ws.rain_count_old)
                    {
                      rainint_new_rain(&remote_list1hr, &ws);
                      rainint_new_rain(&remote_list24hr, &ws);
                    }
                    ws.rainint1hr = rainint_readout(&remote_list1hr);
                    ws.rainint24hr = rainint_readout(&remote_list24hr);
                  }

                  state_new(state_remote_done) ;
                  /* Set timeout for next read */
                  remote_timeout = time(NULL) + ws.interval + 30 ;
                  break ;

                case OWW_TRX_MSG_UPDT:
                  /* Server has changed its update interval */
                  werr(WERR_DEBUG0,
                    "Server has changed its update interval") ;
                  remote_timeout = time(NULL) + ws.interval + 30 ;
                  break ;

                case OWW_TRX_MSG_MORT:
                  /* Server is about o die */
                  /* Close the current connexion */
                  werr(WERR_WARNING, _("Remote server dying")) ;
                  client_sock = client_kill(client_sock) ;
                  state_new(state_remote_connect) ;
                  applctn_smtest_at(csGettick() + 100L) ;
                  break ;

              }
              break ;

            case 1: /* Not ready yet - try again soon */
              applctn_smtest_at(csGettick() + 25L) ;
              /* But have we timed out? */
              if (time(NULL) >= remote_timeout)
              {
                /* Yes - we're 30s overdue */
                if (0 != client_check_conn(client_sock))
                {
                  client_sock = client_kill(client_sock) ;
                  state_new(state_remote_connect) ;
                }
              }
              break ;

            case -1: /* Connexion is dead - reconnect */
              werr(WERR_WARNING,
                _("Remote host connexion failed - trying again")) ;
              /* Close the current connexion */
              client_sock = client_kill(client_sock) ;
              state_new(state_remote_connect) ;
              applctn_smtest_at(csGettick() + 100L) ;
              break ;

            case -2: /* Connexion closed remotely */
              /* Close the current connexion */
              client_sock = client_kill(client_sock) ;
              werr(WERR_WARNING,
                _("Remote host closed connexion")) ;
              werr(WERR_WARNING + WERR_AUTOCLOSE,
                _("Reconnecting in %us"), LOCAL_RETRY/100L) ;
              state_new(state_remote_connect) ;
              applctn_smtest_at(csGettick() + LOCAL_RETRY) ;
              break ;
          }
        }
        else
        {
          /* Have we timed out? */
          if ((0 != remote_timeout) && (time(NULL) >= remote_timeout))
          {
            /* Yes - we're 30s overdue */
            /*if (0 != client_check_conn(client_sock))
            {*/
              werr(WERR_WARNING + WERR_AUTOCLOSE,
                _("Remote data source timed out - reconnecting")) ;
              client_sock = client_kill(client_sock) ;
              state_new(state_remote_connect) ;
              remote_timeout = 0 ;
            /*}*/
          }
          applctn_smtest_at(csGettick() + 25L) ;
        }
      }
      break ;


#ifdef RISCOS
    case state_devclaim:
      /* RISC OS only */
      /* Try to claim the port */
      werr(WERR_DEBUG0, "devclaim");
      #ifdef NOGUI
        werr(0, "state_devclaim!");
        state_new(state_port_ok) ;
        break ;
      #else /*}*/
      devclaim_send_claim() ;
      /* Now idle and wait for response */
      state_new(state_idle) ;
      /* Default fast polling OK whilst waiting */
      break ;

    case state_noport:
      /* devclaim failed */
      /* Try again in a bit */
      werr(WERR_DEBUG2,
        "Calling applctn_smtest_at(%d)",
        csGettick() + 100UL) ;
      applctn_smtest_at(csGettick() + 100UL) ;
      state_new(state_devclaim) ; /* Next call will try again */
      break ;
      #endif
#endif

    case state_port_ok:
    {
      /* Received all-clear to use comm port */

      devices_remote_used = 0 ; /* These data are local */
      /* Now ready to read for real */
      if (!weather_init()) {
        werr(WERR_WARNING + WERR_AUTOCLOSE,
          _("Check connexions and setup")) ;
        /* Initialization of WS failed - aprise user */
        state_new(state_zombie) ;
        resurrect = csGettick() + LOCAL_RETRY ;
        applctn_smtest_at(resurrect) ;
        werr(WERR_WARNING + WERR_AUTOCLOSE,
          "RETRY:Will restart in %u seconds",
          LOCAL_RETRY/100UL) ;
      } else {
        /* All OK. Continue */
        /* Search for weather station, at least to get any couplers */
        int found ;

        /* Revert to devices list */
        setup_loaded_devices = setup_load_devices() ;
        found = FindWeatherStation() ;

        werr(WERR_DEBUG0,
             "FindWeatherStation returned %d",
             found) ;
        state_new(state_ready) ;
        applctn_smtest_at(SMTEST_AT_NOW) ; /* re-enter immediately  */
      }
      break ;
    }

    case state_wsdead:
      /* micro LAN in strange state or WS has gone AWOL */
      /* Shut down COM and re-initialize */
      werr(WERR_DEBUG0,
        _("Problem with link or weather station")) ;
      weather_shutdown() ;

      resurrect = csGettick() + LOCAL_RETRY ;
      applctn_smtest_at(resurrect) ;
      state_new(state_zombie) ;
      werr(WERR_WARNING + WERR_AUTOCLOSE,
        _("RETRY:Will restart in %u seconds"),
        LOCAL_RETRY/100UL) ;
      break ;

    case state_zombie: /* Still dead? Try to resurrect. */
      if (resurrect <= csGettick())
      {
        devices_remote_used = 0 ; /* These data are local */
        if (weather_init())
        {
          state_new(state_restart) ;
          applctn_smtest_at(SMTEST_AT_ASAP) ;
          werr(WERR_DEBUG0, "state_zombie -> state_ready") ;
        }
        else
        {
          state_new(state_wsdead) ;
          resurrect = csGettick() + LOCAL_RETRY ;
          applctn_smtest_at(resurrect) ;
          werr(WERR_DEBUG0, "state_zombie -> state_wsdead") ;
        }
      }
      break ;

    case state_idle:
      /* Waiting on WS connection issue e.g dead, devclaim */
      /* Just continue with fast polling */
      break ;

    case state_dallas_startrx:
      /* Ready to fetch remote data - initialize logging, &c */
      time(&the_time) ;
#     ifndef RISCOS
      next_update = csGettick() ; /* What's the time? */
#     else
      next_animate = next_update = csGettick() ;
#     endif
      log_poll() ; /* Might want to start a log now */
      state_new(state_dallas_rx) ;
      break ;

    case state_dallas_rx:
      cs = csGettick() ;
      if (cs >= next_update) {
        next_update += setup_interval * 100L ;
        if (cs >= next_update)
          next_update += setup_interval * 100L *
            (1UL + (cs - next_update) / (setup_interval * 100L)) ;
        sendwx_recv_data(state_update_mainwin) ;
      }
#     ifdef RISCOS
      /* Might need to run animations */
      applctn_smtest_at(next_animate) ;
#     else
      applctn_smtest_at(cs + 10L) ;
#     endif
#     ifndef NOGUI
      /*cs = csGettick() ;*/
#     ifdef RISCOS
      mainwin_check_animation(cs) ;
#     endif
#     endif
      break ;

    case state_ready:   /* Normal startup */
    case state_restart: /* Restarted - log_poll() does nothing */
      /* setup timer and go to next state */
      time(&the_time) ;
      cs = csGettick() ; /* What's the time? */

      /* If we're here for the first time we need to set next_update */
      if (!next_update)
      {
        /* update now */
        next_update = cs /*+ setup_interval * 100UL*/ ;
        last_gust = next_update  ; /* Will fire after setup_gust_time */
      }
#     ifdef RISCOS
      next_animate = csGettick() ;
#     endif
      log_poll() ; /* Might want to start a log now */
      state_new(state_readanemstart) ;
      break ;

      case state_waiting:
      /* idle polling between WS reading */
    	if (die_next)
    	{
    	  oww_main_quit();
    	  return 0;
    	}
    	devices_poll();
      /* interval measured in seconds - must convert to cs */
      /*time(&wtime) ;*/
      cs = csGettick() ;
      werr(WERR_DEBUG2, "%li > %li?", cs, next_update);
      if (cs >= next_update) {
        next_update += setup_interval * 100UL ;
        if (cs >= next_update)
        {
          next_update += setup_interval * 100UL *
            (1UL + (cs - next_update) / (setup_interval * 100UL)) ;
          werr(WERR_DEBUG2, "Jump required to next_update");
        }
        werr(WERR_DEBUG2, "next_update = %li", next_update);
        last_gust = cs ;
        state_new(state_readanemstart) ;
      }
      else if ((setup_gust_time != 0) && (cs >= last_gust + setup_gust_time))
      {
        last_gust = cs ;
        if (weather_read_wsgust() == 1) // Ok? - Just ignore failed gust readings
        {
          stats_new_anem(ws, &logstats) ;
          stats_new_anem(ws, &sendwxstats) ;
          //weather_read_ws_read_gpc() ;
          weather_sub_update_gpcs() ;
          stats_new_gpc(ws, &logstats) ;
          stats_new_gpc(ws, &sendwxstats) ;
        }
      }
#     ifdef RISCOS
      /* Might need to run animations */
      applctn_smtest_at(next_animate) ;
#     else
      applctn_smtest_at(cs + 10UL) ;
#     endif
#     ifndef NOGUI
      /*cs = csGettick() ;*/
#     ifdef RISCOS
      mainwin_check_animation(cs) ;
#     endif
#     endif
      break ;

    case state_starttempconv:
      /* tempconv will be ready in 1 sec */
      end_temp_time = csGettick() + TEMP_CONV_TIME ;
      /* no break */
    case state_readanemstart:
    case state_endtempconv:
    case state_read_humidity:
    case state_read_barometer:
    case state_read_gpc:
    case state_read_solar:
    case state_read_uv:
    case state_read_adc:
    case state_read_tc:
    case state_read_moist:
      cs = csGettick() ;
      if // Check if we're due a gust reading
      (
       (prog_state != state_starttempconv) &&
       (setup_gust_time != 0) &&
       (cs >= last_gust + setup_gust_time)
      )
      {
        last_gust = cs ;
        if (weather_read_wsgust() == 1)
        {
          stats_new_anem(ws, &logstats) ;
          stats_new_anem(ws, &sendwxstats) ;
          //weather_read_ws_read_gpc() ;
          weather_sub_update_gpcs() ;
          stats_new_gpc(ws, &logstats) ;
          stats_new_gpc(ws, &sendwxstats) ;
        }
      }
      /* no break */
    case state_readanemend:
      if (!weather_read_ws()) {
        werr(WERR_WARNING + WERR_AUTOCLOSE,
          "Error reading from weather station: weather_read_ws failed") ;
        state_new(state_wsdead) ;
      }
      break ;

    case state_waiting_tempconv:
      /* idle polling until tempconv ready */
#     ifdef RISCOS
      applctn_smtest_at(next_animate) ;
#     else
      applctn_smtest_at(end_temp_time) ;
#     endif
      cs = csGettick() ;
      if (cs >= end_temp_time) {
        /* Time to read temp */
        state_new(state_endtempconv) ;
      }
      #ifndef NOGUI
#     ifdef RISCOS
      mainwin_check_animation(cs) ;
#     endif
      #endif
      break ;

    case state_read_ws_done:
      weather_read_ws(); // This will just update stats and transmit data to clients
      devices_remote_used = 0 ; /* These data are local */
      /* no break */

    case state_remote_done:
      /* Finished reading WS values - local or remote */
      time(&the_time) ;
      /*ws.t = the_time ;*/ /* Update time in WS record for Arne bcst, &c */
      #ifndef NOGUI
      mainwin_update(1 /* New data */) ;
//      checkvane("state_remote_done - about to call auxwin_update");
      auxwin_update(&ws) ;
      #endif

      /* Output to LCDs */
      if (!devices_remote_used)
        weather_lcd_output() ;

      /* Data for log */
      stats_new_data(ws, &logstats) ;
      log_poll() ;

      /* Data for http upload */
      stats_new_data(ws, &sendwxstats) ;

      /* Might want to do some uploads */
      sendwx_send() ;

      /* Should I die now? (Run and exit mode, or exit requested) */
      if (setup_interval == 0)
      {
        /* Might need to wait on http transactions */
        while (sendwx_poll()) ;
        state_machine_quit() ;
      }

      /* Next state depends on local or remote data source */
      if (prog_state == state_remote_done)
      {
        state_new(state_remote_read) ;
        applctn_smtest_at(csGettick() + 25UL) ;
      }
      else
      {
        state_new(state_waiting) ;
      /* Extra debug info - counts by each prog state */
      /*if (werr_will_output(WERR_DEBUG0))
        state_readout() ;*/
      }
      break ;

    case state_learn:
      /* Learn component IDs */
      devices_remote_used = 0 ; /* These data are local */
      if (weather_init())
      {
        int found ;

        found = FindWeatherStation() ;

        werr(WERR_DEBUG0,
             "FindWeatherStation returned %d",
             found) ;
        /*setupd_update_devices() ;*/

#       ifdef NOGUI
        /* Save device list now if we could not load it earlier */
        if (!setup_loaded_devices && found)
        {
          setup_save_devices() ;
          setup_loaded_devices = 1 ;
        }
#       endif

        state_new(state_startup) ;
        /* setupd_poll will pick this up and call setupd_update_devices */
      }
      else
      {
        werr(WERR_WARNING, "weather_init failed") ;
      }
      break ;

    case state_newcom:
      /* Shutdown micro Lan and restart with new COM port */
      weather_shutdown() ;
      #ifdef RISCOS
      /*setupd_new_com() ;*/
      #endif
      state_new(state_startup) ;
      break ;

    default:
      werr(1, "Program error: Unknown program state in main loop.") ;
      /* no break */
  }

  werr_report_owerr() ;

  return 1 ; /* OK */
}

void state_machine_exit_request(int urgent) {
  die_next = 1;
}

void state_machine_quit()
{
  if (setup_datasource  == data_source_local)
//	die_next = 1;
	state_change_datasource(data_source_none);
  else
	oww_main_quit();
}
