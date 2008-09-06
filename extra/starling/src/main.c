/*
   Copyright (C) 2006 Alberto García Hierro
        <skyhusker@handhelds.org>
   Copyright (C) 2007, 2008 Neal H. Walfield <neal@walfield.org>
  
   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */


#include <string.h>
#include <locale.h>
#include <gst/gst.h>

#include "starling.h"
#include "config.h"
#include "lastfm.h"
#include "errorbox.h"
#include "lyrics.h"
#include "utils.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef ENABLE_GPE
#   include <gpe/init.h>
#endif

#ifdef IS_HILDON
/* Hildon includes */
#if HILDON_VER > 0
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>
#include <hildon/hildon-file-chooser-dialog.h>
#else
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
#endif /* HILDON_VER */

#include <libosso.h>
#define APPLICATION_DBUS_SERVICE "starling"
#endif /* IS_HILDON */

#define BUTTONSIZE 32

static void
scroll_to_current (Starling *st)
{
  int count = play_list_count (st->pl);
  if (count == 0)
    return;

  GtkAdjustment *vadj
    = GTK_ADJUSTMENT (gtk_scrolled_window_get_vadjustment
		      (st->treeview_window));
  int pos = play_list_get_current (st->pl);
  gtk_adjustment_set_value
    (vadj,
     ((gdouble) (pos) / count) * (vadj->upper - vadj->lower)
     - vadj->page_size / 2 + vadj->lower);
}


static void
playlist_add_cb (GtkWidget *w, Starling *st, GtkFileChooserAction action)
{
  GtkWidget *fc;

#if IS_HILDON
    fc = hildon_file_chooser_dialog_new
      (GTK_WINDOW (st->window), action);
#else
    fc =  gtk_file_chooser_dialog_new
      (_("Select Files or Directories"), GTK_WINDOW (st->window),
       action,
       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
       GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
#endif

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (fc), TRUE);
    if (st->fs_last_path)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fc),
					     st->fs_last_path);

    if (gtk_dialog_run (GTK_DIALOG (fc)) != GTK_RESPONSE_OK)
      {
	gtk_widget_destroy (fc);
	return;
      }
    gtk_widget_hide (fc); 

    GSList *files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (fc));
    GSList *i;
    for (i = files; i; i = g_slist_next (i))
      {
	GError *error = NULL;
	char *file = i->data;
        play_list_add_recursive (st->pl, file, &error);
	if (error)
	  {
	    g_warning ("Reading %s: %s", file, error->message);
	    starling_error_box_fmt ("Reading %s: %s",
				    file, error->message);
	    g_error_free (error);
	  }
	g_free (file);
      }
    g_slist_free (files);


    if (st->fs_last_path) 
      g_free (st->fs_last_path);
    st->fs_last_path
      = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (fc));

    gtk_widget_destroy (fc);
}

static void
playlist_add_file_cb (GtkWidget *w, Starling *st)
{
  playlist_add_cb (w, st, GTK_FILE_CHOOSER_ACTION_OPEN);
}

static void
playlist_add_directory_cb (GtkWidget *w, Starling *st)
{
  playlist_add_cb (w, st, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
}

static void
do_quit (Starling *st)
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
starling_x_quit (GtkWidget *w, GdkEvent *e, Starling *st)
{
  do_quit (st);
}

static void
starling_menu_quit (GtkWidget *w, Starling *st)
{
  do_quit (st);
}

static void
set_random_cb (GtkWidget *w, Starling *st)
{
  gboolean value = gtk_check_menu_item_get_active
    (GTK_CHECK_MENU_ITEM (st->random));
  play_list_set_random (st->pl, value);
}

static void
playlist_remove_cb (GtkWidget *w, Starling *st)
{
  int pos = gtk_tree_view_get_position (GTK_TREE_VIEW (st->treeview));
  if (pos >= 0)
    play_list_remove (st->pl, pos);
}

static void
playlist_clear_cb (GtkWidget *w, Starling *st)
{   
    play_list_clear (st->pl);
}

#ifdef IS_HILDON
#if HILDON_VER > 0
static void
toggle_fullscreen (GtkCheckMenuItem *menuitem, gpointer user_data)
{
  if (gtk_check_menu_item_get_active (menuitem))
    gtk_window_fullscreen(GTK_WINDOW(user_data));
  else
    gtk_window_unfullscreen(GTK_WINDOW(user_data));
}
#else
static void
toggle_fullscreen (GtkCheckMenuItem *menuitem, gpointer user_data)
{
  hildon_appview_set_fullscreen (HILDON_APPVIEW (user_data),
				 gtk_check_menu_item_get_active (menuitem));
}
#endif /* HILDON_VER */
#endif /*IS_HILDON*/

#if 0
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
#endif

static void
playlist_activated_cb (GtkTreeView *view, GtkTreePath *path,
		       GtkTreeViewColumn *col, Starling *st)
{
  gint *pos;

  pos = gtk_tree_path_get_indices (path);

  play_list_goto (st->pl, *pos);
}

static void
playlist_next_cb (GtkWidget *w, Starling *st)
{
  play_list_next (st->pl);
  scroll_to_current (st);  
}

static void
playlist_prev_cb (GtkWidget *w, Starling *st)
{
  play_list_prev (st->pl);
  scroll_to_current (st);  
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
			    NULL, NULL, &artist, NULL, NULL, &title, NULL);

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

      char *artist_buffer;
      char *title_buffer;
      char *uri_buffer;

      play_list_get_info (pl, -1, NULL, &uri_buffer,
			  &artist_buffer, NULL, NULL, &title_buffer, NULL);
      char *uri = uri_buffer;
      if (! uri)
	uri = "unknown";
      char *artist = artist_buffer;
      char *title = title_buffer;

      //if (st->last_state != GST_STATE_PAUSED) {
      if (!st->has_lyrics && artist && title) {
	st->has_lyrics = TRUE;
	lyrics_display (artist, title, GTK_TEXT_VIEW (st->textview));
      }

      if (! title)
	{
	  title = strrchr (uri, '/');
	  if (! title)
	    title = uri;
	  else
	    title ++;
	}

#define STARLING "Starling: "
      char *title_bar;
      if (artist)
	title_bar = g_strdup_printf (_("%s %s - %s"),
				     _(STARLING),
				     artist, title);
      else
	title_bar = g_strdup_printf (_("%s %s"), _(STARLING), title);

      gtk_label_set_text (GTK_LABEL (st->title),
			  title_bar
			  + sizeof (STARLING) - 1 /* NULL */ + 1 /* ' ' */);

#ifdef IS_HILDON
#if HILDON_VER > 0
      gtk_window_set_title (GTK_WINDOW (st->window), title_bar);
#else
      hildon_app_set_title (HILDON_APP (st->window), title_bar);
#endif /* HILDON_VER */
#else
      gtk_window_set_title (GTK_WINDOW (st->window), title_bar);
#endif /* IS_HILDON */
      g_free (title_bar);
      g_free (artist_buffer);
      g_free (title_buffer);
      g_free (uri_buffer);

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
lastfm_submit_cb (GtkWidget *w, Starling *st)
{
    const gchar *username;
    const gchar *passwd;

    username = gtk_entry_get_text (GTK_ENTRY (st->webuser_entry));
    passwd = gtk_entry_get_text (GTK_ENTRY (st->webpasswd_entry));

    if (username && passwd && strlen (username) && strlen (passwd))
        lastfm_submit (username, passwd, st);
}

static int
key_press_event (GtkWidget *widget, GdkEventKey *k, Starling *st)
{
  /* in hildon there is nothing like control, shift etc buttons */
  switch (k->keyval)
    {
#ifdef IS_HILDON
    case GDK_F6:
#if 0
      /* toggle button for going full screen */
      gtk_check_menu_item_set_active
	(st->fullscreen, ! gtk_check_menu_item_get_active (st->fullscreen));
#endif
      /* Make full screen a play/pause button.  */
      gtk_toggle_tool_button_set_active
	(st->playpause, ! gtk_toggle_tool_button_get_active (st->playpause));
      return TRUE;
#endif

    case GDK_F7:
      playlist_next_cb (NULL, st);
      return TRUE;
    case GDK_F8:
      playlist_prev_cb (NULL, st);
      return TRUE;
    }

  return FALSE;
}


static void
title_data_func (GtkCellLayout *cell_layout,
		 GtkCellRenderer *cell_renderer,
		 GtkTreeModel *model,
		 GtkTreeIter *iter,
		 gpointer data)
{
  char *artist;
  char *title;
  char *buffer = NULL;
  gtk_tree_model_get (model, iter, PL_COL_TITLE, &buffer,
		      PL_COL_ARTIST, &artist, -1);
  if (buffer)
    title = buffer;
  else
    /* Try to show the last three path components as the directory
       structure is often artist/album/file.  */
    {
      gtk_tree_model_get (model, iter, PL_COL_SOURCE, &buffer, -1);
      if (buffer)
	{
	  char *slashes[3];
	  int i = 0;
	  char *t = strchr (buffer, '/');
	  do
	    {
	      while (t[1] == '/')
		t ++;
	      slashes[(i ++) % 3] = t;
	    }
	  while ((t = strchr (t + 1, '/')));

	  if (i >= 3)
	    title = slashes[(i - 3) % 3] + 1;
	  else if (i)
	    title = slashes[0] + 1;
	  else
	    title = buffer;
	}
      else
	title = "unknown";
    }

  if (artist)
    {
      char *t = g_strdup_printf ("%s - %s", artist, title);
      g_free (buffer);
      title = buffer = t;
    }

  g_object_set (cell_renderer, "text", title, NULL);
  g_free (buffer);

  int uid;
  gtk_tree_model_get (model, iter, PL_COL_INDEX, &uid, -1);
  g_object_set (cell_renderer,
		"cell-background-set",
		uid == play_list_get_current (PLAY_LIST (model)),
		NULL);
}

int
main(int argc, char *argv[])
{
  Starling st_;
  Starling *st = &st_;

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

#ifdef ENABLE_GPE
  gpe_application_init (&argc, &argv);
#else
  gtk_init (&argc, &argv);
#endif

#ifdef IS_HILDON
  osso_context_t *osso_context;

  /* Initialize maemo application */
  osso_context = osso_initialize(APPLICATION_DBUS_SERVICE, "1.0", TRUE, NULL);

  /* Check that initialization was ok */
  if (osso_context == NULL)
    {
      g_critical ("Failed to initialize OSSO context!");
      return OSSO_ERROR;
    }
#endif

  const char *home = g_get_home_dir ();
  char *dir = g_strdup_printf ("%s/.starling", home);
  /* We don't check the error here because either it already exists
     (which isn'treally an error), or we could created it and this
     error will be caught below.  */
  g_mkdir (dir, 0755);
  char *file = g_strdup_printf ("%s/.starling/playlist", home);

  GError *err = NULL;
  st->pl = play_list_open (file, &err);
  g_free (file);
  if (err)
    {
      g_warning (err->message);
      starling_error_box (err->message);
      g_error_free (err);
      exit (1);
    }

  g_signal_connect (G_OBJECT (st->pl), "eos",
		    G_CALLBACK (playlist_eos_cb), st);
  g_signal_connect (G_OBJECT (st->pl), "state-changed",
		    G_CALLBACK (playlist_state_changed_cb), st);
    
  /* Build the GUI.  */
  GtkWidget *hbox1 = NULL;
  GtkWidget *hbox2 = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *label = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkWidget *scrolled = NULL;

#ifdef IS_HILDON
#if HILDON_VER > 0
  HildonProgram *program = HILDON_PROGRAM (hildon_program_get_instance());
  g_set_application_name ("Starling");
  st->window = GTK_WIDGET (hildon_window_new());
  hildon_program_add_window (program, HILDON_WINDOW(st->window));
#else
  st->window = hildon_app_new ();
  hildon_app_set_two_part_title (HILDON_APP (st->window), FALSE);
  GtkWidget *main_appview = hildon_appview_new (_("Main"));
  hildon_app_set_appview (HILDON_APP (st->window),
			  HILDON_APPVIEW (main_appview));

  hildon_app_set_title (HILDON_APP (st->window), "Starling");
#endif /* HILDON_VER */
#else    
  st->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (st->window), "Starling");
#endif /* IS_HILDON */
  g_signal_connect (G_OBJECT (st->window), "delete-event", 
		    G_CALLBACK (starling_x_quit), st);

  GtkBox *main_box = GTK_BOX (gtk_vbox_new (FALSE, 5));
#ifdef IS_HILDON
#if HILDON_VER > 0
  gtk_container_add (GTK_CONTAINER (st->window), GTK_WIDGET (main_box));
#else
  gtk_container_add (GTK_CONTAINER (main_appview), GTK_WIDGET (main_box));
#endif /* HILDON_VER */
#else
  gtk_container_add (GTK_CONTAINER (st->window), GTK_WIDGET (main_box));
#endif /* IS_HILDON */

  g_signal_connect (G_OBJECT (st->window), "key_press_event", 
		    G_CALLBACK (key_press_event), st);


  /* Menu bar.  */
  GtkMenuShell *menu_main;
#ifdef IS_HILDON
#if HILDON_VER > 0
  menu_main = GTK_MENU_SHELL (gtk_menu_new ());
#else
  menu_main
    = GTK_MENU_SHELL (hildon_appview_get_menu (HILDON_APPVIEW (main_appview)));
#endif /* HILDON_VER */
#else
  menu_main = GTK_MENU_SHELL (gtk_menu_bar_new ());
  gtk_box_pack_start (main_box, GTK_WIDGET (menu_main), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (menu_main));
#endif /* IS_HILDON */

  GtkMenuShell *menu;
  GtkWidget *mitem;

#ifndef IS_HILDON
  /* File menu.  */
  menu = GTK_MENU_SHELL (gtk_menu_new ());
  mitem = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), GTK_WIDGET (menu));
  gtk_menu_shell_append (menu_main, GTK_WIDGET (mitem));
  gtk_widget_show (mitem);
#endif
    
  /* File -> Open.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("Import _File"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (playlist_add_file_cb), st);
  gtk_widget_show (mitem);
#ifdef IS_HILDON
  gtk_menu_shell_append (menu_main, mitem);
#else
  gtk_menu_shell_append (menu, mitem);
#endif

  /* File -> Open directory.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("Import _Directory"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (playlist_add_directory_cb), st);
  gtk_widget_show (mitem);
#ifdef IS_HILDON
  gtk_menu_shell_append (menu_main, mitem);
#else
  gtk_menu_shell_append (menu, mitem);
#endif

  /* File -> Quit.  */
#ifdef IS_HILDON
  GtkWidget *quit_item = mitem = gtk_menu_item_new_with_mnemonic (_("Quit"));
#else
  mitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
#endif
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (starling_menu_quit), st);
  gtk_widget_show (mitem);
#ifndef IS_HILDON
  /* Don't append this to the file menu in hildon but the main menu
     (which we do at the very end).  */
  gtk_menu_shell_append (menu, mitem);
#endif


  /* Options menu.  */
  menu = GTK_MENU_SHELL (gtk_menu_new ());
  mitem = gtk_menu_item_new_with_mnemonic (_("_Options"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), GTK_WIDGET (menu));
  gtk_menu_shell_append (menu_main, GTK_WIDGET (mitem));
  gtk_widget_show (mitem);
    
  /* Options -> Random.  */
  st->random = gtk_check_menu_item_new_with_mnemonic (_("_Random"));
  g_signal_connect (G_OBJECT (st->random), "toggled",
		    G_CALLBACK (set_random_cb), st);
  gtk_widget_show (st->random);
  gtk_menu_shell_append (menu, st->random);

  mitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* Options -> Remove selected.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("Remove Selecte_d"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (playlist_remove_cb), st);
  gtk_widget_show (mitem);
  gtk_menu_shell_append (menu, mitem);

  /* Options -> Clear all.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("_Clear all"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (playlist_clear_cb), st);
  gtk_widget_show (mitem);
  gtk_menu_shell_append (menu, mitem);

#ifdef IS_HILDON
  mitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* Options -> Full Screen.  */
  mitem = gtk_check_menu_item_new_with_mnemonic (_("_Full Screen"));
  st->fullscreen = GTK_CHECK_MENU_ITEM(mitem);
#if HILDON_VER > 0
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (toggle_fullscreen), st->window);
#else
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (toggle_fullscreen), main_appview);
#endif /* HILDON_VER */
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);
#endif

#ifdef IS_HILDON
  /* Finally attach close item. */
  mitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (menu_main, mitem);
  gtk_widget_show (mitem);

  gtk_menu_shell_append (menu_main, quit_item);
#if HILDON_VER > 0
  hildon_window_set_menu (HILDON_WINDOW(st->window), GTK_MENU(menu_main));
#endif /* HILDON_VER */
#endif /* IS_HILDON */



  /* The toolbar.  */
  GtkWidget *toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);
  gtk_widget_show (toolbar);

#ifdef IS_HILDON
#if HILDON_VER == 0
  hildon_appview_set_toolbar (HILDON_APPVIEW (main_appview),
			      GTK_TOOLBAR (toolbar));
  gtk_widget_show_all (main_appview);
#endif /* HILDON_VER */
#else
  gtk_box_pack_start (GTK_BOX (main_box), toolbar, FALSE, FALSE, 0);
#endif /* IS_HILDON */

  /* Previous button.  */
  GtkToolItem *item;
  item = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS);
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (playlist_prev_cb), st);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Play/pause.  */
  item = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
  st->playpause = GTK_WIDGET (item);
  g_signal_connect (G_OBJECT (st->playpause), "toggled",
		    G_CALLBACK (playlist_playpause_cb), st);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Next.  */
  item = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_NEXT);
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (playlist_next_cb), st);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Scale.  */
  item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (item, TRUE);

  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
  gtk_container_add (GTK_CONTAINER (item), GTK_WIDGET (hbox));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

#ifdef IS_HILDON
#if HILDON_VER > 0
  /* Add toolbar to the HildonWindow */
  hildon_window_add_toolbar (HILDON_WINDOW(st->window), GTK_TOOLBAR(toolbar));
#endif
#endif

  st->position = GTK_LABEL (gtk_label_new ("0:00"));
  gtk_label_set_width_chars (st->position, 5);
  gtk_misc_set_alignment (GTK_MISC (st->position), 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->position),
		      FALSE, FALSE, 0);

  st->scale = gtk_hscale_new_with_range (0, 100, 1);
  gtk_scale_set_draw_value (GTK_SCALE (st->scale), FALSE);
  g_signal_connect (G_OBJECT (st->scale), "button-release-event", 
		    G_CALLBACK (scale_button_released_cb), st);
  g_signal_connect (G_OBJECT (st->scale), "button-press-event",
		    G_CALLBACK (scale_button_pressed_cb), st);
  g_timeout_add (1000, (GSourceFunc) scale_update_cb, st);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->scale),
		      TRUE, TRUE, 0);
    
  st->duration = GTK_LABEL (gtk_label_new ("0:00"));
  gtk_label_set_width_chars (st->duration, 5);
  gtk_misc_set_alignment (GTK_MISC (st->duration), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->duration),
		      FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 5);


  /* Stuff the title label in an hbox to prevent it from being
     centered.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  st->title = gtk_label_new (_("Not playing"));
  gtk_misc_set_alignment (GTK_MISC (st->title), 0, 0.5);
  gtk_label_set_ellipsize (GTK_LABEL (st->title), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (hbox, st->title, TRUE, TRUE, 0);

  /* Jump to the currently playing song.  */
  GtkWidget *jump_to = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (jump_to),
			gtk_image_new_from_stock (GTK_STOCK_JUMP_TO,
						  GTK_ICON_SIZE_BUTTON));
  g_signal_connect_swapped (G_OBJECT (jump_to), "clicked",
			    G_CALLBACK (scroll_to_current), st);
  gtk_box_pack_start (hbox, jump_to, FALSE, FALSE, 0);

  st->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (st->pl));
  g_signal_connect (G_OBJECT (st->treeview), "row-activated",
		    G_CALLBACK (playlist_activated_cb), st);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (st->treeview), FALSE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (st->treeview), TRUE);
  gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (st->treeview), TRUE);

  gtk_widget_grab_focus (GTK_WIDGET (st->treeview));

  GtkTreeViewColumn *col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (st->treeview), col);
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "cell-background", "gray", NULL);
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      title_data_func,
				      NULL, NULL);
    
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  st->treeview_window = GTK_SCROLLED_WINDOW (scrolled);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
    
  gtk_container_add (GTK_CONTAINER (scrolled), st->treeview);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled);
    
  /* XXX: These are currently disable.  Up and down are pretty
     useless.  We'd rather have drag and drog (and multi-select).
     Save as is broken (see playlist.c).  */
#if 0
  create_button (&st->up, GTK_STOCK_GO_UP, l_button_box);
  g_signal_connect (G_OBJECT (st->up), "clicked",
		    G_CALLBACK (playlist_move_up_cb), st);
  
  create_button (&st->down, GTK_STOCK_GO_DOWN, l_button_box);
  g_signal_connect (G_OBJECT (st->down), "clicked",
		    G_CALLBACK (playlist_move_down_cb), st);
    
  create_button (&st->save, GTK_STOCK_SAVE_AS, l_button_box);
  g_signal_connect (G_OBJECT (st->save), "clicked",
		    G_CALLBACK (playlist_save_cb), st);
#endif
    
  GtkWidget *notebook = gtk_notebook_new ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new (_("Player")));

  gtk_container_add (GTK_CONTAINER (main_box), GTK_WIDGET (notebook));

  /* Lyrics tab */

  vbox = gtk_vbox_new (FALSE, 5);
  st->textview = gtk_text_view_new ();
  GTK_TEXT_VIEW (st->textview)->editable = FALSE;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
    
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

  gtk_container_add (GTK_CONTAINER (scrolled), st->textview);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 3);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new (_("Lyrics")));

  /* Web services tab (for now, last.fm) */

  hbox1 = gtk_hbox_new (FALSE, 2);
  label = gtk_label_new (_("Username:"));
  st->webuser_entry = gtk_entry_new ();

  gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1), st->webuser_entry, TRUE, TRUE, 0);
    
  hbox2 = gtk_hbox_new (FALSE, 2);
  label = gtk_label_new (_("Password:"));
  st->webpasswd_entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (st->webpasswd_entry), FALSE);
    
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), st->webpasswd_entry, TRUE, TRUE, 0);
    
  /*gtk_box_pack_start (GTK_BOX (hbox), st->webuser_entry, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  */
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);

  st->web_count = gtk_label_new ("");
  st->web_submit = gtk_button_new_with_label (_("Submit"));
  g_signal_connect (G_OBJECT (st->web_submit), "clicked",
		    G_CALLBACK (lastfm_submit_cb), st);
  gtk_box_pack_start (GTK_BOX (vbox), st->web_count, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), st->web_submit, FALSE, FALSE, 0);
    
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new (_("last.fm")));


  /* Init some variables */

    st->scale_pressed = FALSE;

    /* Just make sure */
    st->fs_last_path = NULL;

    st->current_length = -1;
    st->has_lyrics = FALSE;
    st->enqueued = FALSE;
    
    lyrics_init ();

    lastfm_init (st);

    config_init (st);

    config_load (st);
    
    gtk_widget_show_all (st->window);
    
    gtk_main();
    
    return 0;
}
