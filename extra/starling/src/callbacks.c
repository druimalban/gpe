/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 * Copyright (C) 2007 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <string.h>

#include <gtk/gtk.h>

#include "errorbox.h"
#include "starling.h"
#include "playlist.h"
#include "interface.h"
#include "config.h"
#include "utils.h"

static void
fs_cancel_cb (GtkWidget *w, Starling *st)
{
    gtk_widget_destroy (st->fs);
}

static void
fs_accept_cb (GtkWidget *w, Starling *st)
{

    gchar *path;
    gchar *tmp;
    gchar **files;
    gint i;

    gtk_widget_hide (st->fs);
    files = gtk_file_selection_get_selections (GTK_FILE_SELECTION (st->fs));

    for ( i = 0; files[i]; i++)
      {
	GError *error = NULL;
        play_list_add_recursive (st->pl, files[i], -1, &error);
	if (error)
	  {
	    g_warning ("Reading %s: %s", files[i], error->message);
	    starling_error_box_fmt ("Reading %s: %s",
				    files[i], error->message);
	    g_error_free (error);
	  }
      }
    g_strfreev (files);
    
    path = (gchar*) gtk_file_selection_get_filename (GTK_FILE_SELECTION (st->fs));

    if (st->fs_last_path) 
        g_free (st->fs_last_path);
    
    if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
        st->fs_last_path = g_strdup_printf ("%s/", path);
    }
    else {
        tmp = g_path_get_dirname (path);
        st->fs_last_path = g_strdup_printf ("%s/", tmp);
        g_free (tmp);
    }

    gtk_widget_destroy (st->fs);

}

static void
playlist_add_cb (GtkWidget *w, Starling *st)
{
    if (st->fs)
        return;

    st->fs = gtk_file_selection_new (_("Select File/Directory"));
    gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (st->fs),
            TRUE);
    if (st->fs_last_path)
        gtk_file_selection_set_filename (GTK_FILE_SELECTION (st->fs),
                st->fs_last_path);
    
    g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (st->fs)->cancel_button),
            "clicked", G_CALLBACK (fs_cancel_cb), st);
    g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (st->fs)->ok_button),
            "clicked", G_CALLBACK (fs_accept_cb), st);
    g_signal_connect (G_OBJECT (st->fs), "destroy",
            G_CALLBACK (gtk_widget_destroyed), &st->fs);
    
    gtk_widget_show (st->fs);
}

static void
playlist_remove_cb (GtkWidget *w, Starling *st)
{
  int pos = gtk_tree_view_get_position (GTK_TREE_VIEW (st->treeview));
  if (pos >= 0)
    play_list_remove (st->pl, pos);
}

static void
playlist_move_up_cb (GtkWidget *w, Starling *st)
{
    int pos = gtk_tree_view_get_position (GTK_TREE_VIEW (st->treeview));
    if (pos > 0)
      play_list_swap_pos (st->pl, pos, pos - 1);
}

static void
playlist_move_down_cb (GtkWidget *w, Starling *st)
{
    int pos = gtk_tree_view_get_position (GTK_TREE_VIEW (st->treeview));
    if (pos + 1 < play_list_count (st->pl))
      play_list_swap_pos (st->pl, pos, pos + 1);
}

static void
playlist_clear_cb (GtkWidget *w, Starling *st)
{   
    play_list_clear (st->pl);
}

static void
playlist_save_helper_cb (GtkWidget *dialog, gint response, gpointer data)
{
    Starling *st = (Starling*) data;
    GList *list;
    GtkEntry *entry = NULL;
    const gchar *path;

    if (GTK_RESPONSE_OK == response) {
        list = gtk_container_get_children (GTK_CONTAINER (
                GTK_DIALOG (dialog)->vbox));

        for ( ; list; list = list->next) {
            if (GTK_IS_ENTRY (list->data)) {
                entry = GTK_ENTRY (list->data);
                break;
            }
        }

        if (!entry) {
            return;
        }

        path = gtk_entry_get_text (entry);
        if (g_str_has_suffix (path, ".m3u")) {
            play_list_save_m3u (st->pl, path);
        } else {
            GString *string = g_string_new (path);
            string = g_string_append (string, ".m3u");
            play_list_save_m3u (st->pl, string->str);
            g_string_free (string, TRUE);
        }
    }

    gtk_widget_destroy (dialog);
}

static void
playlist_save_cb (GtkWidget *w, Starling *st)
{
    GtkWidget *dialog;
    GtkWidget *entry;

    dialog = gtk_dialog_new_with_buttons (_("Filename:"), 
                                        GTK_WINDOW (st->window),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
    entry = gtk_entry_new ();

    g_signal_connect (G_OBJECT (dialog), "response", 
                G_CALLBACK (playlist_save_helper_cb), st);

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), entry);
    gtk_widget_show_all (dialog);
}

static void
playlist_activated_cb (GtkTreeView *view, GtkTreePath *path,
		       GtkTreeViewColumn *col, Starling *st)
{
  gint *pos;

  pos = gtk_tree_path_get_indices (path);

  play_list_goto (st->pl, *pos);
}

static void
set_random_cb (GtkWidget *w, Starling *st)
{
  gboolean value = gtk_toggle_tool_button_get_active
    (GTK_TOGGLE_TOOL_BUTTON (st->random));
  play_list_set_random (st->pl, value);
}

static void
playlist_next_cb (GtkWidget *w, Starling *st)
{
    play_list_next (st->pl);
}

static void
playlist_prev_cb (GtkWidget *w, Starling *st)
{
    play_list_prev (st->pl);
}

static void
playlist_playpause_cb (GtkWidget *w, Starling *st)
{

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (w)))
    play_list_unpause (st->pl);
  else
    play_list_pause (st->pl);
}

static gboolean
scale_button_released_cb (GtkRange *range, GdkEventButton *event,
        Starling *st)
{
    gint percent;
    GstFormat fmt = GST_FORMAT_TIME;

    if (st->current_length < 0) {
        play_list_query_duration (st->pl, &fmt, &st->current_length, -1);
    }

    percent = gtk_range_get_value (range);
    play_list_seek (st->pl, GST_FORMAT_TIME, 
            (st->current_length / 100) * percent);
    
    st->scale_pressed = FALSE;
    
    return FALSE;
}

static gboolean
scale_button_pressed_cb (GtkRange *range, GdkEventButton *event,
        Starling *st)
{
    st->scale_pressed = TRUE;
    /* Avoid sending the track if the user seeks */
    // Commented for testing
    //st->enqueued = FALSE;
    
    return FALSE;
}

static gboolean
scale_update_cb (Starling *st)
{
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 position;
    gfloat percent;
    gint total_seconds;
    gint position_seconds;

    if (st->current_length < 0)
      {
	if (! play_list_query_duration (st->pl, &fmt, &st->current_length, -1))
	  return TRUE;

	total_seconds = st->current_length / 1e9;
	char *d = g_strdup_printf ("%d:%02d",
				   total_seconds / 60, total_seconds % 60);
	gtk_label_set_text (GTK_LABEL (st->duration), d);
	g_free (d);
      }

    play_list_query_position (st->pl, &fmt, &position);

    percent = (((gfloat) (position)) / st->current_length) * 100; 

    total_seconds = st->current_length / 1e9;
    position_seconds = (total_seconds / 100.0) * percent;

    char *p = g_strdup_printf ("%d:%02d",
			       position_seconds / 60, position_seconds % 60);
    gtk_label_set_text (GTK_LABEL (st->position), p);
    g_free (p);

    if (!st->scale_pressed && percent <= 100) {
        gtk_range_set_value (GTK_RANGE (st->scale), percent);
    }

    if (G_UNLIKELY (!st->enqueued && total_seconds > 30 && 
        (position_seconds > 240 || position_seconds * 2 >= total_seconds)))
      {
	char *artist;
	char *title;
	play_list_get_info (st->pl, -1,
			    NULL, NULL, &artist, &title, NULL, NULL);

	st->enqueued = TRUE;
	lastfm_enqueue (artist, title, total_seconds, st);

	g_free (artist);
	g_free (title);
    }

    return TRUE;
}

static void
playlist_state_changed_cb (PlayList *pl, const GstState state, Starling *st)
{
  if (GST_STATE_PLAYING == state)
    {
      if (! gtk_toggle_tool_button_get_active
	  (GTK_TOGGLE_TOOL_BUTTON (st->playpause)))
	gtk_toggle_tool_button_set_active
	  (GTK_TOGGLE_TOOL_BUTTON (st->playpause), TRUE);

      char *artist;
      char *title;
      char *uri;

      play_list_get_info (pl, -1, &uri, NULL, &artist, &title, NULL, NULL);

      //if (st->last_state != GST_STATE_PAUSED) {
      if (!st->has_lyrics && artist && title) {
	st->has_lyrics = TRUE;
	lyrics_display (artist, title, GTK_TEXT_VIEW (st->textview));
      }

      if (! title && uri)
	title = g_strdup (uri ? strrchr (uri, '/') + 1 : uri);

      char *t;
      if (artist)
	t = g_strdup_printf ("%s - %s", artist, title ?: "unknown");
      else
	t = g_strdup (title ?: "unknown");

      g_free (artist);
      g_free (title);
      g_free (uri);

      gtk_label_set_text (GTK_LABEL (st->title), t);
      g_free (t);

      st->current_length = -1;
      st->has_lyrics = FALSE;
      st->enqueued = FALSE;
    }
  else if (state == GST_STATE_PAUSED || state == GST_STATE_NULL)
    {
      if (gtk_toggle_tool_button_get_active
	  (GTK_TOGGLE_TOOL_BUTTON (st->playpause)))
	gtk_toggle_tool_button_set_active
	  (GTK_TOGGLE_TOOL_BUTTON (st->playpause), FALSE);
    }
}

static void
playlist_eos_cb (PlayList *pl, Starling *st)
{
  play_list_next (pl);
}

static void
starling_main_quit (GtkWidget *w, GdkEvent *e, Starling *st)
{   
    const gchar *user;
    const gchar *passwd;

    user = gtk_entry_get_text (GTK_ENTRY (st->webuser_entry));
    passwd = gtk_entry_get_text (GTK_ENTRY (st->webpasswd_entry));
    config_store_lastfm (user, passwd, st); 
    config_save (st);
    gtk_main_quit();
}

static void
lastfm_submit_cb (GtkWidget *w, Starling *st)
{
    const gchar *username;
    const gchar *passwd;

    username = gtk_entry_get_text (GTK_ENTRY (st->webuser_entry));
    passwd = gtk_entry_get_text (GTK_ENTRY (st->webpasswd_entry));

    if (username && passwd && strlen (username) && strlen (passwd))
        lastfm_submit (username, passwd, st);
}

void
callbacks_setup (Starling *st)
{
    g_signal_connect (G_OBJECT (st->window), "delete-event", 
            G_CALLBACK (starling_main_quit), st);

    g_signal_connect (G_OBJECT (st->add), "clicked",
            G_CALLBACK (playlist_add_cb), st);
    
    g_signal_connect (G_OBJECT (st->remove), "clicked",
            G_CALLBACK (playlist_remove_cb), st);
    g_signal_connect (G_OBJECT (st->up), "clicked",
            G_CALLBACK (playlist_move_up_cb), st);
  
    g_signal_connect (G_OBJECT (st->down), "clicked",
            G_CALLBACK (playlist_move_down_cb), st);
    
    g_signal_connect (G_OBJECT (st->random), "toggled",
            G_CALLBACK (set_random_cb), st);
    g_signal_connect (G_OBJECT (st->prev), "clicked",
            G_CALLBACK (playlist_prev_cb), st);

    g_signal_connect (G_OBJECT (st->next), "clicked",
            G_CALLBACK (playlist_next_cb), st);

    g_signal_connect (G_OBJECT (st->playpause), "toggled",
            G_CALLBACK (playlist_playpause_cb), st);

    g_signal_connect (G_OBJECT (st->clear), "clicked",
            G_CALLBACK (playlist_clear_cb), st);
    
    g_signal_connect (G_OBJECT (st->save), "clicked",
            G_CALLBACK (playlist_save_cb), st);
    
    g_signal_connect (G_OBJECT (st->web_submit), "clicked",
            G_CALLBACK (lastfm_submit_cb), st);

    g_signal_connect (G_OBJECT (st->scale), "button-release-event", 
            G_CALLBACK (scale_button_released_cb), st);

    g_signal_connect (G_OBJECT (st->scale), "button-press-event",
            G_CALLBACK (scale_button_pressed_cb), st);

    g_signal_connect (G_OBJECT (st->treeview), "row-activated",
            G_CALLBACK (playlist_activated_cb), st);

    g_signal_connect (G_OBJECT (st->pl), "eos",
            G_CALLBACK (playlist_eos_cb), st);
    
    g_signal_connect (G_OBJECT (st->pl), "state-changed",
            G_CALLBACK (playlist_state_changed_cb), st);
    
    g_timeout_add (500, (GSourceFunc) scale_update_cb, st);
}

