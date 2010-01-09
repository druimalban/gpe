/* starling.c - Starling.
   Copyright (C) 2007, 2008, 2009, 2010 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro <skyhusker@handhelds.org>

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

#define _GNU_SOURCE

#define PROGRAM_NAME "Starling"
#define ERROR_DOMAIN() g_quark_from_static_string (PROGRAM_NAME)

#include "starling.h"

#include "config.h"
#include "lastfm.h"
#include "errorbox.h"
#include "lyrics.h"
#include "utils.h"
#include "playlist.h"
#include "playlists.h"
#include "player.h"
#include "search.h"
#include "caption.h"

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <glib/gtypes.h>
#include <glib/gkeyfile.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#if HAVE_HILDON
/* Hildon includes */
# if HAVE_HILDON_VERSION > 0
#  include <hildon/hildon-program.h>
#  include <hildon/hildon-window.h>
#  include <hildon/hildon-file-chooser-dialog.h>
#  include <hildon/hildon-button.h>
#  include <hildon/hildon-check-button.h>
# else
#  include <hildon-widgets/hildon-app.h>
#  include <hildon-widgets/hildon-appview.h>
#  include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
# endif /* HAVE_HILDON_VERSION */

# if HAVE_HILDON_VERSION >= 202
#  include <hildon/hildon-defines.h>
#  include <hildon/hildon-picker-button.h>
# endif

# include <libosso.h>
# define APPLICATION_DBUS_SERVICE "starling"
#endif /* HAVE_HILDON */

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
# include <hildon/hildon-touch-selector.h>
#endif
#ifdef HAVE_HILDON_PANNABLE_AREA
# include <hildon/hildon-pannable-area.h>
#endif
#ifdef HAVE_HILDON_GTK_TREE_VIEW
# include <hildon/hildon-gtk.h>
#endif

struct play_list_view_config
{
  int position;
  GList *selected_rows;
};

struct _Starling {
  GtkWidget *window;

  GtkLabel *position;
  GtkLabel *duration;
  GtkWidget *playpause;
  int playpause_toggled_signal_id;
  GtkWidget *position_slider;
  int user_seeking;

#if defined(HAVE_GTK_VOLUME_BUTTON_NEW) \
  && (! defined (HAVE_MAEMO) || HAVE_MAEMO_VERSION < 500)
# define HAVE_VOLUME_BUTTON
  GtkWidget *volume_button;
#endif

  GtkWidget *title;
  GtkComboBox *rating;
  int rating_change_signal_id;

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  GtkWidget *notebook;
  GtkWidget *queue_tab;
#endif
  GtkWidget *library_tab;
  guint library_tab_update_source;
  GtkWidget *lyrics_tab;
  GtkWidget *lastfm_tab;

  GtkLabel *status;

  GtkWidget *library_view_window;
  GtkWidget *library_view;
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  GtkWidget *library_view_loading_label;
#endif
  struct caption *caption;
  char *caption_format;
  PlayLists *play_lists_model;
  GtkWidget *play_list_selector;
  GtkBox *playlist_alpha_seek;
  int playlist_alpha_current_height;
  int playlist_alpha_seek_timer_source;
  int play_list_selector_changed_signal;

  GtkWidget *search_bar;
#if HAVE_MAEMO && HAVE_MAEMO_VERSION >= 500
#define USE_SEARCH_BAR_TOGGLE
  GtkWidget *search_bar_toggle;
  guint search_bar_toggle_toggled_signal;
#endif
  GtkToggleButton *search_enabled;
  GtkWidget *search_entry;
  GtkListStore *searches;

  /* The current grouping.  */
  enum mdb_fields group_by;
  GtkWidget *group_by_button;
  GSList *group_by_radio_widgets;

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  GtkWidget *queue_view_window;
  GtkWidget *queue_view;
#endif

  /* Lastfm tab.  */
  GtkWidget *webuser_entry;
  GtkWidget *webpasswd_entry;
  GtkWidget *web_submit;
  GtkWidget *web_count;
  GtkSpinButton *lastfm_autosubmit;

  GtkWidget *random;
  gchar *fs_last_path;

  GtkWidget *lyrics_textview;
  gboolean has_lyrics;

  gint64 current_length;
  gboolean enqueued;
#if HAVE_HILDON && !(HAVE_MAEMO && HAVE_MAEMO_VERSION >= 500)
# define HAVE_TOGGLE_FULLSCREEN
  GtkWidget *fullscreen;
#endif
  GtkWidget *download_lyrics;

  Player *player;
  MusicDB *db;
  PlayList *library;
#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  PlayList *queue;
#endif

  /* UID of the loaded song.  */
  int loaded_song;

  /* Source of the position updater.  */
  int position_update;

  /* Position to seek to next time it is possible (because we cannot
     seek when the player is in GST_STATE_NULL).  */
  int pending_seek;

  /* Hash mapping play lists to their position.  */
  GHashTable *playlists_conf;

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  struct
  {
    GtkWindow *window;
  } play_list_selector_view;
#endif
};

/* Forward declarations.  */
static void playlist_alpha_seek_build_queue (Starling *st);
static void update_library_count (Starling *st);

/* Return the currectly selected playlist or NULL if none.  The
   returned string must be freed.  */
static char *
play_list_selector_get_active (Starling *st)
{
  GtkTreeIter iter;
  bool have_iter = false;
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  GtkTreePath *path = hildon_touch_selector_get_last_activated_row
    (HILDON_TOUCH_SELECTOR (st->play_list_selector), 0);
  if (path)
    {
      have_iter = gtk_tree_model_get_iter
	(GTK_TREE_MODEL (st->play_lists_model), &iter, path);
      gtk_tree_path_free (path);
    }
#else
  have_iter = gtk_combo_box_get_active_iter
    (GTK_COMBO_BOX (st->play_list_selector), &iter);
#endif
  char *value = NULL;
  if (have_iter)
    gtk_tree_model_get (GTK_TREE_MODEL (st->play_lists_model), &iter,
			PLS_COL_NAME, &value, -1);

  return value;
}

static void
set_title (Starling *st)
{
  char *title_bar;
  bool free_title_bar = true;

  struct music_db_info info;
  info.fields = MDB_SOURCE | MDB_ARTIST | MDB_TITLE | MDB_RATING;
  if (! music_db_get_info (st->db, st->loaded_song, &info))
    {
      title_bar = PROGRAM_NAME;
      free_title_bar = false;
      goto do_set;
    }

  char *uri = info.source;
  if (! uri)
    uri = "unknown";
  char *artist = info.artist;
  char *title = info.title;

  if (!st->has_lyrics && artist && title)
    {
      st->has_lyrics = TRUE;
      lyrics_display
	(artist, title, GTK_TEXT_VIEW (st->lyrics_textview),
#ifdef HAVE_HILDON_APP_MENU
	 hildon_check_button_get_active ((void *) st->download_lyrics),
#else
	 gtk_check_menu_item_get_active ((void *) st->download_lyrics),
#endif
	 FALSE);
    }

  if (st->rating)
    {
      g_signal_handler_block (st->rating, st->rating_change_signal_id);
      gtk_combo_box_set_active (st->rating, info.rating);
      g_signal_handler_unblock (st->rating, st->rating_change_signal_id);
    }

  if (! title)
    {
      title = strrchr (uri, '/');
      if (! title)
	title = uri;
      else
	title ++;
    }

  char *caption;
  if (artist)
    caption = g_strdup_printf (_("%s by %s"), title, artist);
  else
    caption = g_strdup (title);

  title_bar = g_strdup_printf (_("%s - %s"),
			       caption, _(PROGRAM_NAME));
  free_title_bar = true;

  if (st->title)
    gtk_label_set_text (GTK_LABEL (st->title), caption);

  free (caption);

  g_free (info.artist);
  g_free (info.title);
  g_free (info.source);

 do_set:
#if HAVE_HILDON
#if HAVE_HILDON_VERSION > 0
  gtk_window_set_title (GTK_WINDOW (st->window), title_bar);
#else
  hildon_app_set_title (HILDON_APP (st->window), title_bar);
#endif /* HAVE_HILDON_VERSION */
#else
  gtk_window_set_title (GTK_WINDOW (st->window), title_bar);
#endif /* HAVE_HILDON */

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  gtk_window_set_title (GTK_WINDOW (st->play_list_selector_view.window),
			title_bar);
  gtk_window_set_title (GTK_WINDOW (st->lyrics_tab), title_bar);
  gtk_window_set_title (GTK_WINDOW (st->lastfm_tab), title_bar);
#endif

  if (free_title_bar)
    g_free (title_bar);
}

static void
meta_data_changed (Starling *st, gint uid, MusicDB *db)
{
  if (uid == st->loaded_song)
    set_title (st);
}

bool
starling_random (Starling *st)
{
#ifdef HAVE_HILDON_APP_MENU
  return hildon_check_button_get_active (HILDON_CHECK_BUTTON (st->random));
#else
  return gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (st->random));
#endif
}

void
starling_random_set (Starling *st, bool value)
{
  if (value == starling_random (st))
    return;

#ifdef HAVE_HILDON_APP_MENU
  hildon_check_button_set_active (HILDON_CHECK_BUTTON (st->random), value);
#else
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (st->random), value);
#endif
}

static gboolean
starling_load (Starling *st, int uid)
{
  int old_idx = -1;
  if (st->loaded_song)
    {
      old_idx = play_list_uid_to_index (st->library, st->loaded_song);

      if (st->current_length > 0)
	{
	  GstFormat fmt = GST_FORMAT_TIME;
	  gint64 position;
	  if (player_query_position (st->player, &fmt, &position)
	      && position > (9 * st->current_length) / 10)
	    /* At least 90% of the song was played.  Increment its
	       play count.  */
	    {
	      /* Note that it has been played.  */
	      struct music_db_info info;
	      memset (&info, 0, sizeof (info));
	      info.fields = MDB_UPDATE_DATE_LAST_PLAYED | MDB_INC_PLAY_COUNT
		| MDB_DURATION;
	      /* Also set the duration now, it doesn't cost anything
		 and many track lack it.  */
	      info.duration = st->current_length / 1e9;

	      music_db_set_info (st->db, st->loaded_song, &info);
	    }
	}
    }

  st->loaded_song = uid;
  st->pending_seek = -1;
  st->current_length = -1;

  struct music_db_info info;
  info.fields = MDB_SOURCE | MDB_PRESENT;
  if (! music_db_get_info (st->db, uid, &info))
    {
      g_warning ("%s: No entry found for %d!", __FUNCTION__, uid);
      return FALSE;
    }

  if (! info.present)
    return FALSE;

  player_set_source (st->player, info.source, (gpointer) uid);

  g_free (info.source);

  if (old_idx != -1)
    play_list_force_changed (st->library, old_idx);
  if (st->loaded_song)
    {
      int idx = play_list_uid_to_index (st->library, st->loaded_song);
      if (idx != -1)
	play_list_force_changed (st->library, idx);

      st->has_lyrics = FALSE;
      st->current_length = -1;
      st->enqueued = FALSE;

      set_title (st);
    }

  return TRUE;
}

gboolean
starling_play (Starling *st)
{
  return player_play (st->player);
}

static void
starling_advance (Starling *st, int delta)
{
  int tries = 0;

  int count = -1;

  int uid;
  do
    {
      /* Try to get something from the queue.  */
      if (starling_random (st))
	{
	  static int rand_init;
	  if (! rand_init)
	    {
	      srand (time (NULL));
	      rand_init = 1;
	    }

	  int count = music_db_count (st->db, "queue", NULL);
	  if (count > 0)
	    {
	      int i = rand () % count;
	      uid = music_db_play_list_query (st->db, "queue", i);
	      music_db_play_list_remove (st->db, "queue", i);

	      if (uid == 0)
		/* It seems the track was removed or something.  */
		continue;
	    }
	  else
	    uid = 0;
	}
      else
	uid = music_db_play_list_dequeue (st->db, "queue");

      if (uid == 0)
	/* Nothing on the queue.  */
	{
	  if (count == -1)
	    count = play_list_count (st->library);

	  if (count == tries)
	    /* Tried everything in the library once.  Something is
	       wrong...  */
	    return;
	  tries ++;

	  int idx;

	  if (starling_random (st))
	    /* Random mode.  */
	    idx = rand () % count;
	  else
	    /* Not random mode.  */
	    {
	      idx = play_list_uid_to_index (st->library, st->loaded_song);
	      if (idx == -1)
		idx = 0;
	      else
		idx += delta;

	      if (delta > 0)
		{
		  if (idx >= count)
		    idx = 0;
		}
	      else
		{
		  if (idx < 0)
		    idx = count - 1;
		}
	    }

	  uid = play_list_index_to_uid (st->library, idx);
	}
    }
  while (! (starling_load (st, uid) && starling_play (st)));
}

void
starling_next (Starling *st)
{
  starling_advance (st, 1);
}

void
starling_prev (Starling *st)
{
  starling_advance (st, -1);
}

void
starling_set_sink (Starling *st, char *sink)
{
  g_debug ("Setting sink to %s\n", sink);
  player_set_sink (st->player, sink);
}

#define CONFIG_FILE "starlingrc"

#define KEYRANDOM "random"
#define KEYLASTPATH "last-path"
#define KEYSINK "sink"
#define KEYVOLUME "volume"
#define KEYLFMUSER "lastfm-user"
#define KEYLFMPASSWD "lastfm-password"
#define KEYLFMAUTOSUBMIT "lastfm-auto-submit"
#define KEY_WIDTH "width"
#define KEY_HEIGHT "height"
#define KEY_LOADED_SONG "loaded-song"
#define KEY_LOADED_SONG_POSITION "loaded-song-position"
#define KEY_PLAYLISTS_CONFIG "playlists-config"
#define KEY_SEARCH_ENABLED "search-enabled"
#define KEY_SEARCH_TEXT "search-text"
#define KEY_SEARCHES "searches"
#define KEY_CURRENT_PAGE "current-page"
#define KEY_CURRENT_PLAYLIST "current-playlist"
#define KEY_LYRICS_DOWNLOAD "lyrics-download"
#define KEY_CAPTION_FORMAT "caption-format"
#define KEY_CAPTION_FORMAT_HISTORY "caption-format-history"

#define GROUP "main"

/* Some state can only really be restored before we show the
   application and some only after.  We register an idle function and
   do the latter once we idle.  */
struct deserialize_bottom_half
{
  Starling *st;
  int page;
  char *playlist;
  bool search_enabled;
  char *search_terms;

  int loaded_song;
};

static void play_list_selector_changed_to
  (Starling *st, const char *play_list, gboolean inital_restore);

static int
deserialize_bottom_half (gpointer data)
{
  struct deserialize_bottom_half *bh = data;

  Starling *st = bh->st;

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  gtk_notebook_set_current_page (GTK_NOTEBOOK (st->notebook), bh->page);
#endif

  /* Set the correct play list.  */
  GtkTreeIter iter;
  if (play_lists_iter_of (st->play_lists_model, &iter, bh->playlist))
    {
      g_debug ("%s: Setting active play list to %s\n",
	       __func__, bh->playlist);

      /* Don't sent the play list selector changed signal as we don't
	 want to save the current selection.  */
      g_signal_handler_block (st->play_list_selector,
			      st->play_list_selector_changed_signal);
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
      hildon_touch_selector_select_iter
	(HILDON_TOUCH_SELECTOR (st->play_list_selector), 0, &iter, TRUE);
#else
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (st->play_list_selector),
				     &iter);
#endif
      g_signal_handler_unblock (st->play_list_selector,
				st->play_list_selector_changed_signal);
      play_list_selector_changed_to (st, bh->playlist, true);
    }
  else
    play_list_selector_changed_to (st, NULL, true);
  g_free (bh->playlist);
  
  gtk_toggle_button_set_active (st->search_enabled, bh->search_enabled);
  gtk_entry_set_text (GTK_ENTRY (st->search_entry), bh->search_terms);
  g_free (bh->search_terms);

  if (bh->loaded_song > 0)
    {
      int pending_seek = st->pending_seek;
      starling_load (st, bh->loaded_song);
      st->pending_seek = pending_seek;
    }

  g_free (data);

  lastfm_init (gtk_entry_get_text (GTK_ENTRY (st->webuser_entry)),
	       gtk_entry_get_text (GTK_ENTRY (st->webpasswd_entry)),
	       gtk_spin_button_get_value (st->lastfm_autosubmit),
	       GTK_LABEL (st->web_count));

  return FALSE;
}

static void search_text_gen (Starling *st, const char *text);

static GKeyFile *
config_file_load (void)
{
  gchar *dir = g_strdup_printf ("%s/%s", g_get_home_dir(), CONFIGDIR);
  g_mkdir (dir, 0755);
  g_free (dir);

  char *path = g_strdup_printf ("%s/%s/%s",
				g_get_home_dir(), CONFIGDIR, CONFIG_FILE);

  GKeyFile *keyfile = g_key_file_new ();
  g_key_file_load_from_file (keyfile, path, 0, NULL);

  g_free (path);

  return keyfile;
}

static void
config_file_save (GKeyFile *keyfile)
{
  gchar *dir = g_strdup_printf ("%s/%s", g_get_home_dir(), CONFIGDIR);
  g_mkdir (dir, 0755);
  g_free (dir);

  char *path = g_strdup_printf ("%s/%s/%s",
				g_get_home_dir(), CONFIGDIR, CONFIG_FILE);

  /* Save the key file to disk.  */
  gsize length;
  char *data = g_key_file_to_data (keyfile, &length, NULL);
  g_key_file_free (keyfile);

  g_file_set_contents (path, data, length, NULL);

  g_free (path);
  g_free (data);
}

static void
deserialize (Starling *st)
{
  /* Open the settings file.  */
  GKeyFile *keyfile = config_file_load ();
  if (! keyfile)
    return;

  /* Audio sink.  */
  char *value = g_key_file_get_string (keyfile, GROUP, KEYSINK, NULL);
  if (value)
    {
      starling_set_sink (st, value);
      g_free (value);
    }

  /* Random mode.  */
  starling_random_set (st, g_key_file_get_boolean (keyfile,
						   GROUP, KEYRANDOM, NULL));

#ifdef HAVE_VOLUME_BUTTON
  /* Volume mode.  */
  double vol = 0.8;
  if (g_key_file_has_key (keyfile, GROUP, KEYVOLUME, NULL))
    vol = g_key_file_get_double (keyfile, GROUP, KEYVOLUME, NULL);
  gtk_scale_button_set_value (GTK_SCALE_BUTTON (st->volume_button), vol);
#endif

  /* Lastfm user data.  */
  if (g_key_file_has_key (keyfile, GROUP, KEYLFMUSER, NULL))
    {
      value = g_key_file_get_string (keyfile, GROUP, KEYLFMUSER, NULL);
      gtk_entry_set_text (GTK_ENTRY (st->webuser_entry), value);
      g_free (value);
    }
    
  if (g_key_file_has_key (keyfile, GROUP, KEYLFMPASSWD, NULL))
    {
      value = g_key_file_get_string (keyfile, GROUP, KEYLFMPASSWD, NULL);
      gtk_entry_set_text (GTK_ENTRY (st->webpasswd_entry), value);
      g_free (value);
    }

  if (g_key_file_has_key (keyfile, GROUP, KEYLFMAUTOSUBMIT, NULL))
    {
      int i = g_key_file_get_integer (keyfile,
				      GROUP,
				      KEYLFMAUTOSUBMIT, NULL);
      gtk_spin_button_set_value (st->lastfm_autosubmit,
				 (double) i);
    }


  /* Window size.  */
  int w = -1;
  int h = -1;
  if (g_key_file_has_key (keyfile, GROUP, KEY_WIDTH, NULL))
    {
      value = g_key_file_get_string (keyfile, GROUP, KEY_WIDTH, NULL);
      if (value)
	w = MAX (100, atoi (value));
      g_free (value);
    }
  if (g_key_file_has_key (keyfile, GROUP, KEY_HEIGHT, NULL))
    {
      value = g_key_file_get_string (keyfile, GROUP, KEY_HEIGHT, NULL);
      if (value)
	h = MAX (100, atoi (value));
      g_free (value);
    }
  gtk_window_set_default_size (GTK_WINDOW (st->window), w, h);

  /* Current directory.  */
  value = g_key_file_get_string (keyfile, GROUP, KEYLASTPATH, NULL);
  if (value && *value)
    st->fs_last_path = value;
  else
    {
      g_free (value);
      st->fs_last_path = NULL;
    }


  /* First get the current song's position.  */
  st->pending_seek = g_key_file_get_integer (keyfile,
					     GROUP,
					     KEY_LOADED_SONG_POSITION, NULL);

  struct deserialize_bottom_half *bh = calloc (sizeof (*bh), 1);
  bh->st = st;

  /* And then load the current song.  */
  bh->loaded_song = g_key_file_get_integer (keyfile,
					    GROUP, KEY_LOADED_SONG, NULL);

  /* The current play list.  */
  bh->playlist = g_key_file_get_string (keyfile,
					GROUP, KEY_CURRENT_PLAYLIST, NULL);

  /* Search text.  We set the constraint and only later set the text
     entry as the attached signal handler will save the selection,
     which is not what we want.  */
  bh->search_enabled = true;
  if (g_key_file_has_key (keyfile, GROUP, KEY_SEARCH_ENABLED, NULL))
    bh->search_enabled
      = g_key_file_get_boolean (keyfile, GROUP, KEY_SEARCH_ENABLED, NULL);
  bh->search_terms
    = g_key_file_get_string (keyfile, GROUP, KEY_SEARCH_TEXT, NULL);
  if (bh->search_enabled)
    search_text_gen (st, bh->search_terms);

  /* Search history.  */
  {
    char *defaults[] = { "played:<24h",
			 "rating:>=4",
			 "rating:>=4 and played:>3D",
			 "added:<1W and play-count:=0",
			 "(rating:=0 or rating:>=2)"
    };

    gtk_list_store_clear (st->searches);

    gsize length = 0;
    char **values = g_key_file_get_string_list (keyfile,
						GROUP, KEY_SEARCHES,
						&length, NULL);

    int count = 0;
    GtkTreeIter iter;

    /* The format is: `search(time stamp)'.  */
    int i;
    for (i = 0; i < length; i ++)
      {
	char *ts = NULL;
	unsigned int ts_val = 0;
	if (values[i][strlen (values[i]) - 1] == ')'
	    && (ts = strrchr (values[i], '(')))
	  {
	    *ts = 0;
	    ts ++;
	    ts_val = (unsigned int) atoll (ts);
	  }

	int j;
	for (j = 0; j < sizeof (defaults) / sizeof (defaults[0]); j ++)
	  if (defaults[j] && strcmp (values[i], defaults[j]) == 0)
	    defaults[j] = NULL;

	gtk_list_store_insert_with_values (st->searches, &iter, count ++,
					   0, values[i],
					   1, ts_val, -1);
      }
    g_strfreev (values);

    for (i = 0; i < sizeof (defaults) / sizeof (defaults[0]); i ++)
      if (defaults[i])
	gtk_list_store_insert_with_values (st->searches, &iter, count,
					   0, defaults[i],
					   1, 0, -1);
  }

  /* The play lists' config.  */
  {
    gsize length = 0;
    char **values = g_key_file_get_string_list (keyfile,
						GROUP, KEY_PLAYLISTS_CONFIG,
						&length, NULL);

    int i = 0;
    while (i < length)
      {
	char *playlist = values[i ++];
	char *position = i < length ? values[i ++] : NULL;
	char *selection = i < length ? values[i ++] : NULL;

	if ((! position || !*position) && (! selection || !*selection))
	  continue;

	struct play_list_view_config *conf
	  = g_hash_table_lookup (st->playlists_conf, playlist);
	if (! conf)
	  {
	    conf = g_malloc (sizeof (*conf));
	    memset (conf, 0, sizeof (*conf));
	    g_hash_table_insert (st->playlists_conf,
				 g_strdup (playlist), conf);
	  }

	conf->position = strtod (position, NULL);

	char *tok;
	for (tok = strtok (selection, ","); tok; tok = strtok (NULL, ","))
	  if (*tok)
	    conf->selected_rows
	      = g_list_append (conf->selected_rows, (gpointer) atoi (tok));
      }

    g_strfreev (values);
  }

  /* The current page.  */
  bh->page = g_key_file_get_integer (keyfile, GROUP, KEY_CURRENT_PAGE, NULL);

  /* Lyrics download.  */
  bool b = g_key_file_get_boolean (keyfile, GROUP, KEY_LYRICS_DOWNLOAD, NULL);
#ifdef HAVE_HILDON_APP_MENU
  hildon_check_button_set_active
    (HILDON_CHECK_BUTTON (st->download_lyrics), b);
#else
  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM (st->download_lyrics), b);
#endif

  /* Caption format.  */
  value = g_key_file_get_string (keyfile, GROUP, KEY_CAPTION_FORMAT, NULL);
  if (value)
    {
      if (st->caption)
	caption_free (st->caption);
      st->caption_format = value;
      st->caption = caption_create (value);
    }

  g_key_file_free (keyfile);

  gtk_idle_add (deserialize_bottom_half, bh);  
}

static void play_list_state_save (Starling *st);

static void
serialize (Starling *st)
{
  /* Load the current settings and the overwrite them.  */
  GKeyFile *keyfile = config_file_load ();

  /* Current directory.  */
  if (st->fs_last_path)
    g_key_file_set_string (keyfile, GROUP, KEYLASTPATH,
			   st->fs_last_path);

  /* Random mode.  */
  g_key_file_set_boolean (keyfile, GROUP, KEYRANDOM,
			  starling_random (st));

#ifdef HAVE_VOLUME_BUTTON
  /* Volume.  */
  g_key_file_set_double
    (keyfile, GROUP, KEYVOLUME,
     gtk_scale_button_get_value (GTK_SCALE_BUTTON (st->volume_button)));
#endif

  /* Window size.  */
  int w, h;
  gtk_window_get_size (GTK_WINDOW (st->window), &w, &h);
  g_key_file_set_integer (keyfile, GROUP, KEY_WIDTH, w);
  g_key_file_set_integer (keyfile, GROUP, KEY_HEIGHT, h);

  /* lastfm user data.  */
  g_key_file_set_string (keyfile, GROUP, KEYLFMUSER,
			 gtk_entry_get_text (GTK_ENTRY (st->webuser_entry)));
  g_key_file_set_string (keyfile, GROUP, KEYLFMPASSWD,
			 gtk_entry_get_text (GTK_ENTRY (st->webpasswd_entry)));
  g_key_file_set_integer (keyfile, GROUP, KEYLFMAUTOSUBMIT,
			  gtk_spin_button_get_value (st->lastfm_autosubmit)); 


  /* Current song.  */
  g_key_file_set_integer (keyfile, GROUP, KEY_LOADED_SONG, st->loaded_song);

  /* And the position in that song.  */
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 pos = 0;
  player_query_position (st->player, &fmt, &pos);
  g_key_file_set_integer (keyfile, GROUP, KEY_LOADED_SONG_POSITION,
			  (int) (pos / 1e9));

  /* Play list positions.  */
  /* Save the current play list's config to the hash.  */
  play_list_state_save (st);

  int positions = 0;
  if (st->playlists_conf)
    positions = g_hash_table_size (st->playlists_conf);

  if (positions)
    {
      char *values[3 * positions];
      int i = 0;

      struct obstack paths;
      obstack_init (&paths);

      void callback (gpointer key, gpointer value, gpointer data)
      {
	values[i ++] = key;

	struct play_list_view_config *conf = value;
	values[i ++] = g_strdup_printf ("%d", conf->position);

	GList *l;
	for (l = conf->selected_rows; l; l = l->next)
	  {
	    if (l != conf->selected_rows)
	      obstack_1grow (&paths, ',');

	    obstack_printf (&paths, "%d", (int) l->data);
	  }
	obstack_1grow (&paths, 0);

	values[i ++] = obstack_finish (&paths);
      }
      g_hash_table_foreach (st->playlists_conf, callback, NULL);

      g_key_file_set_string_list (keyfile, GROUP, KEY_PLAYLISTS_CONFIG,
				  (void *) values, 3 * positions);

      for (i = 1; i < 3 * positions; i += 3)
	g_free (values[i]);

      obstack_free (&paths, NULL);
    }
  else
    g_key_file_remove_key (keyfile, GROUP, KEY_PLAYLISTS_CONFIG, NULL);

  /* The current play list.  */
  char *value = play_list_selector_get_active (st);
  printf ("Saving play list %s\n", value);
  g_key_file_set_string (keyfile, GROUP, KEY_CURRENT_PLAYLIST, value ?: "");
  g_free (value);

  /* Search text.  */
  g_key_file_set_boolean (keyfile, GROUP, KEY_SEARCH_ENABLED,
			  gtk_toggle_button_get_active (st->search_enabled));
  g_key_file_set_string (keyfile, GROUP, KEY_SEARCH_TEXT,
			 gtk_entry_get_text (GTK_ENTRY (st->search_entry)));

  /* Search history.  */
  struct obstack history;
  obstack_init (&history);
  bool have_one = false;
  gboolean callback (GtkTreeModel *model, GtkTreePath *path,
		     GtkTreeIter *iter, gpointer data)
  {
    char *val = NULL;
    int ts = 0;
    gtk_tree_model_get (model, iter, 0, &val, 1, &ts, -1);
    if (! val)
      return TRUE;

    if (have_one)
      obstack_1grow (&history, ';');
    have_one = true;

    obstack_printf (&history, "%s(%u)", val, ts);

    g_free (val);
    return FALSE;
  }
  gtk_tree_model_foreach (GTK_TREE_MODEL (st->searches), callback, NULL);

  obstack_1grow (&history, 0);
  
  g_key_file_set_string (keyfile, GROUP, KEY_SEARCHES,
			 obstack_finish (&history));
  obstack_free (&history, NULL);


#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  /* The current page.  */
  int page = gtk_notebook_get_current_page (GTK_NOTEBOOK (st->notebook));
  g_key_file_set_integer (keyfile, GROUP, KEY_CURRENT_PAGE, page);
#endif

  /* Lyrics download.  */
  g_key_file_set_boolean
    (keyfile, GROUP, KEY_LYRICS_DOWNLOAD, 
#ifdef HAVE_HILDON_APP_MENU
     hildon_check_button_get_active ((void *) st->download_lyrics)
#else
     gtk_check_menu_item_get_active ((void *) st->download_lyrics)
#endif
     );

  config_file_save (keyfile);
}

void
starling_scroll_to (Starling *st, int idx)
{
  if (idx == -1)
    {
      if (! st->loaded_song)
	return;

      idx = play_list_uid_to_index (st->library, st->loaded_song);
      if (idx == -1)
	return;
    }

  int count = play_list_count (st->library);
  if (count == 0)
    return;

  GtkTreePath *path = gtk_tree_path_new_from_indices (idx, -1);
  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (st->library_view),
				path, NULL, true, 0.33, 0.0);
  gtk_tree_path_free (path);
}

static void
play_list_state_save (Starling *st)
{
  const char *play_list = play_list_get (st->library);
  if (! play_list)
    play_list = "Library";

  struct play_list_view_config *conf
    = g_hash_table_lookup (st->playlists_conf, play_list);
  if (! conf)
    {
      conf = g_malloc (sizeof (*conf));
      memset (conf, 0, sizeof (*conf));
      g_hash_table_insert (st->playlists_conf,
			   g_strdup (play_list), conf);
    }

  GtkTreeSelection *selection
    = gtk_tree_view_get_selection (GTK_TREE_VIEW (st->library_view));


  /* Figure out the first song that is visible and selected.  */
  conf->position = -1;
  GtkTreePath *top_path;
  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (st->library_view),
				     1, 1, &top_path, NULL, NULL, NULL))
    {
      gint *indices = gtk_tree_path_get_indices (top_path);
      int top_idx = indices[0];
      gtk_tree_path_free (top_path);

      GtkTreePath *bottom_path;
      int bottom_idx;
      if (gtk_tree_view_get_path_at_pos
	  (GTK_TREE_VIEW (st->library_view),
	   1, GTK_WIDGET (st->library_view)->allocation.height - 1,
	   &bottom_path, NULL, NULL, NULL))
	{
	  gint *indices = gtk_tree_path_get_indices (bottom_path);
	  bottom_idx = indices[0];
	  gtk_tree_path_free (bottom_path);
	}
      else
	bottom_idx = play_list_count (st->library);

      int i;
      for (i = top_idx; i <= bottom_idx; i ++)
	{
	  GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
	  if (gtk_tree_selection_path_is_selected (selection, path))
	    {
	      gtk_tree_path_free (path);
	      break;
	    }
	  gtk_tree_path_free (path);
	}
      if (i == bottom_idx + 1)
	/* Nothing selected.  Take entry at the top third.  */
	i = top_idx + ((bottom_idx - top_idx) / 3);

      conf->position = play_list_index_to_uid (st->library, i);
    }

  /* Now get the current selection.  */
  if (conf->selected_rows)
    g_list_free (conf->selected_rows);
  conf->selected_rows = NULL;

  void callback (GtkTreeModel *model,
		 GtkTreePath *path, GtkTreeIter *iter,
		 gpointer data)
  {
    gint *indices = gtk_tree_path_get_indices (path);
    int uid = play_list_index_to_uid (st->library, indices[0]);
    conf->selected_rows = g_list_append (conf->selected_rows,
					 (gpointer) uid);
  }
  gtk_tree_selection_selected_foreach (selection, callback, NULL);
}

static void
play_list_state_restore (Starling *st)
{
  const char *play_list = play_list_get (st->library);
  if (! play_list)
    play_list = "Library";

  /* Restore the position and the selected rows.  */
  struct play_list_view_config *conf
    = g_hash_table_lookup (st->playlists_conf, play_list);
  if (conf)
    {
      if (conf->position)
	{
	  int idx;

	  if (conf->position == -1)
	    idx = -1;
	  else
	    idx = play_list_uid_to_index (st->library, conf->position);

	  starling_scroll_to (st, idx);
	}

      /* Restore the current selection.  */
      GtkTreeSelection *selection
	= gtk_tree_view_get_selection (GTK_TREE_VIEW (st->library_view));

      gtk_tree_selection_unselect_all (selection);
      GList *l;
      GList *n = conf->selected_rows;
      while ((l = n))
	{
	  /* Grab the next pointer in case we remove L.  */
	  n = l->next;

	  int idx = play_list_uid_to_index (st->library, (int) l->data);
	  if (idx == -1)
	    {
	      conf->selected_rows
		= g_list_delete_link (conf->selected_rows, l);
	      continue;
	    }

	  GtkTreePath *path = gtk_tree_path_new_from_indices (idx, -1);
	  gtk_tree_selection_select_path (selection, path);
	  gtk_tree_path_free (path);
	}
    }
}

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
static gboolean
play_list_combo_set_library (gpointer user_data)
{
  Starling *st = user_data;

  return FALSE;
}
#endif

struct play_list_selector_changed
{
  Starling *st;
  bool initial_restore;
  char *play_list;
};

static bool
play_list_selector_changed_flush (struct play_list_selector_changed *info)
{
  Starling *st = info->st;
  char *play_list = info->play_list;
  bool initial_restore = info->initial_restore;
  g_free (info);

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  if (! play_list)
    /* Currently the combo box is set to the default value, i.e.,
       "" but we really want it to be set to "Library".  If we set
       it here, we end up getting a seg fault (likely due to other
       functions not be reentrant--setting would cause the
       "changed" signal to be emitted again but it is currently
       being emitted).  */
    {
      g_signal_handler_block (st->play_list_selector,
			      st->play_list_selector_changed_signal);
      gtk_combo_box_set_active (GTK_COMBO_BOX (st->play_list_selector), 0);
      g_signal_handler_unblock (st->play_list_selector,
				st->play_list_selector_changed_signal);
    }
#endif

  /* Save the current position and selection.  */
  if (! initial_restore)
    play_list_state_save (st);

  play_list_set (st->library,
		 ! play_list || strcmp (play_list, "Library") == 0
		 ? NULL : play_list);


  play_list_state_restore (st);

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  update_library_count (st);
#endif

  /* Rebuild the alpha seek bar.  */
  playlist_alpha_seek_build_queue (st);

  g_free (play_list);

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  gtk_widget_hide (st->library_view_loading_label);
  gtk_widget_show (st->library_view_window);
#endif

  return FALSE;
}

static void
play_list_selector_changed_to (Starling *st,
			       const char *play_list,
			       gboolean initial_restore)
{
  const char *old = play_list_get (st->library);
  if (! old)
    old = "Library";

  if (initial_restore || ! play_list || strcmp (old, play_list) != 0)
    {
      struct play_list_selector_changed *info = g_malloc (sizeof (*info));

      info->st = st;
      info->play_list = g_strdup (play_list);
      info->initial_restore = initial_restore;

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
      gtk_widget_hide (GTK_WIDGET (st->playlist_alpha_seek));
      gtk_widget_hide (st->library_view_window);
      gtk_widget_show (st->library_view_loading_label);
#endif

      gtk_idle_add (play_list_selector_changed_flush, info);  
    }

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  gtk_widget_show (st->window);
#endif
}

static void
play_list_selector_changed (Starling *st,
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
			    int column,
#endif
			    gpointer user_data)
{
  char *play_list = play_list_selector_get_active (st);
  play_list_selector_changed_to (st, play_list, false);
  g_free (play_list);
}

static void
add_cb (GtkWidget *w, Starling *st, GtkFileChooserAction action)
{
  GtkWidget *fc;

#if HAVE_HILDON
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
        music_db_add_recursive (st->db, file, &error);
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
add_file_cb (Starling *st, GtkWidget *w)
{
  add_cb (w, st, GTK_FILE_CHOOSER_ACTION_OPEN);
}

static void
add_directory_cb (Starling *st, GtkWidget *w)
{
  add_cb (w, st, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
}


void
starling_come_to_front (Starling *st)
{
  gtk_window_present (GTK_WINDOW (st->window));
}

void
starling_quit (Starling *st)
{
  /* Make the application appear to close faster...  */
  gtk_widget_hide (GTK_WIDGET (st->window));
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  gtk_widget_hide (GTK_WIDGET (st->play_list_selector_view.window));
  gtk_widget_hide (GTK_WIDGET (st->lyrics_tab));
  gtk_widget_hide (GTK_WIDGET (st->lastfm_tab));
#endif

  serialize (st);
  gtk_main_quit();
}

static gboolean
window_x (Starling *st, GdkEvent *event, GtkWidget *w)
{
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  if (w != GTK_WIDGET (st->play_list_selector_view.window))
    gtk_widget_hide (w);
  else
#endif
    starling_quit (st);

  /* Don't propagate it.  */
  return TRUE;
}

static void
set_random_cb (Starling *st, GtkWidget *w)
{
  serialize (st);
}

static void
remove_cb (Starling *st, GtkWidget *w)
{
  /* We need to remove them in reverse numerical order.  Consider: 1
     and 3 are selected.  We remove the track at index 1 and then 3.
     After removing the track at index 1, the entry referred to be
     index 3 is different!  */

  GtkWidget *view = NULL;
  PlayList *pl;
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  view = st->library_view;
  pl = st->library;
#else
  switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (st->notebook)))
    {
    case 0:
      view = st->library_view;
      pl = st->library;
      break;
    case 1:
      view = st->queue_view;
      pl = st->queue;
      break;
    }

  if (! view)
    return;
#endif

  GtkTreeSelection *selection
    = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  GQueue *selected = g_queue_new ();

  void callback (GtkTreeModel *model,
		 GtkTreePath *path, GtkTreeIter *iter, gpointer data)
  {
    gint *indices = gtk_tree_path_get_indices (path);
    g_queue_push_head (selected, (gpointer) indices[0]);
  }
  gtk_tree_selection_selected_foreach (selection, callback, NULL);


  while (! g_queue_is_empty (selected))
    {
      int idx = (int) g_queue_pop_head (selected);
      play_list_remove (pl, idx);
    }

  g_queue_free (selected);

  gtk_tree_selection_unselect_all (selection);
}

static void
clear_cb (Starling *st, GtkWidget *w)
{
#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (st->notebook)))
    {
    case 0:
      {
#endif
	char *text = play_list_selector_get_active (st);
	if (strcmp (text, "Library") == 0)
	  {
	    player_pause (st->player);
	    music_db_clear (st->db);
	  }
	else
	  music_db_play_list_clear (st->db, text);

	if (strcmp (text, "Library") == 0)
	  starling_load (st, 0);

	g_free (text);

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
	break;
      }
    case 1:
      music_db_play_list_clear (st->db, "queue");
      break;
    }
#endif
}

#ifdef HAVE_TOGGLE_FULLSCREEN
static void
toggle_fullscreen (gpointer user_data, GtkWidget *menuitem)
{
# if HAVE_HILDON_VERSION == 0
  hildon_appview_set_fullscreen (HILDON_APPVIEW (user_data),
				 gtk_check_menu_item_get_active (menuitem));
# else
  if (GTK_IS_CHECK_MENU_ITEM (menuitem)
      ? gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))
      : hildon_check_button_get_active (HILDON_CHECK_BUTTON (menuitem)))
    gtk_window_fullscreen(GTK_WINDOW(user_data));
  else
    gtk_window_unfullscreen(GTK_WINDOW(user_data));
# endif
}
#endif

static void
change_caption_format (gpointer user_data, GtkMenuItem *menuitem)
{
  Starling *st = user_data;

#define CAPTION_FMT_DEFAULT "%u?!(artist - title (album track #) m:ss)%a?(%.50a - )%t?(%.50t)(%.-90u)%A?( (%A%T?( %T)))(%T?( %T))%d?( %m:%02s)"

  const char *defaults[] = {
    CAPTION_FMT_DEFAULT,
    "%u?!(artist - title or source)%a?(%.50a%t?( - )(%u?( - )))%t?(%.50t)(%.-90u)",
  };

  GKeyFile *keyfile = config_file_load ();
  char *caption = NULL;
  char **history = NULL;
  unsigned int history_len;
  if (keyfile)
    {
      caption = g_key_file_get_string (keyfile, GROUP,
				       KEY_CAPTION_FORMAT, NULL);
      history = g_key_file_get_string_list (keyfile, GROUP,
					    KEY_CAPTION_FORMAT_HISTORY,
					    &history_len, NULL);
    }

  GtkWidget *dialog = gtk_dialog_new_with_buttons
    (_("New caption format"),
     GTK_WINDOW (st->window), GTK_DIALOG_DESTROY_WITH_PARENT,
     GTK_STOCK_OK, GTK_RESPONSE_OK,
     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
     NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				   GTK_RESPONSE_OK);
  GtkWidget *label = gtk_label_new ("New caption format:");
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);

  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  label = gtk_label_new (_(" %a - artist\n"
			   " %p - performer\n"
			   " %A - album\n"
			   " %t - title\n"
			   " %T - track\n"
			   " %v - volume\n"
			   " %V - volume count\n"
			   " %D - date"));
  gtk_container_add (GTK_CONTAINER (hbox), label);

  label = gtk_label_new (_(" %u - URI\n"
			   " %g - genre\n"
			   " %r - rating\n"
			   " %d - duration\n"
			   " %s - seconds\n"
			   " %m - minutes\n"
			   " %c - play count\n"
			   ""));
  gtk_container_add (GTK_CONTAINER (hbox), label);

  label = gtk_label_new (
    _(" %[width][.][precision]c - width and precision, negative right aligns\n"
      " %X?(true clause)(fault clase) - conditional"));
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);

  GtkWidget *combo = gtk_combo_box_entry_new_text ();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), combo);

  /* Load it with the history and the defaults.  */
  if (caption)
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), caption); 

  int i;
  for (i = 0; history && history[i]; i ++)
    if (! caption || strcmp (caption, history[i]) != 0)
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo), history[i]); 

  /* Add the defaults, but only those that are not already in the
     history.  */
  for (i = 0; i < sizeof (defaults) / sizeof (defaults[0]); i ++)
    {
      bool in_history = false;
      int j;
      for (j = 0; history && history[j]; j ++)
	if (strcmp (history[j], defaults[i]) == 0)
	  {
	    in_history = true;
	    break;
	  }

      if (! in_history && (! caption || strcmp (caption, defaults[i]) != 0))
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), defaults[i]); 
    }

  g_free (caption);

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);


  GtkWidget *entry = gtk_bin_get_child (GTK_BIN (combo));

  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_widget_show_all (dialog);

  gint result = gtk_dialog_run (GTK_DIALOG (dialog));
  switch (result)
    {
    case GTK_RESPONSE_OK:
      if (st->caption)
	caption_free (st->caption);

      st->caption_format
	= gtk_combo_box_get_active_text (GTK_COMBO_BOX (combo));
      st->caption = caption_create (st->caption_format);
      g_key_file_set_string (keyfile, GROUP, KEY_CAPTION_FORMAT,
			     st->caption_format);

      /* See if ST->CAPTION_FORMAT is already in the history.  */
      int i;
      for (i = 0; history && history[i]; i ++)
	if (strcmp (history[i], st->caption_format) == 0)
	  /* Already in the history.  */
	  break;

      if (! history || ! history[i])
	/* Not in the history.  */
	{
	  struct obstack s;
	  obstack_init (&s);
	  
	  obstack_printf (&s, "%s", st->caption_format);

	  if (history)
	    for (i = 0; history[i]; i ++)
	      {
		obstack_1grow (&s, ';');
		obstack_printf (&s, "%s", history[i]);
	      }
	  obstack_1grow (&s, 0);

	  g_key_file_set_string (keyfile, GROUP, KEY_CAPTION_FORMAT_HISTORY,
				 obstack_finish (&s));

	  obstack_free (&s, NULL);
	}

      gtk_widget_queue_draw (st->library_view);
#ifndef HAVE_HILDON_STACKABLE_WINDOWS
      gtk_widget_queue_draw (st->queue_view);
#endif

      break;

    default:
      break;
    }

  g_strfreev (history);

  gtk_widget_destroy (dialog);

  if (keyfile)
    config_file_save (keyfile);
}

#if 0
static void
save_helper_cb (GtkWidget *dialog, gint response, gpointer data)
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
            play_list_save_m3u (st->library, path);
        } else {
            GString *string = g_string_new (path);
            string = g_string_append (string, ".m3u");
            play_list_save_m3u (st->library, string->str);
            g_string_free (string, TRUE);
        }
    }

    gtk_widget_destroy (dialog);
}

static void
save_cb (GtkWidget *w, Starling *st)
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
                G_CALLBACK (save_helper_cb), st);

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), entry);
    gtk_widget_show_all (dialog);
}
#endif

static void
play (Starling *st, PlayList *pl, int uid)
{
  if (pl == st->library && st->group_by)
    gtk_check_menu_item_set_active
      (GTK_CHECK_MENU_ITEM (st->group_by_radio_widgets->data), true);

  bool from_queue;
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  char *play_list = play_list_get (st->library);
  if (play_list)
    from_queue = strcmp (play_list, "queue") == 0;
#else
  from_queue = (pl == st->queue);
#endif
  if (from_queue)
    /* When playing a song from the queue, remove it.  */
    {
      int idx = play_list_uid_to_index (pl, uid);
      music_db_play_list_remove (st->db, "queue", idx);
    }

  starling_load (st, uid);
  starling_play (st);
}

static void track_popup_menu (Starling *st, int idx, GdkEventButton *event);

static void
activated_cb (GtkTreeView *view, GtkTreePath *path,
	      GtkTreeViewColumn *col, Starling *st)
{
  g_assert (gtk_tree_path_get_depth (path) == 1);

  PlayList *pl = st->library;
#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  if (view != GTK_TREE_VIEW (st->library_view))
    pl = st->queue;
#endif

  if (pl == st->library && st->group_by)
    {
      gtk_check_menu_item_set_active
	(GTK_CHECK_MENU_ITEM (st->group_by_radio_widgets->data), true);
      return;
    }

  gint *pos = gtk_tree_path_get_indices (path);
  int idx = pos[0];

#if HAVE_HILDON && HAVE_HILDON_VERSION >= 202
  track_popup_menu (st, idx, NULL);
#else
  int uid = play_list_index_to_uid (pl, idx);
  play (st, pl, uid);
#endif
}

static void
next_cb (GtkWidget *w, Starling *st)
{
  starling_next (st);
  starling_scroll_to (st, -1);  
}

static void
prev_cb (GtkWidget *w, Starling *st)
{
  starling_prev (st);
  starling_scroll_to (st, -1);  
}

static void
playpause_cb (GtkWidget *w, Starling *st)
{
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (w)))
    player_unpause (st->player);
  else
    player_pause (st->player);
}

static void
rate (Starling *st)
{
  if (! st->loaded_song)
    return;

  struct music_db_info info;
  info.fields = MDB_RATING;
  info.rating = gtk_combo_box_get_active (st->rating);

  music_db_set_info (st->db, st->loaded_song, &info);
}

static void
jump_to_current (Starling *st)
{
  starling_scroll_to (st, -1);
#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  gtk_notebook_set_current_page (GTK_NOTEBOOK (st->notebook), 0);
#endif
}

/* Update the search button's state to reflect whether a constraint
   is active.  */
static void
search_bar_toggle_toggle_update (Starling *st)
{
#ifdef USE_SEARCH_BAR_TOGGLE
  bool active = gtk_toggle_button_get_active (st->search_enabled);
  if (active)
    {
      const char *text = gtk_entry_get_text (GTK_ENTRY (st->search_entry));
      if (! (text && *text))
	active = false;
    }

  if (active != (gtk_toggle_button_get_active
		 (GTK_TOGGLE_BUTTON (st->search_bar_toggle))))
    {
      g_signal_handler_block (st->search_bar_toggle,
			      st->search_bar_toggle_toggled_signal);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (st->search_bar_toggle),
				    active);
      g_signal_handler_unblock (st->search_bar_toggle,
				st->search_bar_toggle_toggled_signal);
    }
#endif
}

#ifdef USE_SEARCH_BAR_TOGGLE
static void
search_bar_toggle_toggled_cb (Starling *st, GtkToggleButton *search_toggle)
{
  bool visible = false;
  g_object_get (G_OBJECT (st->search_bar), "visible", &visible, NULL);
  if (visible)
    gtk_widget_hide (st->search_bar);
  else
    {
      gtk_widget_show (st->search_bar);
      gtk_editable_select_region (GTK_EDITABLE (st->search_entry), 0, -1);
      gtk_widget_grab_focus (GTK_WIDGET (st->search_entry));
    }

  search_bar_toggle_toggle_update (st);
}
#endif

static int search_text_save_source;

static int
search_text_save (Starling *st)
{
  search_text_save_source = 0;

  const char *search = gtk_entry_get_text (GTK_ENTRY (st->search_entry));

  gboolean callback (GtkTreeModel *model, GtkTreePath *path,
		     GtkTreeIter *iter, gpointer data)
  {
    char *val = NULL;
    gtk_tree_model_get (model, iter, 0, &val, -1);
    if (val && strcasecmp (search, val) == 0)
      /* Already in the history.  Update time stamp.  */
      {
	gtk_list_store_remove (st->searches, iter);
	g_free (val);
	return TRUE;
      }
    g_free (val);

    return FALSE;
  }
  gtk_tree_model_foreach (GTK_TREE_MODEL (st->searches), callback, NULL);

  GtkTreeIter iter;
  gtk_list_store_insert_with_values (st->searches, &iter, 0,
				     0, search,
				     1, (unsigned int) time (NULL), -1);

  return FALSE;
}

static void
search_text_gen (Starling *st, const char *text)
{
  if (search_text_save_source)
    {
      g_source_remove (search_text_save_source);
      search_text_save_source = 0;
    }

  if (! text || ! *text)
    {
      play_list_constrain (st->library, NULL);
      return;
    }

  char *result;
  void *state = NULL;
  if (yyparse (text, &result, &state) == 0)
    {
      play_list_constrain (st->library, result);
      g_free (result);

      /* Save the search in the history in 60 seconds, if it hasn't
	 changed.  */
      search_text_save_source
	= g_timeout_add (60 * 1000, (GSourceFunc) search_text_save, st);
    }
}

/* Regenerate the alpha scroll bar accordingly.  */
static bool
playlist_alpha_seek_build (Starling *st)
{
  st->playlist_alpha_seek_timer_source = 0;

  const char *play_list = play_list_get (st->library);
  if (play_list && strcmp (play_list, "Library") != 0)
    {
      gtk_widget_hide (GTK_WIDGET (st->playlist_alpha_seek));
      return FALSE;
    }
  else
    gtk_widget_show (GTK_WIDGET (st->playlist_alpha_seek));

  /* First, remove any labels.  */
  int label_height = 0;
  int labels = 0;
  bool first = true;
  void callback (GtkWidget *widget, gpointer ignore)
  {
    g_assert (GTK_IS_LABEL (widget));

    labels ++;

    if (first)
      label_height = widget->allocation.height;

    if (label_height > 1)
      gtk_widget_destroy (widget);

    first = false;
  }
  gtk_container_foreach (GTK_CONTAINER (st->playlist_alpha_seek),
			 callback, NULL);
  if (label_height == 1)
    /* The size has not settled yet.  Note: when stable, we always add
       at least two labels and request a minimum height of 2
       pixels.  */
    return TRUE;

  int space = GTK_WIDGET (st->playlist_alpha_seek)->allocation.height;

  GtkLabel *display (int idx, int rubber)
  {
    /* Load the entry at IDX.  */
    int uid = play_list_index_to_uid (st->library, idx);
    struct music_db_info info;
    memset (&info, 0, sizeof (info));
    /* We sort firstly by artist.  */
    info.fields = MDB_ARTIST;
    bool succ = music_db_get_info (st->db, uid, &info);
    g_assert (succ);

    /* Create the label.  */
    char buffer[2] = "?";
    if (info.artist)
      {
	strncpy (buffer, info.artist, sizeof (buffer) - 1);
	buffer[0] = toupper (buffer[0]);
	int i;
	for (i = 1; i < sizeof (buffer) - 1; i ++)
	  buffer[i] = tolower (buffer[i]);
	buffer[sizeof (buffer) - 1] = 0;
      }
    GtkLabel *label = GTK_LABEL (gtk_label_new (buffer));

    free (info.artist);

    /* Add it to the container.  */
    gtk_box_pack_start (GTK_BOX (st->playlist_alpha_seek), GTK_WIDGET (label),
			rubber, rubber, 0);

    int w = -1;
#if HAVE_MAEMO && HAVE_MAEMO_VERSION >= 500
    w = HILDON_ICON_PIXEL_SIZE_FINGER;
#endif

    if (rubber)
      gtk_widget_set_size_request (GTK_WIDGET (label), w, 2);
    gtk_widget_show (GTK_WIDGET (label));

    return label;
  }

  int count = play_list_count (st->library);
  if (count > 0)
    {
      if (labels != 1)
	/* We need to get the proper height of the label.  Insert a label,
	   get it to display and then actually build the bar.  */
	{
	  /* Don't allow the label to stretch.  This way we can get its
	     natural height.  */
	  GtkLabel *l = display (0, FALSE);
	  g_signal_connect_swapped
	    (G_OBJECT (l), "size-allocate",
	     G_CALLBACK (playlist_alpha_seek_build_queue),
	     st);
	  return FALSE;
	}

      if (count > 1)
	{
	  display (0, TRUE);
	  g_assert (label_height);

	  int labels = (space - label_height)
	    / (label_height + label_height / 10);

	  if (labels > 26)
	    labels = 26;
	  if (labels > count)
	    labels = count;
	  if (labels < 1)
	    /* We display at least 2.  */
	    labels = 1;

	  int i;
	  for (i = 1; i <= labels; i ++)
	    display ((i * count / labels) - 1, TRUE);
	}
    }

  st->playlist_alpha_current_height = 
    GTK_WIDGET (st->playlist_alpha_seek)->allocation.height;

  return FALSE;
}

static void
playlist_alpha_seek_build_queue (Starling *st)
{
  if (st->playlist_alpha_seek_timer_source)
    g_source_remove (st->playlist_alpha_seek_timer_source);
  st->playlist_alpha_seek_timer_source
    = g_timeout_add (200, (GSourceFunc) playlist_alpha_seek_build, st);
}

static void
playlist_alpha_seek_build_queue_size_allocate (Starling *st)
{
  if (GTK_WIDGET (st->playlist_alpha_seek)->allocation.height
      != st->playlist_alpha_current_height)
    /* Ignore non-height changes.  */
    playlist_alpha_seek_build_queue (st);
}

static gboolean
playlist_alpha_seek_clicked (Starling *st,
			     GdkEventButton *event, GtkWidget *widget)
{
  if (event->button != 1)
    /* Not button 1.  Propagate further.  */
    return FALSE;
  if (event->type != GDK_BUTTON_RELEASE)
    /* Not a release event.  Propagate further.  */
    return FALSE;

  int count = play_list_count (st->library);
  if (count == 0)
    return TRUE;

  int height = GTK_WIDGET (st->playlist_alpha_seek)->allocation.height;
  int idx = event->y * count / height;
  starling_scroll_to (st, idx);

  return TRUE;
}

static int regen_source;

static int
search_text_regen (gpointer data)
{
  Starling *st = data;

  regen_source = 0;

  const char *text = gtk_entry_get_text (GTK_ENTRY (st->search_entry));

  if ((! gtk_toggle_button_get_active (st->search_enabled)
       || ! text || !*text)
      && ! play_list_constraint_get (st->library))
    /* No change.  */
    return FALSE;

  play_list_state_save (st);

  if (gtk_toggle_button_get_active (st->search_enabled))
    search_text_gen (st, text);
  else
    search_text_gen (st, NULL);

  play_list_state_restore (st);

  playlist_alpha_seek_build_queue (st);

  return FALSE;
}

static void
search_text_changed (Starling *st)
{
  /* When the user changes the source text, we don't update
     immediately but try to group updates.  The assumption is that
     users will type a few characters.  Thus, we wait until there is
     no activity for 200ms before triggering the update.  */
  if (regen_source)
    g_source_remove (regen_source);
  regen_source = g_timeout_add (200, search_text_regen, st);

  search_bar_toggle_toggle_update (st);
}

static void
search_text_clear (Starling *st)
{
  gtk_entry_set_text (GTK_ENTRY (st->search_entry), "");
}


static void
group_by (Starling *st, enum mdb_fields scope)
{
  if (scope == st->group_by)
    return;

  play_list_state_save (st);

  st->group_by = scope;
  if (st->caption)
    {
      caption_free (st->caption);
      st->caption = NULL;
    }
  play_list_group_by (st->library, scope);

  play_list_state_restore (st);

  playlist_alpha_seek_build_queue (st);

  const char *id = NULL;
  switch (scope)
    {
    case 0:
      id = GTK_STOCK_ZOOM_100;
      break;
    case MDB_ARTIST:
      id = GTK_STOCK_ZOOM_IN;
      break;
    case MDB_ALBUM:
      id = GTK_STOCK_ZOOM_FIT;
      break;
    default:
      g_assert (! "Impossible value");
      return;
    }
  gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (st->group_by_button), id);
}

static void
group_by_cb (Starling *st, GtkRadioMenuItem *radiomenuitem)
{
  int i;
  GSList *n;
  for (i = 0, n = st->group_by_radio_widgets;
       ! gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (n->data));
       i ++, n = n->next)
    ;

  enum mdb_fields scope[] = { 0, MDB_ARTIST, MDB_ALBUM };
  g_assert (i < sizeof (scope) / sizeof (scope[0]));

  group_by (st, scope[i]);
}

static void
group_by_button_clicked_cb (Starling *st, GtkToolButton *toolbutton)
{
  enum mdb_fields scopes[] = { 0, MDB_ARTIST, MDB_ALBUM };
  GSList *n;
  int i;
  for (i = 0, n = st->group_by_radio_widgets;
       i < sizeof (scopes) / sizeof (scopes[0]); i ++, n = n->next)
    if (st->group_by == scopes[i])
      break;
  if (n->next)
    n = n->next;
  else
    n = st->group_by_radio_widgets;

  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (n->data), true);
}

static void
set_position_text (Starling *st, int seconds)
{
  char buffer[16];
  snprintf (buffer, sizeof (buffer), "%d:%02d", seconds / 60, seconds % 60);
  buffer[sizeof (buffer) - 1] = 0;

  gtk_label_set_text (GTK_LABEL (st->position), buffer);
}

static gboolean
user_seeking_update (Starling *st)
{
  if (st->current_length < 0)
    {
      GstFormat fmt = GST_FORMAT_TIME;
      player_query_duration (st->player, &fmt, &st->current_length, -1);
    }

  gfloat percent = gtk_range_get_value (GTK_RANGE (st->position_slider));

  set_position_text (st,
		     ((gint64) (((gfloat) st->current_length * percent)
				/ 100))
		     / 1e9);

  return TRUE;
}

static void
try_seek (Starling *st)
{
  if (st->pending_seek >= 0)
    {
      if (st->current_length < 0)
	{
	  GstFormat fmt = GST_FORMAT_TIME;
	  if (! player_query_duration (st->player,
				       &fmt, &st->current_length, -1))
	    {
	      st->current_length = -1;
	      g_debug ("%s: query duration failed", __FUNCTION__);
	    }
	}

      if (st->current_length >= 0)
	/* Only if the song has actually loaded.  */
	{
	  if (st->pending_seek * 1e9 > st->current_length)
	    /* Don't seek beyond the end of the track.  */
	    {
	      g_debug ("%s: seek %d > length %d",
		       __FUNCTION__,
		       st->pending_seek, (int) (st->current_length / 1e9));
	      st->pending_seek = (st->current_length / 1e9) - 1;
	    }

	  if (player_seek (st->player,
			   GST_FORMAT_TIME, (gint64) st->pending_seek * 1e9))
	    st->pending_seek = -1;
	  else
	    g_debug ("%s: seeking failed.  Will try again.", __FUNCTION__);
	}
    }
}

static gboolean
position_slider_button_released_cb (GtkRange *range, GdkEventButton *event,
				    Starling *st)
{
  /* At least on Maemo, we can get a released without having first
     gotten a pressed signal.  */
  if (! st->user_seeking)
    return FALSE;

  g_source_remove (st->user_seeking);
  st->user_seeking = 0;

  if (st->current_length < 0)
    return FALSE;

  gint percent = gtk_range_get_value (range);
  st->pending_seek = (st->current_length / 100) * percent / 1e9;
  try_seek (st);

  return FALSE;
}

static gboolean
position_slider_button_pressed_cb (GtkRange *range, GdkEventButton *event,
				   Starling *st)
{
  if (! st->user_seeking)
    st->user_seeking
      = g_timeout_add (100, (GSourceFunc) user_seeking_update, st);

  return FALSE;
}

static gboolean
position_update (Starling *st)
{
  if (st->user_seeking)
    /* User is sliding, don't change the position.  */
    return TRUE;

  GstFormat fmt = GST_FORMAT_TIME;
  gint64 position;
  gfloat percent;
  gint total_seconds = 0;
  gint position_seconds = 0;

  /* Set position text.  */
  if (! player_query_position (st->player, &fmt, &position))
    {
      g_debug ("%s: Failed to get position", __FUNCTION__);

      if (st->pending_seek >= 0)
	set_position_text (st, st->pending_seek);
    }
  else
    {
      percent = (((gfloat) (position)) / st->current_length) * 100; 

      total_seconds = st->current_length / 1e9;
      position_seconds = (total_seconds / 100.0) * percent;

      set_position_text (st, position_seconds);
      gtk_range_set_value (GTK_RANGE (st->position_slider), percent);
    }

  /* Set duration text.  */
  if (st->current_length < 0)
    {
      if (! player_query_duration (st->player,
				   &fmt, &st->current_length, -1))
	{
	  g_debug ("%s: Failed to get duration", __FUNCTION__);
	  return TRUE;
	}

      total_seconds = st->current_length / 1e9;
      char *d = g_strdup_printf ("%d:%02d",
				 total_seconds / 60, total_seconds % 60);
      gtk_label_set_text (GTK_LABEL (st->duration), d);
      g_free (d);
    }

  /* Update some statistics.  */
  if (G_UNLIKELY (!st->enqueued
		  && (position_seconds > (9 * total_seconds) / 10)))
    /* 90% of the track has been played.  */
    {
      if (total_seconds > 30)
	/* And the track is at least 30 seconds long.  Submit it to
	   lastfm.  */
	{
	  struct music_db_info info;
	  info.fields = MDB_ARTIST | MDB_TITLE;
	  music_db_get_info (st->db, st->loaded_song, &info);

	  lastfm_enqueue (info.artist, info.title, total_seconds);

	  g_free (info.artist);
	  g_free (info.title);
	}

      st->enqueued = TRUE;
    }

  /* Seek if required.  */
  if (st->pending_seek >= 0)
    try_seek (st);

  return TRUE;
}

#ifdef HAVE_VOLUME_BUTTON
static gboolean
volume_changed (GtkScaleButton *button, gdouble value, Starling *st)
{
  /* VALUE is betwen 0 and 1.  We need to translate this to one
     between 0 and 10.  This is not simply a question of multiplying
     by a factor: our volume control should be linear, however, the
     underlying mechanism appears to be exponential.  */

  /* Map 0 to 0.8 to 0 to 1 and 0.8 to 1.0 to 1 to 10.  */
  double t;
  if (value < 0.8)
    /* Map 0 to 0.8 -> 0 to 1.  */ 
    t = value / 0.8;
  else
    /* Map 0.8 to 1.0 -> 1 to 10.  */
    t = 1.0 + ((10.0 - 1.0) / (1.0 - 0.8)) * (value - 0.8);

  player_set_volume (st->player, t);

  return FALSE;
}
#endif

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
static void
update_queue_count (Starling *st)
{
  int count = play_list_count (st->queue);
  g_assert (count >= 0);
  static int last_count = -1;

  if (count == last_count)
    return;

  char *text = g_strdup_printf (_("Queue (%d)"), count);

  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (st->notebook),
				   st->queue_tab, text);

  g_free (text);

  last_count = count;
}

static gboolean
update_library_count_flush (gpointer user_data)
{
  Starling *st = user_data;

  assert (st->library_tab_update_source);

  int count = play_list_count (st->library);
  int total = play_list_total (st->library);

  char *playlist = play_list_selector_get_active (st);
  const char *search_text = play_list_constraint_get (st->library);
  char *text;
  if (search_text && *search_text)
    text = g_strdup_printf (_("%s (%d/%d)"),
			    playlist ?: _("Library"), count, total);
  else
    text = g_strdup_printf (_("%s (%d)"),
			    playlist ?: _("Library"), count);

  g_free (playlist);
  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (st->notebook),
				   st->library_tab, text);
  g_free (text);

  st->library_tab_update_source = 0;
  return FALSE;
}

static void
update_library_count (Starling *st)
{
  if (! st->library_tab_update_source)
    st->library_tab_update_source
      = gtk_idle_add (update_library_count_flush, st);  
}
#endif


static void
lyrics_download (Starling *st)
{
  struct music_db_info info;
  info.fields = MDB_ARTIST | MDB_TITLE;
  if (! music_db_get_info (st->db, st->loaded_song, &info))
    return;

  st->has_lyrics = TRUE;
  lyrics_display (info.artist ?: "", info.title ?: "",
		  GTK_TEXT_VIEW (st->lyrics_textview),
		  TRUE, TRUE);

  g_free (info.artist);
  g_free (info.title);
}

static void
player_state_changed (Player *pl, gpointer uid, int state, Starling *st)
{
  if (gtk_toggle_tool_button_get_active
       (GTK_TOGGLE_TOOL_BUTTON (st->playpause))
      != (GST_STATE_PLAYING == state))
    {
      g_signal_handler_block (st->playpause, st->playpause_toggled_signal_id);
      gtk_toggle_tool_button_set_active
	(GTK_TOGGLE_TOOL_BUTTON (st->playpause), state == GST_STATE_PLAYING);
      g_signal_handler_unblock (st->playpause, st->playpause_toggled_signal_id);
    }

  if (GST_STATE_PLAYING == state)
    {
      if (! st->position_update)
	st->position_update
	  = g_timeout_add (1000, (GSourceFunc) position_update, st);
    }
  else if (state == GST_STATE_PAUSED || state == GST_STATE_NULL)
    {
      if (st->position_update)
	{
	  g_source_remove (st->position_update);
	  st->position_update = 0;
	}
    }

  position_update (st);

  if (st->pending_seek >= 0
      && (state == GST_STATE_PLAYING
#if HAVE_HILDON
	  || state == GST_STATE_PAUSED
#endif
	  ))
    try_seek (st);
}

static void
lastfm_submit_cb (GtkWidget *w, Starling *st)
{
  const gchar *username;
  const gchar *passwd;

  username = gtk_entry_get_text (GTK_ENTRY (st->webuser_entry));
  passwd = gtk_entry_get_text (GTK_ENTRY (st->webpasswd_entry));

  if (! username || ! *username || !passwd || !*passwd)
    starling_error_box (_("You need to enter a username and password."));
  else
    {
      lastfm_user_data_set (username, passwd,
			    gtk_spin_button_get_value (st->lastfm_autosubmit));
      lastfm_submit ();
    }
}

static int
key_press_event (GtkWidget *widget, GdkEventKey *k, Starling *st)
{
  if ((k->state & GDK_MODIFIER_MASK) == GDK_CONTROL_MASK)
    /* Control is pressed.  */
    {
      if (k->keyval == 'l' || k->keyval == 'L')
	/* Control-L.  */
	{
	  gtk_editable_select_region (GTK_EDITABLE (st->search_entry), 0, -1);
	  gtk_widget_grab_focus (GTK_WIDGET (st->search_entry));

	  return TRUE;
	}
    }

  /* in hildon there is nothing like control, shift etc buttons */
  switch (k->keyval)
    {
#if HAVE_HILDON
    case GDK_F6:
#if 0
      /* toggle button for going full screen */
      gtk_check_menu_item_set_active
	(st->fullscreen, ! gtk_check_menu_item_get_active (st->fullscreen));
#endif
      /* Make full screen a play/pause button.  */
      gtk_toggle_tool_button_set_active
	(GTK_TOGGLE_TOOL_BUTTON (st->playpause),
	 ! gtk_toggle_tool_button_get_active
	    (GTK_TOGGLE_TOOL_BUTTON (st->playpause)));
      return TRUE;
#endif

    case GDK_F7:
      next_cb (NULL, st);
      return TRUE;
    case GDK_F8:
      prev_cb (NULL, st);
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
  Starling *st = data;

  if (G_UNLIKELY (! st->caption))
    switch (st->group_by)
      {
      case 0:
	st->caption
	  = caption_create (st->caption_format ?: CAPTION_FMT_DEFAULT);
	break;
      case MDB_ALBUM:
	st->caption = caption_create ("%a?(%.50a)(%.-90u) - %A");
	break;
      case MDB_ARTIST:
	st->caption = caption_create ("%a?(%.50a)(%.-90u)");
	break;
      default:
	g_assert (! "Unexpected value for ST->GROUP_BY");
	return;
      }


  int uid;
  gtk_tree_model_get (model, iter, PL_COL_UID, &uid, -1);

  struct music_db_info info;
  info.fields = MDB_PRESENT;
  music_db_get_info (st->db, uid, &info);

  char *text = caption_render (st->caption, st->db, uid);

  g_object_set (cell_renderer,
		"text", text,
		"weight",
		info.present ? PANGO_WEIGHT_NORMAL : PANGO_WEIGHT_NORMAL / 4,
		"cell-background-set", uid == st->loaded_song,
		NULL);

  g_free (text);
}


enum menu_op
  {
    play_song,
    add_song,
    add_album,
    add_artist,
    add_all,
    add_sel,
    add_count,
  };

struct menu_info
{
  struct obstack obstack;

  Starling *st;
  GtkWidget *menu;

  int uid;
  char *artist;
  char *album;
  GList *selection;

  int choosen_op;
};

struct menu_info_sub
{
  struct menu_info *info;
  enum menu_op op;
};

struct menu_info_sub_sub
{
  struct menu_info_sub *sub;
  char *list;
};


static void
menu_destroy (GtkWidget *widget, gpointer d)
{
  struct menu_info *info = d;
  g_free (info->artist);
  g_free (info->album);
  g_list_free (info->selection);

  obstack_free (&info->obstack, NULL);

  if (widget && widget != info->menu)
    gtk_widget_destroy (widget);

  g_free (info);
}

static void
queue_cb (GtkWidget *widget, gpointer d)
{
  struct menu_info_sub_sub *sub_sub = d;
  const char *list = sub_sub->list;
  struct menu_info_sub *sub = sub_sub->sub;
  struct menu_info *info = sub->info;
  Starling *st = info->st;

  if (strcmp (list, "new play list") == 0)
    {
      GtkWidget *dialog = gtk_dialog_new_with_buttons
	(_("New play list's name:"),
	 GTK_WINDOW (st->window), GTK_DIALOG_DESTROY_WITH_PARENT,
	 GTK_STOCK_OK, GTK_RESPONSE_OK,
	 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	 NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				       GTK_RESPONSE_OK);

      GtkWidget *entry = gtk_entry_new ();
      gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), entry);
      gtk_widget_show_all (dialog);

      gint result = gtk_dialog_run (GTK_DIALOG (dialog));
      bool ret = false;
      switch (result)
	{
	case GTK_RESPONSE_OK:
	  {
	    const char *s = gtk_entry_get_text (GTK_ENTRY (entry));
	    list = obstack_copy (&info->obstack, s, strlen (s) + 1);
	    break;
	  }

	default:
	  ret = true;
	  break;
	}

      gtk_widget_destroy (dialog);
      if (ret)
	return;
    }

  int callback (int uid, struct music_db_info *i)
  {
    music_db_play_list_enqueue (st->db, list, uid);

    return 0;
  }

  char *s;
  bool need_free = true;
  const char *constraint = play_list_constraint_get (st->library);

  switch (sub->op)
    {
    case add_song:
      music_db_play_list_enqueue (st->db, list, info->uid);
      s = NULL;
      break;

    case add_album:
      if (info->artist)
	s = sqlite_mprintf ("(artist like '%q' and album like '%q')",
			    info->artist, info->album);
      else
	s = sqlite_mprintf ("(album like '%q')",
			    info->album);
      break;

    case add_artist:
      s = sqlite_mprintf ("(artist like '%q')",
			  info->artist);
      break;

    case add_all:
      s = (char *) constraint;
      need_free = false;
      break;

    case add_sel:
      g_list_foreach (info->selection, (GFunc) callback, NULL);
      s = NULL;
      break;

    default:
      g_assert (! "Unknown op value");
      return;
    }

  if (s)
    {
      enum mdb_fields sort_order[]
	= { MDB_ARTIST, MDB_ALBUM, MDB_TRACK, MDB_TITLE, MDB_SOURCE, 0 };
      music_db_for_each (st->db, play_list_get (st->library),
			 callback, sort_order, 0, s);
      if (need_free)
	sqlite_freemem (s);
    }
}

#if HAVE_HILDON && HAVE_HILDON_VERSION >= 202
static void
track_popup_menu_cb (struct menu_info_sub *sub, HildonPickerButton *button)
{
  struct menu_info *info = sub->info;
  Starling *st = info->st;

  sub->op = info->choosen_op;
  switch (sub->op)
    {
    case play_song:
      play (st, st->library, info->uid);
      break;

    case add_song:
    case add_album:
    case add_artist:
    case add_all:
      {
	HildonTouchSelector *selector
	  = hildon_picker_button_get_selector (button);
	char *list = hildon_touch_selector_get_current_text (selector);

	struct menu_info_sub_sub sub_sub;
	memset (&sub_sub, 0, sizeof (sub_sub));
	sub_sub.sub = sub;
	sub_sub.list = list;

	g_signal_handlers_disconnect_by_func (G_OBJECT (info->menu),
					      G_CALLBACK (menu_destroy), info);

	gtk_widget_destroy (info->menu);
	info->menu = NULL;

	queue_cb (NULL, &sub_sub);

	g_free (list);
	menu_destroy (NULL, info);

	break;
      }

    default:
      g_assert (!"Bad resposne code!");
      break;
    }

  if (info->menu)
    gtk_widget_destroy (info->menu);
}

/* See comment in the add function in track_popup_menu below.  */
static void
track_popup_menu_button_clicked (struct menu_info_sub *sub,
				 GtkButton *button)
{
  sub->info->choosen_op = sub->op;

  if (sub->op == play_song)
    track_popup_menu_cb (sub, NULL);
}
#endif

static void
track_popup_menu (Starling *st, int idx, GdkEventButton *event)
{
#if HAVE_HILDON && HAVE_HILDON_VERSION >= 202
#define ITALICS_O
#define ITALICS_C
#define ELLIPSES "..."
#define WIDTHS "80"
#define WIDTH 80
#else
#define USE_MARKUP
#define ITALICS_O "<i>" 
#define ITALICS_C "</i>"
#define ELLIPSES
#define WIDTHS "20"
#define WIDTH 20
#endif

  int uid = play_list_index_to_uid (st->library, idx);

  struct music_db_info info;
  info.fields = MDB_SOURCE | MDB_ARTIST | MDB_ALBUM | MDB_TITLE;
  music_db_get_info (st->db, uid, &info);

#ifdef USE_MARKUP
  if (info.source)
    info.source = html_escape_string (info.source);
  if (info.artist)
    info.artist = html_escape_string (info.artist);
  if (info.album)
    info.album = html_escape_string (info.album);
  if (info.title)
    info.title = html_escape_string (info.title);
#endif

  struct menu_info *main_info = g_malloc (sizeof (*main_info));
  memset (main_info, 0, sizeof (*main_info));
  obstack_init (&main_info->obstack);
  main_info->st = st;
  main_info->uid = uid;
  main_info->artist = info.artist;
  main_info->album = info.album;

#if HAVE_HILDON && HAVE_HILDON_VERSION >= 202
  GtkWidget *menu = gtk_dialog_new_with_buttons
    ("" /*_("Action:")*/, GTK_WINDOW (st->window),
     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
     NULL);
  g_signal_connect (G_OBJECT (menu), "destroy",
		    G_CALLBACK (menu_destroy), main_info);

  GtkWidget *selector = hildon_touch_selector_new_text ();
  gtk_widget_show (selector);
  bool connected_selector = false;
#else
  GtkMenu *menu = GTK_MENU (gtk_menu_new ());
  g_signal_connect (G_OBJECT (menu), "selection-done",
		    G_CALLBACK (menu_destroy), main_info);

  GtkMenu *submenus[add_count];
  struct menu_info_sub *subs[add_count];
  int item_count = 0;
#endif
  main_info->menu = GTK_WIDGET (menu);

  void add (char *text, int op)
  {
    struct menu_info_sub *sub
      = obstack_alloc (&main_info->obstack, sizeof (*sub));
    sub->info = main_info;
    sub->op = op;

#if HAVE_HILDON && HAVE_HILDON_VERSION >= 202
    GtkWidget *item;
    if (op == play_song)
      {
	item = hildon_button_new
	  (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
	   HILDON_BUTTON_ARRANGEMENT_VERTICAL);
	hildon_button_set_title (HILDON_BUTTON (item), text);
      }
    else
      {
	item = hildon_picker_button_new
	  (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
	   HILDON_BUTTON_ARRANGEMENT_VERTICAL);
	hildon_button_set_title (HILDON_BUTTON (item), text);
	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (item),
					   HILDON_TOUCH_SELECTOR (selector));
	if (! connected_selector)
	  /* Since the selector is shared, we only need to add the
	     value-changed signal handler to once picker instance: it
	     doesn't matter which picker instance is used, all see the
	     selectors selection and emit a value-changed signal.  */
	  {
	    g_signal_connect_swapped (G_OBJECT (item), "value-changed",
				      (GCallback) track_popup_menu_cb, sub);
	    connected_selector = true;
	  }
      }

    /* Given the above, to figure out which picker was choosen, we
       connect to the clicked function of the button and save SUB->OP
       in INFO->CHOOSEN_OP.  */
    g_signal_connect_swapped (G_OBJECT (item), "clicked",
			      (GCallback) track_popup_menu_button_clicked, sub);
    gtk_widget_show (item);
    gtk_container_add
      (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (menu))), item);
#else
    /* Allocate the menu item.  */
    GtkWidget *item = gtk_menu_item_new_with_label ("");
    gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), text);
    gtk_widget_show (item);
    gtk_menu_attach (menu, item, 0, 1, item_count, item_count + 1);

    /* And the sub menu.  */
    submenus[item_count] = GTK_MENU (gtk_menu_new ());
    gtk_widget_show (GTK_WIDGET (submenus[item_count]));
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
			       GTK_WIDGET (submenus[item_count]));

    subs[item_count] = sub;
    item_count ++;
#endif

    g_free (text);
  }
	    

  char *str;
#if HAVE_HILDON && HAVE_HILDON_VERSION >= 202
  /* Create a "Play track" button.  */
  if (info.title)
    str = g_strdup_printf (_("Play %s%s%s"),
			     info.title,
			     info.artist ? " by " : "",
			     info.artist ?: "");
  else
    str = g_strdup_printf (_("Play %s"), info.source);
  add (str, play_song);
#endif

  /* Create a "Add this track" button.  */
  if (info.title)
    str = g_strdup_printf (_("Add track "
			     ITALICS_O"%."WIDTHS"s%s"ITALICS_C" to"ELLIPSES),
			   info.title,
			   strlen (info.title) > WIDTH ? "..." : "");
  else
    {
      char *s = info.source;
      if (strlen (s) > 40)
	s = s + strlen (s) - 37;

      str = g_strdup_printf (_("Add track "
			       ITALICS_O"%s%s"ITALICS_C" to"ELLIPSES),
			     s == info.source ? "" : "...", s);
    }
  add (str, add_song);

  /* Create a "Add this album" button.  */
  if (info.album)
    {
      str = g_strdup_printf (_("Add album "
			       ITALICS_O"%."WIDTHS"s%s"ITALICS_C" to"ELLIPSES),
			     info.album,
			     strlen (info.album) > WIDTH ? "..." : "");
      add (str, add_album);
    }

  /* Create an "Add this artist" button.  */
  if (info.artist)
    {
      str = g_strdup_printf (_("Add tracks by "
			       ITALICS_O"%."WIDTHS"s%s"ITALICS_C" to"ELLIPSES),
			     info.artist,
			     strlen (info.artist) > WIDTH ? "..." : "");
      add (str, add_artist);
    }

  /* Create an "Add tracks matching" button.  */
  if (gtk_toggle_button_get_active (st->search_enabled))
    {
      const char *search_text
	= gtk_entry_get_text (GTK_ENTRY (st->search_entry));
      if (search_text && *search_text)
	{
	  str = g_strdup_printf (_("Add tracks matching "
				   ITALICS_O"%s"ITALICS_C" to"ELLIPSES),
				 search_text);
	  add (str, add_all);
	}
    }

  /* Multi-selection is not enabled on Hildon 2.2.  */
#ifndef HAVE_HILDON_GTK_TREE_VIEW
  GtkTreeSelection *selection
    = gtk_tree_view_get_selection (GTK_TREE_VIEW (st->library_view));
  int sel_count = 0;
  GList *selection_list = NULL;
  char *first_source = NULL;
  char *first_title = NULL;

  bool first = true;
  void sel_callback (GtkTreeModel *model,
		     GtkTreePath *path, GtkTreeIter *iter, gpointer data)
  {
    gint *indices = gtk_tree_path_get_indices (path);

    int uid = play_list_index_to_uid (st->library, indices[0]);
    if (first)
      {
	struct music_db_info info;
	info.fields = MDB_SOURCE | MDB_TITLE;
	music_db_get_info (st->db, uid, &info);

	first_title = info.title;
	first_source = info.source;

	first = false;
      }

    selection_list
      = g_list_append (selection_list, (gpointer) uid);
    sel_count ++;
  }
  gtk_tree_selection_selected_foreach (selection, sel_callback, NULL);
  main_info->selection = selection_list;

  if (sel_count)
    {
      if (sel_count > 1)
	str = g_strdup_printf (_("Add %d selected tracks to"ELLIPSES),
			       sel_count);
      else
	str = g_strdup_printf (_("Add selected track "
				 ITALICS_O"%."WIDTHS"s%s"ITALICS_C
				 " to"ELLIPSES),
			       first_title ?: first_source,
			       strlen (first_title ?: first_source) > WIDTH
			       ? "..." : "");

      g_free (first_title);
      g_free (first_source);

      add (str, add_sel);
    }
#endif

  /* Populate the submenus.  On Hildon, we add popup a new note.  */

  int submenu_pos = 0;
  bool saw_queue = false;
  int callback (const char *list)
  {
    if (strcmp (list, "queue") == 0)
      {
	if (saw_queue)
	  return 0;
	else
	  saw_queue = true;
      }

#if HAVE_HILDON && HAVE_HILDON_VERSION >= 202
    hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (selector), list);
#else
    char *s = obstack_copy (&main_info->obstack, list, strlen (list) + 1);
    int i;
    for (i = 0; i < item_count; i ++)
      {
	struct menu_info_sub_sub *sub_sub
	  = obstack_alloc (&main_info->obstack, sizeof (*sub_sub));
	sub_sub->sub = subs[i];
	sub_sub->list = s;

	GtkWidget *widget = gtk_menu_item_new_with_label (list);
	g_signal_connect (G_OBJECT (widget), "activate",
			  G_CALLBACK (queue_cb), sub_sub);
	gtk_widget_show (widget);
	gtk_menu_attach (submenus[i], widget, 0, 1,
			 submenu_pos, submenu_pos + 1);
	submenu_pos ++;
      }
#endif

    return 0;
  }

  /* Add queue at the beginning of the menu.  */
  callback ("queue");
  callback ("new play list");
  music_db_play_lists_for_each (st->db, callback);

  /* Show the menu.  */
#if HAVE_HILDON && HAVE_HILDON_VERSION >= 202
  gtk_widget_show (menu);
#else
  gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
		  event ? event->button : 0,
		  event ? event->time : gtk_get_current_event_time());
#endif

  g_free (info.source);
  g_free (info.title);
}

static gboolean
library_button_press_event (GtkWidget *widget, GdkEventButton *event,
			    Starling *st)
{
  if (event->button == 3)
    {
      GtkTreePath *path;
      if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (st->library_view),
					   event->x, event->y,
					   &path, NULL, NULL, NULL))
	return FALSE;

      g_assert (gtk_tree_path_get_depth (path) == 1);

      gint *indices = gtk_tree_path_get_indices (path);
      int idx = indices[0];

      gtk_tree_path_free (path);

      track_popup_menu (st, idx, event);

      return TRUE;
    }

  return FALSE;
}

#if HAVE_HILDON
static void
library_tap_and_hold_cb (GtkWidget *widget, gpointer user_data)
{
  Starling *st = user_data;

  int x, y;
  gdk_display_get_pointer (gdk_drawable_get_display (widget->window),
			   NULL, &x, &y, NULL);

  GdkEventButton event;
  event.x = x;
  event.y = y;
  event.button = 3;
  event.time = GDK_CURRENT_TIME;

  library_button_press_event (st->library_view, &event, st);
}
#endif

static void
status_update (MusicDB *db, char *status, Starling *st)
{
  if (status)
    {
      gtk_label_set_text (st->status, status);
      gtk_widget_show (GTK_WIDGET (st->status));
    }
  else
    gtk_widget_hide (GTK_WIDGET (st->status));
}

Starling *
starling_run (void)
{
  Starling *st = calloc (sizeof (*st), 1);
  st->current_length = -1;
  st->pending_seek = -1;
  st->playlists_conf = g_hash_table_new (g_str_hash, g_str_equal);
    
  const char *home = g_get_home_dir ();
  char *dir = g_strdup_printf ("%s/.starling", home);
  /* We don't check the error here because either it already exists
     (which isn't really an error), or we could created it and this
     error will be caught below.  */
  g_mkdir (dir, 0755);
  g_free (dir);
  char *file = g_strdup_printf ("%s/.starling/playlist", home);

  GError *err = NULL;
  st->db = music_db_open (file, &err);
  g_free (file);
  if (err)
    {
      g_warning ("%s", err->message);
      starling_error_box (err->message);
      g_error_free (err);
      exit (1);
    }

  g_signal_connect_swapped (G_OBJECT (st->db), "changed-entry",
			    G_CALLBACK (meta_data_changed), st);

  /* We set it initially to an impossible play list.  This way when we
     change play lists, we haven't wasted time loading the library
     (which is potentially expensive).  */
  st->library = play_list_new (st->db, "{103985430985435830534095803245083}");
#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  st->queue = play_list_new (st->db, "queue");
#endif

  st->player = player_new ();

  g_signal_connect_swapped (G_OBJECT (st->player), "eos",
			    G_CALLBACK (starling_next), st);
  g_signal_connect (G_OBJECT (st->player), "state-changed",
		    G_CALLBACK (player_state_changed), st);
  g_signal_connect_swapped (G_OBJECT (st->player), "tags",
			    G_CALLBACK (music_db_set_info_from_tags), st->db);

  /* If we use the note book, then we have a queue tab.  If we use
     stackable windows, then we don't and we need to show the
     queue.  */
  st->play_lists_model = play_lists_new (st->db,
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
					 TRUE
#else
					 FALSE
#endif
					 );

  /* Build the GUI.  */
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  {
    /* We have stackable windows.  Specialize the GUI using them.  */

    /* The top view is a menu allowing the user to choose among the
       available play lists.  */
    st->play_list_selector_view.window
      = GTK_WINDOW (hildon_stackable_window_new ());
    // gtk_window_fullscreen (GTK_WINDOW (st->play_list_selector_view.window));
    gtk_widget_show (GTK_WIDGET (st->play_list_selector_view.window));
    g_signal_connect_swapped (G_OBJECT (st->play_list_selector_view.window),
			      "delete-event", G_CALLBACK (window_x), st);

    st->play_list_selector = hildon_touch_selector_new ();
    gtk_widget_show (GTK_WIDGET (st->play_list_selector));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

    hildon_touch_selector_append_column
      (HILDON_TOUCH_SELECTOR (st->play_list_selector),
       GTK_TREE_MODEL (st->play_lists_model),
       renderer, "text", PLS_COL_NAME, NULL);

    hildon_touch_selector_set_column_selection_mode
      (HILDON_TOUCH_SELECTOR (st->play_list_selector),
       HILDON_TOUCH_SELECTOR_SELECTION_MODE_SINGLE);

    hildon_touch_selector_set_hildon_ui_mode
      (HILDON_TOUCH_SELECTOR (st->play_list_selector),
       HILDON_UI_MODE_NORMAL);

    st->play_list_selector_changed_signal
      = g_signal_connect_swapped (G_OBJECT (st->play_list_selector),
				  "changed",
				  G_CALLBACK (play_list_selector_changed), st);
    gtk_container_add (GTK_CONTAINER (st->play_list_selector_view.window),
		       GTK_WIDGET (st->play_list_selector));
  }
#endif  


  GtkWidget *hbox1 = NULL;
  GtkWidget *hbox2 = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *label = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkWidget *scrolled = NULL;

#if HAVE_HILDON
#if HAVE_HILDON_VERSION > 0
  HildonProgram *program = HILDON_PROGRAM (hildon_program_get_instance());
  g_set_application_name (PROGRAM_NAME);
# ifdef HAVE_HILDON_STACKABLE_WINDOWS
  st->window = GTK_WIDGET (hildon_stackable_window_new());
# else
  st->window = GTK_WIDGET (hildon_window_new());
# endif
  hildon_program_add_window (program, HILDON_WINDOW(st->window));
#else
  st->window = hildon_app_new ();
  hildon_app_set_two_part_title (HILDON_APP (st->window), FALSE);
  GtkWidget *main_appview = hildon_appview_new (_("Main"));
  hildon_app_set_appview (HILDON_APP (st->window),
			  HILDON_APPVIEW (main_appview));
#endif /* HAVE_HILDON_VERSION */
#else    
  st->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif /* HAVE_HILDON */
  g_signal_connect_swapped (G_OBJECT (st->window),
			    "delete-event", G_CALLBACK (window_x), st);

  GtkBox *main_box = GTK_BOX (gtk_vbox_new (FALSE, 0));
#if HAVE_HILDON && HAVE_HILDON_VERSION == 0
  gtk_container_add (GTK_CONTAINER (main_appview), GTK_WIDGET (main_box));
#else
  gtk_container_add (GTK_CONTAINER (st->window), GTK_WIDGET (main_box));
#endif

  g_signal_connect (G_OBJECT (st->window), "key_press_event", 
		    G_CALLBACK (key_press_event), st);

  GtkWidget *menu_item_add (GtkWidget *menu,
			    bool check_menu_item,
			    const char *text, const char *stock, GCallback cb,
			    gpointer user_data)
    
  {
    g_assert (text || stock);

    GtkWidget *item;
    char *signal;
    void (*append) (void *menu, void *item);

    /* A hildon app menu consists of buttons.  */
#ifdef HAVE_HILDON_APP_MENU
    /* Remove any _:s.  */
    char t[strlen (text) + 1];
    char *p = t;
    int i;
    for (i = 0; text[i]; i ++)
      if (text[i] != '_')
	{
	  *p = text[i];
	  p ++;
	}
    *p = 0;

    if (check_menu_item)
      {
	item = hildon_check_button_new (HILDON_SIZE_AUTO);
	gtk_button_set_label (GTK_BUTTON (item), t);
      }
    else
      item = hildon_button_new_with_text (HILDON_SIZE_AUTO,
					  HILDON_BUTTON_ARRANGEMENT_VERTICAL,
					  t, NULL);
    signal = "clicked";
    append = (void *) hildon_app_menu_append;
#else
    if (check_menu_item)
      {
	item = gtk_check_menu_item_new_with_mnemonic (text);
	signal = "toggled";
      }
    else
      {
	if (stock && !HAVE_HILDON)
	  item = gtk_image_menu_item_new_from_stock (stock, NULL);
	else
	  item = gtk_menu_item_new_with_mnemonic (text);
	signal = "activate";
      }
    append = (void *) gtk_menu_shell_append;
#endif

    if (cb)
      g_signal_connect_swapped (G_OBJECT (item), signal,
				(GCallback) cb, user_data);
    gtk_widget_show (item);
    append (menu, item);

    return item;
  }

  void menu_separator (GtkWidget *menu)
  {
#ifndef HAVE_HILDON_APP_MENU
    GtkWidget *mitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mitem);
    gtk_widget_show (mitem);
#endif
  }

  GtkWidget *menu;

#if !(defined (HAVE_HILDON_APP_MENU) || HAVE_HILDON)
# define MENU_HIERARCHY
  /* Menu bar.  */
  GtkWidget *menu_bar = gtk_menu_bar_new ();
  gtk_box_pack_start (main_box, menu_bar, FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (menu_bar));
#endif

  /* Get the top-level menu.  */
#ifdef HAVE_HILDON_APP_MENU
  /* Create a hildon application menu.  */
  menu = hildon_app_menu_new ();
  hildon_window_set_app_menu (HILDON_WINDOW (st->window), HILDON_APP_MENU (menu));
#elif HAVE_HILDON && HAVE_HILDON_VERSION == 0
  /* Hildon version 0 has a built in menu.  */
  menu = hildon_appview_get_menu (HILDON_APPVIEW (main_appview));
#elif HAVE_HILDON && HAVE_HILDON_VERSION > 0
  /* Hildon version 2 requires a GtkMenu.  */
  menu = gtk_menu_new ();
  hildon_window_set_menu (HILDON_WINDOW(st->window), GTK_MENU (menu));
#else
  menu = gtk_menu_new ();
#endif

  GtkWidget *mitem;

#ifdef MENU_HIERARCHY
  /* File menu.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), GTK_WIDGET (menu));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), GTK_WIDGET (mitem));
  gtk_widget_show (mitem);
#endif
    
#if 0
  /* XXX: Disable adding individual files.  */
  menu_item_add (menu, false, _("Add _File"), NULL,
		 G_CALLBACK (add_file_cb), st);
#endif
  menu_item_add (menu, false, _("Add _Directory"), NULL,
		 G_CALLBACK (add_directory_cb), st);
#ifdef MENU_HIERARCHY
  menu_item_add (menu, false, _("_Quit"), GTK_STOCK_QUIT,
		 G_CALLBACK (starling_quit), st);
#endif

#ifdef MENU_HIERARCHY
  /* Options menu.  */
  menu = gtk_menu_new ();
  mitem = gtk_menu_item_new_with_mnemonic (_("_Options"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), GTK_WIDGET (menu));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), GTK_WIDGET (mitem));
  gtk_widget_show (mitem);
#endif

#ifdef HAVE_TOGGLE_FULLSCREEN
  st->fullscreen = menu_item_add (menu, true, _("_Full Screen"), NULL,
				  G_CALLBACK (toggle_fullscreen),
# if HAVE_HILDON_VERSION == 0
				  main_appview
# else
				  st->window
# endif
				  );
#endif
    
  st->random = menu_item_add (menu, true, _("_Random"), NULL,
			      G_CALLBACK (set_random_cb), st);
  menu_separator (menu);
  menu_item_add (menu, false, _("Remove Selecte_d"), NULL,
		 G_CALLBACK (remove_cb), st);
  menu_item_add (menu, false, _("_Clear list"), NULL,
		 G_CALLBACK (clear_cb), st);
  menu_separator (menu);


  menu_item_add (menu, false, _("_Caption Format"), NULL,
		 G_CALLBACK (change_caption_format), st);
  /* check_menu.  */
  st->download_lyrics
    = menu_item_add (menu, true, _("Download _Lyrics"), NULL, NULL, NULL);




  /* The toolbar.  */
  GtkWidget *toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);
  gtk_widget_show (toolbar);

#if HAVE_HILDON && HAVE_HILDON_VERSION == 0
  hildon_appview_set_toolbar (HILDON_APPVIEW (main_appview),
			      GTK_TOOLBAR (toolbar));
  gtk_widget_show_all (main_appview);
#elif HAVE_HILDON && HAVE_HILDON_VERSION > 0
  /* Add toolbar to the HildonWindow */
  hildon_window_add_toolbar (HILDON_WINDOW(st->window), GTK_TOOLBAR(toolbar));
#else
  gtk_box_pack_start (GTK_BOX (main_box), toolbar, FALSE, FALSE, 0);
#endif

  /* Previous button.  */
  GtkToolItem *item;
  item = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS);
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (prev_cb), st);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Play/pause.  */
  item = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
  st->playpause = GTK_WIDGET (item);
  st->playpause_toggled_signal_id
    = g_signal_connect (G_OBJECT (st->playpause), "toggled",
			G_CALLBACK (playpause_cb), st);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Next.  */
  item = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_NEXT);
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (next_cb), st);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Position slider.  */
  item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (item, TRUE);

  GtkBox *hbox = GTK_BOX (gtk_hbox_new (FALSE, 0));
  gtk_container_add (GTK_CONTAINER (item), GTK_WIDGET (hbox));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);



  st->position = GTK_LABEL (gtk_label_new ("0:00"));
  gtk_label_set_width_chars (st->position, 5);
  gtk_misc_set_alignment (GTK_MISC (st->position), 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->position),
		      FALSE, FALSE, 0);

  st->position_slider = gtk_hscale_new_with_range (0, 100, 1);
  gtk_scale_set_draw_value (GTK_SCALE (st->position_slider), FALSE);
  g_signal_connect (G_OBJECT (st->position_slider), "button-release-event", 
		    G_CALLBACK (position_slider_button_released_cb), st);
  g_signal_connect (G_OBJECT (st->position_slider), "button-press-event",
		    G_CALLBACK (position_slider_button_pressed_cb), st);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->position_slider),
		      TRUE, TRUE, 0);
    
  st->duration = GTK_LABEL (gtk_label_new ("0:00"));
  gtk_label_set_width_chars (st->duration, 5);
  gtk_misc_set_alignment (GTK_MISC (st->duration), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->duration),
		      FALSE, FALSE, 0);

#ifdef HAVE_VOLUME_BUTTON
  st->volume_button = gtk_volume_button_new ();
  g_signal_connect (G_OBJECT (st->volume_button), "value-changed", 
		    G_CALLBACK (volume_changed), st);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->volume_button),
		      FALSE, FALSE, 0);
#endif
    
  /* Jump to the currently playing song.  In Maemo, we add this to the
     standard tool bar.  Otherwise, we add it to the currently playing
     toolbar.  */
  GtkWidget *jump_to = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (jump_to),
			gtk_image_new_from_stock (GTK_STOCK_JUMP_TO,
						  GTK_ICON_SIZE_BUTTON));
  g_signal_connect_swapped (G_OBJECT (jump_to), "clicked",
			    G_CALLBACK (jump_to_current), st);
#if (HAVE_MAEMO && HAVE_MAEMO_VERSION >= 500)
  gtk_box_pack_start (hbox, jump_to, FALSE, FALSE, 0);
#endif

#if ! (HAVE_MAEMO && HAVE_MAEMO_VERSION >= 500)
  /* The currently playing song.  */
  /* Stuff the title label in an hbox to prevent it from being
     centered.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 0));
  gtk_box_pack_start (main_box, GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  st->title = gtk_label_new (_("Not playing"));
  gtk_misc_set_alignment (GTK_MISC (st->title), 0, 0.5);
  gtk_label_set_ellipsize (GTK_LABEL (st->title), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (hbox, st->title, TRUE, TRUE, 0);

  st->rating = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  gtk_combo_box_append_text (st->rating, "-");
  gtk_combo_box_set_active (st->rating, 0);
  int i;
  for (i = 1; i <= 5; i ++)
    {
      char buffer[2];
      sprintf (buffer, "%d", i);
      gtk_combo_box_append_text (st->rating, buffer);
    }
  st->rating_change_signal_id
    = g_signal_connect_swapped (G_OBJECT (st->rating), "changed",
				G_CALLBACK (rate), st);
  gtk_box_pack_start (hbox, GTK_WIDGET (st->rating), FALSE, FALSE, 0);

  gtk_box_pack_start (hbox, jump_to, FALSE, FALSE, 0);
#endif

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  /* The notebook containing the tabs.  */
  st->notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (main_box), GTK_WIDGET (st->notebook));
#endif

#ifdef USE_SEARCH_BAR_TOGGLE
  /* Add a button to the main tool bar to show the search bar.  */
  {
    GtkWidget *search
      = hildon_gtk_toggle_button_new (HILDON_SIZE_FINGER_HEIGHT);
    st->search_bar_toggle = search;
    gtk_button_set_image (GTK_BUTTON (search),
			  gtk_image_new_from_stock (GTK_STOCK_FIND,
						    GTK_ICON_SIZE_BUTTON));
    st->search_bar_toggle_toggled_signal
      = g_signal_connect_swapped (G_OBJECT (search), "toggled",
				  G_CALLBACK (search_bar_toggle_toggled_cb),
				  st);
    gtk_box_pack_start (hbox, search, FALSE, FALSE, 0);
  }
#endif

  /* Library view tab.  Place a search field at the top and the
     library view at the bottom.  */

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);

  /* The currently playing song.  */
  /* Stuff the title label in an hbox to prevent it from being
     centered.  */
  st->search_bar = hbox = GTK_BOX (gtk_hbox_new (FALSE, 0));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  st->play_list_selector
    = gtk_combo_box_new_with_model (GTK_TREE_MODEL (st->play_lists_model));
  g_object_unref (st->play_lists_model);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (st->play_list_selector),
			      renderer, false);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (st->play_list_selector),
				  renderer, "text", PLS_COL_NAME, NULL);

  st->play_list_selector_changed_signal
    = g_signal_connect_swapped (G_OBJECT (st->play_list_selector),
				"changed",
				G_CALLBACK (play_list_selector_changed), st);
  gtk_widget_show (st->play_list_selector);
  gtk_box_pack_start (hbox, st->play_list_selector,
		      FALSE, FALSE, 0);
#endif

  st->search_enabled = GTK_TOGGLE_BUTTON (gtk_check_button_new ());
  g_signal_connect_swapped (G_OBJECT (st->search_enabled), "toggled",
			    G_CALLBACK (search_text_changed), st);
  gtk_widget_show (GTK_WIDGET (st->search_enabled));
  gtk_box_pack_start (hbox, GTK_WIDGET (st->search_enabled), FALSE, FALSE, 0);

  st->searches = gtk_list_store_new (2, GTK_TYPE_STRING, GTK_TYPE_UINT);

  GtkWidget *vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (hbox, vbox2, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (vbox2));

  GtkWidget *search_entry_combo
    = gtk_combo_box_entry_new_with_model (GTK_TREE_MODEL (st->searches), 0);
  gtk_box_pack_end (GTK_BOX (vbox2), search_entry_combo, FALSE, TRUE, 0);
  gtk_widget_show (search_entry_combo);

  st->search_entry = gtk_bin_get_child (GTK_BIN (search_entry_combo));
  g_signal_connect_swapped (G_OBJECT (st->search_entry),
			    "changed",
			    G_CALLBACK (search_text_changed), st);
  GtkEntryCompletion *completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (st->searches));
  gtk_entry_completion_set_text_column (completion, 0);
  gtk_entry_set_completion (GTK_ENTRY (st->search_entry), completion);

  GtkWidget *clear = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (clear),
			gtk_image_new_from_stock (GTK_STOCK_CLEAR,
						  GTK_ICON_SIZE_BUTTON));
  g_signal_connect_swapped (G_OBJECT (clear), "clicked",
			    G_CALLBACK (search_text_clear), st);
  gtk_widget_show (clear);
  gtk_box_pack_start (hbox, clear, FALSE, FALSE, 0);

  /* Group by button.  */
  {
    GtkWidget *group
      = GTK_WIDGET (gtk_menu_tool_button_new_from_stock (GTK_STOCK_ZOOM_100));
    st->group_by_button = group;
    g_signal_connect_swapped (G_OBJECT (group), "clicked",
			      G_CALLBACK (group_by_button_clicked_cb), st);
    gtk_widget_show (group);
    gtk_box_pack_start (hbox, group, FALSE, FALSE, 0);

    GtkWidget *menu = gtk_menu_new ();
    gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (group), menu);
    gtk_widget_show (menu);

    char *options[] = { _("None"), _("Artist"), _("Album") };
    GSList *radio = NULL;
    int i;
    for (i = 0; i < sizeof (options) / sizeof (options[0]); i ++)
      {
	GtkWidget *item
	  = gtk_radio_menu_item_new_with_label (radio, options[i]);
	st->group_by_radio_widgets
	  = g_slist_append (st->group_by_radio_widgets, item);
	radio = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);
	if (i == 0)
	  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);

	g_signal_connect_swapped (G_OBJECT (item), "toggled",
				  G_CALLBACK (group_by_cb), st);
      }
  }


  hbox = GTK_BOX (gtk_hbox_new (FALSE, 0));
  gtk_widget_show (GTK_WIDGET (hbox));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), TRUE, TRUE, 0);

#ifdef HAVE_HILDON_GTK_TREE_VIEW
  st->library_view =
    hildon_gtk_tree_view_new_with_model (HILDON_UI_MODE_NORMAL,
					 GTK_TREE_MODEL (st->library));
#else
  st->library_view =
    gtk_tree_view_new_with_model (GTK_TREE_MODEL (st->library));
#endif
  g_signal_connect (G_OBJECT (st->library_view), "row-activated",
		    G_CALLBACK (activated_cb), st);
  g_signal_connect (G_OBJECT (st->library_view), "button-press-event",
		    G_CALLBACK (library_button_press_event), st);
#if HAVE_HILDON
  g_signal_connect (G_OBJECT (st->library_view),
		    "tap-and-hold", G_CALLBACK (library_tap_and_hold_cb), st);
  gtk_widget_tap_and_hold_setup (st->library_view, NULL, NULL, 0);
#endif
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (st->library_view), FALSE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (st->library_view), TRUE);
  gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (st->library_view), TRUE);
  GtkTreeSelection *sel = gtk_tree_view_get_selection
    (GTK_TREE_VIEW (st->library_view));
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (st->library_view), FALSE);
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);

  gtk_widget_grab_focus (GTK_WIDGET (st->library_view));

  GtkTreeViewColumn *col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (st->library_view), col);
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "cell-background", "gray", NULL);
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      title_data_func,
				      st, NULL);
  gtk_widget_show (st->library_view);

#ifdef HAVE_HILDON_PANNABLE_AREA
  scrolled = hildon_pannable_area_new ();
#else
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
#endif
  st->library_view_window = scrolled;

  gtk_container_add (GTK_CONTAINER (scrolled), st->library_view);
  gtk_widget_show (scrolled);
  gtk_container_add (GTK_CONTAINER (hbox), scrolled);

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  st->library_view_loading_label = gtk_label_new (_("Loading..."));
  gtk_box_pack_start (GTK_BOX (hbox),
		      GTK_WIDGET (st->library_view_loading_label),
		      TRUE, TRUE, 0);
  gtk_widget_show (st->library_view_loading_label);
#endif


  /* Build the alpha scroller.  */
  GtkEventBox *event_box = GTK_EVENT_BOX (gtk_event_box_new ());
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (event_box),
		      FALSE, FALSE, 0);
  /* Make the event box invisible.  */
  gtk_event_box_set_visible_window (event_box, FALSE);
  /* Set it above the contained widget.  */
  gtk_event_box_set_above_child (event_box, TRUE);
  gtk_widget_show (GTK_WIDGET (event_box));
  g_signal_connect_swapped (G_OBJECT (event_box),
			    "button-release-event",
			    G_CALLBACK (playlist_alpha_seek_clicked),
			    st);
  
  st->playlist_alpha_seek = GTK_BOX (gtk_vbox_new (FALSE, 0));
  gtk_container_add (GTK_CONTAINER (event_box),
		     GTK_WIDGET (st->playlist_alpha_seek));
  gtk_widget_show (GTK_WIDGET (st->playlist_alpha_seek));

  /* We connect to ST->LIBRARY_VIEW_WINDOW's size-allocate signal and
     not ST->PLAYLIST_ALPHA_SEEK's as after we repopulate the latter,
     a new size-allocate signal is emitted, which creates an infinate
     loop.  */
  g_signal_connect_swapped (G_OBJECT (st->library_view_window),
			    "size-allocate",
			    G_CALLBACK (playlist_alpha_seek_build_queue_size_allocate),
			    st);

  
  
    
  /* XXX: Currently disable as "save as" functionality is
     unimplemented (see musicdb.c).  */
#if 0
  create_button (&st->save, GTK_STOCK_SAVE_AS, l_button_box);
  g_signal_connect (G_OBJECT (st->save), "clicked",
		    G_CALLBACK (save_cb), st);
#endif
    
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
  gtk_box_pack_start (GTK_BOX (main_box), vbox, TRUE, TRUE, 0);
  st->library_tab = GTK_WIDGET (st->window);
#else
  st->library_tab = GTK_WIDGET (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
			    gtk_label_new (_("Library")));
  update_library_count (st);

  g_signal_connect_swapped (G_OBJECT (st->library), "row-inserted",
			    G_CALLBACK (update_library_count), st);
  g_signal_connect_swapped (G_OBJECT (st->library), "row-deleted",
			    G_CALLBACK (update_library_count), st);
#endif


  /* Play queue tab.  */

#ifndef HAVE_HILDON_STACKABLE_WINDOWS
  vbox = gtk_vbox_new (FALSE, 0);

  st->queue_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (st->queue));
  g_signal_connect (G_OBJECT (st->queue_view), "row-activated",
		    G_CALLBACK (activated_cb), st);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (st->queue_view), FALSE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (st->queue_view), TRUE);
  gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (st->queue_view), TRUE);
  sel = gtk_tree_view_get_selection
    (GTK_TREE_VIEW (st->queue_view));
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (st->queue_view), FALSE);
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (st->queue_view), col);
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "cell-background", "gray", NULL);
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      title_data_func,
				      st, NULL);
    
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  st->queue_view_window = scrolled;
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
    
  gtk_container_add (GTK_CONTAINER (scrolled), st->queue_view);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled);

  st->queue_tab = GTK_WIDGET (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox, NULL);
  update_queue_count (st);

  g_signal_connect_swapped (G_OBJECT (st->queue), "row-inserted",
			    G_CALLBACK (update_queue_count), st);
  g_signal_connect_swapped (G_OBJECT (st->queue), "row-deleted",
			    G_CALLBACK (update_queue_count), st);
#endif


  /* Lyrics tab */
  {
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
    st->lyrics_tab = hildon_stackable_window_new ();
    g_signal_connect_swapped (G_OBJECT (st->lyrics_tab),
			      "delete-event", G_CALLBACK (window_x),
			      st->lyrics_tab);
    menu_item_add (menu, false, _("Lyrics"), NULL,
		   G_CALLBACK (gtk_widget_show), st->lyrics_tab);
#endif

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
    gtk_container_add (GTK_CONTAINER (st->lyrics_tab), vbox);
#else
    st->lyrics_tab = vbox;
    gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
			      gtk_label_new (_("Lyrics")));
#endif

    /* [Download].  */
    GtkWidget *download = gtk_button_new_with_label (_("Download"));
    g_signal_connect_swapped (G_OBJECT (download), "clicked",
			      G_CALLBACK (lyrics_download), st);
    gtk_widget_show (download);
    gtk_box_pack_start (GTK_BOX (vbox), download, FALSE, FALSE, 0);

    /* ...Lyrics...  */
    GtkWidget *scrolled;
#ifdef HAVE_HILDON_PANNABLE_AREA
    scrolled = hildon_pannable_area_new ();
#else
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
#endif
    gtk_widget_show (scrolled);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 3);

    st->lyrics_textview = gtk_text_view_new ();
    GTK_TEXT_VIEW (st->lyrics_textview)->editable = FALSE;
    gtk_widget_show (st->lyrics_textview);
    gtk_container_add (GTK_CONTAINER (scrolled), st->lyrics_textview);
  }

  /* Web services tab (for now, last.fm) */
  {
#ifdef HAVE_HILDON_STACKABLE_WINDOWS
    st->lastfm_tab = hildon_stackable_window_new ();
    g_signal_connect_swapped (G_OBJECT (st->lastfm_tab),
			      "delete-event", G_CALLBACK (window_x),
			      st->lastfm_tab);
    menu_item_add (menu, false, _("Last FM"), NULL,
		   G_CALLBACK (gtk_widget_show), st->lastfm_tab);
#endif

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

#ifdef HAVE_HILDON_STACKABLE_WINDOWS
    gtk_container_add (GTK_CONTAINER (st->lastfm_tab), vbox);
#else
    gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
			      gtk_label_new (_("last.fm")));
    st->lastfm_tab = vbox;
#endif

    /* Username: [         ]  */
    GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    GtkWidget *label = gtk_label_new (_("Username:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    st->webuser_entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), st->webuser_entry, TRUE, TRUE, 0);
    gtk_widget_show (st->webuser_entry);


    /* Password: [         ]  */
    hbox = gtk_hbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Password:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
  
    st->webpasswd_entry = gtk_entry_new ();
    gtk_entry_set_visibility (GTK_ENTRY (st->webpasswd_entry), FALSE);
    gtk_widget_show (st->webpasswd_entry);
    gtk_box_pack_start (GTK_BOX (hbox), st->webpasswd_entry, TRUE, TRUE, 0);


    /* Auto submit after [   ] pending submissions.  */
    hbox = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Auto submit after"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    st->lastfm_autosubmit
      = GTK_SPIN_BUTTON (gtk_spin_button_new_with_range (0, 100, 1));
    gtk_widget_show (GTK_WIDGET (st->lastfm_autosubmit));
    gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->lastfm_autosubmit),
			FALSE, FALSE, 0);

    label = gtk_label_new (_("pending submissions."));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);


    /* [  ] tracks pending.  */
    st->web_count = gtk_label_new ("");
    gtk_widget_show (st->web_count);
    gtk_box_pack_start (GTK_BOX (vbox), st->web_count, FALSE, FALSE, 0);

    /* [Submit]  */
    st->web_submit = gtk_button_new_with_label (_("Submit Now"));
    g_signal_connect (G_OBJECT (st->web_submit), "clicked",
		      G_CALLBACK (lastfm_submit_cb), st);
    gtk_widget_show (st->web_submit);
    gtk_box_pack_start (GTK_BOX (vbox), st->web_submit, FALSE, FALSE, 0);
  }

  /* The status label used to show the amount of background work
     (e.g., files to scan).  */
  st->status = GTK_LABEL (gtk_label_new (NULL));
  gtk_box_pack_start (GTK_BOX (main_box), GTK_WIDGET (st->status),
		      FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (st->db), "status",
		    G_CALLBACK (status_update), st);

  lyrics_init ();

  deserialize (st);

  gtk_widget_show_all (st->window);

  gtk_widget_hide (GTK_WIDGET (st->status));
#ifdef USE_SEARCH_BAR_TOGGLE
  /* Hide the search bar by default.  */
  gtk_widget_hide (GTK_WIDGET (st->search_bar));
  gtk_widget_hide (GTK_WIDGET (st->library_view_window));
#endif

  set_title (st);

  return st;
}
