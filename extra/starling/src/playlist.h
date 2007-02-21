/*
 * Copyright 2007 Neal H. Walfield <neal@walfield.org>
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <glib-object.h>

#include <gst/gstelement.h>
#include <gst/gstformat.h>

typedef struct _PlayList PlayList;
typedef struct _PlayListClass PlayListClass;

#define PLAY_LIST_TYPE              (play_list_get_type ())
#define PLAY_LIST(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLAY_LIST_TYPE, PlayList))
#define PLAY_LIST_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PLAY_LIST_TYPE, PlayListClass))
#define IS_PLAY_LIST(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLAY_LIST_TYPE))
#define IS_PLAY_LIST_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAY_LIST_TYPE))
#define PLAY_LIST_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAY_LIST_TYPE, PlayListClass))

struct _PlayListClass {
    GObjectClass parent;
    guint new_stream_signal_id;
    guint deleted_stream_signal_id;
    guint cleared_signal_id;
    guint swap_signal_id; 
    guint state_changed_signal_id;
    guint eos_signal_id;
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
    PL_COL_TITLE,
    PL_COL_ALBUM,
    PL_COL_DURATION,
    PL_COL_COUNT
  };

/** play_list_open:

  Create a playlist associated with file FILE.  Any error is returned
  in ERROR.  */
extern PlayList *play_list_open (const char *file, GError **error);

extern gboolean play_list_set_sink (PlayList *self, const gchar *sink);

extern void play_list_set_random (PlayList *self, gboolean random);

extern gboolean play_list_get_random (PlayList *self);

/** play_list_count:

  Returns the number of entries in the playlist.  */
extern gint play_list_count (PlayList *pl) __attribute__ ((pure));

/** play_list_play:

  Start the playlist PL playing.  Returns TRUE if starting to play the
  current entry was successful.  */
extern gboolean play_list_play (PlayList *pl);

/** play_list_pause:

  Pause the playlist PL.  */
extern void play_list_pause (PlayList *pl);

/** play_list_pause:

  Pause the player if playing otherwise, starting playing.  */
extern void play_list_play_pause_toggle (PlayList *pl);

extern gboolean play_list_playing (PlayList *pl);

extern gint play_list_get_current (PlayList *self);

/** play_list_goto:

  Start playing at the entry at position POS.  */
extern void play_list_goto (PlayList *pl, gint pos);

/** play_list_next:

  Play the next entry.  If in random mode, selects a random entry.  */
extern void play_list_next (PlayList *self);

/** play_list_prev:

  Play the previous entry.  If in random mode, selects a random
  entry.  */
extern void play_list_prev (PlayList *pl);

extern gboolean play_list_seek (PlayList *self, GstFormat format, gint64 pos);

extern gboolean play_list_query_position (PlayList *self, GstFormat *fmt,
					  gint64 *pos);

extern gboolean play_list_query_duration (PlayList *self, GstFormat *fmt,
					  gint64 *pos, gint n);

/** play_list_add_m3u:

  Add M3U file FILE to playlist PL at position INDEX.  If INDEX is -1,
  then entries are appended to PL.  Any error is returned in ERROR.
  Returns the number of entries added to PL.
 */
extern int play_list_add_m3u (PlayList *self, const gchar *file, gint index,
			      GError **error);

/** play_list_add_uri:

  Add URI to playlist PL at position INDEX.  If INDEX is -1, then URI
  is appended to PL.  Any error is returned in ERROR.
 */
extern void play_list_add_uri (PlayList *pl, const gchar *uri, gint index,
			       GError **error);

/** play_list_add_file:

  Add FILE to playlist PL at position INDEX.  If INDEX is -1, then
  FILE is appended to PL.  Any error is returned in ERROR.
 */
extern void play_list_add_file (PlayList *self, const gchar *file,
				gint index, GError **error);

/** play_list_add_recursive:

  Add files under PATH to playlist PL at position INDEX.  If INDEX is
  -1, then entries are appended to PL.  Any error is returned in
  ERROR.  Returns the number of entries added to PL.
 */
extern int play_list_add_recursive (PlayList *pl, const gchar *path,
				    int index, GError **error);

extern void play_list_save_m3u (PlayList *self, const gchar *path);

/** play_list_swap_pos:

  Swap the positions of entries LEFT and RIGHT in playlist PL.  */
extern void play_list_swap_pos (PlayList *pl, gint left, gint right);

/** play_list_remove:

  Remove the entry in playlist PL at position POS.  */
extern void play_list_remove (PlayList *pl, gint pos);

/** play_list_clear:

  Empty the play list.  Use carefully.  */
extern void play_list_clear (PlayList *pl);

/**
  play_list_get_info:

  Returns the information regarding entry INDEX in playlist PL.  If
  INDEX is -1, information regarding the currently selected entry is
  returned.  Any of SOURCE, UID, ARIST, TITLE, ALBUM, DURATION may be
  NULL to indicate don't cares.  The caller must free the returned
  strings.  */
extern void play_list_get_info (PlayList *pl, gint index,
				char **source, char **uid,
				char **artist, char **title, char **album,
				int *duration);

#endif

