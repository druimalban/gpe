/*
* This file is part of GPE-Memo
*
* Copyright (C) 2007 Alberto García Hierro
*	<skyhusker@rm-fr.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
*/

#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#include <gtk/gtkmain.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktoolitem.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktoolbutton.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkseparatortoolitem.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkfilechooser.h>
#include <gtk/gtkfilechooserbutton.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkentry.h>

#include <gpe/errorbox.h>
#include <gpe/question.h>


#include "interface.h"
#include "sound.h"
#include "settings.h"

#define _(x) x

enum {
    COL_NAME,
    COL_LENGTH,
    COL_NUMCOLS
};

typedef enum {
    ACTION_PLAY,
    ACTION_RECORD
} action_t;

static void
toolbar_button_new (GtkWidget *toolbar, const gchar *stock_id, 
    void (*callback) (GtkWidget *, gpointer), gpointer data)
{
    GtkToolItem *item;

    item = gtk_tool_button_new_from_stock (stock_id);
    g_signal_connect (G_OBJECT (item), "clicked",
            G_CALLBACK (callback), data);
    GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);

    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
}

static void
toolbar_vseparator_new (GtkWidget *toolbar)
{
    GtkToolItem *item;

    item = gtk_separator_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
}

static GtkWidget *
dialog_cancel_accept_new (GtkWidget *parent, const gchar *title)
{
    GtkWidget *dialog;

    dialog = gtk_dialog_new_with_buttons (title, 
            GTK_WINDOW (parent),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
            GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
            GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
            NULL);

    return dialog;
}

static GtkWidget *
dialog_audio_new (action_t action, GtkWidget *parent, const gchar *title)
{
    GtkWidget *dialog;
    GtkWidget *pbar;
    GtkWidget *label;
    
    switch (action) {
        case ACTION_RECORD:
            dialog = dialog_cancel_accept_new (parent, title);
            break;
        case ACTION_PLAY:
            dialog = gtk_dialog_new_with_buttons (title, 
                    GTK_WINDOW (parent),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
                    GTK_STOCK_CLOSE, GTK_RESPONSE_NONE,
                    NULL);
            break;
        default:
            return NULL;
    }

    pbar = gtk_progress_bar_new ();
    gtk_widget_show (pbar);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), pbar,
            FALSE, FALSE, 0);
    
    label = gtk_label_new (NULL);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
            FALSE, FALSE, 0);

    g_object_set_data (G_OBJECT (dialog), "label", label);
    g_object_set_data (G_OBJECT (dialog), "pbar", pbar);

    return dialog;
}

static gboolean
progress_bar_pulse_cb (gpointer data)
{
    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (data));

    return TRUE;
}

static gboolean
label_update_time_cb (gpointer data)
{
    GtkWidget *label = data;
    GTimer *timer;
    gint seconds;
    gchar str[256];

    timer = g_object_get_data (G_OBJECT (label), "timer");

    seconds = (gint) g_timer_elapsed (timer, NULL);

    g_snprintf (str, sizeof (str), "%d:%02d", seconds / 60, seconds % 60);

    gtk_label_set_text (GTK_LABEL (label), str);

    return TRUE;
}

static gchar *
filename_cook (void)
{
    struct tm ltime;
    time_t now;
    gchar *memodir;
    gchar *retval;
    gchar buf[256];

    now = time (NULL);
    localtime_r (&now, &ltime);

    strftime (buf, sizeof (buf), "%Y-%m-%d_%H:%M:%S" ,&ltime);

    memodir = settings_get_memo_dir ();
    retval = g_strdup_printf ("%s/%s%s", memodir, buf, ".gsm");

    g_free (memodir);
    return retval;
}

static gboolean
start_recording (const gchar *filename)
{
    int infd;
    int outfd;

    infd = record_fd ();

    if (infd < 0) {
        gpe_error_box_fmt (_("Error opening sound device for recording: %s"),
                strerror (errno));
        return FALSE;
    }

    outfd = file_fd (filename, O_WRONLY);
    
    if (outfd < 0) {
        gpe_error_box_fmt (_("Error opening file %s for writing: %s"),
                filename, strerror (errno));
        return FALSE;
    }
        
    sound_record (infd, outfd);

    return TRUE;
}

static gboolean
start_playing (const gchar *filename, Interface *in)
{
    int infd;
    int outfd;

    outfd = play_fd ();

    if (outfd < 0) {
        gpe_error_box_fmt (_("Error opening sound device for playing: %s"),
                strerror (errno));
        return FALSE;
    }

    infd = file_fd (filename, O_RDONLY);
    
    if (infd < 0) {
        gpe_error_box_fmt (_("Error opening file %s for reading: %s"),
                filename, strerror (errno));
        return FALSE;
    }

    sound_play (infd, outfd);

    return TRUE;
}

static void
populate_store (GtkListStore *store)
{
    gchar *memodir;
    gchar *length;
    gchar *path;
    const gchar *name;
    GDir *dir;
    GtkTreeIter iter;
    gint seconds;

    gtk_list_store_clear (store);

    memodir = settings_get_memo_dir ();

    dir = g_dir_open (memodir, 0, NULL);


    if (!dir) {
        g_free (memodir);
        return;
    }

    while ((name = g_dir_read_name (dir)) != NULL) {
        if (!g_str_has_suffix (name, ".gsm")) {
            continue;
        }
        path = g_build_filename (memodir, name, NULL);
        seconds = sound_get_length (path);
        length = g_strdup_printf ("%d:%02d", seconds / 60, seconds % 60);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 
                COL_NAME, name,
                COL_LENGTH, length,
                -1);

        g_free (path);
        g_free (length);
    }
        
    g_free (memodir);
    g_dir_close (dir);
}


static void
button_new_clicked_cb (GtkWidget *w, gpointer data)
{
    interface_record ((Interface*) data, NULL);
}

static void
tree_view_get_select_name (Interface *in, gchar **filename)
{
    GtkTreeSelection *select;
    GtkTreeIter iter;
    GtkTreeModel *model;

    select = gtk_tree_view_get_selection (GTK_TREE_VIEW (in->treeview));

    if (gtk_tree_selection_get_selected (select, &model, &iter)) {
        gtk_tree_model_get (model, &iter, COL_NAME, filename, -1);
    } else {
        *filename = NULL;
    }
}

static gboolean
pbar_update_cb (gpointer data)
{
    GtkWidget *label;
    GtkWidget *dialog;
    GTimer *timer;
    gchar *txt;
    GtkWidget *pbar = data;
    gfloat elapsed;
    gint length;
    gfloat fraction;

    timer = g_object_get_data (G_OBJECT (pbar), "timer");
    label = g_object_get_data (G_OBJECT (pbar), "label");
    length = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pbar), "length"));

    elapsed = g_timer_elapsed (timer, NULL);

    if (elapsed - length > 0.2) {
        dialog = g_object_get_data (G_OBJECT (pbar), "dialog");
        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_NONE);

        return FALSE;
    }

    fraction = elapsed / length;

    if (fraction > 1) {
        fraction = 1;
    }

    printf ("%f\n", fraction);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), fraction);

    txt = g_strdup_printf ("%d:%02d/%d:%02d", ((gint) elapsed) / 60,
            ((gint) elapsed) % 60,
            length / 60, length % 60);

    gtk_label_set_text (GTK_LABEL (label), txt);
    g_free (txt);

    return TRUE;
}

static void
button_play_clicked_cb (GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *pbar;
    GtkWidget *label;
    GTimer *timer;
    gchar *filename;
    gchar *memodir;
    gchar *path;
    Interface *in = data;
    guint source_id;
    gint length;
    gboolean playing;

    tree_view_get_select_name (in, &filename);

    if (!filename) {
        return;
    }

    memodir = settings_get_memo_dir ();

    path = g_build_filename (memodir, filename, NULL);
    
    dialog = dialog_audio_new (ACTION_PLAY, in->main_window, _("Playing"));
    label = g_object_get_data (G_OBJECT (dialog), "label");
    pbar = g_object_get_data (G_OBJECT (dialog), "pbar");

    timer = g_timer_new ();
    length = sound_get_length (path);

    g_object_set_data (G_OBJECT (pbar), "label", label);
    g_object_set_data (G_OBJECT (pbar), "timer", timer);
    g_object_set_data (G_OBJECT (pbar), "dialog", dialog);
    g_object_set_data (G_OBJECT (pbar), "length", GINT_TO_POINTER (length));

    source_id = g_timeout_add (250, pbar_update_cb, pbar);

    playing = start_playing (path, in);

    if (playing) {
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_hide (dialog);
        sound_stop ();
    }

    g_source_remove (source_id);

    gtk_widget_destroy (dialog);
    
    g_free (memodir);
    g_free (filename);
    g_free (path);
}
    

static void
entry_activated_cb (GtkEntry *entry, gpointer data)
{
     gtk_dialog_response (GTK_DIALOG (data), GTK_RESPONSE_ACCEPT);
}

static void
button_edit_clicked_cb (GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *label;
    GtkWidget *entry;
    gchar *txt;
    gchar *memodir;
    gchar *filename;
    gchar *path;
    gchar *newpath;
    const gchar *newfilename;
    Interface *in = data;
    gint response;
    
    tree_view_get_select_name (in, &filename);
    if (NULL == filename) {
        return;
    }
    
    dialog = dialog_cancel_accept_new (in->main_window, _("Rename"));

    txt = g_strdup_printf (_("Enter new name for %s:"), filename);
    label = gtk_label_new (txt);
    gtk_widget_show (label);

    g_free (txt);

    entry = gtk_entry_new ();
    g_signal_connect (G_OBJECT (entry), "activate", 
            G_CALLBACK (entry_activated_cb), dialog);
    gtk_widget_show (entry);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
        FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry,
        FALSE, FALSE, 0);


    response = gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_hide (dialog);

    if (GTK_RESPONSE_ACCEPT == response) {
        newfilename = gtk_entry_get_text (GTK_ENTRY (entry));
        memodir = settings_get_memo_dir ();
        path = g_build_filename (memodir, filename, NULL);
        if (g_str_has_suffix (newfilename, ".gsm")) {
            newpath = g_build_filename (memodir, newfilename, NULL);
        } else {
            newpath = g_strdup_printf ("%s/%s.gsm", memodir, newfilename);
        }

        g_rename (path, newpath);

        g_free (memodir);
        g_free (path);
        g_free (newpath);
        g_free (filename);
    }

    gtk_widget_destroy (dialog);

    populate_store (in->store);
}
    
    
static void
button_delete_clicked_cb (GtkWidget *w, gpointer data)
{
    Interface *in = data;
    gchar *memodir;
    gchar *filename;
    gchar *path;
    gchar *prompt;
    gint response;

    tree_view_get_select_name (in, &filename);

    if (NULL == filename)
        return;

    prompt = g_strdup_printf (_("Do you really want to delete %s?"), filename);
    response = gpe_question_ask_yn (prompt);
    g_free (prompt);

    if (response) {
        memodir = settings_get_memo_dir ();
        path = g_build_filename (memodir, filename, NULL);
        g_unlink (path);
        
        g_free (memodir);
        g_free (path);

        populate_store (in->store);
    }

    g_free (filename);
}

static void
button_settingsure_clicked_cb (GtkWidget *w, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *frame;
    GtkWidget *dirchooser;
    gchar *path;
    Interface *in = data;
    gint response;

    dialog = dialog_cancel_accept_new (in->main_window, _("Options"));

    dirchooser = gtk_file_chooser_button_new (_("Select a directory"),
             GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    path = settings_get_memo_dir ();

    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dirchooser),
            path);

    g_free (path);

    gtk_widget_show (dirchooser);

    frame = gtk_frame_new (_("Memo directory"));
    gtk_widget_show (frame);
    gtk_container_add (GTK_CONTAINER (frame), dirchooser);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
            FALSE, FALSE, 0);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_hide (dialog);

    if (GTK_RESPONSE_ACCEPT == response) {
        path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dirchooser));
        settings_set_memo_dir (path);
        g_free (path);
    }
    
    gtk_widget_destroy (dialog);

    populate_store (in->store);
}

static void
button_exit_clicked_cb (GtkWidget *w, gpointer data)
{
    gtk_main_quit ();
}

void
interface_init (Interface *in)
{
    GtkCellRenderer *renderer;

    in->main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_signal_connect (G_OBJECT (in->main_window), "delete-event",
        gtk_main_quit, NULL);

    gtk_window_set_title (GTK_WINDOW (in->main_window), _("Voice memo"));
    gtk_widget_set_size_request (in->main_window, 240, 320);

    in->vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (in->main_window), in->vbox);

    in->toolbar = gtk_toolbar_new ();
    gtk_toolbar_set_orientation (GTK_TOOLBAR (in->toolbar), 
            GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style (GTK_TOOLBAR (in->toolbar), GTK_TOOLBAR_ICONS);
    GTK_WIDGET_UNSET_FLAGS (in->toolbar, GTK_CAN_FOCUS);

    gtk_box_pack_start (GTK_BOX (in->vbox), in->toolbar, FALSE, FALSE, 0);

    toolbar_button_new (in->toolbar, GTK_STOCK_NEW,
            button_new_clicked_cb, in);
    toolbar_vseparator_new (in->toolbar);
    toolbar_button_new (in->toolbar, GTK_STOCK_MEDIA_PLAY,
            button_play_clicked_cb, in);
    toolbar_button_new (in->toolbar, GTK_STOCK_EDIT,
            button_edit_clicked_cb, in);
    toolbar_button_new (in->toolbar, GTK_STOCK_DELETE,
            button_delete_clicked_cb, in);
    toolbar_vseparator_new (in->toolbar);
    toolbar_button_new (in->toolbar, GTK_STOCK_PREFERENCES,
            button_settingsure_clicked_cb, in);
    toolbar_button_new (in->toolbar, GTK_STOCK_QUIT,
            button_exit_clicked_cb, in);

    in->store = gtk_list_store_new (COL_NUMCOLS, G_TYPE_STRING, G_TYPE_STRING);
    in->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (in->store));
    g_object_unref (in->store);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (in->treeview), TRUE);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (in->treeview), TRUE);
    
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (in->treeview),
            -1, _( "File" ), renderer, "text", COL_NAME, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (in->treeview),
            -1, _( "Length" ), renderer, "text", COL_LENGTH, NULL);

    gtk_box_pack_start (GTK_BOX (in->vbox), in->treeview, TRUE, TRUE, 0);

    populate_store (in->store);

    gtk_widget_show_all (in->main_window);
}

void
interface_record (Interface *in, const gchar *filename)
{
    GtkWidget *parent;
    GtkWidget *dialog;
    GtkWidget *pbar;
    GtkWidget *label;
    GTimer *timer;
    gchar *fn;
    guint source_id_pbar;
    guint source_id_label;
    gint response;
    gboolean recording;

    if (in) {
        parent = in->main_window;
    } else {
        parent = NULL;
    }

    dialog = dialog_audio_new (ACTION_RECORD, parent, _("Recording"));
    label = g_object_get_data (G_OBJECT (dialog), "label");
    pbar = g_object_get_data (G_OBJECT (dialog), "pbar");

    timer = g_timer_new ();
    g_object_set_data (G_OBJECT (label), "timer", timer);
    
    source_id_pbar = g_timeout_add (100, progress_bar_pulse_cb, pbar);
    source_id_label = g_timeout_add (500, label_update_time_cb, label);

    if (filename) {
        fn = g_strdup (filename);
    } else {
        fn = filename_cook ();
    }

    recording = start_recording (fn);
    
    /* Reset the timer */
    g_timer_start (timer);

    if (recording) {
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        sound_stop();
    } else {
        response = GTK_RESPONSE_CANCEL;
    }

    g_source_remove (source_id_pbar);
    g_source_remove (source_id_label);

    gtk_widget_destroy (dialog);

    if (GTK_RESPONSE_ACCEPT != response) {
        g_unlink (filename);
    } else {
        if (in)
            populate_store (in->store);
    }
    
    g_free (fn);
}

