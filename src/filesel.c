/*
 * Auth: Eric Harlow
 * File: filesel.c
 */

#include <gtk/gtk.h>
#include <string.h>

typedef struct {

    void (*func)();
    GtkWidget *filesel;

} typFileSelectionData;

/*
 * --- Filename, remember it.
 */
static   char        *sFilename = NULL;

char *GetExistingFile ()
{
    return (sFilename);
}

/*
 * FileOk
 *
 * The "Ok" button has been clicked
 * Call the function (func) to do what is needed
 * to the file.
 */
void FileOk (GtkWidget *w, gpointer data)
{
    typFileSelectionData *localdata;
    GtkWidget *filew;
    int last ;

    localdata = (typFileSelectionData *) data;
    filew = localdata->filesel;

    /* --- Free old memory --- */
    if (sFilename) g_free (sFilename);

    /* --- Which file? --- */
    sFilename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (filew)));

    /* Remove any trailing '/' */
    last = strlen(sFilename)-1 ;
    if ((last >= 0) && (sFilename[last] == '/'))
      sFilename[last] = '\0' ;

    /* --- Call the function that does the work. --- */
    (*(localdata->func)) (sFilename);

    /* --- Close the dialog --- */
    gtk_widget_destroy (filew);
}

/*
 * destroy
 *
 * Function to handle the destroying of the dialog.  We must
 * release the focus that we grabbed.
 */
static void destroy (GtkWidget *widget, gpointer *data)
{
    /* --- Remove the focus. --- */
    gtk_grab_remove (widget);

    g_free (data);
}

/*
 * GetFilename
 *
 * Show a dialog with a title and if "Ok" is selected
 * call the function with the name of the file.
 */
void GetFilename (char *sTitle, char *sDefault, void (*callback) (char *),
  int showFiles)
{
    GtkWidget *filew = NULL;
    typFileSelectionData *data;

    /* --- Create a new file selection widget --- */
    filew = gtk_file_selection_new (sTitle);

    data = g_malloc (sizeof (typFileSelectionData));
    data->func = callback;
    data->filesel = filew;

    gtk_signal_connect (GTK_OBJECT (filew), "destroy",
            (GtkSignalFunc) destroy, data);

    /* --- Connect the "ok" button --- */
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
            "clicked", (GtkSignalFunc) FileOk, data);

    /* --- Connect the cancel button --- */
    gtk_signal_connect_object (
             GTK_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button),
             "clicked", (GtkSignalFunc) gtk_widget_destroy,
             (gpointer) filew);

    if (sDefault) {
      /* Want to change the default */
      if (sFilename) g_free(sFilename) ;
      sFilename = g_strdup (sDefault) ;
    }

    if (sFilename) {

        /* --- Set the default filename --- */
        gtk_file_selection_set_filename (GTK_FILE_SELECTION (filew),
                                         sFilename);
    }

    /* Sometimes we hide the list of files */

    if (!showFiles) gtk_widget_hide(GTK_FILE_SELECTION(filew)->file_list->parent) ;

    /* fileop buttons aren't working for some reason - hide them! */
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION (filew)) ;

    /* --- Show the dialog --- */
    gtk_widget_show (filew);

    /* --- Grab the focus. --- */
    gtk_grab_add (filew);
}
