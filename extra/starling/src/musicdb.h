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

#include <config.h>

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

  /* "added-to-play-list" signal: A new entry was added to a play
     list.  Passed the list (char *), position (guint) and UID (guint)
     of the new entry.  */
  guint added_to_play_list_signal_id;
  /* "removed-from-play-list" signal: An entry was removed from a play
     list.  Passed the play list (char *) and position (guint).  */
  guint removed_from_play_list_signal_id;

  /* "status" signal: Indicates the amount of work to be done.  Passed
     a status message (char *).  When no work remains, passed
     NULL.  */
  guint status_signal_id;
};

extern GType music_db_get_type (void);

/** music_db_open:

  Create a music db associated with file FILE.  Any error is returned
  in ERROR.  */
extern MusicDB *music_db_open (const char *file, GError **error);

/** music_db_count:

  Returns the number of entries in the music db that match constraint
  CONSTAINT.  See music_db_for_each for valid values of
  constraint.  */
extern gint music_db_count (MusicDB *db, const char *list,
			    const char *constraint) __attribute__ ((pure));


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

enum mdb_fields
  {
    MDB_SOURCE = 1 << 0,
    MDB_ARTIST = 1 << 1,
    MDB_ALBUM = 1 << 2,
    MDB_TRACK = 1 << 3,
    MDB_TITLE = 1 << 4,
    MDB_DURATION = 1 << 5,
    MDB_GENRE = 1 << 6,
    MDB_DATE_ADDED = 1 << 7,
    MDB_PLAY_COUNT = 1 << 8,
    MDB_DATE_LAST_PLAYED = 1 << 9,
    MDB_DATE_TAGS_UPDATED = 1 << 10,
    MDB_RATING = 1 << 11,
    MDB_PRESENT = 1 << 12,
    MDB_MTIME = 1 << 13,

    /* These take precedent of the simple set variants.  */
    MDB_INC_PLAY_COUNT = 1 << 29,
    MDB_UPDATE_DATE_LAST_PLAYED = 1 << 30,
    MDB_UPDATE_DATE_TAGS_UPDATED = 1 << 31,
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
  char *genre;

  int play_count;
  int date_added;
  int date_last_played;
  int date_tags_updated;

  int rating;

  bool present;

  /* Source's last modification time.  */
  time_t mtime;
};

/** music_db_get_info:

  Returns the information regarding the entry with uid UID in the
  music db DB.  INFO->FIELDS indicates the interested fields.  The
  caller must free any returned strings.  */
extern bool music_db_get_info (MusicDB *db, int uid,
			       struct music_db_info *info);


/* Set the meta-data associated with UID according to INFO.
   INFO->FIELDS determines which fields to set.  */
extern void music_db_set_info (MusicDB *db, int uid,
			       struct music_db_info *info);

/* Set the meta-data associated with UID according to the tags
   TAGS.  */
extern void music_db_set_info_from_tags (MusicDB *db, int uid,
					 GstTagList *tags);

/* Iterate over each entry in the list.  Order the results according
   to ORDER, which is a zero-terminated array of enum mdb_fields.  If
   CONSTRAINT is non-NULL, it is where SQL clause (without the where),
   e.g., `artist in ("Foo", "Bar")'.  */
extern int music_db_for_each (MusicDB *db, const char *list,
			      int (*cb) (int uid, struct music_db_info *info),
			      enum mdb_fields *order,
			      const char *constraint);

/* Enqueue UID to the end of the play list.  */
extern void music_db_play_list_enqueue (MusicDB *db, const char *list, int uid);
 
/* Dequeue an element from the play list, return its UID.  Returns
   0 if the list is empty or an error occurs.  */
extern int music_db_play_list_dequeue (MusicDB *db, const char *list);

/* Returns the UID of the OFFSET element in the list (0 based).  If
   the returned value is zero, there is no such element at that
   offset.  */
extern int music_db_play_list_query (MusicDB *db, const char *list,
				     int offset);

/* Removes the element at OFFSET in the play list.  */
extern void music_db_play_list_remove (MusicDB *db, const char *list,
				       int offset);

/* Clear the play list.  */
extern void music_db_play_list_clear (MusicDB *db, const char *list);

/* Iterate over each play list in the database.  */
extern int music_db_play_lists_for_each (MusicDB *db,
					 int (*cb) (const char *list));
#endif
