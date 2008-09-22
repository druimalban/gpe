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
    PL_COL_COUNT
  };

enum play_list_mode
  {
    PLAY_LIST_LIBRARY,
    PLAY_LIST_QUEUE
  };

/** play_list_open:

    Create a new play list based on the music database DB.  */
extern PlayList *play_list_new (MusicDB *db, enum play_list_mode mode);

/* Return the number of entries.  */
extern gint play_list_count (PlayList *pl);

/** play_list_get_current:

    Return the current entry. */
extern gint play_list_get_current (PlayList *pl);

/** play_list_goto:

    Set the current entry.  */
extern void play_list_goto (PlayList *pl, gint idx);

/* Permanently remove entry IDX.  */
extern void play_list_remove (PlayList *pl, gint idx);

/** play_list_get_info:

    Returns the information regarding entry INDEX in playlist PL.  If
    INDEX is -1, information regarding the currently selected entry is
    returned.  Any of SOURCE, UID, ARIST, TITLE, ALBUM, DURATION may
    be NULL to indicate don't cares.  The caller must free the
    returned strings.  */
extern void play_list_get_info (PlayList *pl, int index, int *uid,
				char **source,
				char **artist, char **album,
				int *track, char **title, int *duration);

/* Return an index (if any) associated with UID.  If none, return
   -1.  */
extern int play_list_uid_to_index (PlayList *pl, int uid);

#endif

