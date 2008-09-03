/* playlist.c - Playlist support.
   Copyright (C) 2007, 2008 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro <skyhusker@handhelds.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#define ERROR_DOMAIN() g_quark_from_static_string ("playlist")

#include <string.h>
#include <glib.h>
#include <gtk/gtktreemodel.h>
#include <gst/gst.h>
#include <gst/audio/gstaudiosink.h>
#include <sqlite.h>

#include "playlist.h"

/* Forward.  */
static void
play_list_get_info_by_uid (PlayList *pl, char *uid,
			   int *index, char **source, 
			   char **artist, char **title, char **album,
			   int *duration);

#define ITER_INIT(model, iter, i) \
  do \
    { \
      (iter)->stamp =  GPOINTER_TO_INT ((model)); \
      (iter)->user_data = GINT_TO_POINTER ((i)); \
    } \
  while (0)

struct _PlayList {
  GObject parent;

  sqlite *sqliteh;
  GstElement *playbin;
  GstState last_state;

  int random;
  /* Position in the playlist.  */
  int position;
  /* Number of entries.  If -1, invalid.  */
  int count;

  /* UIDs of files whose metadata we should try to read.  */
  /* The idle handler.  */
  guint meta_data_reader;
  GQueue *meta_data_reader_pending;
};

static void play_list_dispose (GObject *obj);
static void play_list_finalize (GObject *object);
static void treemodel_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (PlayList, play_list, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                treemodel_iface_init));

static void
play_list_class_init (PlayListClass *klass)
{
  play_list_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = play_list_finalize;
  object_class->dispose = play_list_dispose;

  PlayListClass *play_list_class = PLAY_LIST_CLASS (klass);

  play_list_class->state_changed_signal_id
    = g_signal_new ("state-changed",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_marshal_VOID__ENUM,
		    G_TYPE_NONE, 1,
		    G_TYPE_INT);
    
  play_list_class->eos_signal_id
    = g_signal_new ("eos",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_marshal_VOID__VOID,
		    G_TYPE_NONE, 0);
}

static void
play_list_init (PlayList *pl)
{
  static int gst_init_init;
  if (! gst_init_init)
    {
      gst_init (NULL, NULL);
      gst_init_init = 1;
    }

  pl->count = -1;
}

static void
play_list_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (play_list_parent_class)->dispose (obj);
}

static void
play_list_finalize (GObject *object)
{
  PlayList *pl = PLAY_LIST (object);

  G_OBJECT_CLASS (play_list_parent_class)->finalize (object);

  if (pl->meta_data_reader)
    g_source_remove (pl->meta_data_reader);

  if (pl->meta_data_reader_pending)
    {
      char *d;
      do
	{
	  d = g_queue_pop_head (pl->meta_data_reader_pending);
	  g_free (d);
	}
      while (d);

      g_queue_free (pl->meta_data_reader_pending);
    }

  if (pl->sqliteh)
    sqlite_close (pl->sqliteh);

  if (pl->playbin)
    {
      gst_element_set_state (pl->playbin, GST_STATE_NULL);
      gst_object_unref (pl->playbin);
    }
}

static void
parse_tags (PlayList *pl, GstMessage *message, char *uid)
{
  char *artist = NULL;
  char *title = NULL;
  char *album = NULL;
  char *genre = NULL;

  void for_each_tag (const GstTagList *list,
		     const gchar *tag, gpointer data)
    {
      gchar **value = NULL;

      if (strcmp (tag, "artist") == 0)
	value = &artist;
      else if (strcmp (tag, "title") == 0)
	value = &title;
      else if (strcmp (tag, "album") == 0)
	value = &album;
      else if (strcmp (tag, "genre") == 0)
	value = &genre;

      if (! value)
	return;

      if (gst_tag_get_type (tag) == G_TYPE_STRING)
	gst_tag_list_get_string_index (list, tag, 0, value);
      else
	*value = g_strdup_value_contents
	  (gst_tag_list_get_value_index (list, tag, 0));
    }

  GstTagList *tags;
  gst_message_parse_tag (message, &tags);
  gst_tag_list_foreach (tags, for_each_tag, NULL);

  if (artist || title || album || genre)
    {
      char *err = NULL;
#define Q \
  "update playlist" \
  " set artist = '%q', title = '%q'," \
  "  album = '%q', genre = '%q'"

      if (! uid)
	sqlite_exec_printf (pl->sqliteh,
			    Q " where idx = %d;",
			    NULL, NULL, &err,
			    artist, title, album, genre, pl->position + 1);
      else
	sqlite_exec_printf (pl->sqliteh,
			    Q " where uid = '%q';",
			    NULL, NULL, &err,
			    artist, title, album, genre, uid);
      if (err)
	{
	  g_warning (err);
	  sqlite_freemem (err);
	}

      int i = pl->position;
      if (uid)
	play_list_get_info_by_uid (pl, uid, &i,
				   NULL, NULL, NULL, NULL, NULL);

      GtkTreePath *path
	= gtk_tree_path_new_from_indices (i, -1);
      GtkTreeIter iter;
      ITER_INIT (pl, &iter, i);
      gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);
      g_free (path);
    }
}

static gboolean
play_list_bus_cb (GstBus *bus, GstMessage *message, gpointer data)
{
    PlayList *pl = PLAY_LIST (data);

    switch (GST_MESSAGE_TYPE (message))
      {
      case GST_MESSAGE_ERROR:
	{
	  GError *err = NULL;
	  gchar *debug;
	  gst_message_parse_error (message, &err, &debug);

	  g_debug ("%s: %s (%s)\n", __func__, err->message, debug);

	  g_error_free (err);
	  g_free (debug);

	  play_list_next (pl);
	  break;
	}

      case GST_MESSAGE_STATE_CHANGED:
	if (GST_MESSAGE_SRC (message) == GST_OBJECT (pl->playbin))
	  {
	    GstState state, pending;
	    gst_message_parse_state_changed (message, NULL, &state, &pending);

	    if (pending == GST_STATE_VOID_PENDING)
	      {
		pl->last_state = state;

		g_signal_emit
		  (pl, PLAY_LIST_GET_CLASS (pl)->state_changed_signal_id,
		   0, state);
	      }
	  }
	break;

      case GST_MESSAGE_EOS:
	g_signal_emit (pl, PLAY_LIST_GET_CLASS (pl)->eos_signal_id, 0);
	break;

      case GST_MESSAGE_TAG:
	parse_tags (pl, message, NULL);
	break;

      default:
	break;
      }

    return TRUE;
}

static void
play_list_create_table (PlayList *pl, gboolean drop_first, GError **error)
{
#define CREATE \
  "create table playlist " \
  " (idx INTEGER PRIMARY KEY, " \
  "  uid STRING not NULL, " \
  "  source STRING, " \
  "  artist STRING, " \
  "  title STRING, " \
  "  album STRING, " \
  "  genre STRING, " \
  "  duration INTEGER, " \
  "  rating INTEGER," \
  /* To determine if your information is up to date.  */ \
  "  fsid INTEGER," \
  "  inode INTEGER," \
  "  mtime INTEGER);"

  char *err = NULL;
  /* Create the main table.  */
  if (drop_first)
    sqlite_exec (pl->sqliteh,
		 "begin transaction;"
		 "drop table playlist;",
		 NULL, NULL, &err);
  else
    sqlite_exec (pl->sqliteh,
		 "begin transaction;",
		 NULL, NULL, &err);

  if (err)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, "%s", err);
      sqlite_freemem (err);
      sqlite_exec (pl->sqliteh,
		   "rollback transaction;",
		   NULL, NULL, NULL);
      return;
    }

  sqlite_exec (pl->sqliteh,
	       CREATE
	       "commit transaction;",
	       NULL, NULL, NULL);
}

PlayList *
play_list_open (const char *file, GError **error)
{
  PlayList *pl = PLAY_LIST (g_object_new (PLAY_LIST_TYPE, NULL));

  char *err = NULL;
  pl->sqliteh = sqlite_open (file, 0, &err);
  if (err)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, "Opening %s: %s",
		   file, err);
      sqlite_freemem (err);
      goto error;
    }

  /* Get the DB's version.  */
  sqlite_exec (pl->sqliteh,
	       "create table playlist_version (version integer NOT NULL)",
	       NULL, NULL, NULL);
  int version = -1;
  int dbinfo_callback (void *arg, int argc, char **argv, char **names)
    {
      if (argc == 1)
	version = atoi (argv[0]);

      return 0;
    }
  /* If the play_list_version table doesn't exist then we
     understand this to mean that this DB is uninitialized.  */
  sqlite_exec (pl->sqliteh,
	       "select version from playlist_version",
	       dbinfo_callback, NULL, &err);
  if (err)
    goto generic_error;

  if (version < 1)
    {
      GError *tmp_error = NULL;
      play_list_create_table (pl, FALSE, &tmp_error);
      if (tmp_error)
	{
	  g_propagate_error (error, tmp_error);
	  goto error;
	}

      /* And the config.  */
      sqlite_exec (pl->sqliteh,
		   "create table playlist_config "
		   " (key, STRING, value STRING);",
		   NULL, NULL, NULL);
      if (err)
	goto generic_error;

      sqlite_exec (pl->sqliteh,
		   "insert into playlist_version values (1);",
		   NULL, NULL, &err);
      if (err)
	goto generic_error;
    }

  int config_callback (void *arg, int argc, char **argv, char **names)
    {
      if (argc != 2)
	return 0;

      if (strcmp (argv[0], "random") == 0)
	pl->random = atoi (argv[1]);
      else if (strcmp (argv[0], "position") == 0)
	pl->position = atoi (argv[1]);

      return 0;
    }

  sqlite_exec (pl->sqliteh,
	       "select * from playlist_config",
	       dbinfo_callback, NULL, &err);
  if (err)
    goto generic_error;

  return pl;

 generic_error:
  g_set_error (error, ERROR_DOMAIN (), 0, "%s: %s", file, err);
  sqlite_freemem (err);

 error:
  g_object_unref (pl);
  return NULL;
}

static void
playbin_ensure (PlayList *pl)
{
  if (! pl->playbin)
    {
#ifdef IS_HILDON
      /* Evil, evil, evil, evil.  playbin is broken on the 770.  */
      pl->playbin = gst_element_factory_make ("playbinmaemo", "player");
#else
      pl->playbin = gst_element_factory_make ("playbin", "player");
#endif
      GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pl->playbin));
      gst_bus_add_watch (bus, play_list_bus_cb, pl);
      gst_object_unref (bus);
    }
}

gboolean
play_list_set_sink (PlayList *pl, const gchar *sink)
{
  GstElement *audiosink = gst_element_factory_make (sink, "sink");
  if (! audiosink)
    return FALSE;

  playbin_ensure (pl);

  g_object_set (G_OBJECT (pl->playbin), "audio-sink", audiosink, NULL);

  return TRUE;
}

void
play_list_set_random (PlayList *pl, gboolean random)
{
  pl->random = random;
}

gboolean
play_list_get_random (PlayList *pl)
{
  return pl->random;
}

gint
play_list_count (PlayList *pl)
{
  if (pl->count != -1)
    return pl->count;

  int count = 0;

  int callback (void *arg, int argc, char **argv, char **names)
    {
      count = atoi (argv[0]);

      return 0;
    }

  char *err = NULL;
  sqlite_exec (pl->sqliteh,
	       "select count (*) from playlist",
	       callback, NULL, &err);
  if (err)
    {
      g_critical ("%s", err);
      sqlite_freemem (err);
      return 0;
    }

  pl->count = count;
  return count;
}

gboolean
play_list_play (PlayList *pl)
{
  playbin_ensure (pl);

  char *source = NULL;
  int callback (void *arg, int argc, char **argv, char **names)
    {
      source = g_strdup (argv[0]);

      return 1;
    }
  char *err = NULL;
  sqlite_exec_printf (pl->sqliteh,
		      "select source from playlist where idx = %d;",
		      callback, NULL, &err, pl->position + 1);
  if (err)
    {
      g_warning (err);
      sqlite_freemem (err);
      return FALSE;
    }
  if (! source)
    {
      g_warning ("No SOURCE found for entry %d", pl->position);
      return FALSE;
    }

  gst_element_set_state (pl->playbin, GST_STATE_NULL);
  g_object_set (G_OBJECT (pl->playbin), "uri", source, NULL);
  GstStateChangeReturn res = gst_element_set_state (pl->playbin,
						    GST_STATE_PLAYING);
  if (res == GST_STATE_CHANGE_FAILURE)
    {
      g_warning ("Failed to play %s", source);
      return FALSE;
    }

  g_free (source);
  return TRUE;
}

void
play_list_pause (PlayList *pl)
{
  if (pl->last_state == GST_STATE_PLAYING && pl->playbin)
    gst_element_set_state (pl->playbin, GST_STATE_PAUSED);
}

void
play_list_unpause (PlayList *pl)
{
  if (pl->playbin)
    {
      if (pl->last_state != GST_STATE_PLAYING)
	gst_element_set_state (pl->playbin, GST_STATE_PLAYING);
    }
  else
    play_list_play (pl);
}

void
play_list_play_pause_toggle (PlayList *pl)
{
  if (! pl->playbin)
    {
      play_list_play (pl);
      return;
    }

  if (play_list_playing (pl))
    gst_element_set_state (pl->playbin, GST_STATE_PAUSED);
  else
    gst_element_set_state (pl->playbin, GST_STATE_PLAYING);
}

gboolean
play_list_playing (PlayList *pl)
{
  GstState state = GST_STATE_PAUSED;
  if (pl->playbin)
    gst_element_get_state (pl->playbin, &state, NULL, GST_CLOCK_TIME_NONE);

  return state == GST_STATE_PLAYING;
}

gint
play_list_get_current (PlayList *pl)
{
  return pl->position;
}

static int
do_goto (PlayList *pl, int n)
{
  g_return_val_if_fail (0 <= n && n <= play_list_count (pl), FALSE);

  int o = pl->position;
  pl->position = n;

  GtkTreePath *path = gtk_tree_path_new_from_indices (o, -1);
  GtkTreeIter iter;
  ITER_INIT (pl, &iter, o);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);
  g_free (path);

  path = gtk_tree_path_new_from_indices (n, -1);
  ITER_INIT (pl, &iter, n);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);
  g_free (path);

  return play_list_play (pl);
}

void
play_list_goto (PlayList *pl, gint n)
{
  do_goto (pl, n);

  if (! play_list_play (pl))
    play_list_next (pl);
}

static gint
play_list_random_next (PlayList *pl)
{
  if (play_list_count (pl) == 0)
    return 0;

  static int rand_init;
  if (! rand_init)
    {
      srand (time (NULL));
      rand_init = 1;
    }

  return rand () % play_list_count (pl);
}
    
void
play_list_next (PlayList *pl)
{
  int c = play_list_count (pl);
  int n;
  do
    {
      if (pl->random)
	n = play_list_random_next (pl);
      else
	{
	  n = pl->position + 1;

	  if (n >= play_list_count (pl))
	    n = 0;
	}

      c --;
    }
  while (! do_goto (pl, n) && c > 0);
}

void
play_list_prev (PlayList *pl)
{
  int c = play_list_count (pl);
  int n;
  do
    {
      if (pl->random)
	n = play_list_random_next (pl);
      else
	{
	  n = pl->position - 1;

	  if (n < 0)
	    n = play_list_count (pl) - 1;
	}

      c --;
    }
  while (! do_goto (pl, n) && c > 0);
}

gboolean
play_list_seek (PlayList *pl, GstFormat format, gint64 pos)
{
  if (! pl->playbin)
    return TRUE;

  return gst_element_seek (GST_ELEMENT (pl->playbin), 1.0, format,
			   GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
			   pos, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

gboolean
play_list_query_position (PlayList *pl, GstFormat *fmt, gint64 *pos)
{
  if (! pl->playbin)
    {
      *pos = 0;
      return TRUE;
    }
  return gst_element_query_position (pl->playbin, fmt, pos); 
}

gboolean
play_list_query_duration (PlayList *pl, GstFormat *fmt, gint64 *pos, gint n)
{
  if (! pl->playbin)
    return FALSE;
  return gst_element_query_duration (pl->playbin, fmt, pos);
}

static char *
make_uid (void)
{
  return g_strdup_printf ("%x%x%x%x",
			  rand (), rand (), rand (),
			  (unsigned int) time (NULL));
}

/* Idle function which is called when meta data should be read.  */
static gboolean
meta_data_reader (gpointer data)
{
  PlayList *pl = PLAY_LIST (data);

  GstElement *pipeline = NULL;
  GstElement *src;
  GstBus *bus;

  /* 200702 - NHW

     A streamer's tags are typically delivered before we reach the
     PAUSED state.  Thus, we just need to wait until the paused state
     is reached.  Unfortunately, there is a bug in the oggdemuxer such
     that if the stream is immediately changed to the NULL state, we
     get a crash.  It is scheduled to be fix in 0.10.12.  We can work
     around it be going to the PLAY.  The tradeoff is that it is a bit
     more expensive.  */
  guint major, minor, micro, nano;
  gst_version (&major, &minor, &micro, &nano);
  GstState target_state = GST_STATE_PAUSED;
  if (major == 0 && minor < 12)
    target_state = GST_STATE_PLAYING;

  while (! g_queue_is_empty (pl->meta_data_reader_pending))
    {
      char *uid = g_queue_pop_head (pl->meta_data_reader_pending);
      g_assert (uid);

      char *source = NULL;
      play_list_get_info_by_uid (pl, uid, NULL,
				 &source, NULL, NULL, NULL, NULL);
      if (! source)
	/* Hmm, entry disappeared.  It's possible: the user may have
	   removed it before we got to processing it.  */
	{
	  g_free (uid);
	  continue;
	}

      if (! pipeline)
	{
	  /* We read the data from a file, send it to decodebin which
	     selects the appropriate decoder and then send the result
	     to /dev/null.  */
	  pipeline = gst_pipeline_new ("meta_data_pipeline");
	  src = gst_element_factory_make ("filesrc", "filesrc");
	  GstElement *decodebin = gst_element_factory_make ("decodebin",
							    "decodebin");
	  GstElement *sink = gst_element_factory_make ("fakesink", "sink");

	  gst_bin_add_many (GST_BIN (pipeline), src, decodebin, sink, NULL);
	  gst_element_link_many (src, decodebin, sink, NULL);

	  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	}

#define FILE_PREFIX "file://"
      g_assert (strlen (source) > sizeof (FILE_PREFIX) - 1);
      g_object_set (G_OBJECT (src),
		    "location", source + sizeof (FILE_PREFIX) - 1, NULL);
      g_free (source);

      GstStateChangeReturn res = gst_element_set_state (pipeline,
							target_state);
      if (res == GST_STATE_CHANGE_FAILURE)
	{
	  g_warning ("Failed to play %s", uid);
	  g_free (uid);
	  continue;
	}

      int c = 0;
      for (;;)
	{
	  c ++;
	  GstMessage *msg = gst_bus_poll (bus,
					  GST_MESSAGE_EOS | GST_MESSAGE_ERROR
					  | GST_MESSAGE_TAG
					  | GST_MESSAGE_STATE_CHANGED,
					  /* Wait about two seconds.  */
					  2 * 1000 * 1000 * 1000);
	  if (! msg)
	    break;

	  switch (GST_MESSAGE_TYPE (msg))
	    {
	    case GST_MESSAGE_TAG:
	      parse_tags (pl, msg, uid);
	      continue;

	    case GST_MESSAGE_STATE_CHANGED:
	      /* Once we've reached the paused state, for most
		 formats, most of the time, all tags will have been
		 reported.  (The exception is essentially streams--but
		 we are only looking at files anyways so this should
		 be relatively rare.)  */
	      {
		GstState state, pending;
		gst_message_parse_state_changed (msg, NULL, &state, &pending);
		if (! (pending == GST_STATE_VOID_PENDING
		       && state == target_state))
		  continue;
	      }
	      break;

	    default:
	      break;
	    }

	  break;
	}

      res = gst_element_set_state (pipeline, GST_STATE_NULL);
      if (res == GST_STATE_CHANGE_FAILURE)
	g_warning ("Failed to put pipeline in NULL state.");
      g_free (uid);
    }

  pl->meta_data_reader = 0;

  if (pipeline)
    {
      gst_object_unref (pipeline);
      gst_object_unref (bus);
    }

  return FALSE;
}

static void
play_list_add (PlayList *pl, char *sources[], int count,
	       gint index, GError **error)
{
  /* We can't insert with a position great than the number of entries
     in the database.  */
  if (index > pl->count)
    index = pl->count;

  char *err = NULL;
  sqlite_exec (pl->sqliteh, "begin transaction;",
	       NULL, NULL, &err);
  if (err)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, "%s", err);
      sqlite_freemem (err);
      return;
    }

  /* XXX: We should stat the SOURCE first to get the metadata.  */
  if (index <= 0)
    /* Append.  */
    index = pl->count;
  else
    /* Insert at a specific position.  */
    {
      sqlite_exec_printf (pl->sqliteh,
			  "update playlist set idx = idx + %d"
			  " where idx >= %d;",
			  NULL, NULL, &err,
			  count, index + 1);
      if (err)
	goto error;
    }

  int i;
  for (i = 0; i < count; i ++)
    {
      char *uid = make_uid ();
      sqlite_exec_printf (pl->sqliteh,
			  "insert into playlist"
			  " (idx, uid, source) values (%d, '%q', '%q');",
			  NULL, NULL, &err,
			  index + 1 + i, uid, sources[i]);
      if (err)
	{
	  g_free (uid);
	  break;
	}

      /* Right now, we only stat local files.  */
      if (strncmp (FILE_PREFIX, sources[i], sizeof (FILE_PREFIX) - 1) == 0)
	{
	  if (! pl->meta_data_reader)
	    pl->meta_data_reader = g_idle_add (meta_data_reader, pl);

	  if (! pl->meta_data_reader_pending)
	    pl->meta_data_reader_pending = g_queue_new ();

	  g_queue_push_tail (pl->meta_data_reader_pending, uid);
	}
    }

 error:
  if (err)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, "%s", err);
      sqlite_freemem (err);

      sqlite_exec (pl->sqliteh,
		   "rollback transaction;", NULL, NULL, NULL);
    }
  else
    sqlite_exec (pl->sqliteh,
		 "commit transaction;", NULL, NULL, NULL);

  pl->count += count;

  GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);

  GtkTreeIter iter;
  ITER_INIT (pl, &iter, index);

  gtk_tree_model_row_inserted (GTK_TREE_MODEL (pl), path, &iter);

  g_free (path);
}

int
play_list_add_m3u (PlayList *pl, const gchar *path, int pos,
		   GError **error) 
{
  gchar *content;
  gchar **lines;
  gint ii;
  gsize size;

  GError *tmp_error = NULL;
  if (! g_file_get_contents (path, &content, &size, &tmp_error))
    {
      g_debug ("Error reading file %s: %s\n", path,
	       tmp_error->message);
      g_propagate_error (error, tmp_error);
      return 0;
    }

  lines = g_strsplit (content, "\n", 2048); /* This should be enough */ 
  g_free (content);

  /* In case we have relative file names (which we likely do), get the
     base.  */
  char *end = strrchr (path, '/');
  if (end)
    *end = '\0';

  int count = 0;
  for (ii = 0; lines[ii]; ii++)
    {
      char *f = lines[ii];
      while (*f == ' ')
	f ++;

      /* Ignore comments.  */
      if (*f != '#' && *f)
	{
	  int l = strlen (f);
	  if (f[l - 1] == '\r')
	    /* Chop off any trailing cr.  */
	    f[l -- - 1] = 0;
	  /* And any trailing spaces.  */
	  while (f[l - 1] == ' ')
	    f[l -- - 1] = 0;

	  char *filename;
	  if (*f != '/')
	    /* Relative path.  */
	    filename = g_strdup_printf ("%s/%s", path, f);
	  else
	    filename = f;

	  play_list_add_file (pl, filename, pos, error);
	  if (pos != -1)
	    pos ++;
	  count ++;
	  if (*f != '/')
	    g_free (filename);
	}
    }

  g_strfreev (lines);

  return count;
}

void
play_list_add_uri (PlayList *pl, const char *uri, int index,
		   GError **error)
{
  char *sources[1] = { (char *) uri };
  play_list_add (pl, sources, 1, index, error);
}

void
play_list_add_file (PlayList *pl, const gchar *file, int index,
		    GError **error)
{
#define P "file://"
  char source[sizeof (P) + strlen (file)];
  strcpy (source, P);
  strcpy (source + sizeof (P) - 1, file);

  /* XXX: We need to escape this!  Filenames with '%' will silently
     fail.  */
  char *sources[1] = { source };
  play_list_add (pl, sources, 1, index, error);
}

static int
has_audio_extension (const char *filename)
{
  const char *whitelist[] = { ".ogg", ".OGG",
			      ".mp3", ".MP3",
			      ".rm", ".RM",
			      ".wav", ".WAV"
  };

  int i;
  for (i = 0; i < sizeof (whitelist) / sizeof (whitelist[0]); i ++)
    if (g_str_has_suffix (filename, whitelist[i]))
      return 1;

  if (i == sizeof (whitelist) / sizeof (whitelist[0]))
    /* Unknown extension.  */
    return 0;
}

int
play_list_add_recursive (PlayList *pl, const gchar *path, int pos,
			 GError **error)
{
  int count = 0;
  GDir *dir = g_dir_open (path, 0, NULL);
  if (dir)
    {
      const char *file;
      while ((file = g_dir_read_name (dir)))
	{
	  char *filename = g_strdup_printf ("%s/%s", path, file);
	  GError *tmp_error = NULL;
	  count += play_list_add_recursive (pl, filename,
					    pos == -1 ? -1 : pos + count,
					    &tmp_error);
	  g_free (filename);
	  if (tmp_error)
	    {
	      g_propagate_error (error, tmp_error);
	      break;
	    }

	  /* We really want this to be a background job.  Invoke the
	     main loop.  */
	  g_main_context_iteration (NULL, FALSE);
	}
        
      g_dir_close (dir);
      return count;
    }
  else
    {
      struct stat stat;
      int ret = g_stat (path, &stat);
      if (ret < 0)
	/* Failed to read it.  Perhaps we don't have permission.  */
	return 0;

      if (! S_ISREG (stat.st_mode))
	/* Not a regular file.  */
	return 0;
    }
    
  if (g_str_has_suffix (path, ".m3u"))
    /* Ignore m3u files: we will normally get the files simply by
       recursing.  */
    return 0;

  if (has_audio_extension (path))
    /* Ignore files that do not appear to be audio files.  */
    play_list_add_file (pl, path, pos, error);

  return 1;
}
    
void
play_list_save_m3u (PlayList *self, const gchar *path)
{
  /* XXX: Implement me.  */
#if 0
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    GList *cur;
    GString *string = NULL;

    if (g_list_length (priv->tracks) == 0) {
        /* Remove the previous playlist */
        g_unlink (path);
        return;
    }

    string = g_string_new ("");

    //g_debug ("PlayList length: %d\n", g_list_length (priv->tracks));

    //play_list_dump (self);
    
    for (cur = priv->tracks; cur; cur = cur->next) {
        Stream *s = cur->data;
        string = g_string_append (string, s->source + 7);
        string = g_string_append (string, "\n");
    }

    g_file_set_contents (path, string->str, string->len, NULL);

    g_string_free (string, TRUE);
#endif
}

void
play_list_swap_pos (PlayList *pl, gint left, gint right)
{
  g_return_if_fail (0 <= left && left < pl->count);
  g_return_if_fail (0 <= right && right < pl->count);

  if (left == right)
    return;

  char *err = NULL;
  sqlite_exec_printf (pl->sqliteh,
		      "begin transaction;"
		      "update playlist set idx = %d where idx = %d;"
		      "update playlist set idx = %d where idx = %d;"
		      "update playlist set idx = %d where idx = %d;"
		      "commit transaction;",
		      NULL, NULL, &err,
		      pl->count + 1, left + 1,
		      left + 1, right + 1,
		      right + 1, pl->count + 1);
  if (err)
    {
      g_warning (err);
      sqlite_freemem (err);
      sqlite_exec (pl->sqliteh, "rollback transaction;", NULL, NULL, NULL);
      return;
    }

  GtkTreePath *path = gtk_tree_path_new_from_indices (left, -1);
  GtkTreeIter iter;
  ITER_INIT (pl, &iter, left);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);
  g_free (path);

  path = gtk_tree_path_new_from_indices (right, -1);
  ITER_INIT (pl, &iter, right);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);
  g_free (path);
}

void
play_list_remove (PlayList *pl, gint index)
{
  g_return_if_fail (0 <= index && index < pl->count);

  char *err = NULL;
  sqlite_exec_printf (pl->sqliteh,
		      "begin transaction;"
		      "delete from playlist where idx = %d;"
		      "update playlist set idx = idx - 1 where idx >= %d;"
		      "commit transaction;",
		      NULL, NULL, &err, index + 1, index + 1);
  if (err)
    {
      sqlite_exec (pl->sqliteh,
		   "rollback transaction;", NULL, NULL, NULL);
      g_warning ("%s: %s", __func__, err);
      sqlite_freemem (err);
    }

  pl->count --;

  GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
  gtk_tree_model_row_deleted (GTK_TREE_MODEL (pl), path);
  g_free (path);
}

void
play_list_clear (PlayList *pl)
{
  GError *tmp_error = NULL;
  play_list_create_table (pl, TRUE, &tmp_error);
  /* XXX */
  if (tmp_error)
    {
      g_error_free (tmp_error);
      return;
    }

  GtkTreePath *path = gtk_tree_path_new_from_indices (0, -1);

  for (; pl->count; pl->count --)
    gtk_tree_model_row_deleted (GTK_TREE_MODEL (pl), path);
  g_free (path);
}

void
play_list_get_info (PlayList *pl, gint index, char **source, char **uid,
		    char **artist, char **title, char **album,
		    int *duration)
{
  if (index < 0)
    index = pl->position;

  if (source)
    *source = NULL;
  if (uid)
    *uid = NULL;
  if (artist)
    *artist = NULL;
  if (title)
    *title = NULL;
  if (album)
    *album = NULL;
  if (duration)
    *duration = 0;

  int callback (void *arg, int argc, char **argv, char **names)
    {
      int i = 0;
      if (source)
	*source = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (uid)
	*uid = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (artist)
	*artist = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (title)
	*title = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (album)
	*album = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (duration)
	*duration = argv[i] ? atoi (argv[i]) : 0;

      return 1;
    }

  char *err = NULL;
  sqlite_exec_printf (pl->sqliteh,
		      "select source, uid, artist, title, album, duration "
		      " from playlist where idx = %d;",
		      callback, NULL, &err, index + 1);
  if (err)
    {
      g_warning (err);
      sqlite_freemem (err);
    }
}

static void
play_list_get_info_by_uid (PlayList *pl, char *uid,
			   int *index, char **source, 
			   char **artist, char **title, char **album,
			   int *duration)
{
  g_assert (uid);

  if (index)
    *index = 0;
  if (source)
    *source = NULL;
  if (artist)
    *artist = NULL;
  if (title)
    *title = NULL;
  if (album)
    *album = NULL;
  if (duration)
    *duration = 0;

  int callback (void *arg, int argc, char **argv, char **names)
    {
      int i = 0;
      if (index)
	*index = argv[i] ? atoi (argv[i]) : 0;

      i ++;
      if (source)
	*source = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (artist)
	*artist = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (title)
	*title = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (album)
	*album = argv[i] ? g_strdup (argv[i]) : NULL;

      i ++;
      if (duration)
	*duration = argv[i] ? atoi (argv[i]) : 0;

      return 1;
    }

  char *err = NULL;
  sqlite_exec_printf (pl->sqliteh,
		      "select idx - 1, source, artist, title, album, duration "
		      " from playlist where uid = '%q';",
		      callback, NULL, &err, uid);
  if (err)
    {
      g_warning (err);
      sqlite_freemem (err);
    }
}

/* Tree model interface.  */
static GtkTreeModelFlags
get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
get_n_columns (GtkTreeModel *tree_model)
{
  return PL_COL_COUNT;
}

static GType
get_column_type (GtkTreeModel *tree_model, gint col)
{
  g_return_val_if_fail (0 <= col && col < PL_COL_COUNT, G_TYPE_INVALID);

  switch (col)
    {
    case PL_COL_INDEX:
    case PL_COL_DURATION:
      return G_TYPE_INT;

    case PL_COL_UID:
    case PL_COL_SOURCE:
    case PL_COL_ARTIST:
    case PL_COL_TITLE:
    case PL_COL_ALBUM:
      return G_TYPE_STRING;

    default:
      g_assert (! "Bad column.");
      return 0;
    }
}

static gboolean
get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
  ITER_INIT (tree_model, iter, -1);

  gint *indices = gtk_tree_path_get_indices (path);
  if (indices[0] == -1)
    return FALSE;

  if (indices[0] >= play_list_count (PLAY_LIST (tree_model)))
    return FALSE;

  ITER_INIT (tree_model, iter, indices[0]);
  return TRUE;
}

static GtkTreePath *
get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (iter->user_data != (gpointer) -1, NULL);

  return gtk_tree_path_new_from_indices ((int) iter->user_data, -1);
}

static void
get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column,
	   GValue *value)
{
  PlayList *pl = PLAY_LIST (tree_model);
  int i = (int) iter->user_data;

  g_return_if_fail (i != -1);

  switch (column)
    {
    case PL_COL_INDEX:
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, i);
      return;

    case PL_COL_DURATION:
      {
	int d;
	play_list_get_info (pl, i, NULL, NULL, NULL, NULL, NULL, &d);
	g_value_init (value, G_TYPE_INT);
	g_value_set_int (value, d);
	return;
      }

    case PL_COL_SOURCE:
      {
	char *s;
	play_list_get_info (pl, i, &s, NULL, NULL, NULL, NULL, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    case PL_COL_UID:
      {
	char *s;
	play_list_get_info (pl, i, NULL, &s, NULL, NULL, NULL, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    case PL_COL_ARTIST:
      {
	char *s;
	play_list_get_info (pl, i, NULL, NULL, &s, NULL, NULL, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    case PL_COL_TITLE:
      {
	char *s;
	play_list_get_info (pl, i, NULL, NULL, NULL, &s, NULL, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    case PL_COL_ALBUM:
      {
	char *s;
	play_list_get_info (pl, i, NULL, NULL, NULL, NULL, &s, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    default:
      g_assert (! "Bad column.");
      return;
    }
}

static gboolean
iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (iter->user_data != (gpointer) -1, FALSE);

  PlayList *pl = PLAY_LIST (tree_model);

  int pos = 1 + (int) iter->user_data;
  ITER_INIT (tree_model, iter, pos);
  if (pos == play_list_count (pl))
    {
      ITER_INIT (tree_model, iter, -1);
      return FALSE;
    }
  return TRUE;
}

static gboolean
iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter,
	       GtkTreeIter *parent)
{
  g_return_val_if_fail (iter->user_data != (gpointer) -1, FALSE);

  iter->stamp = -1;
  iter->user_data = (gpointer) -1;

  if (! parent && play_list_count (PLAY_LIST (tree_model)) > 0)
    {
      ITER_INIT (tree_model, iter, 0);
      return TRUE;
    }
  return FALSE;
}

static gboolean
iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_assert (iter->user_data != (gpointer) -1);

  return FALSE;
}

static gint
iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_assert (iter->user_data != (gpointer) -1);

  if (! iter)
    return play_list_count (PLAY_LIST (tree_model));
  return 0;
}

static gboolean
iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
		GtkTreeIter *parent, gint n)
{
  if (! parent)
    {
      if (n >= play_list_count (PLAY_LIST (tree_model)))
	/* Not a valid node.  */
	return FALSE;

      ITER_INIT (tree_model, iter, n);
      return TRUE;
    }

  g_assert (parent->user_data != (gpointer) -1);

  return FALSE;
}

static gboolean
iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
  g_assert (child->user_data != (gpointer) -1);
  ITER_INIT (tree_model, iter, -1);
  return FALSE;
}

static void
ref_node (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
}

static void
unref_node (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
}

static void
treemodel_iface_init (gpointer g_iface, gpointer iface_data)
{
  GtkTreeModelIface *iface = (GtkTreeModelIface *) g_iface;

  iface->get_flags = get_flags;
  iface->get_n_columns = get_n_columns;
  iface->get_column_type = get_column_type;
  iface->get_iter = get_iter;
  iface->get_path = get_path;
  iface->get_value = get_value;
  iface->iter_next = iter_next;
  iface->iter_children = iter_children;
  iface->iter_has_child = iter_has_child;
  iface->iter_n_children = iter_n_children;
  iface->iter_nth_child = iter_nth_child;
  iface->iter_parent = iter_parent;
  iface->ref_node = ref_node;
  iface->unref_node = unref_node;
}
