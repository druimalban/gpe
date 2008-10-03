/* starling.c - Starling.
   Copyright (C) 2007, 2008 Neal H. Walfield <neal@walfield.org>
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

#define ERROR_DOMAIN() g_quark_from_static_string ("starling")

#include "starling.h"

#include "config.h"
#include "lastfm.h"
#include "errorbox.h"
#include "lyrics.h"
#include "utils.h"
#include "playlist.h"
#include "player.h"
#include "search.h"
#include "caption.h"

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gtypes.h>
#include <glib/gkeyfile.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#define CAPTION_FMT_DEFAULT "%.50a - %.50t"

#ifdef IS_HILDON
/* Hildon includes */
# if HILDON_VER > 0
#  include <hildon/hildon-program.h>
#  include <hildon/hildon-window.h>
#  include <hildon/hildon-file-chooser-dialog.h>
# else
#  include <hildon-widgets/hildon-app.h>
#  include <hildon-widgets/hildon-appview.h>
#  include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
# endif /* HILDON_VER */

# include <libosso.h>
# define APPLICATION_DBUS_SERVICE "starling"
#endif /* IS_HILDON */

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
  GtkWidget *scale;
  int scale_sliding;

  GtkWidget *title;
  GtkComboBox *rating;
  int rating_change_signal_id;

  GtkWidget *notebook;
  GtkWidget *library_tab;
  GtkWidget *queue_tab;
  GtkWidget *playlist_tab;

  GtkScrolledWindow *library_view_window;
  GtkWidget *library_view;
  struct caption *caption;
  GtkComboBox *playlist;
  int playlist_count;
  int playlist_changed_signal;
  GtkToggleButton *search_enabled;
  GtkWidget *search_entry;
  GtkListStore *searches;

  GtkScrolledWindow *queue_view_window;
  GtkWidget *queue_view;

  GtkWidget *random;
  gchar *fs_last_path;
  GtkWidget *textview;
  GtkWidget *webuser_entry;
  GtkWidget *webpasswd_entry;
  GtkWidget *web_submit;
  GtkWidget *web_count;
  gboolean has_lyrics;
  gint64 current_length;
  gboolean enqueued;
#ifdef IS_HILDON
  GtkCheckMenuItem *fullscreen;
#endif
  GtkCheckMenuItem *download_lyrics;

  Player *player;
  MusicDB *db;
  PlayList *library;
  PlayList *queue;

  /* UID of the loaded song.  */
  int loaded_song;

  /* Source of the position updater.  */
  int position_update;

  /* Position to seek to next time it is possible (because we cannot
     seek when the player is in GST_STATE_NULL).  */
  int pending_seek;

  /* Hash mapping play lists to their position.  */
  GHashTable *playlists_conf;
};

static void
set_title (Starling *st)
{
  char *title_bar;
  bool free_title_bar = true;

  struct music_db_info info;
  info.fields = MDB_SOURCE | MDB_ARTIST | MDB_TITLE | MDB_RATING;
  if (! music_db_get_info (st->db, st->loaded_song, &info))
    {
      title_bar = "Starling";
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
      lyrics_display (artist, title, GTK_TEXT_VIEW (st->textview),
		      gtk_check_menu_item_get_active (st->download_lyrics));
    }

  g_signal_handler_block (st->rating, st->rating_change_signal_id);
  gtk_combo_box_set_active (st->rating, info.rating);
  g_signal_handler_unblock (st->rating, st->rating_change_signal_id);

  if (! title)
    {
      title = strrchr (uri, '/');
      if (! title)
	title = uri;
      else
	title ++;
    }

#define PREFIX "Starling: "
  if (artist)
    title_bar = g_strdup_printf (_("%s %s - %s"),
				 _(PREFIX),
				 artist, title);
  else
    title_bar = g_strdup_printf (_("%s %s"), _(PREFIX), title);

  gtk_label_set_text (GTK_LABEL (st->title),
		      title_bar
		      + sizeof (PREFIX) - 1 /* NULL */ + 1 /* ' ' */);

  g_free (info.artist);
  g_free (info.title);
  g_free (info.source);

 do_set:
#ifdef IS_HILDON
#if HILDON_VER > 0
  gtk_window_set_title (GTK_WINDOW (st->window), title_bar);
#else
  hildon_app_set_title (HILDON_APP (st->window), title_bar);
#endif /* HILDON_VER */
#else
  gtk_window_set_title (GTK_WINDOW (st->window), title_bar);
#endif /* IS_HILDON */

  if (free_title_bar)
    g_free (title_bar);
}

bool
starling_random (Starling *st)
{
  return gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (st->random));
}

void
starling_random_set (Starling *st, bool value)
{
  if (value == starling_random (st))
    return;

  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (st->random), value);
}

static gboolean
starling_load (Starling *st, int uid)
{
  int old_idx = -1;
  if (st->loaded_song)
    old_idx = play_list_uid_to_index (st->library, st->loaded_song);

  st->loaded_song = uid;

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
	    {
	      static int rand_init;
	      if (! rand_init)
		{
		  srand (time (NULL));
		  rand_init = 1;
		}

	      idx = rand () % count;
	    }
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
  player_set_sink (st->player, sink);
}

#define CONFIG_FILE "starlingrc"

#define KEYRANDOM "random"
#define KEYLASTPATH "last-path"
#define KEYSINK "sink"
#define KEYLFMUSER "lastfm-user"
#define KEYLFMPASSWD "lastfm-password"
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
};

static void play_list_combo_refresh (Starling *st, char *active,
				     bool save_old_config);

static int
deserialize_bottom_half (gpointer data)
{
  struct deserialize_bottom_half *bh = data;

  Starling *st = bh->st;

  gtk_notebook_set_current_page (GTK_NOTEBOOK (st->notebook), bh->page);

  play_list_combo_refresh (st, bh->playlist, false);
  g_free (bh->playlist);

  gtk_toggle_button_set_active (st->search_enabled, bh->search_enabled);
  gtk_entry_set_text (GTK_ENTRY (st->search_entry), bh->search_terms);
  g_free (bh->search_terms);

  g_free (data);
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

  /* And then load the current song.  */
  starling_load (st, g_key_file_get_integer (keyfile,
					     GROUP, KEY_LOADED_SONG, NULL));

  struct deserialize_bottom_half *bh = calloc (sizeof (*bh), 1);
  bh->st = st;

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
    gtk_list_store_clear (st->searches);

    gsize length = 0;
    char **values = g_key_file_get_string_list (keyfile,
						GROUP, KEY_SEARCHES,
						&length, NULL);

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

	gtk_list_store_insert_with_values (st->searches, NULL, -1,
					   0, values[i],
					   1, ts_val, -1);
      }
    g_strfreev (values);
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
  gtk_check_menu_item_set_active
    (st->download_lyrics,
     g_key_file_get_boolean (keyfile, GROUP, KEY_LYRICS_DOWNLOAD, NULL));

  /* Caption format.  */
  value = g_key_file_get_string (keyfile, GROUP, KEY_CAPTION_FORMAT, NULL);
  if (value)
    {
      if (st->caption)
	caption_free (st->caption);
      st->caption = caption_create (value);
      g_free (value);
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
  char *value = gtk_combo_box_get_active_text (st->playlist);
  g_key_file_set_string (keyfile, GROUP, KEY_CURRENT_PLAYLIST, value);
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


  /* The current page.  */
  int page = gtk_notebook_get_current_page (GTK_NOTEBOOK (st->notebook));
  g_key_file_set_integer (keyfile, GROUP, KEY_CURRENT_PAGE, page);

  /* Lyrics download.  */
  g_key_file_set_boolean (keyfile, GROUP, KEY_LYRICS_DOWNLOAD, 
			  gtk_check_menu_item_get_active
			  (st->download_lyrics));


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

static void
play_list_combo_changed (Starling *st, gpointer do_save)
{
  const char *play_list = play_list_get (st->library);
  if (! play_list)
    play_list = "Library";

  if (do_save)
    /* Save the current position and selection.  */
    play_list_state_save (st);

  char *active = gtk_combo_box_get_active_text (st->playlist);
  if (! do_save
      || ! active
      || strcmp (play_list, active) != 0)
    {
      play_list_set (st->library,
		     ! active || strcmp (active, "Library") == 0
		     ? NULL : active);

      play_list_state_restore (st);
    }
  g_free (active);
}

static void
play_list_combo_refresh (Starling *st, char *active, bool save_old_config)
{
  g_signal_handler_block (st->playlist, st->playlist_changed_signal);

  printf ("%s\n", __FUNCTION__);

  bool free_active = false;
  if (! active)
    {
      free_active = true;
      active = gtk_combo_box_get_active_text (st->playlist);
    }

  for (; st->playlist_count > 0; st->playlist_count --)
    gtk_combo_box_remove_text (st->playlist, 0);

  gtk_combo_box_append_text (st->playlist, _("Library"));
  st->playlist_count = 1;

  int active_i = 0;
  int playlist_iter (const char *list)
  {
    if (strcmp ("queue", list) != 0)
      {
	gtk_combo_box_append_text (st->playlist, list);
	if (active && strcmp (list, active) == 0)
	  active_i = st->playlist_count;

	st->playlist_count ++;
      }
    return 0;
  }
  music_db_play_lists_for_each (st->db, playlist_iter);

  if (free_active)
    g_free (active);

  g_signal_handler_unblock (st->playlist, st->playlist_changed_signal);

  gtk_combo_box_set_active (st->playlist, active_i);
}

static void
add_cb (GtkWidget *w, Starling *st, GtkFileChooserAction action)
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
add_file_cb (GtkWidget *w, Starling *st)
{
  add_cb (w, st, GTK_FILE_CHOOSER_ACTION_OPEN);
}

static void
add_directory_cb (GtkWidget *w, Starling *st)
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
  serialize (st);
  gtk_main_quit();
}

static void
set_random_cb (GtkWidget *w, Starling *st)
{
  serialize (st);
}

static void
remove_cb (GtkWidget *w, Starling *st)
{
  /* We need to remove them in reverse numerical order.  Consider: 1
     and 3 are selected.  We remove the track at index 1 and then 3.
     After removing the track at index 1, the entry referred to be
     index 3 is different!  */

  GtkWidget *view = NULL;
  switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (st->notebook)))
    {
    case 0:
      view = st->library_view;
      break;
    case 1:
      view = st->queue_view;
      break;
    }

  if (! view)
    return;

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
      play_list_remove (st->library, idx);
    }

  g_queue_free (selected);

  gtk_tree_selection_unselect_all (selection);

  play_list_combo_refresh (st, NULL, true);
}

static void
clear_cb (GtkWidget *w, Starling *st)
{   
  char *text = gtk_combo_box_get_active_text (st->playlist);

  if (strcmp (text, "Library") == 0)
    {
      player_pause (st->player);
      music_db_clear (st->db);
    }
  else
    music_db_play_list_clear (st->db, text);

  g_free (text);

  play_list_combo_refresh (st, NULL, false);

  if (strcmp (text, "Library") == 0)
    starling_load (st, 0);
}

static void
clear_queue_cb (GtkWidget *w, Starling *st)
{   
  music_db_play_list_clear (st->db, "queue");
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

static void
change_caption_format (GtkMenuItem *menuitem, gpointer user_data)
{
  Starling *st = user_data;

  const char *defaults[] = {
    CAPTION_FMT_DEFAULT,
    "%a - %t",
    "%.40a - %.40t - %.40A",
    "%a - %t - (%A %T)",
    "%a - %t - (%A %T) %m:%02s",
    "%a - %t (%c plays)" };

  GKeyFile *keyfile = config_file_load ();
  char *caption = NULL;
  char **history = NULL;
  int history_len;
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
  GtkWidget *label = gtk_label_new
    ("New caption format:\n"
     " %a - artist\n"
     " %A - album\n"
     " %t - title\n"
     " %T - track\n"
     " %g - genre\n"
     " %r - rating\n"
     " %d - duration\n"
     " %s - seconds\n"
     " %m - minutes\n"
     " %c - play count\n"
     " %[width][.][precision]c - width and precision.");
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), label);

  GtkWidget *combo = gtk_combo_box_entry_new_text ();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), combo);

  /* Load it with the history and the defaults.  */
  if (caption)
    {
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo), caption); 
      g_free (caption);
    }

  int i;
  for (i = 0; history && history[i]; i ++)
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), history[i]); 
  /* Add the defaults, but only those that are not already in the
     history.  */
  for (i = 0; i < sizeof (defaults) / sizeof (defaults[0]); i ++)
    {
      int j;
      for (j = 0; history && history[j]; j ++)
	if (strcmp (history[j], defaults[i]) == 0)
	  break;

      if (! history || ! history[j])
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), defaults[i]); 
    }

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

      char *fmt = gtk_combo_box_get_active_text (GTK_COMBO_BOX (combo));
      st->caption = caption_create (fmt);
      g_key_file_set_string (keyfile, GROUP, KEY_CAPTION_FORMAT, fmt);

      /* See if FMT is already in the history.  */
      int i;
      for (i = 0; history && history[i]; i ++)
	if (strcmp (history[i], fmt) == 0)
	  /* Already in the history.  */
	  break;

      if (! history || ! history[i])
	/* Not in the history.  */
	{
	  struct obstack s;
	  obstack_init (&s);
	  
	  obstack_printf (&s, "%s", fmt);

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

      g_free (fmt);

      gtk_widget_queue_draw (st->library_view);
      gtk_widget_queue_draw (st->queue_view);

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
activated_cb (GtkTreeView *view, GtkTreePath *path,
	      GtkTreeViewColumn *col, Starling *st)
{
  gint *pos;

  g_assert (gtk_tree_path_get_depth (path) == 1);

  pos = gtk_tree_path_get_indices (path);

  int uid = play_list_index_to_uid (st->library, pos[0]);

  starling_load (st, uid);
  starling_play (st);
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
  gtk_notebook_set_current_page (GTK_NOTEBOOK (st->notebook), 0);
}

static int search_text_save_source;

static int
search_text_save (Starling *st)
{
  search_text_save_source = 0;

  const char *search = gtk_entry_get_text (GTK_ENTRY (st->search_entry));

  gboolean found = FALSE;
  gboolean callback (GtkTreeModel *model, GtkTreePath *path,
		     GtkTreeIter *iter, gpointer data)
  {
    char *val = NULL;
    gtk_tree_model_get (model, iter, 0, &val, -1);
    if (val && strcasecmp (search, val) == 0)
      /* Already in the history.  Update time stamp.  */
      {
	gtk_list_store_set (st->searches, iter,
			    1, (unsigned int) time (NULL),
			    -1);
	found = TRUE;
      }
    g_free (val);

    return found;
  }
  gtk_tree_model_foreach (GTK_TREE_MODEL (st->searches), callback, NULL);

  if (! found)
    gtk_list_store_insert_with_values (st->searches, NULL, -1,
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
}

static void
search_text_clear (Starling *st)
{
  gtk_entry_set_text (GTK_ENTRY (st->search_entry), "");
}

static void
set_position_text (Starling *st, int seconds)
{
  char *p = g_strdup_printf ("%d:%02d",
			     seconds / 60, seconds % 60);
  gtk_label_set_text (GTK_LABEL (st->position), p);
  g_free (p);
}

static gboolean
scale_sliding_update (Starling *st)
{
  if (st->current_length < 0)
    {
      GstFormat fmt = GST_FORMAT_TIME;
      player_query_duration (st->player, &fmt, &st->current_length, -1);
    }

  gfloat percent = gtk_range_get_value (GTK_RANGE (st->scale));

  set_position_text (st,
		     ((gint64) (((gfloat) st->current_length * percent)
				/ 100))
		     / 1e9);

  return TRUE;
}

static gboolean
scale_button_released_cb (GtkRange *range, GdkEventButton *event,
        Starling *st)
{
  g_assert (st->scale_sliding);
  g_source_remove (st->scale_sliding);
  st->scale_sliding = 0;

  gint percent;
  GstFormat fmt = GST_FORMAT_TIME;

  if (st->current_length < 0)
    player_query_duration (st->player, &fmt, &st->current_length, -1);

  percent = gtk_range_get_value (range);
  player_seek (st->player, GST_FORMAT_TIME, 
	       (st->current_length / 100) * percent);
    
  return FALSE;
}

static gboolean
scale_button_pressed_cb (GtkRange *range, GdkEventButton *event,
			 Starling *st)
{
  g_assert (! st->scale_sliding);
  st->scale_sliding
    = g_timeout_add (100, (GSourceFunc) scale_sliding_update, st);

  return FALSE;
}

static gboolean
position_update (Starling *st)
{
  if (st->scale_sliding)
    /* User is sliding, don't change the position.  */
    return TRUE;

  GstFormat fmt = GST_FORMAT_TIME;
  gint64 position;
  gfloat percent;
  gint total_seconds;
  gint position_seconds;

  if (st->current_length < 0)
    {
      if (! player_query_duration (st->player, &fmt, &st->current_length, -1))
	return TRUE;

      total_seconds = st->current_length / 1e9;
      char *d = g_strdup_printf ("%d:%02d",
				 total_seconds / 60, total_seconds % 60);
      gtk_label_set_text (GTK_LABEL (st->duration), d);
      g_free (d);
    }

  player_query_position (st->player, &fmt, &position);

  percent = (((gfloat) (position)) / st->current_length) * 100; 

  total_seconds = st->current_length / 1e9;
  position_seconds = (total_seconds / 100.0) * percent;

  if (!st->scale_sliding)
    {
      set_position_text (st, position_seconds);
      gtk_range_set_value (GTK_RANGE (st->scale), percent);
    }

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

	  lastfm_enqueue (info.artist, info.title, total_seconds, st);

	  g_free (info.artist);
	  g_free (info.title);
	}

      /* Note that it has been played.  */
      struct music_db_info info;
      memset (&info, 0, sizeof (info));
      info.fields = MDB_UPDATE_DATE_LAST_PLAYED | MDB_INC_PLAY_COUNT
	| MDB_DURATION;
      /* Also set the duration now, it doesn't cost anything and many
	 track lack it.  */
      info.duration = total_seconds;

      music_db_set_info (st->db, st->loaded_song, &info);

      st->enqueued = TRUE;
    }

  return TRUE;
}

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

static void
update_library_count (Starling *st)
{
  int count = play_list_count (st->library);

  static int old_count;
  static int old_total;

  const char *search_text = play_list_constraint_get (st->library);
  int total = 0;
  if (search_text && *search_text)
    {
      total = play_list_total (st->library);

      if (old_count == count && old_total == total)
	return;

      old_total = total;
    }
  else
    {
      if (old_count == count)
	return;
    }
  old_count = count;


  char *playlist = gtk_combo_box_get_active_text (st->playlist);

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
}

static void
player_state_changed (Player *pl, gpointer uid, int state, Starling *st)
{
  if (GST_STATE_PLAYING == state)
    {
      if (! gtk_toggle_tool_button_get_active
	  (GTK_TOGGLE_TOOL_BUTTON (st->playpause)))
	gtk_toggle_tool_button_set_active
	  (GTK_TOGGLE_TOOL_BUTTON (st->playpause), TRUE);

      st->current_length = -1;
      st->has_lyrics = FALSE;
      st->enqueued = FALSE;

      if (! st->position_update)
	st->position_update
	  = g_timeout_add (1000, (GSourceFunc) position_update, st);
    }
  else if (state == GST_STATE_PAUSED || state == GST_STATE_NULL)
    {
      if (gtk_toggle_tool_button_get_active
	  (GTK_TOGGLE_TOOL_BUTTON (st->playpause)))
	gtk_toggle_tool_button_set_active
	  (GTK_TOGGLE_TOOL_BUTTON (st->playpause), FALSE);

      if (st->position_update)
	{
	  g_source_remove (st->position_update);
	  st->position_update = 0;
	}
    }

  position_update (st);

  if (st->pending_seek >= 0
      && (state == GST_STATE_PLAYING || state == GST_STATE_PAUSED))
    {
      player_seek (st->player,
		   GST_FORMAT_TIME, (gint64) st->pending_seek * 1e9);
      st->pending_seek = -1;
    }
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
    st->caption = caption_create (CAPTION_FMT_DEFAULT);

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
    add_song,
    add_album,
    add_artist,
    add_all,
    add_sel,
    add_count,
  };

struct menu_info
{
  Starling *st;
  int uid;
  char *artist;
  char *album;
  char *list;
  GList *selection;
  enum menu_op op;
  struct menu_info *next;
};

static void
menu_destroy (GtkWidget *widget, gpointer d)
{
  struct menu_info *info = d;
  g_free (info->artist);
  g_free (info->album);
  g_list_free (info->selection);

  struct menu_info *i;
  struct menu_info *n = info->next;
  while ((i = n))
    {
      n = i->next;

      g_free (i->list);
      g_free (i);
    }

  gtk_widget_destroy (widget);
}

static void
queue_cb (GtkWidget *widget, gpointer d)
{
  struct menu_info *info = d;
  Starling *st = info->st;

  bool new_list = false;
  if (strcmp (info->list, "new play list") == 0)
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
      switch (result)
	{
	case GTK_RESPONSE_OK:
	  g_free (info->list);
	  info->list = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	  new_list = true;
	  break;

	default:
	  return;
	}

      gtk_widget_destroy (dialog);
    }

  int callback (int uid, struct music_db_info *i)
  {
    music_db_play_list_enqueue (st->db, info->list, uid);

    return 0;
  }

  char *s;
  bool need_free = true;
  const char *constraint = play_list_constraint_get (st->library);

  switch (info->op)
    {
    case add_song:
      music_db_play_list_enqueue (st->db, info->list, info->uid);
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
			 callback, sort_order, s);
      if (need_free)
	sqlite_freemem (s);
    }

  if (new_list)
    play_list_combo_refresh (st, NULL, true);
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

      GtkMenu *menu = GTK_MENU (gtk_menu_new ());

      int uid = play_list_index_to_uid (st->library, idx);

      struct music_db_info info;
      info.fields = MDB_SOURCE | MDB_ARTIST | MDB_ALBUM | MDB_TITLE;
      music_db_get_info (st->db, uid, &info);

      GtkMenu *submenus[add_count];
      int ops[add_count];
      int submenu_count = 0;

      /* Create a "queue this track" button.  */
      GtkWidget *button = gtk_menu_item_new_with_label ("");
      char *str;
      if (info.title)
	str = g_strdup_printf (_("Add track <i>%.20s%s</i> to"),
			       info.title,
			       strlen (info.title) > 20 ? "..." : "");
      else
	{
	  char *s = info.source;
	  if (strlen (s) > 40)
	    s = s + strlen (s) - 37;

	  str = g_strdup_printf (_("Add track <i>%s%s</i> to"),
				 s == info.source ? "" : "...", s);
	}
      gtk_label_set_markup (GTK_LABEL (GTK_BIN (button)->child), str);
      g_free (str);
      gtk_widget_show (button);
      gtk_menu_attach (menu, button, 0, 1, submenu_count, submenu_count + 1);

      submenus[submenu_count] = GTK_MENU (gtk_menu_new ());
      gtk_widget_show (GTK_WIDGET (submenus[submenu_count]));
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (button),
				 GTK_WIDGET (submenus[submenu_count]));

      ops[submenu_count] = add_song;

      submenu_count ++;

      if (info.album)
	/* Create a "queue this album button."  */
	{
	  button = gtk_menu_item_new_with_label ("");
	  str = g_strdup_printf (_("Add album <i>%.20s%s</i> to"),
				 info.album,
				 strlen (info.album) > 20 ? "..." : "");
	  gtk_label_set_markup (GTK_LABEL (GTK_BIN (button)->child), str);
	  g_free (str);
	  gtk_widget_show (button);
	  gtk_menu_attach (menu, button, 0, 1,
			   submenu_count, submenu_count + 1);

	  submenus[submenu_count] = GTK_MENU (gtk_menu_new ());
	  gtk_widget_show (GTK_WIDGET (submenus[submenu_count]));
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (button),
				     GTK_WIDGET (submenus[submenu_count]));

	  ops[submenu_count] = add_album;

	  submenu_count ++;
	}

      if (info.album)
	/* Create a "queue this artist button."  */
	{
	  button = gtk_menu_item_new_with_label ("");
	  str = g_strdup_printf (_("Add tracks by <i>%.20s%s</i> to"),
				 info.artist,
				 strlen (info.artist) > 20 ? "..." : "");
	  gtk_label_set_markup (GTK_LABEL (GTK_BIN (button)->child), str);
	  g_free (str);
	  gtk_widget_show (button);
	  gtk_menu_attach (menu, button, 0, 1,
			   submenu_count, submenu_count + 1);

	  submenus[submenu_count] = GTK_MENU (gtk_menu_new ());
	  gtk_widget_show (GTK_WIDGET (submenus[submenu_count]));
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (button),
				     GTK_WIDGET (submenus[submenu_count]));

	  ops[submenu_count] = add_artist;

	  submenu_count ++;
	}

      if (gtk_toggle_button_get_active (st->search_enabled))
	{
	  const char *search_text
	    = gtk_entry_get_text (GTK_ENTRY (st->search_entry));
	  if (search_text && *search_text)
	    {
	      button = gtk_menu_item_new_with_label ("");
	      str = g_strdup_printf (_("Add tracks matching <i>%s</i> to"),
				     search_text);
	      gtk_label_set_markup (GTK_LABEL (GTK_BIN (button)->child), str);
	      gtk_widget_show (button);
	      gtk_menu_attach (menu, button, 0, 1,
			       submenu_count, submenu_count + 1);

	      submenus[submenu_count] = GTK_MENU (gtk_menu_new ());
	      gtk_widget_show (GTK_WIDGET (submenus[submenu_count]));
	      gtk_menu_item_set_submenu (GTK_MENU_ITEM (button),
					 GTK_WIDGET (submenus[submenu_count]));

	      ops[submenu_count] = add_all;

	      submenu_count ++;
	    }
	}

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

      if (sel_count)
	{
	  button = gtk_menu_item_new_with_label ("");
	  if (sel_count > 1)
	    str = g_strdup_printf (_("Add %d selected tracks to"),
				   sel_count);
	  else
	    str = g_strdup_printf (_("Add selected track <i>%.15s%s</i> to"),
				   first_title ?: first_source,
				   strlen (first_title ?: first_source) > 15
				     ? "..." : "");

	  g_free (first_title);
	  g_free (first_source);

	  gtk_label_set_markup (GTK_LABEL (GTK_BIN (button)->child), str);
	  g_free (str);
	  gtk_widget_show (button);
	  gtk_menu_attach (menu, button, 0, 1,
			   submenu_count, submenu_count + 1);

	  submenus[submenu_count] = GTK_MENU (gtk_menu_new ());
	  gtk_widget_show (GTK_WIDGET (submenus[submenu_count]));
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (button),
				     GTK_WIDGET (submenus[submenu_count]));

	  ops[submenu_count] = add_sel;

	  submenu_count ++;
	}


      /* Add queue at the beginning of the menu.  */
      int submenu_pos = 0;

      struct menu_info *main_info = g_malloc (sizeof (*main_info));
      memset (main_info, 0, sizeof (*main_info));
      main_info->st = st;
      main_info->uid = uid;
      main_info->artist = info.artist;
      main_info->album = info.album;
      main_info->selection = selection_list;

      g_signal_connect (G_OBJECT (menu), "selection-done",
			G_CALLBACK (menu_destroy), main_info);


      bool force = true;
      int callback (const char *list)
      {
	if (strcmp (list, "queue") == 0)
	  {
	    if (force)
	      force = false;
	    else
	      return 0;
	  }

	int i;
	for (i = 0; i < submenu_count; i ++)
	  {
	    struct menu_info *info = g_malloc (sizeof (*info));
	    *info = *main_info;

	    info->next = main_info->next;
	    main_info->next = info;

	    info->op = ops[i];
	    info->list = g_strdup (list);

	    button = gtk_menu_item_new_with_label (list);
	    g_signal_connect (G_OBJECT (button), "activate",
			      G_CALLBACK (queue_cb), info);
	    gtk_widget_show (button);
	    gtk_menu_attach (submenus[i], button, 0, 1,
			     submenu_pos, submenu_pos + 1);
	    submenu_pos ++;
	  }

	return 0;
      }

      callback ("queue");
      callback ("new play list");
      music_db_play_lists_for_each (st->db, callback);

      g_free (info.source);
      g_free (info.title);

      gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
		      event->button, event->time);
      return TRUE;
    }

  return FALSE;
}


Starling *
starling_run (void)
{
  Starling *st = calloc (sizeof (*st), 1);
  st->current_length = -1;
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
      g_warning (err->message);
      starling_error_box (err->message);
      g_error_free (err);
      exit (1);
    }

  st->library = play_list_new (st->db, NULL);
  st->queue = play_list_new (st->db, "queue");

  st->player = player_new ();

  g_signal_connect_swapped (G_OBJECT (st->player), "eos",
			    G_CALLBACK (starling_next), st);
  g_signal_connect (G_OBJECT (st->player), "state-changed",
		    G_CALLBACK (player_state_changed), st);
  g_signal_connect_swapped (G_OBJECT (st->player), "tags",
			    G_CALLBACK (music_db_set_info_from_tags), st->db);
    
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
  g_signal_connect_swapped (G_OBJECT (st->window), "delete-event", 
			    G_CALLBACK (starling_quit), st);

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
  mitem = gtk_menu_item_new_with_mnemonic (_("Add _File"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (add_file_cb), st);
  gtk_widget_show (mitem);
#ifdef IS_HILDON
  gtk_menu_shell_append (menu_main, mitem);
#else
  gtk_menu_shell_append (menu, mitem);
#endif

  /* File -> Open directory.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("Add _Directory"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (add_directory_cb), st);
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
  g_signal_connect_swapped (G_OBJECT (mitem), "activate",
			    G_CALLBACK (starling_quit), st);
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
		    G_CALLBACK (remove_cb), st);
  gtk_widget_show (mitem);
  gtk_menu_shell_append (menu, mitem);

  /* Options -> Clear play list.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("_Clear play list"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (clear_cb), st);
  gtk_widget_show (mitem);
  gtk_menu_shell_append (menu, mitem);

  /* Options -> Clear queue.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("Clear _queue"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (clear_queue_cb), st);
  gtk_widget_show (mitem);
  gtk_menu_shell_append (menu, mitem);

  mitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

#ifdef IS_HILDON
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

  /* Options -> Caption Format.  */
  mitem = gtk_menu_item_new_with_mnemonic (_("_Caption Format"));
  g_signal_connect (G_OBJECT (mitem), "activate",
		    G_CALLBACK (change_caption_format), st);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

  /* Options -> Download Lyrics.  */
  mitem = gtk_check_menu_item_new_with_mnemonic (_("Download _Lyrics"));
  st->download_lyrics = GTK_CHECK_MENU_ITEM(mitem);
  gtk_menu_shell_append (menu, mitem);
  gtk_widget_show (mitem);

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
		    G_CALLBACK (prev_cb), st);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Play/pause.  */
  item = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
  st->playpause = GTK_WIDGET (item);
  g_signal_connect (G_OBJECT (st->playpause), "toggled",
		    G_CALLBACK (playpause_cb), st);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  /* Next.  */
  item = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_NEXT);
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (next_cb), st);
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
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->scale),
		      TRUE, TRUE, 0);
    
  st->duration = GTK_LABEL (gtk_label_new ("0:00"));
  gtk_label_set_width_chars (st->duration, 5);
  gtk_misc_set_alignment (GTK_MISC (st->duration), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (st->duration),
		      FALSE, FALSE, 0);

  /* The currently playing song.  */
  /* Stuff the title label in an hbox to prevent it from being
     centered.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
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

  /* Jump to the currently playing song.  */
  GtkWidget *jump_to = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (jump_to),
			gtk_image_new_from_stock (GTK_STOCK_JUMP_TO,
						  GTK_ICON_SIZE_BUTTON));
  g_signal_connect_swapped (G_OBJECT (jump_to), "clicked",
			    G_CALLBACK (jump_to_current), st);
  gtk_box_pack_start (hbox, jump_to, FALSE, FALSE, 0);


  /* The notebook containing the tabs.  */
  st->notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (main_box), GTK_WIDGET (st->notebook));


  /* Library view tab.  Place a search field at the top and the
     library view at the bottom.  */

  vbox = gtk_vbox_new (FALSE, 5);

  /* The currently playing song.  */
  /* Stuff the title label in an hbox to prevent it from being
     centered.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 5));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  st->playlist = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  st->playlist_changed_signal
    = g_signal_connect_swapped (G_OBJECT (st->playlist), "changed",
				G_CALLBACK (play_list_combo_changed), st);
  gtk_box_pack_start (hbox, GTK_WIDGET (st->playlist), FALSE, FALSE, 0);

  st->search_enabled = GTK_TOGGLE_BUTTON (gtk_check_button_new ());
  g_signal_connect_swapped (G_OBJECT (st->search_enabled), "toggled",
			    G_CALLBACK (search_text_changed), st);
  gtk_box_pack_start (hbox, GTK_WIDGET (st->search_enabled), FALSE, FALSE, 0);

  st->searches = gtk_list_store_new (2, GTK_TYPE_STRING, GTK_TYPE_UINT);

  GtkWidget *search_entry_combo
    = gtk_combo_box_entry_new_with_model (GTK_TREE_MODEL (st->searches), 0);
  gtk_box_pack_start (hbox, search_entry_combo, TRUE, TRUE, 0);

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
  gtk_box_pack_start (hbox, clear, FALSE, FALSE, 0);

  st->library_view
    = gtk_tree_view_new_with_model (GTK_TREE_MODEL (st->library));
  g_signal_connect (G_OBJECT (st->library_view), "row-activated",
		    G_CALLBACK (activated_cb), st);
  g_signal_connect (G_OBJECT (st->library_view), "button-press-event",
		    G_CALLBACK (library_button_press_event), st);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (st->library_view), FALSE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (st->library_view), TRUE);
  gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (st->library_view), TRUE);
  GtkTreeSelection *sel = gtk_tree_view_get_selection
    (GTK_TREE_VIEW (st->library_view));
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
    
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  st->library_view_window = GTK_SCROLLED_WINDOW (scrolled);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
    
  gtk_container_add (GTK_CONTAINER (scrolled), st->library_view);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled);
    
  /* XXX: Currently disable as "save as" functionality is
     unimplemented (see musicdb.c).  */
#if 0
  create_button (&st->save, GTK_STOCK_SAVE_AS, l_button_box);
  g_signal_connect (G_OBJECT (st->save), "clicked",
		    G_CALLBACK (save_cb), st);
#endif
    
  st->library_tab = GTK_WIDGET (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
			    gtk_label_new (_("Library")));
  update_library_count (st);

  g_signal_connect_swapped (G_OBJECT (st->library), "row-inserted",
			    G_CALLBACK (update_library_count), st);
  g_signal_connect_swapped (G_OBJECT (st->library), "row-deleted",
			    G_CALLBACK (update_library_count), st);


  /* Play queue tab.  */

  vbox = gtk_vbox_new (FALSE, 5);

  st->queue_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (st->queue));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (st->queue_view), FALSE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (st->queue_view), TRUE);
  gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (st->queue_view), TRUE);
  sel = gtk_tree_view_get_selection
    (GTK_TREE_VIEW (st->queue_view));
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
  st->queue_view_window = GTK_SCROLLED_WINDOW (scrolled);
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


  /* Lyrics tab */

  vbox = gtk_vbox_new (FALSE, 5);
  st->textview = gtk_text_view_new ();
  GTK_TEXT_VIEW (st->textview)->editable = FALSE;

  scrolled = gtk_scrolled_window_new (NULL, NULL);
    
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

  gtk_container_add (GTK_CONTAINER (scrolled), st->textview);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 3);

  gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
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
    
  gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
			    gtk_label_new (_("last.fm")));


  lyrics_init ();

  lastfm_init (st);

  deserialize (st);

  gtk_widget_show_all (st->window);

  return st;
}
