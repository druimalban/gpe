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
  bool present;

  char *source;
  char *artist;
  char *album;
  char *title;
  int track;
  int duration;
  char *genre;
  char *date;
  int volume_number;
  int volume_count;
  char *performer;

  int play_count;
  int date_added;
  int date_last_played;
  int date_tags_updated;

  int rating;

  time_t mtime;
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
  free (e->date);
  free (e->performer);
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

  /* A worker thread, which scans for files and for reading
     meta-data.  */
  GThread *worker;
  /* We don't start the thread immediately but wait until the first
     idle period.  */
  int worker_idle;
  bool exit;
  GMutex *work_lock;
  GCond *work_cond;

  /* UIDs of files (int *) whose metadata the worker should try to
     read.  */
  GQueue *meta_data_pending;
  /* List of directories (char *) that the worker should scan.  */
  GQueue *dirs_pending;

  /* If non-zero, the source id of the status handler.  */
  guint status_source;
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

  music_db_class->status_signal_id
    = g_signal_new ("status",
		    G_TYPE_FROM_CLASS (klass),
		    G_SIGNAL_RUN_FIRST,
		    0, NULL, NULL,
		    g_cclosure_marshal_VOID__STRING,
		    G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
music_db_init (MusicDB *db)
{
  if (! g_thread_supported ())
    g_thread_init (NULL);

  db->work_lock = g_mutex_new ();
  db->work_cond = g_cond_new ();
  db->meta_data_pending = g_queue_new ();
  db->dirs_pending = g_queue_new ();
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

  if (db->worker)
    {
      db->exit = true;
      g_mutex_lock (db->work_lock);
      g_cond_signal (db->work_cond);

      g_thread_join (db->worker);
    }

  g_cond_free (db->work_cond);
  g_mutex_free (db->work_lock);

  if (db->worker_idle)
    g_source_remove (db->worker_idle);

  char *d;
  do
    {
      d = g_queue_pop_head (db->meta_data_pending);
      g_free (d);
    }
  while (d);
  g_queue_free (db->meta_data_pending);

  do
    {
      d = g_queue_pop_head (db->dirs_pending);
      g_free (d);
    }
  while (d);
  g_queue_free (db->dirs_pending);


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
	       "  date STRING, "
	       "  volume_number INTEGER, "
	       "  volume_count INTEGER, "
	       "  performer STRING, "

	       "  rating INTEGER,"
	       "  play_count INTEGER,"

	       "  date_added INTEGER,"
	       "  date_last_played INTEGER,"
	       "  date_tags_updated INTEGER,"

	       /* The date the file was found to no longer be
		  present.  */
	       "  removed INTEGER,"

	       /* To determine if the information is up to date.  */
	       "  mtime INTEGER);"

	       /* Create the table for the directories.  */
	       "create table dirs (filename STRING); "

	       /* Create a table for the play lists.  */
	       "create table playlists (list STRING, uid INTEGER);"

	       "commit transaction;",
	       NULL, NULL, NULL);
}

struct worker
{
  MusicDB *db;
  sqlite *sqliteh;
};

/* Forward.  */
static gboolean worker_start (gpointer data);

static int
busy_handler (void *cookie, const char *table, int retries)
{
  if (cookie)
    {
      int *was_busy = cookie;
      *was_busy = 1;
    }

  /* If this is the main thread, then we'd like to recursively invoke
     the main loop, however, not everything is reentrant so...  */

  if (retries > 4)
    retries = 4;

  /* In milliseconds.  */
  int timeout = 50 << (retries - 1);

  /* Sleep and then try again.  */
  struct timespec ts;
  ts.tv_nsec = (timeout % 1000) * (1000000ULL);
  ts.tv_sec = timeout / 1000;

  nanosleep (&ts, &ts);

  return 1;
}

/* The number of times the busy handler has been called in the main
   context recently.  */
static int main_thread_busy;

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

  sqlite_busy_handler (db->sqliteh, busy_handler, &main_thread_busy);

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

      struct worker *worker = calloc (sizeof (*worker), 1);
      worker->db = db;
      worker->sqliteh = sqliteh;

      db->worker_idle = g_idle_add (worker_start, worker);
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
			"select count (*) from files"
			" where removed isnull %s%s%s",
			callback, NULL, &err,
			constraint ? "and (" : "",
			constraint ? constraint : "",
			constraint ? ")" : "");
  if (err)
    {
      g_critical ("%s", err);
      sqlite_freemem (err);
      return 0;
    }

  return count;
}

static void
music_db_add (MusicDB *db, sqlite *sqliteh,
	      const char *sources[], int count, GError **error)
{
  /* Ignore any entries that do already exist in the DB.  */
  const char *s[count];
  int uids[count];
  bool crawl[count];
  int total = 0;


  char *err = NULL;
  sqlite_exec (sqliteh, "begin transaction;",
	       NULL, NULL, &err);
  if (err)
    {
      g_set_error (error, ERROR_DOMAIN (), 0, "%s", err);
      sqlite_freemem (err);
      return;
    }

  int i;
  for (i = 0; i < count; i ++)
    {
      /* First, get the status of the entry.  */
      enum { present, removed, noentry };
      int status = noentry;
      int uid = 0;

      int source_status (void *arg, int argc, char **argv, char **names)
      {
	if (argv[0])
	  status = removed;
	else
	  status = present;

	if (argv[1])
	  uid = atoi (argv[1]);

	return 0;
      }

      char *err = NULL;
      sqlite_exec_printf (sqliteh,
			  "select removed, ROWID from files"
			  "  where source = '%q';",
			  source_status, NULL, &err,
			  sources[i]);
      if (err)
	{
	  g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
	  sqlite_freemem (err);
	  break;
	}

      switch (status)
	{
	case present:
	  /* It's already there.  Nothing to do.  */
	  g_assert (uid);
	  break;

	case removed:
	  /* It's there, we just need to restore it.  */
	  g_assert (uid);

	  sqlite_exec_printf (sqliteh,
			      "update files set removed = null"
			      "  where ROWID = %d",
			      NULL, NULL, &err, uid);
	  if (err)
	    {
	      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
	      sqlite_freemem (err);
	    }

	  break;

	case noentry:
	  g_assert (! uid);

	  sqlite_exec_printf (sqliteh,
			      "insert into files (source, date_added) "
			      "  values ('%q', strftime('%%s', 'now'));",
			      NULL, NULL, &err, sources[i]);
	  if (err)
	    {
	      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
	      sqlite_freemem (err);
	    }
	  else
	    uid = sqlite_last_insert_rowid (sqliteh);
	  break;

	default:
	  g_assert (! "Invalid status value!");
	}

      if (err)
	break;

      if (status == removed || status == noentry)
	{
	  uids[total] = uid;
	  s[total] = sources[i];
	  crawl[total] = status == noentry;

	  total ++;
	}
    }

  if (err)
    {
      sqlite_exec (sqliteh,
		   "rollback transaction;", NULL, NULL, NULL);
      return;
    }

  sqlite_exec (sqliteh,
	       "commit transaction;", NULL, NULL, &err);
  if (err)
    {
      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);
      return;
    }

  if (total == 0)
    /* Nothing to do.  */
    return;

  bool have_one = false;
  for (i = 0; i < total; i ++)
    {
      g_signal_emit (db, MUSIC_DB_GET_CLASS (db)->new_entry_signal_id, 0,
		     uids[i]);

      /* Queue the file in the meta-data crawler (if appropriate).  */
      if (crawl[i] && s[i][0] == '/')
	{
	  have_one = true;

	  int *uidp = g_malloc (sizeof (int));
	  *uidp = uids[i];

	  g_mutex_lock (db->work_lock);
	  g_queue_push_tail (db->meta_data_pending, uidp);
	  g_mutex_unlock (db->work_lock);
	}
    }

  if (have_one)
    g_cond_signal (db->work_cond);
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
  music_db_add (db, db->sqliteh, &uri, 1, error);
}

static void
music_db_add_file_internal (MusicDB *db, sqlite *sqliteh,
			    const gchar *file, GError **error)
{
  music_db_add (db, sqliteh, &file, 1, error);
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
  if (db->sqliteh != sqliteh && main_thread_busy > 0)
    {
      /* The main thread was busy...  Back off a bit.  */
      int timeout = 200;
      struct timespec ts;
      ts.tv_nsec = (timeout % 1000) * (1000000ULL);
      ts.tv_sec = timeout / 1000;

      nanosleep (&ts, &ts);

      main_thread_busy --;
    }


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

  g_mutex_lock (db->work_lock);
  g_queue_push_tail (db->dirs_pending, g_strdup (path));
  g_mutex_unlock (db->work_lock);
  g_cond_signal (db->work_cond);
  return TRUE;
}

void
music_db_remove (MusicDB *db, gint uid)
{
  char *err = NULL;
  sqlite_exec_printf (db->sqliteh,
		      "delete from files where ROWID = %d;"
		      "delete from playlists where uid = %d;",
		      NULL, NULL, &err, uid, uid);
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

static bool
music_db_get_info_internal (MusicDB *db, sqlite *sqliteh,
			    int uid, struct music_db_info *info)
{
  struct info_cache_entry *e = simple_cache_find (&info_cache, uid);
  if (! e)
    {
      e = g_malloc (sizeof (struct info_cache_entry));

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

	i ++;
	e->play_count = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->date_added = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->date_last_played = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->date_tags_updated = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->rating = argv[i] ? atoi (argv[i]) : 0;

	/* NULL => present, not-NULL => date of removal.  */
	i ++;
	e->present = argv[i] ? false : true;

	i ++;
	e->mtime = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->date = argv[i] ? g_strdup (argv[i]) : NULL;

	i ++;
	e->volume_number = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->volume_count = argv[i] ? atoi (argv[i]) : 0;

	i ++;
	e->performer = argv[i] ? g_strdup (argv[i]) : NULL;

	return 1;
      }

      char *err = NULL;
      sqlite_exec_printf (sqliteh,
			  "select source, artist, album, "
			  "  track, title, duration, genre, "
			  "  play_count, date_added, "
			  "  date_last_played, date_tags_updated, "
			  "  rating, removed, mtime, date,"
			  "  volume_number, volume_count, performer"
			  " from files where ROWID = %d;",
			  callback, NULL, &err, (int) uid);
      if (err)
	{
	  g_warning ("%s:%d %s", __FUNCTION__, __LINE__, err);
	  sqlite_freemem (err);
	}

      if (! found)
	{
	  g_free (e);
	  return false;
	}

      simple_cache_add (&info_cache, uid, e);
    }

  if ((info->fields & MDB_SOURCE))
    info->source = e->source ? g_strdup (e->source) : NULL;
  if ((info->fields & MDB_ARTIST))
    info->artist = e->artist ? g_strdup (e->artist) : NULL;
  if ((info->fields & MDB_ALBUM))
    info->album = e->album ? g_strdup (e->album) : NULL;
  if ((info->fields & MDB_TRACK))
    info->track = e->track;
  if ((info->fields & MDB_TITLE))
    info->title = e->title ? g_strdup (e->title) : NULL;
  if ((info->fields & MDB_DURATION))
    info->duration = e->duration;
  if ((info->fields & MDB_GENRE))
    info->genre = e->genre ? g_strdup (e->genre) : NULL;
  if ((info->fields & MDB_PLAY_COUNT))
    info->play_count = e->play_count;
  if ((info->fields & MDB_DATE_ADDED))
    info->date_added = e->date_added;
  if ((info->fields & MDB_DATE_LAST_PLAYED))
    info->date_last_played = e->date_last_played;
  if ((info->fields & MDB_DATE_TAGS_UPDATED))
    info->date_tags_updated = e->date_tags_updated;
  if ((info->fields & MDB_RATING))
    info->rating = e->rating;
  if ((info->fields & MDB_PRESENT))
    info->present = e->present;
  if ((info->fields & MDB_MTIME))
    info->mtime = e->mtime;
  if ((info->fields & MDB_DATE))
    info->date = e->date ? g_strdup (e->date) : NULL;
  if ((info->fields & MDB_VOLUME_NUMBER))
    info->volume_number = e->volume_number;
  if ((info->fields & MDB_VOLUME_COUNT))
    info->volume_count = e->volume_count;
  if ((info->fields & MDB_PERFORMER))
    info->performer = e->performer ? g_strdup (e->performer) : NULL;

  return true;
}

bool
music_db_get_info (MusicDB *db, int uid, struct music_db_info *info)
{
  return music_db_get_info_internal (db, db->sqliteh, uid, info);
}

static void
music_db_set_info_internal (MusicDB *db, sqlite *sqliteh,
			    int uid, struct music_db_info *info)
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

  void munge_lit (char *key, const char *val)
  {
    g_assert (key);

    if (need_comma)
      obstack_1grow (&sql, ',');
    need_comma = true;

    obstack_printf (&sql, "%s = %s", key, val);
  }

  obstack_init (&sql);
  obstack_printf (&sql, "update files set ");

  if ((info->fields & MDB_SOURCE))
    munge_str ("source", info->source);
  if ((info->fields & MDB_ARTIST))
    munge_str ("artist", info->artist);
  if ((info->fields & MDB_PERFORMER))
    munge_str ("performer", info->performer);
  if ((info->fields & MDB_ALBUM))
    munge_str ("album", info->album);
  if ((info->fields & MDB_TITLE))
    munge_str ("title", info->title);
  if ((info->fields & MDB_GENRE))
    munge_str ("genre", info->genre);

  if ((info->fields & MDB_TRACK))
    munge_int ("track", info->track);
  if ((info->fields & MDB_VOLUME_NUMBER))
    munge_int ("volume_number", info->volume_number);
  if ((info->fields & MDB_VOLUME_COUNT))
    munge_int ("volume_count", info->volume_count);
  if ((info->fields & MDB_DURATION))
    munge_int ("duration", info->duration);

  if ((info->fields & MDB_DATE))
    munge_str ("date", info->date);

  if ((info->fields & MDB_DATE_ADDED))
    munge_int ("date_added", info->date_added);

  if ((info->fields & MDB_INC_PLAY_COUNT))
    munge_lit ("play_count", "coalesce (play_count, 0) + 1");
  else if ((info->fields & MDB_PLAY_COUNT))
    munge_int ("play_count", info->play_count);

  if ((info->fields & MDB_UPDATE_DATE_LAST_PLAYED))
    munge_lit ("date_last_played", "strftime('%s', 'now')");
  else if ((info->fields & MDB_DATE_LAST_PLAYED))
    munge_int ("date_last_played", info->date_last_played);

  if ((info->fields & MDB_UPDATE_DATE_TAGS_UPDATED))
    munge_lit ("date_tags_updated", "strftime('%s', 'now')");
  else if ((info->fields & MDB_DATE_TAGS_UPDATED))
    munge_int ("date_tags_updated", info->date_tags_updated);

  if ((info->fields & MDB_RATING))
    munge_int ("rating", info->rating);

  if ((info->fields & MDB_MTIME))
    munge_int ("mtime", info->mtime);

  obstack_printf (&sql, " where ROWID = %d;", uid);

  obstack_1grow (&sql, 0);
  char *statement = obstack_finish (&sql);

  char *err = NULL;
  sqlite_exec (sqliteh, statement, NULL, NULL, &err);
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
music_db_set_info (MusicDB *db, int uid, struct music_db_info *info)
{
  music_db_set_info_internal (db, db->sqliteh, uid, info);
}

static void
music_db_set_info_from_tags_internal (MusicDB *db, sqlite *sqliteh,
				      int uid, GstTagList *tags)
{
  struct music_db_info info;
  memset (&info, 0, sizeof (info));

  void for_each_tag (const GstTagList *list,
		     const gchar *tag, gpointer data)
    {
      gchar **s = NULL;
      int *i = NULL;
      int factor = 1;

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
      else if (strcmp (tag, "duration") == 0)
	{
	  info.fields |= MDB_DURATION;
	  i = &info.duration;
	  factor = 1e9;
	}
      else if (strcmp (tag, GST_TAG_DATE) == 0)
	{
	  info.fields |= MDB_DATE;
	  s = &info.date;
	}
      else if (strcmp (tag, GST_TAG_ALBUM_VOLUME_NUMBER) == 0)
	{
	  info.fields |= MDB_VOLUME_NUMBER;
	  i = &info.volume_number;
	}
      else if (strcmp (tag, GST_TAG_ALBUM_VOLUME_COUNT) == 0)
	{
	  info.fields |= MDB_VOLUME_COUNT;
	  i = &info.volume_count;
	}
      else if (strcmp (tag, GST_TAG_PERFORMER) == 0)
	{
	  info.fields |= MDB_PERFORMER;
	  s = &info.performer;
	}

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
	  if (i)
	    *i = atoll (str) / factor;
	  g_free (str);
	}
    }

  gst_tag_list_foreach (tags, for_each_tag, NULL);

  info.fields |= MDB_UPDATE_DATE_TAGS_UPDATED;

  music_db_set_info_internal (db, sqliteh, uid, &info);

  free (info.artist);
  free (info.title);
  free (info.album);
  free (info.genre);
  free (info.date);
  free (info.performer);
}

void
music_db_set_info_from_tags (MusicDB *db, int uid, GstTagList *tags)
{
  music_db_set_info_from_tags_internal (db, db->sqliteh, uid, tags);
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
		    " title, duration from files where removed isnull ");

  if (constraint && *constraint)
    obstack_printf (&sql, "and (%s) ", constraint);

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

	  int is_string = true;
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
	      is_string = false;
	      break;
	    case MDB_TITLE:
	      s = "title";
	      break;
	    case MDB_DURATION:
	      s = "duration";
	      is_string = false;
	      break;

	    default:
	      g_assert (! "Invalid value for order!");
	      continue;
	    }

	  /* We'd like to use %s collate nocase but that does seem to
	     work, at least for sqlite 2...  */
	  if (is_string)
	    obstack_printf (&sql,
			    "%s isnull, lower(%s)",
			    s, s);
	  else
	    obstack_printf (&sql,
			    "%s isnull, %s",
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
		      callback, NULL, &err, list, offset);
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

static int
status_update (MusicDB *db)
{
  int dirs_pending = g_queue_get_length (db->dirs_pending);
  int files_pending = g_queue_get_length (db->meta_data_pending);
  char *message = NULL;
  if (dirs_pending && files_pending)
    message = g_strdup_printf (_("Scanning %d %s and %d %s."),
			       dirs_pending,
			       dirs_pending > 1 ? "directories" : "directory",
			       files_pending,
			       files_pending > 1 ? "files" : "file");
  else if (dirs_pending)
    message = g_strdup_printf (_("Scanning %d %s."),
			       dirs_pending,
			       dirs_pending > 1 ? "directories" : "directory");
  else if (files_pending)
    message = g_strdup_printf (_("Scanning %d %s."),
			       files_pending,
			       files_pending > 1 ? "files" : "file");

  g_signal_emit (db,
		 MUSIC_DB_GET_CLASS (db)->status_signal_id,
		 0, message);
  g_free (message);

  return message ? TRUE : FALSE;
}

static void
status_kick (MusicDB *db)
{
  if (db->status_source)
    return;

  status_update (db);
  db->status_source = g_timeout_add (5 * 1000,
				     (GSourceFunc) status_update, db);
}

static void
new_decoded_pad (GstElement *decodebin, GstPad *pad,
		 gboolean last, gpointer data)
{
  GstCaps *caps;
  GstStructure *str;
  GstPad *audiopad;

  /* only link once */
  audiopad = gst_element_get_pad (GST_ELEMENT (data), "sink");
  if (GST_PAD_IS_LINKED (audiopad))
    {
      g_object_unref (audiopad);
      return;
    }

  /* check media type */
  caps = gst_pad_get_caps (pad);
  str = gst_caps_get_structure (caps, 0);
  if (!g_strrstr (gst_structure_get_name (str), "audio"))
    {
      gst_caps_unref (caps);
      gst_object_unref (audiopad);
      return;
    }
  gst_caps_unref (caps);

  /* link'n'play */
  gst_pad_link (pad, audiopad);
  gst_object_unref (audiopad);
}

static void
meta_data_reader (MusicDB *db, sqlite *sqliteh)
{
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
      if (main_thread_busy > 0)
	{
	  /* The main thread was busy...  Back off a bit.  */
	  int timeout = 200;
	  struct timespec ts;
	  ts.tv_nsec = (timeout % 1000) * (1000000ULL);
	  ts.tv_sec = timeout / 1000;

	  nanosleep (&ts, &ts);

	  main_thread_busy --;
	}


      g_mutex_lock (db->work_lock);

      if (g_queue_is_empty (db->meta_data_pending))
	{
	  g_mutex_unlock (db->work_lock);
	  break;
	}

      status_kick (db);

      int *uidp = g_queue_pop_head (db->meta_data_pending);

      g_mutex_unlock (db->work_lock);

      g_assert (uidp);
      int uid = *uidp;
      g_free (uidp);

      struct music_db_info info;
      info.fields = MDB_SOURCE | MDB_DATE_TAGS_UPDATED;
     
      if (! music_db_get_info_internal (db, sqliteh, uid, &info))
	/* Hmm, entry disappeared.  It's possible: the user may have
	   removed it before we got to processing it.  */
	continue;

      char *source = info.source;

      /* We read the data from a file, send it to decodebin which
	 selects the appropriate decoder and then send the result
	 to /dev/null.  */
      GstElement *src = gst_element_factory_make ("filesrc", "filesrc");
      GstElement *decodebin = gst_element_factory_make ("decodebin",
							"decodebin");
      GstElement *sink = gst_element_factory_make ("fakesink", "sink");

      g_signal_connect (decodebin, "new-decoded-pad",
			G_CALLBACK (new_decoded_pad), sink);

      GstElement *pipeline = gst_pipeline_new ("meta_data_pipeline");
      gst_bin_add_many (GST_BIN (pipeline), src, decodebin, sink, NULL);
      gst_element_link_many (src, decodebin, NULL);

      GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));


      g_object_set (G_OBJECT (src), "location", source, NULL);

      GstStateChangeReturn res = gst_element_set_state (pipeline,
							target_state);
      if (res == GST_STATE_CHANGE_FAILURE)
	{
	  g_warning ("%s: Failed to play %d", __FUNCTION__, (int) uid);

	  if (! info.date_tags_updated)
	    {
	      info.fields = MDB_UPDATE_DATE_TAGS_UPDATED;
	      music_db_set_info_internal (db, sqliteh, uid, &info);
	    }

	  continue;
	}

      GstMessage *msg = NULL;
      for (;;)
	{
	  if (msg)
	    gst_message_unref (msg);

	  msg = gst_bus_timed_pop_filtered
	    (bus,
	     /* Wait about two seconds.  */
	     2 * GST_SECOND,
	     GST_MESSAGE_EOS | GST_MESSAGE_ERROR
	     | GST_MESSAGE_TAG | GST_MESSAGE_STATE_CHANGED);

	  if (! msg)
	    break;

	  switch (GST_MESSAGE_TYPE (msg))
	    {
	    case GST_MESSAGE_TAG:
	      {
		GstTagList *tags;
		gst_message_parse_tag (msg, &tags);

		music_db_set_info_from_tags_internal (db, sqliteh, uid, tags);

		gst_tag_list_free (tags);

		continue;
	      }

	    case GST_MESSAGE_STATE_CHANGED:
	      /* Once we've reached the paused state, for most
		 formats, most of the time, all tags will have been
		 reported.  (The exception is essentially streams--but
		 we are only looking at files anyways so this should
		 be relatively rare.)  */
	      if (GST_MESSAGE_SRC (msg) == (void *) pipeline)
		{
		  GstState state, pending;
		  gst_message_parse_state_changed (msg, NULL,
						   &state, &pending);
		  if (pending == GST_STATE_VOID_PENDING
		      && state == target_state)
		    break;
		}
	      continue;

	    case GST_MESSAGE_ERROR:
	      {
		GError *err = NULL;
		gchar *debug;
		gst_message_parse_error (msg, &err, &debug);

		g_debug ("%s: %s (%s)\n", __func__, err->message, debug);

		g_error_free (err);
		g_free (debug);

		break;
	      }

	    case GST_MESSAGE_EOS:
	      break;

	    default:
	      g_debug ("%s: unknown message %s, %d, ignoring\n",
		       __FUNCTION__, source, GST_MESSAGE_TYPE (msg));
	      break;
	    }

	  gst_message_unref (msg);

	  info.fields = MDB_UPDATE_DATE_TAGS_UPDATED;

	  struct stat st;
	  int ret = g_stat (source, &st);
	  if (ret == 0)
	    {
	      info.fields |= MDB_MTIME;
	      info.mtime = st.st_mtime;
	    }

	  music_db_set_info_internal (db, sqliteh, uid, &info);

	  break;
	}

      g_free (source);

      gst_element_set_state (pipeline, GST_STATE_NULL);
      gst_object_unref (pipeline);
      gst_object_unref (bus);
    }
}

static gpointer
worker_thread (gpointer data)
{
  struct worker *w = data;
  MusicDB *db = MUSIC_DB (w->db);
  sqlite *sqliteh = w->sqliteh;

  GQueue *q = g_queue_new ();

  /* Find dead files.  */
  struct track
  {
    char *filename;
    int uid;
    time_t mtime;
  };

  int check_dead_cb (void *arg, int argc, char **argv, char **names)
  {
    struct track *track = g_malloc (sizeof (struct track));
    track->uid = atoi (argv[0]);
    track->filename = g_strdup (argv[1]);
    track->mtime = argv[2] ? atoi (argv[2]) : 0;

    g_queue_push_tail (q, track);

    return 0;
  }

  char *err = NULL;
  sqlite_exec (sqliteh,
	       "select ROWID, source, mtime from files"
	       " where removed isnull and source like '/%';",
	       check_dead_cb, NULL, &err);
  if (err)
    {
      g_debug ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);
    }


  while (! g_queue_is_empty (q))
    {
      struct track *track = g_queue_pop_head (q);
      
      struct stat st;
      int ret = g_stat (track->filename, &st);
      if (ret < 0)
	/* File cannot be stated.  Mark it as missing.  */
	{
	  sqlite_exec_printf (sqliteh,
			      "update files"
			      " set removed = strftime('%%s', 'now')"
			      " where ROWID = %d;",
			      NULL, NULL, &err, track->uid);
	  if (err)
	    {
	      g_warning ("%s:%d: %s", __FUNCTION__, __LINE__, err);
	      sqlite_freemem (err);
	    }
	  else
	    {
	      simple_cache_shootdown (&info_cache, track->uid);
	      g_signal_emit (db,
			     MUSIC_DB_GET_CLASS (db)->deleted_entry_signal_id,
			     0, track->uid);
	    }

	}
      else if (st.st_mtime != track->mtime)
	/* mtime changed.  Rescan tags.  */
	{
	  int *uidp = malloc (sizeof (int));
	  *uidp = track->uid;

	  g_mutex_lock (db->work_lock);
	  g_queue_push_tail (db->meta_data_pending, uidp);
	  g_mutex_unlock (db->work_lock);
	}

      g_free (track->filename);
      g_free (track);
    }


  /* Find new files in directories that we watch.  */
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

  g_mutex_lock (db->work_lock);


  /* Find entries for which tags have not been read.  */
  int no_tags_cb (void *arg, int argc, char **argv, char **names)
  {
    int *uidp = malloc (sizeof (int));
    *uidp = atoi (argv[0]);
    g_queue_push_tail (db->meta_data_pending, uidp);
    return 0;
  }
  err = NULL;
  sqlite_exec_printf (sqliteh,
		      "select (ROWID) from files "
		      "  where date_tags_updated isnull"
		      "    and substr (source, 1, 1) = '/';",
		      no_tags_cb, NULL, &err);
  if (err)
    {
      g_debug ("%s:%d: %s", __FUNCTION__, __LINE__, err);
      sqlite_freemem (err);
    }


  /* Wait for work.  */
  while (! db->exit)
    {
      while (! g_queue_is_empty (db->dirs_pending))
	{
	  status_kick (db);

	  char *filename = g_queue_pop_head (db->dirs_pending);
	  g_mutex_unlock (db->work_lock);

	  music_db_add_recursive_internal (db, sqliteh, filename, NULL);
	  g_free (filename);

	  g_mutex_lock (db->work_lock);
	}

      g_mutex_unlock (db->work_lock);

      meta_data_reader (db, sqliteh);

      g_mutex_lock (db->work_lock);
      g_cond_wait (db->work_cond, db->work_lock);
    }

  g_mutex_unlock (db->work_lock);

  sqlite_close (sqliteh);
  g_free (w);

  return NULL;
}

/* Idle function which is called when meta data should be read.  */
static gboolean
worker_start (gpointer data)
{
  struct worker *w = data;
  MusicDB *db = MUSIC_DB (w->db);
  db->worker_idle = 0;

  db->worker = g_thread_create (worker_thread, w, TRUE, NULL);

  return false;
}
