/*
 * oww.c
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * August - December 1999
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/* main calls init, sets state machine and calls polling loop */

#include <stdlib.h>
#include <time.h>

#ifndef RISCOS
#include <unistd.h>
#include <string.h>
#else
#include "utility.h"
#endif

#include "wstypes.h"
#include "applctn.h"
#include "progstate.h"
#include "setup.h"
//#include "message.h"
#include "werr.h"
#include "weather.h"
#include "choices.h"
#include "server.h"
#include "oww_trx.h"
#include "client.h"
#include "devices.h"
#include "globaldef.h"

char temp_string[temp_string_len] = "", temp_string2[temp_string_len] = "" ;

extern wsstruct ws ;
extern int client_sock ;

void oww_main_quit(void)
{
  /* Bye bye */

  switch (setup_datasource)
  {
    case data_source_none:
      break ;

    case data_source_local:
      setup_save_stats() ;
      weather_shutdown() ;
      /* Inform any clients of our demise */
      oww_trx_tx(&ws, OWW_TRX_MSG_MORT) ;
      break ;

    case data_source_remote_oww:
    case data_source_arne:
      client_sock = client_kill(client_sock) ;
      break ;

    case data_source_dallas:
      break ;
  }

  server_shutdown(&server_arne) ;
  server_shutdown(&server_oww) ;
  server_shutdown(&server_txt_tcp) ;
#ifdef HAVE_SYS_UN_H
  server_shutdown(&server_oww_un) ;
  server_shutdown(&server_txt_un) ;
#endif

  exit(0) ;
}

int main(int argc, char *argv[])
{
  applctn_pre_init(&argc, &argv) ;

  /* Start with nonsense T value */
  ws.T[0] = UNDEFINED_T ;

  /* Allocate file names , checking for extra args */
  /*setup_setup = strdup((argc > 1) ? argv[1] : OWW_SETUP) ;
  if (!setup_setup)
    werr(1, get_message("STRMEM", NULL)) ;
  setup_devices = strdup((argc > 2) ? argv[2] : OWW_DEVICES) ;
  if (!setup_devices)
    werr(1, get_message("STRMEM", NULL)) ;*/

  choices_find_files(argc, argv) ;

  weather_initialize_vals() ; /* Clear out the wsstruct */

  /* Load old setup from file */
  setup_loaded_setup = setup_load_setup() ;

  /* Set some default string values */
  if (!setup_format_logline)
    setup_format_logline = strdup(DEFAULT_LOGF) ;

  if (!setup_txt_format)
    setup_txt_format = strdup(DEFAULT_TXTSERVE_FORMAT) ;

  if (!setup_lcd_format)
    setup_lcd_format = strdup(DEFAULT_LCD_FORMAT) ;

  if (!setup_postupdate)
    setup_postupdate = strdup("") ;

  if (!setup_postlog)
    setup_postlog = strdup("") ;

  if (!setup_httpprecom)
    setup_httpprecom = strdup("") ;

  if (!setup_httppostcom)
    setup_httppostcom = strdup("") ;

  /* Build devices_list array */
  if (-1 == devices_build_devices_list()) exit(1) ;

  /* Load old devices from file */
  setup_loaded_devices = setup_load_devices() ;

  /* Load old daily stats from file */
  setup_loaded_stats = setup_load_stats() ;

  /* Check gust time is within sensible range */
  if (setup_gust_time != 0)
  {
    if (setup_gust_time < 50) // Too short?
      setup_gust_time = 50 ;
  }

  /* Init server */
  server_init() ;

  applctn_init(&argc, &argv) ; /* Architecture-specific initialization */

  state_new(state_startup) ;

  /*
   * poll loop
   */

  applctn_polling_loop() ;

  return 0 ;
}
