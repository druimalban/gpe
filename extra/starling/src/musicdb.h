/* musicdb.h - Music DB interface.
   Copyright 2007 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro
        <skyhusker@handhelds.org>

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

#ifndef MUSICDB_H
#define MUSICDB_H

#include <glib-object.h>

#include <gst/gstelement.h>
#include <gst/gstformat.h>

#include <stdbool.h>

typedef struct _MusicDB MusicDB;
typedef struct _MusicDBClass MusicDBClass;

#define MUSIC_DB_TYPE              (music_db_get_type ())
#define MUSIC_DB(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MUSIC_DB_TYPE, MusicDB))
#define MUSIC_DB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUSIC_DB_TYPE, MusicDBClass))
#define IS_MUSIC_DB(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MUSIC_DB_TYPE))
#define IS_MUSIC_DB_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUSIC_DB_TYPE))
#define MUSIC_DB_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUSIC_DB_TYPE, MusicDBClass))

struct _MusicDBClass
{
  GObjectClass parent;

  /* "new-entry" signal: A new entry was added to the database.
     Passed the UID (guint) of the new entry.  */
  guint new_entry_signal_id;
  /* "changed-entry" signal: An entry's meta-data was updated.  Passed
     the UID (guint) of the changed entry.  */
  guint changed_entry_signal_id;
  /* "deleted-entry" signal: An entry was removed.  Passed the UID
     (guint) of the removed entry.  */
  guint deleted_entry_signal_id;
  /* "cleared" signal: All entries in the DB were removed.  */
  guint cleared_signal_id;

  /* "added-to-queue" signal: A new entry was added to the play queue.
     Passed the position (guint) and UID (guint) of the new entry.  */
  guint added_to_queue_signal_id;
  /* "removed-from-queue" signal: An entry was removed from the play
     queue.  Passed the position (guint).  */
  guint removed_from_queue_signal_id;
};

extern GType music_db_get_type (void);

/** music_db_open:

  Create a music db associated with file FILE.  Any error is returned
  in ERROR.  */
extern MusicDB *music_db_open (const char *file, GError **error);

/** music_db_count:

  Returns the number of entries in the music db.  */
extern gint music_db_count (MusicDB *db) __attribute__ ((pure));


/** music_db_add_m3u:

  Add M3U file FILE to music db DB.  Any error is returned in ERROR.
  Returns the number of entries added to DB.
 */
extern int music_db_add_m3u (MusicDB *self, const gchar *file,
			      GError **error);

/** music_db_add_uri:

  Add URI to music db DB.  Any error is returned in ERROR.
 */
extern void music_db_add_uri (MusicDB *db, const gchar *uri,
			       GError **error);

/** music_db_add_file:

  Add FILE to music db DB.  If INDEX is -1, then FILE is appended to
  DB.  Any error is returned in ERROR.
 */
extern void music_db_add_file (MusicDB *self, const gchar *file,
				GError **error);

/** music_db_add_recursive:

  Add files under PATH to music db DB.  Any error is returned in
  ERROR.  Returns the number of entries added to DB.
 */
extern int music_db_add_recursive (MusicDB *db, const gchar *path,
				    GError **error);


/** music_db_remove:

  Remove the entry in the music db DB with ID UID.  */
extern void music_db_remove (MusicDB *db, gint uid);

/** music_db_clear:

  Empty the play list.  Use carefully.  */
extern void music_db_clear (MusicDB *db);

/** music_db_get_info:

  Returns the information regarding the entry with uid UID in the
  music db DB.  Any of SOURCE, UID, ARIST, TITLE, ALBUM, DURATION may
  be NULL to indicate don't cares.  The caller must free the returned
  strings.  */
extern bool music_db_get_info (MusicDB *db, int uid,
			       char **source,
			       char **artist, char **album,
			       int *track, char **title, int *duration);


enum mdb_fields
  {
    MDB_SOURCE = 1 << 0,
    MDB_ARTIST = 1 << 1,
    MDB_ALBUM = 1 << 2,
    MDB_TRACK = 1 << 3,
    MDB_TITLE = 1 << 4,
    MDB_DURATION = 1 << 5
  };

struct music_db_info
{
  enum mdb_fields fields;

  char *source;
  char *artist;
  char *album;
  int track;
  char *title;
  int duration;
};

/* Set the meta-data associated with UID according to INFO.
   INFO->FIELDS determines which fields to set.  */
extern void music_db_set_info (MusicDB *db, int uid,
			       struct music_db_info *info);

/* Set the meta-data associated with UID according to the tags
   TAGS.  */
extern void music_db_set_info_from_tags (MusicDB *db, int uid,
					 GstTagList *tags);

/* Call CB for each entry in database DB.  Order the results according
   to ORDER, which is a zero-terminated array of enum mdb_fields.  */
extern int music_db_for_each (MusicDB *db,
			      int (*cb) (int uid, struct music_db_info *info),
			      enum mdb_fields *order);

/* Enqueue UID to the end of the play queue.  */
extern void music_db_play_queue_enqueue (MusicDB *db, int uid);
 
/* Dequeue an element from the play queue, return its UID.  Returns
   0 if the queue is empty or an error occurs.  */
extern int music_db_play_queue_dequeue (MusicDB *db);

/* Returns the number of elements in the play queue.  */
extern int music_db_play_queue_count (MusicDB *db);

/* Returns the UID of the OFFSET element in the queue (0 based).  If
   the returned value is zero, there is no such element at that
   offset.  */
extern int music_db_play_queue_query (MusicDB *db, int offset);

/* Removes the element at OFFSET in the play queue.  */
extern void music_db_play_queue_remove (MusicDB *db, int offset);

/* Iterate over each entry in the queue, in order.  */
extern int music_db_queue_for_each (MusicDB *db,
				    int (*cb) (int uid,
					       struct music_db_info *info));
#endif
