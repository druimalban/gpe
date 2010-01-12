/*
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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <glib-object.h>

#include "musicdb.h"

typedef struct _PlayList PlayList;
typedef struct _PlayListClass PlayListClass;

#define PLAY_LIST_TYPE              (play_list_get_type ())
#define PLAY_LIST(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLAY_LIST_TYPE, PlayList))
#define PLAY_LIST_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PLAY_LIST_TYPE, PlayListClass))
#define IS_PLAY_LIST(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLAY_LIST_TYPE))
#define IS_PLAY_LIST_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAY_LIST_TYPE))
#define PLAY_LIST_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAY_LIST_TYPE, PlayListClass))

struct _PlayListClass
{
  GObjectClass parent;
};

extern GType play_list_get_type (void);

/** A PLAY_LIST implements the GtkTreeModel interface.  The following
   are the columns which it provides.  */
enum
  {
    PL_COL_INDEX,
    PL_COL_SOURCE,
    PL_COL_UID,
    PL_COL_ARTIST,
    PL_COL_ALBUM,
    PL_COL_TRACK,
    PL_COL_TITLE,
    PL_COL_DURATION,
    PL_COL_GENRE,
    PL_COL_PLAY_COUNT,
    PL_COL_DATE_ADDED,
    PL_COL_DATE_LAST_PLAYED,
    PL_COL_DATE_TAGS_UPDATED,
    PL_COL_RATING,

    PL_COL_COUNT
  };

/** play_list_open:

    Create a new play list object based on music database DB.  If LIST
    is NULL, uses the library.  */
extern PlayList *play_list_new (MusicDB *db, const char *list);

/* Set the play list to LIST.  */
extern void play_list_set (PlayList *pl, const char *list);

/* Return the current play list.  */
extern const char *play_list_get (PlayList *pl);


/* Narrow the visible tracks according to CONSTRAINT.  CONSTRAINT is
   interpreted as for music_db_for_each.  */
extern void play_list_constrain (PlayList *pl, const char *constraint);

/* Return the current constraint.  */
extern const char *play_list_constraint_get (PlayList *pl);


/* Group records according to the field SCOPE and only return a single
   record for each group.  SCOPE may be iether 0, MDB_ARTIST or
   MDB_ALBUM.  */
extern void play_list_group_by (PlayList *pl, enum mdb_fields scope);

/* Return the currenting grouping parameter.  */
extern enum mdb_fields play_list_group_by_get (PlayList *pl);


/* Return the number of entries.  */
extern gint play_list_count (PlayList *pl);

/* Return the number of entries ignoring the constraint.  */
extern gint play_list_total (PlayList *pl);

/* Permanently remove entry IDX.  */
extern void play_list_remove (PlayList *pl, gint idx);

/** play_list_get_info:

    Returns the uid associated with INDEX.  If invalid, returns 0.
    (A valid UID is guaranteed to be non-0.)  */
extern guint play_list_index_to_uid (PlayList *pl, int index);

/* Return an index (if any) associated with UID.  If none, return
   -1.  */
extern int play_list_uid_to_index (PlayList *pl, guint uid);

/* Force a "row-changed" signal to be emitted for row N.  */
extern void play_list_force_changed (PlayList *pl, gint n);

#endif

