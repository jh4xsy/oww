#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <time.h>
#include <string.h>
#include <assert.h>
#include <gtk/gtk.h>
//#include <glade/glade.h>
#include <stdio.h>
#include <stdlib.h>
#include "support.h"
/*#include "gtkTrace.h"*/

#include "events.h"

#include "werr.h"
/*#include "gadgut.h"*/
//#include "message.h"
#include "wstypes.h"
#include "log.h"
#include "progstate.h"
#include "setup.h"
#include "oww.h"
#include "mainwin.h"
#include "auxwin.h"
#include "globaldef.h"
#include "weather.h"
#include "devices.h"
#include "oww_trx.h"
#include "sendwx.h"
#include "convert.h"
#include "meteo.h"

extern GtkWidget *setup_id, *devices_id ;

extern GtkBuilder *builder;

static GtkWidget
  *setupd_logname,
  *setupd_startstop,
  *setupd_onoff,
  *setupd_getlogname,
  *setupd_anim,
  *setupd_resetrain,
  *setupd_usetrh,
  *gadget_d_known,
  *gadget_d_searchlist,
  *gadget_d_type,
//  *gadget_d_allocentry,
  *gadget_d_alloc,
  *gadget_d_devname,
  *gadget_d_calcombo,
//  *gadget_d_calentry,
  *gadget_d_calval,
  *setupd_owwremotehost,
  *setupd_arneremotehost,
  *setupd_ds_local,
  *setupd_ds_oww,
  *setupd_ds_arne,
  *setupd_ds_dallas,
//  *setupd_dallas_entry,
  *setupd_dallas_list,
  *setupd_werr_file_entry
;

static GtkListStore *device_store = NULL  ;
static GtkTreeIter device_iter ;
static GtkTreeSelection* device_selection = NULL ;

enum DeviceListColumns {
  Id_Column = 0,
  Device_Column,
  Family_Column,
  N_Columns
} ;

static char IdFamily[] = "Monospace" ;

extern time_t log_time ;
extern int wv_known ;
extern unsigned long int next_update ;

static int setupd_startstop_call_start = 1 ;
static int setupd_alloc_item = -1 ;
static unsigned char setupd_alloc_id[12] = "" ;
static int setupd_calnum = 0 ;
static int ignore_select_row_events = 0 ;



static void
device_selection_changed(GtkTreeSelection *selection, gpointer data) ;

void GetFilename (char *sTitle, char *sDefault, void (*callback) (char *),
  int showFiles);

static GtkWidget *setupd_register_var(GtkWidget *w, const gchar *name, gpointer var)
{
//	  GtkWidget *g = lookup_widget(w, name) ;
	  GtkWidget *g = GTK_WIDGET(gtk_builder_get_object(builder, name)) ;
  if (g != NULL)
  {
    g_object_set_data(G_OBJECT(g), "setup_var", var);
  }
  else
    werr(WERR_DEBUG0, "%s widget not found", name);

  return g ;
}

static GtkWidget *setupd_register_var_value(GtkWidget *w, const gchar *name, gpointer var, gint value)
{
  GtkWidget *g = setupd_register_var(w, name, var);
  if (g != NULL)
  {
    g_object_set_data(G_OBJECT(g), "setup_value", GINT_TO_POINTER(value));
  }
  return g ;
}

static gpointer setupd_get_var(GtkWidget *g)
{
  return (g_object_get_data(G_OBJECT(g), "setup_var"));
}

static int setupd_get_value(GtkWidget *g)
{
  return (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(g), "setup_value")));
}

static GtkWidget *setupd_register_toggle_button(GtkWidget *w, const gchar *name, int *var)
{
  GtkWidget *g = setupd_register_var(w, name, (gpointer) var);
  if ((g != NULL) && (*var != 0))
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g), 1);
  }
  //werr(WERR_DEBUG0, "setupd_register_toggle_button %s %d", name, *var);
  return g ;
}

#if 0
static GtkWidget *setupd_register_menu(GtkWidget *w, const gchar *name, int *var)
{
  GtkWidget *g = setupd_register_var(w, name, (gpointer) var);
  if ((g != NULL) && (*var != 0))
  {

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g), 1);
  }
  //werr(WERR_DEBUG0, "setupd_register_toggle_button %s %d", name, *var);
  return g ;
}
#endif

void
on_ToggleButton_toggled(GtkToggleButton *togglebutton,
                                            gpointer         user_data)
{
  int *var ;
  var = setupd_get_var(GTK_WIDGET(togglebutton));

  if (var != NULL)
    *var = gtk_toggle_button_get_active(togglebutton) ;
  else
    werr(WERR_DEBUG0, "Variable not registered");

  mainwin_update(0) ;
  auxwin_update(&ws);
}

static GtkWidget *setupd_register_combobox(GtkWidget *w, const gchar *name, int *var, int base)
{
  GtkWidget *g = setupd_register_var_value(w, name, (gpointer) var, base);
  if (g != NULL)
  {
    gtk_combo_box_set_active(GTK_COMBO_BOX(g), *var-base);
  }

  return g ;
}

void
on_ComboBox_changed(GtkComboBox *combobox, gpointer user_data)
{
  int *var, nn ;
  var = setupd_get_var(GTK_WIDGET(combobox));
  nn = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)) ;

  if (nn >= 0)
  {
    *var = nn + setupd_get_value(GTK_WIDGET(combobox)) ;
    mainwin_update(0) ;
    auxwin_update(&ws) ;
  }
}

static GtkWidget *setupd_register_radio_button(GtkWidget *w, const gchar *name, int *var, int value)
{
  GtkWidget *g = setupd_register_var_value(w, name, (gpointer) var, value);
  if ((g != NULL) && (*var == value))
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g), 1) ;
  }

  return g ;
}

void
on_RadioButton_toggled(GtkButton *button, gpointer user_data)
{
  if (GTK_TOGGLE_BUTTON(button)->active)
  {
    int *var, val ;
    var = setupd_get_var(GTK_WIDGET(button));
    val = setupd_get_value(GTK_WIDGET(button));
    werr(WERR_DEBUG1, "on_RadioButton_toggled -> %d", val);
    if (var != NULL)
    {
      *var = val ;
    }
  }
}

static GtkWidget *setupd_register_spin_button_int(GtkWidget *w, const gchar *name, int *var)
{
  GtkWidget *g = setupd_register_var(w, name, (gpointer) var);
  if (g != NULL)
  {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g), (gdouble) *var) ;

    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g), 0) ;
  }

  return g ;
}

static GtkWidget *setupd_register_spin_button_float(GtkWidget *w, const gchar *name, float *var, int digits)
{
  GtkWidget *g = setupd_register_var(w, name, (gpointer) var);
  if (g != NULL)
  {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g), (gdouble) *var) ;

    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g), digits) ;
  }

  return g ;
}

void
on_SpinButton_int_changed(GtkSpinButton    *spinner, gpointer user_data)
{
  int *var ;
  var = setupd_get_var(GTK_WIDGET(spinner));
  *var = gtk_spin_button_get_value_as_int (
      GTK_SPIN_BUTTON(spinner)) ;
  werr(WERR_DEBUG1, "on_SpinButton_int_changed -> %d", *var);
}

void
on_SpinButton_float_changed(GtkSpinButton    *spinner, gpointer user_data)
{
  float *var ;
  var = setupd_get_var(GTK_WIDGET(spinner));
  if (var == NULL) return ;
  *var = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinner)) ;
  werr(WERR_DEBUG1, "on_SpinButton_float_changed -> %g", (double) *var);
}

static GtkWidget *setupd_register_entry(GtkWidget *w, const gchar *name, char *var, int length)
{
  GtkWidget *g = setupd_register_var_value(w, name, (gpointer) var, length);
  if (g != NULL)
  {
    gtk_entry_set_text(GTK_ENTRY(g), var) ;
  }

  return g ;
}

void
on_Entry_activate(GtkEditable *editable, gpointer user_data)
{
  char *var ;
  int length ;
  var = setupd_get_var(GTK_WIDGET(editable));
  if (var==NULL) return ;

  length = setupd_get_value(GTK_WIDGET(editable));
  strncpy(var, gtk_entry_get_text(GTK_ENTRY(editable)), length) ;
  var[length] = '\0' ;
  werr(WERR_DEBUG1, "on_Entry_activate -> %s", var) ;
}



static int setupd_killanim(void)
{
  /* Force anim off */

  setup_anim = 0 ;

  gtk_widget_set_sensitive(setupd_anim, 0) ;
  mainwin_kill_anim = 0 ;

  return 1 ;
}

static int setupd_set_rain_state(void)
{
  static int rain_status = -1 ;

  if (devices_have_rain() || devices_have_any_gpc())
  {
    if (rain_status != 1)
    {
      rain_status = 1 ;
      gtk_widget_set_sensitive(setupd_resetrain, 1) ;
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetManual")), 1);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetDaily")), 1);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetWeekly")), 1);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetMonthly")), 1);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetHour")), 1);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetDay")), 1);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetDate")), 1);
    }
  }
  else
  {
    if (rain_status != 0)
    {
      rain_status = 0 ;
      gtk_widget_set_sensitive(setupd_resetrain, 0) ;
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetManual")), 0);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetDaily")), 0);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetWeekly")), 0);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetMonthly")), 0);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetHour")), 0);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetDay")), 0);
      gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "RainResetDate")), 0);
    }
  }

  return 1 ;
}

static int setupd_set_usetrh_state(void)
{
  int i ;
  static int usetrh_status = -1 ;

  /* Look for an humidity sensor */

  for (i=0; i<MAXHUMS; ++i)
    if (devices_have_hum(i))
    {
      if (usetrh_status != 1)
      {
        usetrh_status = 1 ;
        gtk_widget_set_sensitive(setupd_usetrh, 1) ;
        setupd_register_toggle_button(setup_id, "SetupUseTrh", &setup_report_Trh1);
      }
      return 1 ;
    }

  /* No humidity sensor found */
  setup_report_Trh1 = 0 ;
  if (usetrh_status != 0)
  {
    usetrh_status = 0 ;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setupd_usetrh),
                                setup_report_Trh1) ;
    gtk_widget_set_sensitive(setupd_usetrh, 0) ;
  }

  return 1 ;
}

void
on_SetupResetRain_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  /* Reset the weather stats on reset button click */

  weather_reset_stats() ;

  mainwin_update(0) ;
  auxwin_update(&ws) ;
}


void
on_SetupSave_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  setup_save_setup(0) ;
}

void
on_SetupSaveNG_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  setup_save_setup(1) ;
}


void
on_SetupUpdateInterval_changed         (GtkSpinButton    *spinner,
                                        gpointer         user_data)
{
  /* New update interval */
  next_update -= setup_interval * 100UL ;
  setup_interval = gtk_spin_button_get_value_as_int (
    GTK_SPIN_BUTTON(spinner)) ;
  next_update += setup_interval * 100UL ;
  if (setup_datasource == data_source_local)
    oww_trx_tx(&ws, OWW_TRX_MSG_UPDT) ;
}




static void setupd_get_log_name(char *name)
{
  strcpy(name, gtk_entry_get_text(GTK_ENTRY(setupd_logname))) ;
}

static void setupd_set_log_name(char *name)
{
  gtk_entry_set_text(GTK_ENTRY(setupd_logname), name) ;
}

static void setupd_set_debug_file_name(char *name)
{
  gtk_entry_set_text(GTK_ENTRY(setupd_werr_file_entry), name) ;
}

static int setupd_fill_in_http(void)
{
  setupd_register_entry(setup_id, "SetupHttpProxy", setup_httpproxy, 127) ;
  setupd_register_entry(setup_id, "SetupHttpDallas", setup_httpdallas, 127) ;
  setupd_register_entry(setup_id, "SetupHttpWund", setup_httpwund, 127) ;
  setupd_register_entry(setup_id, "SetupHttpWundUser", setup_httpwund_user, 39) ;
  setupd_register_entry(setup_id, "SetupHttpWundPass", setup_httpwund_pass, 39) ;
  setupd_register_entry(setup_id, "SetupHttpWow", setup_httpwow, 127) ;
  setupd_register_entry(setup_id, "SetupHttpWowId", setup_httpwow_id, 39) ;
  setupd_register_entry(setup_id, "SetupHttpWowPin", setup_httpwow_pin, 39) ;
  setupd_register_entry(setup_id, "SetupHttpHam", setup_httpham, 127) ;
  setupd_register_entry(setup_id, "SetupHttpHamUser", setup_httpham_user, 39) ;
  setupd_register_entry(setup_id, "SetupHttpHamPass", setup_httpham_pass, 39) ;
  setupd_register_entry(setup_id, "SetupCwopServer", setup_cwop_server, 127) ;
  setupd_register_entry(setup_id, "SetupCwopUser", setup_cwop_user, 39) ;
  setupd_register_entry(setup_id, "SetupCwopPass", setup_cwop_pass, 39) ;

  setupd_register_spin_button_int(setup_id, "SetupCwopPort", &setup_cwop_port) ;
  setupd_register_spin_button_float(setup_id, "SettupHttpLat", &setup_latitude, 6) ;
  setupd_register_spin_button_float(setup_id, "SetupHttpLong", &setup_longitude, 6) ;

  setupd_register_combobox(setup_id, "SetupHttpWundType", &setup_httpwund_type, 0);
  setupd_register_combobox(setup_id, "SetupHttpWowType", &setup_httpwow_type, 0);

  setupd_register_spin_button_int(setup_id, "SetupHttpInterval", &setup_http_interval) ;

  return 0 ;
}

static int setupd_show_log_state(void)
{
  /* Button will stop log if logging, or start log if not */

  setupd_startstop_call_start = (log_time == 0) ;

  gtk_label_set_text(GTK_LABEL(GTK_BIN(setupd_startstop)->child),
    (log_time != 0) ? _("Stop") : _("Start")) ;

  gtk_label_set_text(GTK_LABEL(setupd_onoff),
    (log_time != 0) ? _("On") : _("Off")) ;

  return 1 ;
}

static int setup_fill_in_logging(void)
{
  /* Fill in logname, start/stop and fade/unfade buttons */
  switch(setup_logtype)
  {
    case LOGTYPE_NONE:
      /* grey-out the icon and name field */
      gtk_widget_set_sensitive(setupd_getlogname, 0) ;
      gtk_widget_set_sensitive(setupd_logname, 0) ;
      gtk_widget_set_sensitive(setupd_startstop, 0) ;
      break ;

    case LOGTYPE_SINGL:
      /* Un-grey the icon and name field. Copy setup_logfile_name */
      gtk_widget_set_sensitive(setupd_getlogname, 1) ;
      gtk_widget_set_sensitive(setupd_logname, 1) ;
      gtk_widget_set_sensitive(setupd_startstop, 1) ;
      setupd_set_log_name(setup_logfile_name) ;
      break ;

    case LOGTYPE_DAILY:
      /* Un-grey the icon and name field. Copy setup_logdir_name */
      gtk_widget_set_sensitive(setupd_getlogname, 1) ;
      gtk_widget_set_sensitive(setupd_logname, 1) ;
      gtk_widget_set_sensitive(setupd_startstop, 1) ;
      setupd_set_log_name(setup_logdir_name) ;
      break ;

  }

  setupd_show_log_state() ;

  return 0 ;
}

static int setup_register_log_radio_buttons(void)
{
  setupd_register_radio_button(setup_id, "SetupNone",   &setup_logtype, LOGTYPE_NONE) ;
  setupd_register_radio_button(setup_id, "SetupSingle", &setup_logtype, LOGTYPE_SINGL) ;
  setupd_register_radio_button(setup_id, "SetupDaily",  &setup_logtype, LOGTYPE_DAILY) ;

  setupd_show_log_state() ;

  return 0 ;
}

static int setupd_start_log_event(int event_code, void *event,
  void *id_block, void * handle)
{
  /* The start/stop button was clicked as 'Start' */
  /* or 'return' from text entry whilst logging stopped */

  /* Get the log file/dir name from the text entry */
  switch(setup_logtype)
  {
    case LOGTYPE_NONE:
      break ;

    case LOGTYPE_SINGL:
      setupd_get_log_name(setup_logfile_name) ;
      break ;

    case LOGTYPE_DAILY:
      setupd_get_log_name(setup_logdir_name) ;
      break ;
  }

  /* Pass on to the function in log.c */
  if (!log_start_log_event(event_code, event, id_block, handle))
  {
    /* log_start_log_event failed */
    log_stop_log_event(event_code, event, id_block, handle) ;
  }

  setup_fill_in_logging() ;
  return 1 ; /* Event dealt with */
}

static int setupd_stop_log_event(int event_code, void *event,
  void *id_block, void *handle)
{
  /* The start/stop button was clicked as 'Stop' */
  /* or 'return' from text entry whilst logging */

  /* If the file name/dir entry has been changed, assume the user
     wants to start a different log, so restart */

  int sn = 0 ;
  char temp_name[MAXFILENAME] ;

  /* Compare the log file/dir name with the text entry */
  setupd_get_log_name(temp_name) ;
  switch(setup_logtype)
  {
    case LOGTYPE_NONE:
      break ;

    case LOGTYPE_SINGL:
      sn = strcmp(setup_logfile_name, temp_name) ;
      break ;

    case LOGTYPE_DAILY:
      sn = strcmp(setup_logdir_name, temp_name) ;
      break ;
  }

  log_stop_log_event(event_code, event, id_block, handle) ;

  if (sn) /* Different text entry - start a new log */
    return setupd_start_log_event(USER_START_LOG, NULL, NULL, NULL) ;

  setup_fill_in_logging() ; /* Indicate that logging has stopped */
  return 1 ;
}

void
on_LogRadioButton_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  if (GTK_TOGGLE_BUTTON(button)->active)
  {
    if (log_time != 0)
      setupd_stop_log_event(5, NULL, NULL, NULL) ;

    setup_logtype = setupd_get_value(GTK_WIDGET(button)) ;

    setup_fill_in_logging() ;
  }
}


void
on_SetupLogname_activate               (GtkEditable     *editable,
                                        gpointer         user_data)
{
  setupd_start_log_event(USER_START_LOG, NULL, NULL, NULL) ;
}

static void debug_file_picked(char *name)
{
  strncpy(debug_file, name, 255) ;
  setupd_set_debug_file_name(name) ;
}

void
on_WerrFileGet_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  GetFilename (_("Choose debug file"),
    debug_file, debug_file_picked, 1) ;
}

void
on_WerrFileEntry_activate               (GtkEditable     *editable,
                                        gpointer         user_data)
{
  GetFilename (_("Choose debug file"),
    debug_file, debug_file_picked, 1) ;
}

static void logname_picked(char *name)
{
  setupd_set_log_name(name) ;
  setupd_start_log_event(USER_START_LOG, NULL, NULL, NULL) ;
}

void
on_SetupGetLogname_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  switch(setup_logtype)
  {
    case LOGTYPE_NONE:
      break ;

    case LOGTYPE_SINGL:
      GetFilename ("Choose log file",
        setup_logfile_name, logname_picked, 1) ;
      break ;

    case LOGTYPE_DAILY:
      GetFilename (_("Choose log directory"),
        setup_logdir_name, logname_picked, 0) ;
      break ;
  }
}

void
on_SetupStartStop_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  if (setupd_startstop_call_start)
    setupd_start_log_event(USER_START_LOG, NULL, NULL, NULL) ;
  else
    setupd_stop_log_event(USER_STOP_LOG, NULL, NULL, NULL) ;
}


/* User wants to update data source settings */
/* = setupd_update_datasource in risc os version */

void
on_DatasourceUpdate_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  werr(WERR_DEBUG0, "on_DatasourceUpdate_clicked") ;

  /* Load string values */

  strncpy(setup_owwremote_host,
          gtk_entry_get_text(GTK_ENTRY(setupd_owwremotehost)),
          127) ;
  setup_owwremote_host[127] = '\0' ;

  strncpy(setup_arneremote_host,
          gtk_entry_get_text(GTK_ENTRY(setupd_arneremotehost)),
          127) ;
  setup_arneremote_host[127] = '\0' ;

  /* Update the data source */

  if (GTK_TOGGLE_BUTTON(setupd_ds_local)->active)
    state_change_datasource(data_source_local) ;
  else if (GTK_TOGGLE_BUTTON(setupd_ds_oww)->active)
    state_change_datasource(data_source_remote_oww) ;
  else if (GTK_TOGGLE_BUTTON(setupd_ds_arne)->active)
    state_change_datasource(data_source_arne) ;
  else if (GTK_TOGGLE_BUTTON(setupd_ds_dallas)->active)
    state_change_datasource(data_source_dallas) ;
}

///* Change to Dallas receive combo entry */
//void on_DallasRemoteEntry_changed(GtkEntry *entry,
//                                     gpointer user_data)
//{
//  char *text ;
//  int i ;
//
//  /* Fetch entry string */
//  text = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1) ;
//
//  /* Which station has this name? */
//  for (i=0; i<sendwx_stn_list_count; ++i)
//  {
//    if (0 == strcmp(sendwx_stn_list[i].name, text))
//    {
//      /* Found it */
//      strcpy(setup_dallasremote_stn, sendwx_stn_list[i].id) ;
//      werr(WERR_DEBUG0, "New station: %s", setup_dallasremote_stn) ;
//      break ;
//    }
//  }
//
//  g_free((gpointer) text) ;
//
//  return ;
//}

void on_DallasRemoteComboBox_changed(GtkComboBox *box, gpointer user_data)
{
  char *text ;
  int i ;
  GtkListStore *list;

  /* Fetch entry string */
  list = GTK_LIST_STORE(gtk_combo_box_get_model (box));
  i = gtk_combo_box_get_active(box);

  if (i>=0)
  {
	strcpy(setup_dallasremote_stn, sendwx_stn_list[i].id) ;
	werr(WERR_DEBUG0, "New station: %s", setup_dallasremote_stn) ;
  }
//  gtk_li
//  list->
//  text = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1) ;
//
//  /* Which station has this name? */
//  for (i=0; i<sendwx_stn_list_count; ++i)
//  {
//    if (0 == strcmp(sendwx_stn_list[i].name, text))
//    {
//      /* Found it */
//      strcpy(setup_dallasremote_stn, sendwx_stn_list[i].id) ;
//      werr(WERR_DEBUG0, "New station: %s", setup_dallasremote_stn) ;
//      break ;
//    }
//  }
//
//  g_free((gpointer) text) ;

  return ;
}

/* Build the stringset for the Dallas fetch menu */

static int
setupd_build_dallas_rx_list(void)
{
  int i, selected=0 ;
  GtkListStore *list;
  GtkTreeIter iter;

  if (sendwx_stn_list_count == 0) {
	werr(WERR_WARNING, "No Dallas stations listed") ;
	return -1;
  }

  list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(setupd_dallas_list)));
  gtk_list_store_clear(list);

  /* Build list entries from station strings */
  for (i=0; i<sendwx_stn_list_count; ++i) {
	gtk_list_store_append (list, &iter);
	gtk_list_store_set (list, &iter, 0, sendwx_stn_list[i].name, -1);
	if (0==strcmp(sendwx_dallasremote_name,sendwx_stn_list[i].name)) {
	  // Matching name
	  selected = i;
	}
  }

  /* Unfade combobox */
  gtk_widget_set_sensitive(setupd_dallas_list, 1) ;

  strcpy(setup_dallasremote_stn, sendwx_stn_list[selected].id) ;
  strcpy(sendwx_dallasremote_name, sendwx_stn_list[selected].name) ;

  gtk_combo_box_set_active(GTK_COMBO_BOX(setupd_dallas_list), selected);

  return 0 ; /* Ok */
}

static int
setupd_parse_and_build_dallas_list(char *body, void *test_data)
{
  return ((sendwx_parse_dallas_list(body, test_data)) &&
          (0 == setupd_build_dallas_rx_list())) ;
}

/* User wants to update the list of stations on the Dallas server */

void
on_DallasRemoteList_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  sendwx_recv_list(setupd_parse_and_build_dallas_list) ;
}

static int setupd_fill_in_datasource(void)
{
  werr(WERR_DEBUG0, "setupd_fill_in_datasource") ;

  /* Write string values */

  gtk_entry_set_text(GTK_ENTRY(setupd_owwremotehost),
    setup_owwremote_host) ;

  gtk_entry_set_text(GTK_ENTRY(setupd_arneremotehost),
    setup_arneremote_host) ;

  /* Set the port numbers */

  setupd_register_spin_button_int(setup_id, "OwwRemotePort",  &setup_owwremote_port) ;
  setupd_register_spin_button_int(setup_id, "ArneRemotePort", &setup_arneremote_port) ;


  /* Update the data source */

  switch (setup_datasource)
  {
    case data_source_local

      :
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setupd_ds_local), 1) ;
      break ;

    case data_source_remote_oww:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setupd_ds_oww), 1) ;
      break ;

    case data_source_arne:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setupd_ds_arne), 1) ;
      break ;

    case data_source_dallas:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setupd_ds_dallas), 1) ;
      break ;
  }

  setupd_register_combobox(setup_id, "combobox_browser", &setup_url_launcher, 0);

  return 1 ; /* Event dealt with */
}


void
on_DevicesSearch_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  state_new(state_learn) ;
}


void
on_DevicesSave_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  setup_save_devices() ;
}

void
on_DevicesWipeVanes_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  devices_wipe_vanes();
}

static void add_device_to_list(char *id_string, char *alloc_string)
{
  /* Add item to scrolling list */

  gtk_list_store_append (device_store, &device_iter) ;

  gtk_list_store_set(device_store, &device_iter,
    Id_Column, id_string,
    Family_Column, IdFamily,
    Device_Column, alloc_string,
    -1
  ) ;
}

static int setupd_update_devices(int) ;

/* Rebuild the scrolllist */

void setupd_build_scrolllist(void)
{
  setupd_update_devices(0) ;
  setupd_set_rain_state(); /* Rain gauge availability may have changed */
  setupd_set_usetrh_state(); /* Trh availability may have changed */
}

void on_DevicesAllocComboBox_changed(GtkComboBox *box, gpointer user_data)
{
  char new_alloc_name[40] ;
//  GtkEntry     *entry ;
//  GtkListStore *list;
  int sel;
  GtkTreeIter iter;
  GtkTreeModel *list_store;
  gchar *str_data;
  GtkTreePath *path;

//  entry = GTK_ENTRY(editable) ;

  list_store = GTK_TREE_MODEL(gtk_combo_box_get_model(box));
  sel = gtk_combo_box_get_active(box);

  if (sel >= 0)
  {
	path = gtk_tree_path_new_from_indices(sel, -1);
	if (gtk_tree_model_get_iter(list_store, &iter, path))
	{
	  // Valid iter

	  gtk_tree_model_get (list_store, &iter, 0, &str_data, -1);

	  strcpy(new_alloc_name, str_data) ;

	  if (setupd_alloc_item == -1)
	  {
		werr(0, "setupd_devices_alloc: bad setupd_alloc_item") ;
		return ;
	  }

	  if (0 == strcmp(new_alloc_name,
		  (devices_search_list[setupd_alloc_item].alloc >= 0) ?
			  devices_list[devices_search_list[setupd_alloc_item].alloc].menu_entry :
			  _("--None--")))
	  {
		werr(WERR_DEBUG1, "Already allocated to \"%s\"", new_alloc_name);
		return ;
	  }

	  g_signal_handlers_block_by_func(
		  GTK_OBJECT(box),
		  on_DevicesAllocComboBox_changed,
		  NULL) ;

	  werr(WERR_DEBUG1, "Reallocate search item %d to \"%s\"",setupd_alloc_item,new_alloc_name);

	  /* Reallocate to this entry */
	  devices_queue_realloc(setupd_alloc_item,
		  new_alloc_name,
		  1, setupd_build_scrolllist) ;

	  g_signal_handlers_unblock_by_func(
		  GTK_OBJECT(box),
		  on_DevicesAllocComboBox_changed,
		  NULL) ;
	}
  }

  return ;
}
///* User wants to change device allocation */
//void
//on_DevicesAllocEntry_changed           (GtkEditable     *editable,
//                                        gpointer         user_data)
//{
//  char new_alloc_name[40] ;
//  GtkEntry     *entry ;
//
//  entry = GTK_ENTRY(editable) ;
//
//  strcpy(new_alloc_name, gtk_entry_get_text(GTK_ENTRY(entry))) ;
//
//  if (setupd_alloc_item == -1)
//  {
//    werr(0, "setupd_devices_alloc: bad setupd_alloc_item") ;
//    return ;
//  }
//
//  if (0 == strcmp(new_alloc_name,
//      (devices_search_list[setupd_alloc_item].alloc >= 0) ?
//      devices_list[devices_search_list[setupd_alloc_item].alloc].menu_entry :
//      _("--None--")))
//  {
//    werr(WERR_DEBUG1, "Already allocated to \"%s\"", new_alloc_name);
//    return ;
//  }
//
//  g_signal_handlers_block_by_func(
//    GTK_OBJECT(editable),
//    on_DevicesAllocEntry_changed,
//    NULL) ;
//
//  werr(WERR_DEBUG1, "Reallocate search item %d to \"%s\"",setupd_alloc_item,new_alloc_name);
//
//  /* Reallocate to this entry */
//  devices_queue_realloc(setupd_alloc_item,
//    new_alloc_name,
//    1, setupd_build_scrolllist) ;
//
//  g_signal_handlers_unblock_by_func(
//    GTK_OBJECT(editable),
//    on_DevicesAllocEntry_changed,
//    NULL) ;
//
//  return ;
//}

void append_to_list(gpointer data, gpointer user_data) {
  GtkListStore *list;
  GtkTreeIter iter;

  list = GTK_LIST_STORE(user_data);
  gtk_list_store_append(list, &iter);

  gtk_list_store_set(list, &iter, 0, data, -1);
//  printf("Append %s\n", data);
//  printf("Iter valid? %s\n",
//	  (gtk_list_store_iter_is_valid(list, &iter))? "Yes":"No");
}

void build_liststore(GtkListStore *list, GList *strings) {
  assert(list!=NULL);

  GtkTreeModel *tree = GTK_TREE_MODEL(list);
  int cols = gtk_tree_model_get_n_columns(tree);
  int i;
//  printf("Model of %d cols\n", cols);
//  for (i=0; i<cols; ++i) {
//	printf("Col %d type %d\n", i, gtk_tree_model_get_column_type(tree, i));
//  }
  gtk_list_store_clear(list);
  if (strings != NULL)
	g_list_foreach(strings, append_to_list, list);
}

/* Set the entries in the allocation stringset */
static void setupd_build_alloc_menu(int search_entry)
{
  int i, devnum, which_selected = -1, list_index = 0, list_length ;
  unsigned char family ;

  GList *strings = NULL ;

  if (search_entry < 0)
  {
    family = NULL_FAMILY ;
    devnum = -1 ;
    strings = g_list_append(strings, "") ;
  }
  else
  {
    family = devices_search_list[search_entry].id[0] ;
    devnum = devices_search_list[search_entry].alloc ;


    /* Build stringset according to devices_list entries that
       want this family */

    for (i=0; i<DEVICES_TOTAL; ++i)
    {
      /* Right family? */
      //if (devices_list[i].family != family) continue ;
      if (!devices_match_family(&devices_list[i], family)) continue ;
      strings = g_list_append(strings, devices_list[i].menu_entry) ;
      if (devices_search_list[search_entry].alloc == i)
        which_selected = list_index ;
      ++list_index ;
    }

    strings = g_list_append(strings,
      _("--None--")) ;
  }

  list_length = g_list_length(strings);

  /* Turn off events on allocentry */

  g_signal_handlers_block_by_func(
    GTK_OBJECT(gadget_d_alloc),
    on_DevicesAllocComboBox_changed,
    NULL) ;

//  gtk_combo_set_popdown_strings(GTK_COMBO(gadget_d_alloc), strings) ;
  build_liststore(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(gadget_d_alloc))), strings);

  g_list_free(strings) ;

  if (search_entry >= 0)
  {
    gtk_widget_set_sensitive(gadget_d_alloc, 1) ;
    gtk_combo_box_set_active(GTK_COMBO_BOX(gadget_d_alloc),
    	(which_selected==-1) ? list_length-1 : which_selected);
  }
  else
  {
    gtk_widget_set_sensitive(gadget_d_alloc, 0) ;
//    gtk_entry_set_text(GTK_ENTRY(gadget_d_allocentry), "") ;
  }

  gtk_entry_set_text(GTK_ENTRY(gadget_d_type), devices_get_type(family)) ;

  gtk_widget_set_sensitive(gadget_d_type, (search_entry >= 0)) ;

  /* Turn events back on */

  g_signal_handlers_unblock_by_func(
    GTK_OBJECT(gadget_d_alloc),
    on_DevicesAllocComboBox_changed,
    NULL) ;
}

/* Set cal stringset and value */
static void setupd_fillin_cal(int search_entry)
{
  int devtype = devtype_unspec, devnum = -1 ;

  /* Sanity checks */
  if (search_entry >= 0)
  {
    devnum = devices_search_list[search_entry].alloc ;
  }

  if ((devnum >= 0) && (devices_list[devnum].ncalib > 0))
  {
    devtype = devices_list[devnum].devtype ;

	/* Generate numeric value */
    if ((devices_list[devnum].ncalib > setupd_calnum) &&
        (setupd_calnum >= 0))
      sprintf(temp_string, "%f",
        devices_list[devnum].calib[setupd_calnum]) ;
    else
      temp_string[0] = '\0' ;

	/* Fill it in */
    gtk_entry_set_text(GTK_ENTRY(gadget_d_calval), temp_string) ;
    gtk_widget_set_sensitive(gadget_d_calval, 1) ;
  }
  else
  {
	/* No cal value - blank it and fade */
    devtype = devtype_unspec ;
    setupd_calnum = 0 ;
    gtk_entry_set_text(GTK_ENTRY(gadget_d_calval), "") ;
    gtk_widget_set_sensitive(gadget_d_calval, 0) ;
  }
}

/* Change to cal combo entry */
void on_DevicesCalComboBox_changed(GtkComboBox *box, gpointer user_data)
{
  GtkTreeModel *list_store;
  int sel;
  GtkTreePath *path;
  GtkTreeIter iter;
//  gchar *str_data;

  char *text ;
  int i=0, devnum = -1 ;

  list_store = GTK_TREE_MODEL(gtk_combo_box_get_model(box));
  sel = gtk_combo_box_get_active(box);
  if (sel == -1)
  {
//	printf("No active item\n");
	return;
  }
  path = gtk_tree_path_new_from_indices(sel, -1);

  if (gtk_tree_model_get_iter (list_store, &iter, path))
  {
	// iter is valid

	gtk_tree_model_get (list_store, &iter, 0, &text, -1);

	/* Check for sane alloc item */
	if (setupd_alloc_item < 0) return ;

	/* Which device is allocated? */
	devnum = devices_search_list[setupd_alloc_item].alloc ;

	if (devnum < 0) return ;

	if (devices_list[devnum].ncalib < 1) return ;

	/* Fetch entry string */
	//	text = gtk_editable_get_chars(GTK_EDITABLE(gadget_d_calentry), 0, -1) ;
	//	text = "";

	gtk_combo_box_get_active_iter(box, &iter);

	/* Which cal has this name? */
	for (i=0; i<devices_list[devnum].ncalib; ++i)
	{
	  if (0 == strcmp(
		  devices_cal_name(devices_list[devnum].devtype, i),
		  text))
	  {
		/* Found it */
		setupd_calnum = i ;
		break ;
	  }
	}

	g_free((gpointer) text) ;

	/* Fill in the numeric value */
	setupd_fillin_cal(setupd_alloc_item) ;
  }
//  else
//  {
//	printf ("Invalid iter\n");
//  }

	return ;
}

/* Change to cal combo entry */
void on_DevicesCalVal_changed(GtkEntry *entry,
                              gpointer user_data)
{
	char *text ;
	int devnum ;

	/* Check for sane alloc item */
	if (setupd_alloc_item < 0) return ;

  /* Which device is allocated? */
  devnum = devices_search_list[setupd_alloc_item].alloc ;

	if (devnum < 0) return ;

	if (setupd_calnum >= devices_list[devnum].ncalib) return ;

	/* Get the entry string */
	text = gtk_editable_get_chars(GTK_EDITABLE(gadget_d_calval), 0, -1) ;

	/* Convert and store */
	devices_list[devnum].calib[setupd_calnum] = atof(text) ;

	g_free((gpointer) text) ;

    return ;
}


static void setupd_build_cal_menu(int search_entry)
{
  GList *strings = NULL ;
  int i=0, devnum = -1, nd ;
  static int devtype = devtype_unspec ;

  if ((search_entry >= 0) &&
      (devices_list[devices_search_list[search_entry].alloc].ncalib > 0))
  {
    char *calstring ;

    /* Which device is allocated? */
    devnum = devices_search_list[search_entry].alloc ;

    /*werr(0, "setupd_build_cal_menu for %s",
      (devnum >= 0) ? devices_list[devnum].menu_entry : "Nothing") ;*/

    /* What is this (new) device type? */
    nd = (devnum >= 0) ? devices_list[devnum].devtype : devtype_unspec ;

    /* Is it the same as for the last build? Might need to reset calnum */
    if (nd != devtype) setupd_calnum = 0 ;

    /* Ok - work with this device type and remember for next build */
    devtype = nd ;

    /* Build stringset from cal strings */
    while ((calstring = devices_cal_name(devtype, i++)), calstring)
      strings = g_list_append(strings, calstring) ;

    /* Turn off events on gadget_d_calcombo */

    g_signal_handlers_block_by_func(
      GTK_OBJECT(gadget_d_calcombo),
      on_DevicesAllocComboBox_changed,
      NULL) ;

	/* Set strings */
//    gtk_combo_set_popdown_strings(GTK_COMBO(gadget_d_calcombo), strings) ;
    build_liststore(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(gadget_d_calcombo))), strings);
    g_list_free(strings) ;

    /* Turn events back on */

    g_signal_handlers_unblock_by_func(
      GTK_OBJECT(gadget_d_calcombo),
      on_DevicesAllocComboBox_changed,
      NULL) ;

    /* Unfade combo */
    gtk_widget_set_sensitive(gadget_d_calcombo, 1) ;
//    printf("Set calname %s\n", devices_cal_name(devtype, setupd_calnum)) ;
    calstring = devices_cal_name(devtype, setupd_calnum);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gadget_d_calcombo), 0);
//    gtk_entry_set_text(GTK_ENTRY(gadget_d_calentry),
//      (calstring!=NULL)?calstring:"") ;
  }
  else
  {
    devtype = devtype_unspec ;
    setupd_calnum = 0 ;

	/* Fade combo */
    gtk_widget_set_sensitive(gadget_d_calcombo, 0) ;
//    gtk_entry_set_text(GTK_ENTRY(gadget_d_calentry), "") ;
  }

  /* Fill in numeric value */
  setupd_fillin_cal(search_entry) ;
}

static int
setupd_init_device_list(void)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  device_store = gtk_list_store_new (N_Columns /* Total number of columns */,
                              G_TYPE_STRING /* Id    */,
                              G_TYPE_STRING /* Device */,
                               G_TYPE_STRING /* Font family */
 ) ;

  gtk_tree_view_set_model(
    GTK_TREE_VIEW(gadget_d_searchlist),
    GTK_TREE_MODEL(device_store)
  ) ;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("ID"),
    renderer,
    "text", Id_Column,
    "family", Family_Column,
    NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (gadget_d_searchlist), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Device"),
                                                   renderer,
                                                   "text", Device_Column,
                                                   NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (gadget_d_searchlist), column);

  device_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gadget_d_searchlist)) ;
  g_assert(device_selection != NULL) ;

  gtk_tree_selection_set_mode (device_selection, GTK_SELECTION_BROWSE);
  g_signal_connect (G_OBJECT (device_selection), "changed",
                  G_CALLBACK (device_selection_changed),
                  NULL);

  return 0 ;
}

static void setupd_select_device_row(int row)
{
  //if (!device_selection) return ;

  GtkTreePath *path ;

  path = gtk_tree_path_new_from_indices (row, -1) ;

  gtk_tree_selection_select_path (device_selection, path) ;
  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gadget_d_searchlist), path, NULL, TRUE, 0.5, 0);

  gtk_tree_path_free(path) ;
}

static int setupd_update_devices(int do_menu)
{
  int i ;

  /* Ensure list store exists, &c, or clear old list */

  if (device_store == NULL)
    setupd_init_device_list() ;
  else
    gtk_list_store_clear(GTK_LIST_STORE(device_store)) ;

  if (!devices_search_list)
  {
    devices_clear_search_list() ;
    devices_build_search_list_from_devices(1) ;
  }

  /* Assume that the selection is out-of-date */
  setupd_alloc_item = -1 ;

  i = 0 ;

  while (i<devices_known)
  {
    /*werr(WERR_DEBUG1, "Adding device %d to list", i) ;*/
    setup_id_to_string(temp_string, devices_search_list[i].id);
    add_device_to_list(temp_string,
      (devices_search_list[i].alloc < 0) ?
      "" : devices_list[devices_search_list[i].alloc].menu_entry
    ) ;

    /* Is this the ID last selected? */
    if (devices_compare_ids(devices_search_list[i].id, setupd_alloc_id))
    {
      GtkTreePath *path ;

      /* We have found the old selection */
      setupd_alloc_item = i ;

      /* Select row */

      path = gtk_tree_path_new_from_indices (i, -1) ;

      ignore_select_row_events = 1 ;
      gtk_tree_view_row_activated(
        GTK_TREE_VIEW(gadget_d_searchlist),
        path,
        NULL) ;

      gtk_tree_path_free (path) ;

      setupd_select_device_row(i) ;
      ignore_select_row_events = 0 ;

    }
    ++i ;
  }

  werr(WERR_DEBUG1, "Finished building list");

  /* No need to build menu - it will get done by the selection event handler */

  /* Make sure stringset is up-to-date */

  setupd_build_cal_menu(setupd_alloc_item) ;

  return 1 ;
}

/* Update devices window if more wind vane IDs found */
int setupd_poll(void)
{
  static int saved_logging = 0 ;
  int logging ;

  /* Update the devices window if necessary */

  static int l = 0 ;

  switch (prog_state) {
    case state_startup:
    case state_ready:
      /*werr(WERR_DEBUG0, "setupd_poll - state_startup");*/
      setupd_update_devices(1) ;
      /* no break */
    case state_remote_done:
      setupd_set_rain_state() ; /* Reset rain gauge available? */
      setupd_set_usetrh_state() ; /* Trh available? */
      break ;
  }

  if (wv_known != l) {
    sprintf(temp_string, "%d", wv_known) ;
    gtk_entry_set_text(GTK_ENTRY(gadget_d_known), temp_string) ;
    l = wv_known ;
    setupd_update_devices(1) ;
    setupd_set_rain_state() ; /* Reset rain gauge available? */
    setupd_set_usetrh_state() ; /* Trh available? */
}

  if (mainwin_kill_anim) setupd_killanim() ;

  /* Check for change of logging state */
  logging = (log_time != 0) ;
  if (logging != saved_logging)
  {
    saved_logging = logging ;
    setupd_show_log_state() ;
  }

  return 1 ;
}

/* Load setup and set inital setup window entries */
void setupd_window_created(int id /* window ID*/)
{
  /* Setup window was just created */

  /* widget pointer is held globally */

 setupd_anim = setupd_register_toggle_button(setup_id, "SetupAnim", &setup_anim);

  setupd_werr_file_entry =  GTK_WIDGET(gtk_builder_get_object(builder, "WerrFileEntry")) ;
  setupd_set_debug_file_name(debug_file) ;

  setupd_owwremotehost =  GTK_WIDGET(gtk_builder_get_object(builder, "OwwRemoteHost")) ;
  setupd_arneremotehost =  GTK_WIDGET(gtk_builder_get_object(builder, "ArneRemoteHost")) ;
  setupd_ds_local =  GTK_WIDGET(gtk_builder_get_object(builder, "DatasourceLocal")) ;
  setupd_ds_oww =  GTK_WIDGET(gtk_builder_get_object(builder, "DatasourceOww")) ;
  setupd_ds_arne =  GTK_WIDGET(gtk_builder_get_object(builder, "DatasourceArne")) ;
  setupd_ds_dallas =  GTK_WIDGET(gtk_builder_get_object(builder, "DatasourceDallas")) ;
//  setupd_dallas_entry =  GTK_WIDGET(gtk_builder_get_object(builder, "DallasRemoteEntry")) ;
  setupd_dallas_list =  GTK_WIDGET(gtk_builder_get_object(builder, "DallasRemoteComboBox")) ;

  setupd_logname = GTK_WIDGET(gtk_builder_get_object(builder, "SetupLogname")) ;
  setupd_startstop = GTK_WIDGET(gtk_builder_get_object(builder, "SetupStartStop")) ;
  setupd_onoff = GTK_WIDGET(gtk_builder_get_object(builder, "SetupOnOff")) ;
  setupd_getlogname = GTK_WIDGET(gtk_builder_get_object(builder, "SetupGetLogname")) ;
  setupd_resetrain = GTK_WIDGET(gtk_builder_get_object(builder, "SetupResetRain")) ;

  setupd_usetrh = GTK_WIDGET(gtk_builder_get_object(builder, "SetupUseTrh")) ;

  setupd_register_spin_button_int(setup_id, "SetupLogInterval", &setup_log_interval) ;

  setupd_register_spin_button_int(setup_id, "RainResetHour", &setup_rainreset_hour) ;
  setupd_register_spin_button_int(setup_id, "RainResetDate", &setup_rainreset_date) ;
  setupd_set_rain_state() ; /* Reset rain gauge available? */
  setupd_register_radio_button(setup_id, "RainResetManual", &setup_rainreset, rain_reset_manual) ;
  setupd_register_radio_button(setup_id, "RainResetDaily", &setup_rainreset, rain_reset_daily) ;
  setupd_register_radio_button(setup_id, "RainResetWeekly", &setup_rainreset, rain_reset_weekly) ;
  setupd_register_radio_button(setup_id, "RainResetMonthly", &setup_rainreset, rain_reset_monthly) ;

  setupd_set_usetrh_state() ; /* Trh available? */

  setup_fill_in_logging() ;
  setupd_fill_in_http() ;
  setup_register_log_radio_buttons() ;
  setupd_fill_in_datasource() ;

  // Register variables for general-purpose callback functions

  setupd_register_toggle_button(setup_id, "SetupLogIntervalSnap", &setup_log_snap);
  setupd_register_toggle_button(setup_id, "SetupHttpSnap", &setup_http_interval_snap);
  setupd_register_toggle_button(setup_id, "SetupHttpProxyOn", &setup_httpuseproxy);
  setupd_register_toggle_button(setup_id, "SetupHttpDallasOn", &setup_httpdallas_enable);
  //setupd_register_toggle_button(setup_id, "SetupHttpWundOn", &setup_httpwund_enable);
  setupd_register_toggle_button(setup_id, "SetupHttpHamOn", &setup_httpham_enable);
  setupd_register_toggle_button(setup_id, "SetupCwopOn", &setup_cwop_enable);

  setupd_register_combobox(setup_id, "combobox_speed", &setup_unit_speed, CONVERT_KPH);
  setupd_register_combobox(setup_id, "combobox_T", &setup_f, CONVERT_CELSIUS);
  setupd_register_combobox(setup_id, "combobox_rain", &setup_mm, CONVERT_INCHES);
  setupd_register_combobox(setup_id, "combobox_bp", &setup_unit_bp, CONVERT_MILLIBAR);
  setupd_register_combobox(setup_id, "combobox_windchill", &setup_wctype, METEO_WINDCHILL_STEADMAN);
  setupd_register_combobox(setup_id, "combobox_heatindex", &setup_hitype, METEO_HI_HEATIND);

  setupd_register_combobox(setup_id, "combobox_werrdisplevel", &werr_message_level, 0);
  setupd_register_combobox(setup_id, "combobox_werrwritelevel", &debug_level, 0);

  setupd_register_combobox(setup_id, "RainResetDay", &setup_rainreset_day, 0);

}

void
on_DevicesDevname_activate               (GtkEditable     *editable,
                                        gpointer         user_data)
{
  /* setup_driver is 32 chars long */

  strncpy(setup_driver, gtk_entry_get_text(GTK_ENTRY(editable)), 32) ;
  setup_driver[32] = '\0' ;
  state_new(state_newcom) ;
}

static void
devname_picked(char *name)
{
  /* Callback for GetFilename file picker dialogue function */

  /* Fill in updated entry */
  gtk_entry_set_text(GTK_ENTRY(gadget_d_devname), name) ;

  /* Update setup value and change state */
  strncpy(setup_driver, name, 32) ;
  setup_driver[32] = '\0' ;
  state_new(state_newcom) ;
}

void
on_DevicesGetdev_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  GetFilename (_("Choose serial device"),
    setup_driver, devname_picked, 1) ;
}


static void
device_selection_changed(GtkTreeSelection *selection, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    GtkTreePath* path ;
    int *indices, row, devnum = -1 ;

    path = gtk_tree_model_get_path(model, &iter) ;
    indices = gtk_tree_path_get_indices(path) ;
    row = indices[0] ;
    gtk_tree_path_free(path) ;

    if (ignore_select_row_events) return ;

    setupd_alloc_item = row ;

    if (devices_search_list==NULL) return;
//    printf("Click on row %d, known = %d\n", row, devices_known);
    if (devices_known<=row) return;

    devices_copy_ids(setupd_alloc_id, devices_search_list[row].id) ;

    devnum = devices_search_list[row].alloc ;

    if (-1 == devnum)
    {
      werr(WERR_DEBUG1, "Search item %d not allocated", row) ;
    }
    else
    {
      werr(WERR_DEBUG1, "Clicked on \"%s\"", devices_list[devnum].menu_entry);
    }

    setupd_build_alloc_menu(row) ;
    setupd_build_cal_menu(row) ;
  }
}

/* Load devices and set inital devices window entries */
int setupd_devices_window_created(int id /* window ID*/)
{
  int i ;

  /* Check for valid vane IDs */
  wv_known = 0 ;

  for (i=0; i<8; ++i)
    if (devices_list[devices_wv0+i].id[0] == DIR_FAMILY) ++wv_known ;

  /* Check that we at least have a thermometer or RH sensor */
  if (!devices_have_something())
  {
    gtk_widget_show(devices_id) ;
  }

  gadget_d_known = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesVanesKnown")) ;

  setupd_register_toggle_button(devices_id, "DevicesReverse", &setup_wv_reverse) ;

  gadget_d_searchlist = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesSearchList")) ;
  gadget_d_type       = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesType")) ;
//  gadget_d_allocentry = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesAllocEntry")) ;
  gadget_d_alloc      = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesAllocComboBox")) ;
  gadget_d_devname    = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesDevname")) ;
  gadget_d_calcombo   = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesCalComboBox")) ;
//  gadget_d_calentry   = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesCalComboEntry")) ;
  gadget_d_calval     = GTK_WIDGET(gtk_builder_get_object(builder, "DevicesCalVal")) ;

  sprintf(temp_string, "%d", wv_known) ;
  gtk_entry_set_text(GTK_ENTRY(gadget_d_known), temp_string) ;


  setupd_register_spin_button_int(devices_id, "DevicesVaneOffset", &setup_wv_offet) ;

  setupd_set_rain_state() ; /* Reset rain gauge available? */
  setupd_set_usetrh_state() ; /* Trh available? */

  gtk_entry_set_text(GTK_ENTRY(gadget_d_devname), setup_driver) ;

  return 1 ;
}
