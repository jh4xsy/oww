/* applctn.c */

/*
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * 1999 - 2000
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/* architecture-specific startup code, &c, for Oww - Linux version  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#ifdef WIN32
#include "config-w32gcc.h"
#else
#include "../config.h"
#endif
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifndef HAVE_INT64_T
typedef long long int		int64_t;
#endif

#include <stdlib.h>
#include <stdio.h>

//#include <string.h>
#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifndef HAVE_SELECT
# undef ENABLE_INTERACTIVE
#endif // ifdef HAVE_SELECT

#ifdef ENABLE_INTERACTIVE
# ifdef HAVE_SELECT_H
#  include <sys/select.h>
# else
#  include <time.h>
#  include <sys/time.h>
# endif // ifdef HAVE_SELECT_H
# include "interactive.h"
#endif //ifdef ENABLE_INTERACTIVE

#include <unistd.h>
#include <assert.h>


#ifndef NOGUI
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "support.h"
#include "mainwin.h"
#include "setupd.h"
#include "../pixmaps/oww_xpm.xpm"
#else
# ifdef HAVE_LOCALE_H
#  include <locale.h>
# endif
#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif

#endif

#include "events.h"
#include "weather.h"
#include "werr.h"
#include "progstate.h"
#include "log.h"
#include "setup.h"
#include "globaldef.h"
#include "oww.h"
#include "sendwx.h"
#include "server.h"
#include "process.h"
#include "oww_trx.h"
#include "txtserve.h"
#include "intl.h"

extern char *choices_setupng_read, *choices_devices_read;

#ifndef NOGUI

#include "wstypes.h"

extern wsstruct ws ;

GtkBuilder *builder;

/* The window widgets */
GtkWidget *mainwin_id = NULL ;
GtkWidget *aux_id = NULL ;
GtkWidget *aux_list = NULL ;
GtkWidget *drawing_area = NULL ;
GtkWidget *proginfo_id = NULL ;
GtkWidget *setup_id = NULL ;
GtkWidget *devices_id = NULL ;
GtkWidget *werr_id = NULL, *werr_clist /*, *sw1*/ ;

void launchurl(char *url) ;

int
auxwin_g_init(void) ;

#endif

//char *path_and_leaf(const char *s1, const char *s2) ;
void msDelay(int len) ;

int messages[] = {0,0,0,0,0} ;
static int64_t next_smtest = SMTEST_AT_NOW ;
int applctn_quit = 0 ;
int daemon_proc = 0 ;

int64_t csGettick(void) ;

#ifndef NOGUI
extern unsigned long int next_update ;
#endif

extern server_struct server_arne, server_oww, server_oww_un ;
extern char debug_file[] ; /* Declared in werr */
extern int arg_daemon, arg_interactive ;

#ifndef HAVE_DAEMON

#ifdef HAVE_UNISTD_H
#include <fcntl.h>
#include <sys/types.h>

/* closeall() -- close all FDs >= a specified value */

static void closeall(int fd)
{
    /*int fdlimit = sysconf(_SC_OPEN_MAX);*/

    while (fd < 64)
      close(fd++);
}

/* daemon() - detach process from user and disappear into the background
 * returns -1 on failure, but you can't do much except exit in that case
 * since we may already have forked. This is based on the BSD version,
 * so the caller is responsible for things like the umask, etc.
 */

/* believed to work on all Posix systems */

int daemon(int nochdir, int noclose)
{
    switch (fork())
    {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);          /* exit the original process */
    }

    if (setsid() < 0)               /* shoudn't fail */
      return -1;

    /* dyke out this switch if you want to acquire a control tty in */
    /* the future -- not normally advisable for daemons */

    switch (fork())
    {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);
    }

    if (!nochdir)
      chdir("/");

    if (!noclose)
    {
        closeall(0);
        open("/dev/null",O_RDWR);
        dup(0); dup(0);
    }

    return 0;
}
#else
int daemon(int nochdir, int noclose)
{ return -1 ; }

#endif /* ifdef HAVE_UNISTD_H */

#endif /* ifndef HAVE_DAEMON */

void applctn_smtest_at(int64_t next_test)
{
  /* Set callback time or polling to test the state machine
     at time next_test */
  werr(WERR_DEBUG2, "next_smtest %lu -> %lu", next_smtest, next_test);
  next_smtest = next_test ;
}

static void
applctn_interactive_check(void)
{
#ifdef ENABLE_INTERACTIVE
  if (arg_interactive)
  {
    /* We are in interactive mode. Poll for user input. */

    static struct timeval select_time = {0, 10} ;
    static fd_set fds, ers ;

    /* Build fd set with just stdin */
    FD_ZERO(&fds) ;
    FD_SET(0, &fds) ;

    FD_ZERO(&ers) ;
    FD_SET(0, &ers) ;

    select(1, &fds, NULL, &ers, &select_time) ;

    /* Error condition? */
    if (FD_ISSET(0, &ers))
      werr(0, "stdin error") ;

    /* Data waiting? */

    if (FD_ISSET(0, &fds))
    {
      char buffer[128] ;
      int count ;

      /* Read the input line */

      count = read(0, buffer, 127) ;

      switch (count)
      {
        case -1:
          /* Error */
          break ;

        case 0:
          /* EOF */
          printf(_("Exiting interactive mode.\n")) ;
          arg_interactive = 0 ;
          break ;

        default:
        {
          char *nl ;
          buffer[count] = '\0' ;
          nl = strchr(buffer, '\n') ;
          if (nl) *nl = '\0' ;
          //printf("\"%s\"\n", buffer) ;
          interactive_parse(buffer) ;
        }
        break ;
      }
    }
  }
#endif // ifdef ENABLE_INTERACTIVE
}

/* Null event handler tests state machine */

static int null_event(int event_code, void *event, void *id_block,void
*handle)
{
  static int lockout = 0 ;
  int smtest_imminent ;
  int smret;
  int64_t nowtick = 0;

  if (lockout) {
    werr(WERR_DEBUG1, "null_event locked out") ;
    return 0 ;
  }

  /* Check for interactive input */
  applctn_interactive_check() ;

  nowtick = csGettick();

  /* Get ready to do the real work */
  if (next_smtest > nowtick) {
    werr(WERR_DEBUG1, "null_event not time to call smtest, %lld < %lld", nowtick, next_smtest) ;
    return 0 ;
  }

  lockout = 1 ; /* Do not re-enter this handler */
  applctn_smtest_at(SMTEST_AT_ASAP) ; /* Default to fast polling */

  werr(WERR_DEBUG2, "null_event: %s - next_smtest %li, now %li", prog_states[prog_state], next_smtest, csGettick()) ;

  do
  {
    smret = state_machine();

    if (!applctn_quit)
      smtest_imminent = ((next_smtest < SMTEST_AT_LATER) ||
    	  (next_smtest - csGettick() < DWELL_TIME)) ;
    else
      smtest_imminent = 0;
    /*msDelay(10) ;*/
#   ifndef NOGUI
    if (smtest_imminent /*|| (next_smtest == SMTEST_AT_ASAP)*/)
      while (gtk_events_pending())
        gtk_main_iteration() ;
#   endif
//    if (applctn_quit) return 0 ;

    #ifndef NOGUI
    setupd_poll() ; /* In case more wind vane IDs found */
    #endif

    /* Collect close states */
    server_poll_clients(&server_arne) ;
    server_poll_clients(&server_oww) ;
    server_poll_clients(&server_txt_tcp) ;
    server_poll_clients(&server_oww_un) ;
    server_poll_clients(&server_txt_un) ;

    /* Check for server connexion requests - but only for local data */
    if (setup_datasource == data_source_local)
    {
      server_poll_accept(&server_arne, NULL) ;
      server_poll_accept(&server_oww, oww_trx_build_announce) ;
      server_poll_accept(&server_txt_tcp, txtserve_build) ;
      server_poll_accept(&server_oww_un, oww_trx_build_announce) ;
      server_poll_accept(&server_txt_un, txtserve_build) ;
    }

    /* Poll http transfers */
    sendwx_poll() ;
  } while (/*(next_smtest < SMTEST_AT_LATER) ||*/ smtest_imminent) ;

  #ifndef NOGUI
  werr_poll() ; /* In case werr wants to close its window */
  #endif

  /* Check status of any child processes to clear zombies */
  process_poll() ;

  if (smret==0)
#ifndef NOGUI
	gtk_main_quit() ;
#else
	exit(0);
#endif

  lockout = 0 ; /* OK to re-enter */

  return 0 ; /* Let WIMP give somebody else this NULL event */
}

#ifndef NOGUI
static gint oww_alarm_handler(gpointer user_data)
{
  /* Just a wrapper around the RISC OS version */

  if (next_smtest > next_update) applctn_smtest_at(next_update) ;

  switch (next_smtest) {
    case -1:
    case 0:
      null_event(0, NULL, NULL, NULL) ;
      break ;

    default:
      if (csGettick() >= next_smtest)
        null_event(0, NULL, NULL, NULL) ;
      break ;
  }

  if (applctn_quit) {
	applctn_quit = 0;
	gtk_main_quit() ;
  }

  return 1 ;
}

static gint anim_alarm_handler(gpointer user_data)
{
  /* Just a wrapper around the RISC OS version */

  mainwin_check_animation(0) ;

  return 1 ;
}

/*
 * Event handler to be called when main window is closed
 */

void applctn_quit_now(GtkMenuItem     *menuitem, gpointer user_data)
{
  applctn_quit = 1 ;
  state_machine_quit(0);
}

void on_about_activate(GtkMenuItem     *menuitem, gpointer user_data)
{
  gtk_widget_show_all(proginfo_id) ;
}

void on_auxilliary_activate(GtkMenuItem     *menuitem, gpointer user_data)
{
  gtk_widget_show_all(aux_id) ;
}

void on_setup_activate(GtkMenuItem     *menuitem, gpointer user_data)
{
  gtk_widget_show_all(setup_id) ;
}

void on_map_activate(GtkMenuItem     *menuitem, gpointer user_data)
{
  sprintf(temp_string,
	setup_map_url,
	ws.latitude,
	ws.longitude) ;

  launchurl(temp_string) ;
}

void on_OwwHome_clicked(GtkButton       *button,
                        gpointer         user_data)
{
  launchurl("http://oww.sourceforge.net/") ;
}

void on_messages_activate(GtkMenuItem     *menuitem, gpointer user_data)
{
  gtk_widget_show_all(werr_id) ;
}

void on_devices_activate(GtkMenuItem     *menuitem, gpointer user_data)
{
  gtk_widget_show_all(devices_id) ;
}

void applctn_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                         const gchar *message,gpointer user_data)
{
  int werr_level = 0 ;

  if (log_level < G_LOG_LEVEL_WARNING) werr_level = 1 ;
  else if (log_level >= G_LOG_LEVEL_DEBUG) werr_level = WERR_DEBUGONLY ;

  werr(werr_level, "%s", message) ;
}

static void set_logging(void)
{
  /* Customize message logging to go to werr */

  g_log_set_handler(NULL, G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL
    | G_LOG_FLAG_RECURSION, applctn_log_handler, NULL) ;

}

#endif

#ifndef NOGUI
static GdkPixmap *icon_pm = NULL;
static GdkBitmap *icon_bm = NULL;

void add_icon(GdkWindow *w)
{
  if (icon_pm == NULL)
  {
    icon_pm = gdk_pixmap_create_from_xpm_d(w, &icon_bm, NULL, (gchar**)oww_xpm);
  }
  gdk_window_set_icon(w, NULL, icon_pm, icon_bm);
  if (mainwin_id)
    gdk_window_set_group(w, mainwin_id->window);
}

#endif

void applctn_polling_loop(void)
{
  /* Application polling loop */
  #ifndef NOGUI
  /* run the main loop */
  gtk_main();
  oww_main_quit() ;

  #else
    while (1)
    {
      null_event(0, NULL, NULL, NULL) ;
#     ifdef NOGUI
      /* Pause a bit between polls */

      msDelay(50) ;
#     endif
    }
  #endif
}

void applctn_pre_init(int *argc, char ***argv)
{
#ifndef NOGUI
	GError *err = NULL;

  #ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
  #endif

  gtk_set_locale ();

  /* initialize gtk */
  gtk_init(argc, argv) ;
  add_pixmap_directory (PACKAGE_PIXMAPS_DIR);

	builder = gtk_builder_new();

	if (gtk_builder_add_from_file(builder, PACKAGE_DATA_DIR "/oww.ui", &err)==0) {
		printf("GtkBuilder load filed. Error %d: %s\n", err->code, err->message);
	}

//  werr_id = create_Werr() ;
	werr_id = GTK_WIDGET(gtk_builder_get_object(builder, "Werr"));

  gtk_widget_realize(werr_id);
  add_icon(werr_id->window) ;
//  werr_clist = lookup_widget(werr_id, "WerrCList") ;
  werr_clist = GTK_WIDGET(gtk_builder_get_object(builder, "WerrCList"));

  set_logging() ;

  gdk_rgb_init();
#else

  // Set locale according to environment variables
# ifdef HAVE_LOCALE_H
  setlocale(LC_ALL, "");
# endif
#endif

  return ;
}


void applctn_init(int *argc, char ***argv)
{
# ifndef NOGUI

//  mainwin_id = create_Window() ;


	mainwin_id = GTK_WIDGET(gtk_builder_get_object(builder, "Window"));

	drawing_area = GTK_WIDGET(gtk_builder_get_object(builder, "drawingarea1"));

	assert(drawing_area != NULL);

//  drawing_area = lookup_widget(mainwin_id, "drawingarea1") ;
  gtk_widget_realize(drawing_area);

  assert(mainwin_id != NULL);
  gtk_widget_realize(mainwin_id);

  mainwin_init(mainwin_id) ;
  add_icon(mainwin_id->window) ;

//  aux_id = create_auxwin() ;
  aux_id = GTK_WIDGET(gtk_builder_get_object(builder, "auxwin"));
  assert(aux_id != NULL);
  gtk_widget_realize(aux_id);
  add_icon(aux_id->window) ;
  aux_list= GTK_WIDGET(gtk_builder_get_object(builder, "AuxList")) ;
  auxwin_g_init() ;

//  proginfo_id = create_Proginfo() ;
  proginfo_id = GTK_WIDGET(gtk_builder_get_object(builder, "Proginfo")) ;
  gtk_widget_realize(proginfo_id);
  add_icon(proginfo_id->window) ;
  gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "ProginfoVersion")), VERSION) ;

//  setup_id = create_Setup() ;
  setup_id = GTK_WIDGET(gtk_builder_get_object(builder, "Setup")) ;
  gtk_widget_realize(setup_id);
  add_icon(setup_id->window) ;

//  devices_id = create_Devices() ;
  devices_id = GTK_WIDGET(gtk_builder_get_object(builder, "Devices")) ;
  gtk_widget_realize(devices_id);
  add_icon(devices_id->window) ;

  gtk_builder_connect_signals (builder, NULL);

  setupd_window_created(0) ;
  setupd_devices_window_created(0) ;

  gtk_timeout_add (100, oww_alarm_handler, NULL) ;
  gtk_timeout_add (ANIMATION_INTERVAL, anim_alarm_handler, NULL) ;
  gtk_widget_show(mainwin_id) ;
# else
  printf("Setup read from: %s\n", choices_setupng_read) ;
  printf("Devices read from: %s\n", choices_devices_read) ;
# endif
}

#ifdef NOGUI
void
applctn_go_daemon(void)
{
  printf(_("Changing to daemon mode\n")) ;
  if (-1 == daemon (1 /* Don't chdir */, 0 /* Do close stdio */))
  {
    fprintf(stderr, _("daemon failed\n")) ;
          exit(1) ;
  }

  daemon_proc = 1 ;

  /* Now we are a daemon, werr should output to syslog */
  openlog("owwnogui", 0, LOG_CONS | LOG_USER) ;
}
#endif

int applctn_startup_finished(void)
{
  /* Has startup completed? */

  werr(WERR_DEBUG0,
    "applctn_startup_finished. Debug to \"%s\"",
    debug_file) ;

  werr(WERR_DEBUG0, "Interactive? %s", (arg_interactive) ? "Yes" : "No") ;

#ifdef NOGUI

    if (arg_daemon)
    {
      if (arg_interactive)
      {
        printf(_("Interactive mode conflicts with daemon mode.\nExiting.\n"))
;
        exit(0) ;
      }
      applctn_go_daemon() ;
    }

    return 1 ;
#else
    if (arg_daemon)
      werr(0, _("daemon mode for owwnogui only")) ;

    return 1 ;
#endif
}

void applctn_quick_poll(unsigned long int when)
{
  /* Poll the GUI and return no later than 'when' */

  /* 'when' ignored under Linux / GTK */
//  when = when ;

  #ifndef NOGUI
  /* Poll GUI once */
  if (gtk_events_pending()) gtk_main_iteration() ;
  #else
  /* Just get on with it */
  #endif

  return ;
}
