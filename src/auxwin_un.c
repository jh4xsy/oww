/* auxwin_un.c */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdarg.h>

#include "intl.h"

extern GtkWidget *aux_list ;

static GtkListStore *aux_store = NULL  ;
static GtkTreeIter iter ;
static int iter_valid = 0 ;

enum AuxColumns {
  Name_Column = 0,
  Value_Column,
  Unit_Column,
  N_Columns
} ;

int
auxwin_g_init(void)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection ;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (aux_list)) ;
  gtk_tree_selection_set_mode(selection,GTK_SELECTION_NONE) ;
  
  aux_store = gtk_list_store_new (N_Columns /* Total number of columns */,
                              G_TYPE_STRING /* Name    */,
                              G_TYPE_STRING /* Value */,
                              G_TYPE_STRING /* Unit */
  ) ;  
  
  gtk_tree_view_set_model(GTK_TREE_VIEW(aux_list), GTK_TREE_MODEL(aux_store)) ;
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                   renderer,
                                                   "text", Name_Column,
                                                 NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (aux_list), column);  
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Value"),
                                                   renderer,
                                                   "text", Value_Column,
                                                   NULL);
                                                   
  gtk_tree_view_append_column (GTK_TREE_VIEW (aux_list), column);  

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Unit"),
                                                   renderer,
                                                   "text", Unit_Column,
                                                   NULL);
                                                   
  gtk_tree_view_append_column (GTK_TREE_VIEW (aux_list), column);  

  return 0 ;
}

int
auxwin_g_start(void)
{
  iter_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(aux_store), &iter) ;

  return 0 ;
}

int
auxwin_g_end(void)
{
  while (iter_valid)
  {
    iter_valid  = gtk_list_store_remove(aux_store, &iter) ;
  }
  
  return 0 ;
}

int
auxwin_g_write(char *name, float val, int prec, char *unit)
{
  char valstring[20], valform[20] /*, *row[3]*/ ;

  if (prec >= 0)
  {
    sprintf(valform, "%%.%df", prec) ;

    sprintf(valstring, valform, val) ;
  }
  else
  {
    sprintf(valstring, "%g", val) ;
  }

  if (!iter_valid)  // Need a new row?
  {
    gtk_list_store_append (aux_store, &iter) ;
    iter_valid = 1 ;
  }
  
  gtk_list_store_set(aux_store, &iter,
    Name_Column, name,
    Value_Column, valstring,
    Unit_Column, unit,
    -1) ;
  
  // Next row
  iter_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(aux_store), &iter)  ;
  
  return 0 ;
}
