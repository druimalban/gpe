/* playlists.h - A treemodel for playlists.
   Copyright 2007, 2010 Neal H. Walfield <neal@walfield.org>

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

#ifndef PLAYLISTS_H
#define PLAYLISTS_H

#include <glib-object.h>
#include <gtk/gtktreemodel.h>

#include "musicdb.h"

typedef struct _PlayLists PlayLists;
typedef struct _PlayListsClass PlayListsClass;

#define PLAY_LISTS_TYPE              (play_lists_get_type ())
#define PLAY_LISTS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLAY_LISTS_TYPE, PlayLists))
#define PLAY_LISTS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PLAY_LISTS_TYPE, PlayListsClass))
#define IS_PLAY_LISTS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLAY_LISTS_TYPE))
#define IS_PLAY_LISTS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAY_LISTS_TYPE))
#define PLAY_LISTS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAY_LISTS_TYPE, PlayListsClass))

struct _PlayListsClass
{
  GObjectClass parent;
};

extern GType play_lists_get_type (void);

/** A PLAY_LISTS implements the GtkTreeModel interface.  The following
   are the columns which it provides.  */
enum
  {
    /* The play list's index.  */
    PLS_COL_INDEX,
    /* The play list's name.  */
    PLS_COL_NAME,
    /* The number of tracks in the playlist.  */
    PLS_COL_TRACKS,

    PLS_COL_COUNT
  };

/** play_lists_open:

    Create a new play lists object based on music database DB.  If
    INCLUDE_QUEUE is TRUE, then the list includes the queue play list.
    Otherwise, the queue play list is excluded.  */
extern PlayLists *play_lists_new (MusicDB *db, gboolean include_queue);

extern void play_lists_sort_set (PlayLists *pls);

/* Return the number of entries.  */
extern gint play_lists_total (PlayLists *pls);

/* Force a "row-changed" signal to be emitted for row N.  */
extern void play_lists_force_changed (PlayLists *pls, gint n);

/* Return the index (= row) of the play list PLAY_LIST.  */
extern gint play_lists_index_of (PlayLists *pls, const char *play_list);

/* Returns true and sets iter to the row containing play list
   PLAY_LIST if PLAY_LIST exists.  Otherwise, returns false.  */
extern gboolean play_lists_iter_of (PlayLists *pls, GtkTreeIter *iter,
				    const char *play_list);

#endif
