/*
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

#include "stream.h"

typedef struct _PlayList PlayList;
typedef struct _PlayListClass PlayListClass;

#define PLAY_LIST_TYPE              (play_list_get_type ())
#define PLAY_LIST(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLAY_LIST_TYPE, PlayList))
#define PLAY_LIST_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PLAY_LIST_TYPE, PlayListClass))
#define IS_PLAY_LIST(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLAY_LIST_TYPE))
#define IS_PLAY_LIST_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PLAY_LIST_TYPE))
#define PLAY_LIST_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PLAY_LIST_TYPE, PlayListClass))

struct _PlayList {
    GObject parent;
};

struct _PlayListClass {
    GObjectClass parent;
    guint new_stream_signal_id;
    guint deleted_stream_signal_id;
    guint cleared_signal_id;
    guint swap_signal_id; 
    guint state_changed_signal_id;
    guint eos_signal_id;
};


GType play_list_get_type (void);

void play_list_init (int *argc, char **argv[]);

PlayList * play_list_new (void);

void play_list_set_random (PlayList *self, gboolean random);

gboolean play_list_get_random (PlayList *self);

void play_list_remove_pos (PlayList *self, gint pos);

void play_list_swap_pos (PlayList *Self, gint left, gint right);

gint play_list_length (PlayList *self);

void play_list_clear (PlayList *self);

void play_list_set_state (PlayList *self, GstState state);

GstState play_list_get_last_state (PlayList *self);

gint play_list_get_current (PlayList *self);

void play_list_set_current (PlayList *self, gint pos);

void play_list_next (PlayList *self);

void play_list_prev (PlayList *self);

gboolean play_list_seek (PlayList *self, GstFormat format, gint64 pos);

gboolean play_list_query_position (PlayList *self, GstFormat *fmt, gint64 *pos);

gboolean play_list_query_duration (PlayList *self, GstFormat *fmt, gint64 *pos, gint n);

gboolean play_list_add_uri (PlayList *self, const gchar *uri, gint pos);

void play_list_add_file (PlayList *self, const gchar *path);

void play_list_add_m3u (PlayList *self, const gchar *path);

void play_list_save_m3u (PlayList *self, const gchar *path);

void play_list_add_recursive (PlayList *self, const gchar *path);

void play_list_dump (PlayList *self);

gboolean play_list_set_sink (PlayList *self, const gchar *sink);

Stream * play_list_get_stream (PlayList *self, gint pos);

#endif

