/*
 * werr.c
 * For !OWW project
 * One-wire weather
 * Dr. Simon J. Melhuish
 * August - December 1999
 * Free for non-comercial use
 * Dallas parts subject to their copyright and conditions
 *
 */

/* Graphical (windowed) error reporting - Linux version */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include "werr.h"
#include "progstate.h"
#include "setup.h"
#include "oww.h"
#include "intl.h"
#include "ownet.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef NOGUI
# ifdef HAVE_SYSLOG_H
#  include <syslog.h>
# endif
#endif

char task_name[] = "Oww" ;


char debug_file[256] = "" ;

//_kernel_oserror *e;
int debug_level = werr_level_debug_0 ;
int werr_message_level = werr_level_errors_and_warnings ;

extern int daemon_proc ; /* Running as daemon? */

#ifndef NOGUI
#include <gtk/gtk.h>

#define WERR_MAX_ROWS 50

enum WerrColumns {
  Time_Column = 0,
  Message_Column,
  Colour_Column,
  N_Columns
} ;

unsigned long int csGettick(void) ;

static int werr_fields_used = 0 ;
static unsigned long int werr_autoclose_time ;
static int werr_autoclose_pending = 0 ;
static int werr_last_critical = -1 ;
extern GtkWidget *werr_id, *werr_clist ;

GdkColor *c0 = NULL, *c1 = NULL, *c2 = NULL, *c3 = NULL ;

static GtkListStore *werr_store = NULL  ;
static GtkTreeIter iter ;

static void
werr_list_scroll(void)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(werr_clist));
  GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(werr_store), &iter);
  gtk_tree_selection_select_path(selection, path);
  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(werr_clist), path, NULL, TRUE, 0.5, 0);
  gtk_tree_path_free(path);
}

int
werr_init_list(void)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  //g_assert((client->data_list != NULL)) ;

  werr_store = gtk_list_store_new (N_Columns,       /* Total number of columns */
                              G_TYPE_STRING  /* Time    */,
                              G_TYPE_STRING   /* Message */,
                              GDK_TYPE_COLOR /* Colour */
  ) ;

  gtk_tree_view_set_model(GTK_TREE_VIEW(werr_clist), GTK_TREE_MODEL(werr_store)) ;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Time",
                                                   renderer,
                                                   "text", Time_Column,
                                                   "foreground-gdk", Colour_Column,
                                                 NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (werr_clist), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Message",
                                                   renderer,
                                                   "text", Message_Column,
                                                   "foreground-gdk", Colour_Column,
                                                   NULL);

  /*column = gtk_tree_view_column_new() ;

  gtk_tree_view_column_set_attributes(column, renderer,
                                      "text", Message_Column,
                                      "foreground-gdk", Colour_Column,
                                      NULL);*/

  gtk_tree_view_append_column (GTK_TREE_VIEW (werr_clist), column);

  return 0 ;
}

static GdkColor *werr_get_colour(int level)
{
  if (level < WERR_WARNING)
  {
    if (level & WERR_FATAL)
    {
      if (!c0) {
        c0 = (GdkColor *) g_malloc (sizeof (GdkColor));
        c0->red = 0xffff ;
        c0->green = 0 ;
        c0->blue = 0 ;
        gdk_color_alloc (gdk_colormap_get_system (), c0);
      }
      return c0 ;
    }
    else
    {
      if (!c3) {
        c3 = (GdkColor *) g_malloc (sizeof (GdkColor));
        c3->red = 0xffff ;
        c3->green = 0xc000 ;
        c3->blue = 0 ;
        gdk_color_alloc (gdk_colormap_get_system (), c3);
      }
      return c3 ;
    }
  }

  if (level < WERR_DEBUGONLY)
  {
    if (!c1) {
      c1 = (GdkColor *) g_malloc (sizeof (GdkColor));
      c1->red = 0 ;
      c1->green = 0 ;
      c1->blue = 0 ;
      gdk_color_alloc (gdk_colormap_get_system (), c1);
    }
    return c1 ;
  }

  if (!c2) {
    c2 = (GdkColor *) g_malloc (sizeof (GdkColor));
    c2->red = 0 ;
    c2->green = 0 ;
    c2->blue = 0xffff ;
    gdk_color_alloc (gdk_colormap_get_system (), c2);
  }
  return c2 ;
}

#endif

static void werr_zap_nl(char *string, int limit)
{
  int i ;

  for (i=0; i<limit; ++i)
  {
    switch (string[i])
    {
      case '\n':
      case '\r':
        string[i] = ' ' ;
        break ;

      case '\0':
        return ;
    }
  }
}

//static int werr_max_flags(void)
//{
//  if (debug_file[0])
//    return (WERR_DEBUGONLY + debug_level * WERR_DBLEVBIT0) ;
//
//  return (WERR_DEBUGONLY - 1) ;
//}

static int werr_check_level(int flags, int level)
{
  switch(level)
  {
    case werr_level_none:
      return 0 ;

    case werr_level_fatal_only:
      return (flags & WERR_FATAL) ;

    case werr_level_errors:
      return (flags < WERR_WARNING) ;

    case werr_level_errors_and_warnings:
      return (flags <= WERR_WARNING) ;

    case werr_level_debug_0:
      return (flags <= WERR_DEBUG0) ;

    case werr_level_debug_1:
      return (flags <= WERR_DEBUG1) ;

    case werr_level_debug_2:
      return (flags <= WERR_DEBUG2) ;

    case werr_level_debug_3:
      return (flags <= WERR_DEBUG3) ;
  }

  return 1 ;
}

static int
werr_will_display(int flags)
{
  /* A fatal error must always call werr, so that it will die */
  return
    ((flags & WERR_FATAL) || werr_check_level(flags, werr_message_level)) ;
}

static int
werr_will_write(int flags)
{
  return ((debug_file[0] != '\0') && werr_check_level(flags, debug_level)) ;
}

int
werr_will_output(int flags)
{
  return (werr_will_display(flags) || werr_will_write(flags)) ;
}

void werr(int flags, char* format, ...)
{
  #define WTS 256
   //_kernel_oserror er;
   va_list va;
   char title[128], timestamp[64] ;
#  ifdef HAVE_VSNPRINTF
   char message[512] ;
#  else
   char message[2048] ;
#  endif // ifdef HAVE_VSNPRINTF
   time_t t ;
   //int max_flags ;

#  ifndef NOGUI
   //char *row[2] ;
   //row[0] = timestamp ;
   //row[1] = message ;
#  else
   int syslog_level ;
#  endif

//   max_flags = werr_max_flags() ;
//
//   if (flags > max_flags) return ;

   if (!werr_will_output(flags)) return ;

   //er.errnum = 0;
   va_start(va, format);
#  ifdef HAVE_VSNPRINTF
   vsnprintf(message, 512, format, va);
#  else
   vsprintf(message, format, va);
#  endif // ifdef HAVE_VSNPRINTF
   va_end(va);

   //strncpy(er.errmess, message, 251) ;

   time(&t) ;
   strftime(timestamp, 63, "%d, %H:%M:%S", localtime(&t)) ;
   sprintf(title, _("Warning from %s [%s]"), task_name, timestamp) ;

//   if (debug_file[0]) {
   if (werr_will_write(flags))
   {
     FILE *df ;
     df = fopen(debug_file, "a") ;
     if (df) {
       fprintf(df, "%s [%s (%s)] %s\n", timestamp,
         prog_states[prog_state], prog_states[prog_state_old], message) ;
       fclose(df) ;
     }
   }

  /* Check reporting level */
  if (werr_check_level(flags, werr_message_level))
  {
#   ifndef NOGUI
    GdkColor *col = NULL ;
#   endif

    werr_zap_nl(message, sizeof(message)) ;

#    ifdef NOGUI
     /* Report to stderr if not daemon */
     if (!daemon_proc)
       fprintf(stderr, "%s\n", message) ;
#    ifdef HAVE_SYSLOG_H
     else
     {
       if (flags < WERR_WARNING)
         syslog_level = LOG_ERR ;
       else if (flags < WERR_DEBUG0)
         syslog_level = LOG_WARNING ;
       else
         syslog_level = LOG_DEBUG ;

       syslog(syslog_level, message) ;
     }
#    endif
#    else  // i.e. ifndef NOGUI
     // Initialize tree_view widget if need be
     if (werr_store == NULL) werr_init_list() ;

     /* Is this a critcal message? */
     if (flags < WERR_AUTOCLOSE) {
       werr_last_critical = WERR_MAX_ROWS ;
       werr_autoclose_pending = 0 ;
     } else {
       --werr_last_critical ;
       if (werr_last_critical < 1) /* No critcal messages left? */
       {
         /* We may close the werr window shortly */
         werr_autoclose_pending = 1 ;
         werr_autoclose_time = csGettick() + 100UL * WERR_AUTOCLOSE_DELAY ;
       } else {
         werr_autoclose_pending = 0 ;
       }
     }

     /* Pause updates */
     //--gtk_clist_freeze(GTK_CLIST(werr_clist)) ;

     col = werr_get_colour(flags) ;
    // Generate new row and get its iterator
    gtk_list_store_append (werr_store, &iter);
    gtk_list_store_set(werr_store, &iter,
      Time_Column, timestamp,
      Message_Column, message,
      Colour_Column, col,
      -1) ;
     werr_list_scroll() ;

    ++werr_fields_used ;

     /* Acquire an iterator */
     //--werr_fields_used = gtk_clist_append(GTK_CLIST(werr_clist), row) ;

     //col = werr_get_colour(flags) ;
     //--if (col != NULL)
       //--gtk_clist_set_foreground(GTK_CLIST(werr_clist), werr_fields_used, col) ;

     /* Append new message - test new row count */
     if (werr_fields_used >= WERR_MAX_ROWS)
     {
       /* Too many rows - remove first row */
       gtk_tree_model_get_iter_first(GTK_TREE_MODEL(werr_store), &iter) ;
       gtk_list_store_remove(werr_store, &iter) ;

       //--gtk_clist_remove(GTK_CLIST(werr_clist), 0) ;
       --werr_fields_used ;
     }

     /* OK to update now */
     //--gtk_clist_thaw(GTK_CLIST(werr_clist)) ;

     //--gtk_clist_moveto(GTK_CLIST(werr_clist),
     //--  werr_fields_used, 0, 1.0, 0.0) ;

     /* Make sure it's visible - user may have hidden it */
     if (setup_popup) /* but only if popup is enabled */
       gtk_widget_show(werr_id) ;
#    endif
  }

   if (flags & WERR_FATAL) state_machine_quit() ;
}

//int
//werr_will_output(int flags)
//{
//  return (flags <= werr_max_flags()) ;
//}

#ifndef NOGUI
void on_WerrClear_clicked(GtkButton *button, gpointer user_data)
{
  //--gtk_clist_clear(GTK_CLIST(werr_clist)) ;
  if (werr_store)
    gtk_list_store_clear(GTK_LIST_STORE(werr_store)) ;
  werr_last_critical = 0 ;
  werr_autoclose_pending = 1 ;
  werr_autoclose_time = csGettick() + 100UL * WERR_AUTOCLOSE_DELAY ;
}

void werr_poll(void)
{
  /* Just check if we should auto close the window */

  if (!werr_autoclose_pending) return ;

  if (csGettick() >= werr_autoclose_time)
  {
    /* Time to close the window */

    if (setup_popup) /* Only close if popup enabled */
      gtk_widget_hide(werr_id) ;
    werr_autoclose_pending = 0 ;
  }
  return ;
}
#endif

static int
werr_owerr_level(int en)
{
  switch (en)
  {
    //case OWERROR_RESET_FAILED:
    //case OWERROR_NO_DEVICES_ON_NET:
    case OWERROR_ACCESS_FAILED:
    case OWERROR_DS2480_NOT_DETECTED:
    case OWERROR_DS2480_WRONG_BAUD:
    case OWERROR_DS2480_BAD_RESPONSE:
    case OWERROR_OPENCOM_FAILED:
    case OWERROR_GET_SYSTEM_RESOURCE_FAILED:
    case OWERROR_SYSTEM_RESOURCE_INIT_FAILED:
    case OWERROR_HANDLE_NOT_AVAIL:
    case OWERROR_HANDLE_NOT_EXIST:
    case OWERROR_OW_SHORTED:
    case OWERROR_ADAPTER_ERROR:
    case OWERROR_PORTNUM_ERROR:
    case OWERROR_LEVEL_FAILED:
      return WERR_WARNING ;
  }

  return WERR_DEBUG0 ;
}

void
werr_report_owerr(void)
{
  while(owHasErrors())
  {
    int en = owGetErrorNum() ;
    werr(werr_owerr_level(en), "1-wire Error: %s", owGetErrorMsg(en)) ;
  }
}
