/* musicdb.c - Music DB support.
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

#define _GNU_SOURCE

#define ERROR_DOMAIN() g_quark_from_static_string ("musicdb")

#include "musicdb.h"
#include "marshal.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <sqlite.h>
#include <string.h>
#include <stdio.h>
#include <gst/gst.h>

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>

/* We cache the DB entries using a simple cache.  Cache entries are
   keyed on the UID.  */
#include "cache.h"

struct info_cache_entry
{
  char *source;
  char *artist;
  char *album;
  char *title;
  int track;
  int duration;
  char *genre;
};

void
info_cache_evict (void *object)
{
  struct info_cache_entry *e = object;

  free (e->source);
  free (e->artist);
  free (e->album);
  free (e->title);
  free (e->genre);
  free (e);
}

static struct simple_cache info_cache;

static void info_cache_constructor (void) __attribute__ ((constructor));

static void
info_cache_constructor (void)
{
  simple_cache_init (&info_cache, info_cache_evict);
}

struct _MusicDB
{
  GObject parent;

  sqlite *sqliteh;

  /* UIDs of files whose metadata we should try to read.  */
  GStaticMutex meta_data_reader_mutex;
  GQueue *meta_data_reader_pending;
  /* The idle handler.  */
  guint meta_data_reader;

  /* The idle handler.  */
  guint fs_scanner;
};

static void music_db_dispose (GObject *obj);
static void music_db_finalize (GObject *object);

G_DEFINE_TYPE (MusicDB, music_db, G_TYPE_OBJECT);

static void
music_db_class_init (MusicDBClass *klass)
{
  music_db_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = music_db_finalize;
  object_class->dispose = music_db_dispose;

  MusicDBClass *music_db_class = MUSIC_DB_CLASS (klass);

  music_db_class->new_entry_signal_id
    = g_signal_new ("new-entry",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_marshal_VOID__UINT,
		    G_TYPE_NONE, 1, G_TYPE_UINT);

  music_db_class->changed_entry_signal_id
    = g_signal_new ("changed-entry",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_marshal_VOID__UINT,
		    G_TYPE_NONE, 1, G_TYPE_UINT);

  music_db_class->deleted_entry_signal_id
    = g_signal_new ("deleted-entry",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_marshal_VOID__UINT,
		    G_TYPE_NONE, 1, G_TYPE_UINT);

  music_db_class->cleared_signal_id
    = g_signal_new ("cleared",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_marshal_VOID__VOID,
		    G_TYPE_NONE, 0);

  music_db_class->added_to_play_list_signal_id
    = g_signal_new ("added-to-play-list",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_user_marshal_VOID__POINTER_INT,
		    G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);

  music_db_class->removed_from_play_list_signal_id
    = g_signal_new ("removed-from-play-list",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_user_marshal_VOID__POINTER_INT,
		    G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);
}

static void
music_db_init (MusicDB *db)
{
}

static void
music_db_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (music_db_parent_class)->dispose (obj);
}

static void
music_db_finalize (GObject *object)
{
  MusicDB *db = MUSIC_DB (object);

  G_OBJECT_CLASS (music_db_parent_class)->finalize (object);

  if (db->meta_data_reader)
    g_source_remove (db->meta_data_reader);

  if (db->fs_scanner)
    g_source_remove (db->fs_scanner);

  if (db->meta_data_reader_pending)
    {
      char *d;
      do
	{
	  d = g_queue_pop_head (db->meta_data_reader_pending);
	  g_free (d);
	}
      while (d);

      g_queue_free (db->meta_data_reader_pending);
    }

  if (db->sqliteh)
    sqlite_close (db->sqliteh);
}

static void
music_db_create_table (MusicDB *db, gboolean drop_first, GError **error)
{
  char *err = NULL;
  /* Create the main table.  */
  if (drop_first)
    sqlite_exec (db->sqliteh,
		 "begin transaction;"
		 "drop table files;"
		 "drop table dirs;"
		 "drop table playlists;",
		 NULL, NULL, &err);
  else
    sqlite_exec (db->sqliteh,
		 "begin transaction;",
		 NULL, NULL, &err);

  if (err)
    {
      g_critical ("%s: %s", __FUNCTION__, err);
      g_set_error (error, ERROR_DOMAIN (), 0, "%s", err);
      sqlite_freemem (err);
      sqlite_exec (db->sqliteh,
		   "rollback transaction;",
		   NULL, NULL, NULL);
      return;
    }

  sqlite_exec (db->sqliteh,
	       /* Create the main table for the files.  */
	       "create table files "
	       " (source STRING NOT NULL UNIQUE, "
	       /* Metadata.  */ 
	       "  artist STRING COLLATE NOCASE, "
	       "  album STRING COLLATE NOCASE, "
	       "  track INTEGER, "
	       "  title STRING COLLATE NOCASE, "
	       "  genre STRING COLLATE NOCASE, "
	       "  duration INTEGER, "
	       "  rating INTEGER,"
	       /* To determine if the information is up to date.  */
	       "  mtime INTEGER);"

	       /* Create the table for the directories.  */
	       "create table dirs (filename STRING); "
	       "commit transaction;"

	       /* Create a table for the play lists.  */
	       "create table playlists (list STRING, uid INTEGER);",
	       NULL, NULL, NULL);
}

struct fs_scanner
{
  MusicDB *db;
  sqlite *sqliteh;
};

/* Forward.  */
static gboolean fs_scanner (gpointer data);

static int
busy_handler (void *cookie, const char *table, int retries)
{
  /* If this is the main thread, then we'd like to recursively invoke
     the main loop, however, not everything is reentrant so...  */

  if (retries > 4)
    retries = 4;

  /* In milliseconds.  */
  int timeout = 100 << (retries - 1);

  /* Sleep and then try again.  */
  struct timespec ts;
  ts.tv_nsec = (timeout % 1000) * (1000000ULL);
  ts.tv_sec = timeout / 1000;

  nanosleep (&ts, &ts);

  return 1;
}


MusicDB *
music_db_open (const char *file, GError **error)
{
  MusicDB *db = MUSIC_DB (g_object_new (MUSIC_DB_TYPE, NULL));

  char *err = NULL;
  db->sqliteh = sqlite_open (file, 0, &err);
  if (err)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, "Opening %s: %s",
		   file, err);
      sqlite_freemem (err);
      goto error;
    }

  sqlite_busy_handler (db->sqliteh, busy_handler, NULL);

  /* Get the DB's version.  */
  sqlite_exec (db->sqliteh,
	       "create table playlist_version (version integer NOT NULL)",
	       NULL, NULL, NULL);
  int version = -1;
  int dbinfo_callback (void *arg, int argc, char **argv, char **names)
    {
      if (argc == 1)
	version = atoi (argv[0]);

      return 0;
    }
  /* If the music_db_version table doesn't exist then we
     understand this to mean that this DB is uninitialized.  */
  sqlite_exec (db->sqliteh,
	       "select version from playlist_version",
	       dbinfo_callback, NULL, &err);
  if (err)
    goto generic_error;

  if (version < 2)
    {
      if (version == 1)
	sqlite_exec (db->sqliteh, "drop table playlist", NULL, NULL, NULL);

      GError *tmp_error = NULL;
      music_db_create_table (db, FALSE, &tmp_error);
      if (tmp_error)
	{
	  g_propagate_error (error, tmp_error);
	  goto error;
	}

      sqlite_exec (db->sqliteh,
		   "insert into playlist_version values (2);",
		   NULL, NULL, &err);
      if (err)
	goto generic_error;
    }

  err = NULL;
  sqlite *sqliteh = sqlite_open (file, 0, &err);
  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      g_free (err);
    }
  else
    {
      sqlite_busy_handler (sqliteh, busy_handler, NULL);

      struct fs_scanner *fs = calloc (sizeof (*fs), 1);
      fs->db = db;
      fs->sqliteh = sqliteh;

      db->fs_scanner = g_idle_add (fs_scanner, fs);
    }

  return db;

 generic_error:
  g_set_error (error, ERROR_DOMAIN (), 0, "%s: %s", file, err);
  sqlite_freemem (err);

 error:
  g_object_unref (db);
  return NULL;
}

gint
music_db_count (MusicDB *db, const char *list, const char *constraint)
{
  if (constraint && ! *constraint)
    constraint = NULL;

  int count = 0;

  int callback (void *arg, int argc, char **argv, char **names)
  {
    count = atoi (argv[0]);

    return 0;
  }

  char *err = NULL;
  if (list)
    sqlite_exec_printf (db->sqliteh,
			"select count(*) from playlists left join files"
			"  on playlists.uid = files.rowid"
			" where list = '%q' %s%s%s",
			callback, NULL, &err, list,
			constraint ? "and (" : "",
			constraint ? constraint : "",
			constraint ? ")" : "");
  else
    sqlite_exec_printf (db->sqliteh,
			"select count (*) from files %s%s",
			callback, NULL, &err,
			constraint ? "where " : "",
			constraint ? constraint : "");
  if (err)
    {
      g_critical ("%s", err);
      sqlite_freemem (err);
      return 0;
    }

  return count;
}

/* Forward.  */
static int music_db_add_recursive_internal (MusicDB *db, sqlite *sqliteh,
					    const gchar *path,
					    GError **error);

static gpointer
fs_scanner_thread (gpointer data)
{
  struct fs_scanner *fs = data;
  MusicDB *db = MUSIC_DB (fs->db);
  sqlite *sqliteh = fs->sqliteh;

  GQueue *q = g_queue_new ();

  int check_dead_cb (void *arg, int argc, char **argv, char **names)
  {
    g_queue_push_tail (q, g_strdup (argv[0] + strlen ("file://")));
    return 0;
  }

  char *err = NULL;
  sqlite_exec_printf (sqliteh,
		      "select (source) from files"
		      " where source like 'file:///%';",
		      check_dead_cb, NULL, &err);
  if (err)
    {
      g_debug ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);
    }


  while (! g_queue_is_empty (q))
    {
      char *filename = g_queue_pop_head (q);
      
      struct stat st;
      int ret = g_stat (filename, &st);
      if (ret < 0 || ! S_ISREG (st.st_mode))
	{
	  sqlite_exec_printf (sqliteh,
			      "delete from files"
			      " where source = 'file://%s';",
			      NULL, NULL, &err, filename);
	  if (err)
	    {
	      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
	      sqlite_freemem (err);
	    }
	}

      g_free (filename);
    }



  int dir_cb (void *arg, int argc, char **argv, char **names)
  {
    g_queue_push_tail (q, g_strdup (argv[0]));
    return 0;
  }

  err = NULL;
  sqlite_exec_printf (sqliteh, "select (filename) from dirs;",
		      dir_cb, NULL, &err);
  if (err)
    {
      g_debug ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);
    }

  while (! g_queue_is_empty (q))
    {
      char *filename = g_queue_pop_head (q);
      music_db_add_recursive_internal (db, sqliteh, filename, NULL);
      g_free (filename);
    }

  g_queue_free (q);

  sqlite_close (sqliteh);
  g_free (fs);

  return NULL;
}

/* Idle function which is called when meta data should be read.  */
static gboolean
fs_scanner (gpointer data)
{
  struct fs_scanner *fs = data;
  MusicDB *db = MUSIC_DB (fs->db);
  db->fs_scanner = 0;

  g_thread_create (fs_scanner_thread, fs, FALSE, NULL);

  return false;
}


/* Idle function which is called when meta data should be read.  */
static gboolean
meta_data_reader (gpointer data)
{
  MusicDB *db = MUSIC_DB (data);

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

  while (1)
    {
      g_static_mutex_lock (&db->meta_data_reader_mutex);

      if (! g_queue_is_empty (db->meta_data_reader_pending))
	{
	  g_static_mutex_unlock (&db->meta_data_reader_mutex);
	  break;
	}

      int *uidp = g_queue_pop_head (db->meta_data_reader_pending);

      g_static_mutex_unlock (&db->meta_data_reader_mutex);


      g_assert (uidp);
      int uid = *uidp;
      g_free (uidp);

      char *source = NULL;
      music_db_get_info (db, uid, &source, NULL, NULL, NULL, NULL, NULL, NULL);
      if (! source)
	/* Hmm, entry disappeared.  It's possible: the user may have
	   removed it before we got to processing it.  */
	continue;

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
	  g_warning ("%s: Failed to play %d", __FUNCTION__, (int) uid);
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
	      {
		GstTagList *tags;
		gst_message_parse_tag (msg, &tags);

		music_db_set_info_from_tags (db, uid, tags);
		continue;
	      }

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
    }

  db->meta_data_reader = 0;

  if (pipeline)
    {
      gst_object_unref (pipeline);
      gst_object_unref (bus);
    }

  return FALSE;
}

static void
music_db_add (MusicDB *db, sqlite *sqliteh,
	      char *sources[], int count, GError **error)
{
  /* Ignore any entries that do already exist in the DB.  */
  char *s[count];
  int total = 0;

  int i;
  for (i = 0; i < count; i ++)
    {
      int source_exists (void *arg, int argc, char **argv, char **names)
      {
	return 1;
      }

      char *err = NULL;
      int ret = sqlite_exec_printf (sqliteh,
				    "select * from files"
				    "  where source = '%q';",
				    source_exists, NULL, &err,
				    sources[i]);
      if (ret == 0)
	/* Entry does not exist.  */
	s[total ++] = sources[i];

      if (ret != SQLITE_ABORT && err)
	g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);

      if (err)
	sqlite_freemem (err);
    }

  if (total == 0)
    /* Nothing to do.  */
    return;



  char *err = NULL;
  sqlite_exec (sqliteh, "begin transaction;",
	       NULL, NULL, &err);
  if (err)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, "%s", err);
      sqlite_freemem (err);
      return;
    }

  for (i = 0; i < total; i ++)
    {
      sqlite_exec_printf (sqliteh,
			  "insert into files (source) values ('%q');",
			  NULL, NULL, &err, s[i]);
      if (err)
	break;

      int uid = sqlite_last_insert_rowid (sqliteh);

      g_signal_emit (db, MUSIC_DB_GET_CLASS (db)->new_entry_signal_id, 0,
		     uid);

      /* Queue the file in the meta-data crawler (if appropriate).  */
      if (strncmp (FILE_PREFIX, s[i], sizeof (FILE_PREFIX) - 1) == 0)
	{
	  g_static_mutex_lock (&db->meta_data_reader_mutex);

	  if (! db->meta_data_reader_pending)
	    db->meta_data_reader_pending = g_queue_new ();

	  int *uidp = g_malloc (sizeof (int));
	  *uidp = uid;
	  g_queue_push_tail (db->meta_data_reader_pending, uidp);

	  if (! db->meta_data_reader)
	    db->meta_data_reader = g_idle_add (meta_data_reader, db);

	  g_static_mutex_unlock (&db->meta_data_reader_mutex);
	}
    }

  if (! err)
    sqlite_exec (sqliteh,
		 "commit transaction;", NULL, NULL, &err);

  if (err)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, "%s", err);
      sqlite_freemem (err);

      sqlite_exec (sqliteh,
		   "rollback transaction;", NULL, NULL, NULL);

      return;
    }

  if (total == 0)
    return;
}

int
music_db_add_m3u (MusicDB *db, const gchar *path, GError **error) 
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

	  music_db_add_file (db, filename, error);
	  count ++;
	  if (*f != '/')
	    g_free (filename);
	}
    }

  g_strfreev (lines);

  return count;
}

void
music_db_add_uri (MusicDB *db, const char *uri, GError **error)
{
  char *sources[1] = { (char *) uri };
  music_db_add (db, db->sqliteh, sources, 1, error);
}

static void
music_db_add_file_internal (MusicDB *db, sqlite *sqliteh,
			    const gchar *file, GError **error)
{
#define P "file://"
  char source[sizeof (P) + strlen (file)];
  strcpy (source, P);
  strcpy (source + sizeof (P) - 1, file);

  /* XXX: We need to escape this!  Filenames with '%' will silently
     fail.  */
  char *sources[1] = { source };
  music_db_add (db, sqliteh, sources, 1, error);
}

void
music_db_add_file (MusicDB *db, const gchar *file, GError **error)
{
  music_db_add_file_internal (db, db->sqliteh, file, error);
}

static int
has_audio_extension (const char *filename)
{
  const char *whitelist[] = { ".ogg", ".OGG",
			      ".mp3", ".MP3",
			      ".rm", ".RM",
			      ".wav", ".WAV",
			      ".flac", ".FLAC"
  };

  int i;
  for (i = 0; i < sizeof (whitelist) / sizeof (whitelist[0]); i ++)
    if (g_str_has_suffix (filename, whitelist[i]))
      return 1;

  /* Unknown extension.  */
  return 0;
}

static int
music_db_add_recursive_internal (MusicDB *db, sqlite *sqliteh,
				 const gchar *path, GError **error)
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
	  count += music_db_add_recursive_internal (db, sqliteh,
						    filename, &tmp_error);
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
      struct stat st;
      int ret = g_stat (path, &st);
      if (ret < 0)
	/* Failed to read it.  Perhaps we don't have permission.  */
	return 0;

      if (! S_ISREG (st.st_mode))
	/* Not a regular file.  */
	return 0;
    }
    
  if (g_str_has_suffix (path, ".m3u"))
    /* Ignore m3u files: we will normally get the files simply by
       recursing.  */
    return 0;

  if (has_audio_extension (path))
    /* Ignore files that do not appear to be audio files.  */
    music_db_add_file_internal (db, sqliteh, path, error);

  return 1;
}

int
music_db_add_recursive (MusicDB *db, const gchar *path, GError **error)
{
  /* If there exists a directory (FILENAME) which is a prefix of PATH,
     then don't add PATH.  */
  int exists (void *arg, int argc, char **argv, char **names)
  {
    return 1;
  }

  char *err = NULL;
  int ret = sqlite_exec_printf (db->sqliteh,
				"select * from dirs"
				/* PATH is a prefix of FILENAME.  */
				"  where '%q/' GLOB filename || '/*';",
				exists, NULL, &err, path);
  if (ret == 0)
    /* Directory does not exist, add it.  Delete any entries for which
       PATH is a prefix.  */
    {
      char *err = NULL;
      sqlite_exec_printf (db->sqliteh,
			  "delete from dirs where filename || '/' GLOB '%q/*';"
			  "insert into dirs (filename) values ('%q');",
			  NULL, NULL, &err, path, path);
      if (err)
	{
	  g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, path);
	  sqlite_freemem (err);
	}
    }
  else if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);
    }

  return music_db_add_recursive_internal (db, db->sqliteh, path, error);
}

void
music_db_remove (MusicDB *db, gint uid)
{
  g_assert (music_db_get_info (db, uid,
			       NULL, NULL, NULL, NULL, NULL, NULL, NULL));

  char *err = NULL;
  sqlite_exec_printf (db->sqliteh,
		      "delete from files where ROWID = %d;",
		      NULL, NULL, &err, (int) uid);
  if (err)
    {
      g_warning ("%s: %s", __func__, err);
      sqlite_freemem (err);
    }

  simple_cache_shootdown (&info_cache, uid);

  g_signal_emit (db, MUSIC_DB_GET_CLASS (db)->deleted_entry_signal_id, 0, uid);
}

void
music_db_clear (MusicDB *db)
{
  GError *tmp_error = NULL;
  music_db_create_table (db, TRUE, &tmp_error);
  /* XXX */
  if (tmp_error)
    {
      g_error_free (tmp_error);
      return;
    }

  simple_cache_drop (&info_cache);
  g_signal_emit (db, MUSIC_DB_GET_CLASS (db)->cleared_signal_id, 0);
}

bool
music_db_get_info (MusicDB *db, int uid,
		   char **source, char **artist, char **album,
		   int *track, char **title, int *duration, char **genre)
{
  struct info_cache_entry *e = simple_cache_find (&info_cache, uid);
  if (! e)
    {
      e = g_malloc (sizeof (struct info_cache_entry));

      if (source)
	*source = NULL;
      if (artist)
	*artist = NULL;
      if (album)
	*album = NULL;
      if (track)
	*track = 0;
      if (title)
	*title = NULL;
      if (duration)
	*duration = 0;
      if (genre)
	*genre = NULL;

      bool found = false;
      int callback (void *arg, int argc, char **argv, char **names)
      {
	found = true;

	int i = 0;
	e->source = argv[i] ? g_strdup (argv[i]) : NULL;

	i ++;
	e->artist = argv[i] ? g_strdup (argv[i]) : NULL;

	i ++;
	e->album = argv[i] ? g_strdup (argv[i]) : NULL;

	i ++;
	e->track = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->title = argv[i] ? g_strdup (argv[i]) : NULL;

	i ++;
	e->duration = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->genre = argv[i] ? g_strdup (argv[i]) : NULL;

	return 1;
      }

      char *err = NULL;
      sqlite_exec_printf (db->sqliteh,
			  "select source, artist, album, "
			  "  track, title, duration, genre "
			  " from files where ROWID = %d;",
			  callback, NULL, &err, (int) uid);
      if (err)
	{
	  g_warning ("%s: %s", __FUNCTION__, err);
	  sqlite_freemem (err);
	}

      if (! found)
	{
	  g_free (e);
	  return false;
	}

      simple_cache_add (&info_cache, uid, e);
    }

  if (source)
    *source = e->source ? strdup (e->source) : NULL;
  if (artist)
    *artist = e->artist ? strdup (e->artist) : NULL;
  if (album)
    *album = e->album ? strdup (e->album) : NULL;
  if (track)
    *track = e->track;
  if (title)
    *title = e->title ? strdup (e->title) : NULL;
  if (duration)
    *duration = e->duration;
  if (genre)
    *genre = e->genre ? strdup (e->genre) : NULL;

  return true;
}

void
music_db_set_info (MusicDB *db, int uid, struct music_db_info *info)
{
  if (! info->fields)
    return;

  struct obstack sql;

  bool need_comma = false;

  void munge_str (char *key, char *val)
  {
    g_assert (key);
    g_assert (val);

    if (need_comma)
      obstack_1grow (&sql, ',');
    need_comma = true;

    char *s = NULL;
    char *has_quote = NULL;

    if (val && val[0])
      {
	has_quote = strchr (val, '\'');
	if (has_quote)
	  s = sqlite_mprintf ("%q", val);
	else
	  s = val;

	obstack_printf (&sql, "%s = '%s'", key, s);

	if (has_quote)
	  sqlite_freemem (s);
      }
    else
      obstack_printf (&sql, "%s = NULL", key);
  }

  void munge_int (char *key, int val)
  {
    g_assert (key);

    if (need_comma)
      obstack_1grow (&sql, ',');
    need_comma = true;

    obstack_printf (&sql, "%s = %d", key, val);
  }

  obstack_init (&sql);
  obstack_printf (&sql, "update files set ");

  if ((info->fields & MDB_SOURCE))
    munge_str ("source", info->source);
  if ((info->fields & MDB_ARTIST))
    munge_str ("artist", info->artist);
  if ((info->fields & MDB_ALBUM))
    munge_str ("album", info->album);
  if ((info->fields & MDB_TITLE))
    munge_str ("title", info->title);
  if ((info->fields & MDB_GENRE))
    munge_str ("genre", info->genre);

  if ((info->fields & MDB_TRACK))
    munge_int ("track", info->track);
  if ((info->fields & MDB_DURATION))
    munge_int ("duration", info->duration);

  obstack_printf (&sql, " where ROWID = %d;", uid);

  obstack_1grow (&sql, 0);
  char *statement = obstack_finish (&sql);

  char *err = NULL;
  sqlite_exec (db->sqliteh, statement, NULL, NULL, &err);
  if (err)
    {
      g_warning ("%s: %s", __FUNCTION__, err);
      sqlite_freemem (err);
    }

  obstack_free (&sql, NULL);

  if (! err)
    {
      simple_cache_shootdown (&info_cache, uid);

      g_signal_emit (db, MUSIC_DB_GET_CLASS (db)->changed_entry_signal_id, 0,
		     uid);
    }
}

void
music_db_set_info_from_tags (MusicDB *db, int uid, GstTagList *tags)
{
  struct music_db_info info;
  memset (&info, 0, sizeof (info));

  void for_each_tag (const GstTagList *list,
		     const gchar *tag, gpointer data)
    {
      gchar **s = NULL;
      int *i = NULL;

      if (strcmp (tag, "artist") == 0)
	{
	  info.fields |= MDB_ARTIST;
	  s = &info.artist;
	}
      else if (strcmp (tag, "title") == 0)
	{
	  info.fields |= MDB_TITLE;
	  s = &info.title;
	}
      else if (strcmp (tag, "album") == 0)
	{
	  info.fields |= MDB_ALBUM;
	  s = &info.album;
	}
      else if (strcmp (tag, "genre") == 0)
	{
	  info.fields |= MDB_GENRE;
	  s = &info.genre;
	}
      else if (strcmp (tag, "track-number") == 0)
	{
	  info.fields |= MDB_TRACK;
	  i = &info.track;
	}

      if (! s && ! i)
	return;

      char *str;
      if (gst_tag_get_type (tag) == G_TYPE_STRING)
	gst_tag_list_get_string_index (list, tag, 0, &str);
      else
	str = g_strdup_value_contents
	  (gst_tag_list_get_value_index (list, tag, 0));

      if (s)
	*s = str;
      else
	{
	  *i = atoi (str);
	  g_free (str);
	}
    }

  gst_tag_list_foreach (tags, for_each_tag, NULL);

  if (info.fields)
    {
      music_db_set_info (db, uid, &info);

      free (info.artist);
      free (info.title);
      free (info.album);
      free (info.genre);
    }
}

struct info_callback_data
{
  int (*user_callback) (int uid, struct music_db_info *info);
  int ret;
};

static int
info_callback (void *arg, int argc, char **argv, char **names)
{
  struct info_callback_data *data = arg;

  g_assert (argc == 7);

  struct music_db_info info;

  int uid = atoi (argv[0]);

  info.fields = 0;

  info.source = argv[1];
  if (info.source)
    info.fields |= MDB_SOURCE;

  info.artist = argv[2];
  if (info.artist)
    info.fields |= MDB_ARTIST;

  info.album = argv[3];
  if (info.album)
    info.fields |= MDB_ALBUM;

  if (argv[4])
    {
      info.track = atoi (argv[4]);
      info.fields |= MDB_TRACK;
    }

  info.title = argv[5];
  if (info.title)
    info.fields |= MDB_TITLE;

  if (argv[6])
    {
      info.duration = atoi (argv[6]);
      info.fields |= MDB_DURATION;
    }

  data->ret = data->user_callback (uid, &info);
  return data->ret;
}

int
music_db_for_each (MusicDB *db, const char *list,
		   int (*user_callback) (int uid, struct music_db_info *info),
		   enum mdb_fields *order,
		   const char *constraint)
{
  if (constraint && ! *constraint)
    constraint = NULL;

  struct obstack sql;

  obstack_init (&sql);

  if (list)
    {
      char *l = sqlite_mprintf ("%q", list);
      obstack_printf (&sql,
		      "select files.ROWID, files.source, files.artist,"
		      "  files.album, files.track, files.title, files.duration"
		      " from playlists left join files"
		      "  on playlists.uid = files.ROWID"
		      " where playlists.list = '%s' ",
		      l);
      sqlite_freemem (l);
    }
  else
    obstack_printf (&sql,
		    "select ROWID, source, artist, album, track,"
		    " title, duration from files ");

  if (constraint && *constraint)
    obstack_printf (&sql, "%s(%s) ", list ? "and " : "where", constraint);

  if (order)
    {
      obstack_printf (&sql, "order by ");

      bool need_comma = false;
      for (; *order; order ++)
	{
	  if (need_comma)
	    obstack_1grow (&sql, ',');
	  else
	    need_comma = true;

	  char *s;
	  switch (*order)
	    {
	    case MDB_SOURCE:
	      s = "source";
	      break;
	    case MDB_ARTIST:
	      s = "artist";
	      break;
	    case MDB_ALBUM:
	      s = "album";
	      break;
	    case MDB_TRACK:
	      s = "track";
	      break;
	    case MDB_TITLE:
	      s = "title";
	      break;
	    case MDB_DURATION:
	      s = "duration";
	      break;

	    default:
	      g_assert (! "Invalid value for order!");
	      continue;
	    }

	  /* We'd like to use %s collate nocase but that does seem to
	     work, at least for sqlite 2...  */
	  obstack_printf (&sql,
			  "%s isnull, lower(%s)",
			  s, s);
	}
    }
  else
    {
      if (list)
	obstack_printf (&sql, "order by playlists.ROWID");
    }

  obstack_1grow (&sql, 0);
  char *statement = obstack_finish (&sql);

  struct info_callback_data data;
  data.ret = 0;
  data.user_callback = user_callback;

  char *err = NULL;
  sqlite_exec (db->sqliteh, statement, info_callback, &data, &err);
  if (err)
    {
      g_warning ("%s:%d: %s (statement: `%s')",
		 __FUNCTION__, __LINE__, err, statement);
      free (err);
    }

  obstack_free (&sql, NULL);

  return data.ret;
}

void
music_db_play_list_enqueue (MusicDB *db, const char *list, int uid)
{
  int count = 0;
  int callback (void *arg, int argc, char **argv, char **names)
  {
    count = atoi (argv[0]);
    return 0;
  }

  char *err = NULL;
  sqlite_exec_printf (db->sqliteh,
		      "insert into playlists (list, uid) values ('%q', %d);"
		      "select count (*) from playlists where list = '%q'",
		      callback, NULL, &err, list, uid, list);
  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);
    }

  g_signal_emit (db, MUSIC_DB_GET_CLASS (db)->added_to_play_list_signal_id, 0,
		 list, count - 1);
}

int
music_db_play_list_dequeue (MusicDB *db, const char *list)
{
  int rowid = 0;
  int uid = 0;
  int callback (void *arg, int argc, char **argv, char **names)
  {
    rowid = atoi (argv[0]);
    uid = atoi (argv[1]);
    return 0;
  }

  char *err = NULL;
  sqlite_exec_printf (db->sqliteh,
		      "select ROWID, uid from playlists where list = '%q'"
		      " limit 1;",
		      callback, NULL, &err, list);

  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);

      return 0;
    }

  sqlite_exec_printf (db->sqliteh,
		      "delete from playlists where ROWID = %d;",
		      NULL, NULL, &err, rowid);
  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);
    }

  g_signal_emit (db, MUSIC_DB_GET_CLASS (db)->removed_from_play_list_signal_id,
		 0, list, 0);

  return uid;
}

int
music_db_play_list_query (MusicDB *db, const char *list, int offset)
{
  int uid = 0;
  int callback (void *arg, int argc, char **argv, char **names)
  {
    uid = atoi (argv[0]);
    return 0;
  }

  char *err = NULL;
  sqlite_exec_printf (db->sqliteh,
		      "select uid from playlists where list = '%q'"
		      " limit 1 offset %d;",
		      callback, NULL, &err, offset, list);
  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);

      return 0;
    }

  return uid;
}

void
music_db_play_list_remove (MusicDB *db, const char *list, int offset)
{
  char *err = NULL;
  sqlite_exec_printf (db->sqliteh,
		      "delete from playlists where ROWID in "
		      " (select ROWID from playlists where list = '%q'"
		      "   limit 1 offset %d);",
		      NULL, NULL, &err, list, offset);
  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);

      return;
    }

  g_signal_emit (db, MUSIC_DB_GET_CLASS (db)->removed_from_play_list_signal_id,
		 0, list, offset);
}

void
music_db_play_list_clear (MusicDB *db, const char *list)
{
  int count = 0;
  int callback (void *arg, int argc, char **argv, char **names)
  {
    count = atoi (argv[0]);
    return 0;
  }

  char *err = NULL;
  sqlite_exec_printf (db->sqliteh,
		      "select count (*) from playlists where list = '%q';"
		      "delete from playlists where list = '%q'",
		      callback, NULL, &err, list, list);
  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      free (err);
    }

  for (; count > 0; count --)
    g_signal_emit (db,
		   MUSIC_DB_GET_CLASS (db)->removed_from_play_list_signal_id,
		   0, list, 0);
}


int
music_db_play_lists_for_each (MusicDB *db,
			      int (*user_callback) (const char *list))
{
  int ret = 0;
  int callback (void *arg, int argc, char **argv, char **names)
  {
    ret = user_callback (argv[0]);
    return ret;
  }

  char *err = NULL;
  sqlite_exec (db->sqliteh,
	       "select distinct list from playlists order by lower(list);",
	       callback, NULL, &err);
  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      free (err);
    }

  return ret;
}
