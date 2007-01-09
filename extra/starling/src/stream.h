/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef STREAM_H
#define STREAM_H

#include <glib-object.h>

#include <glib/gtypes.h>
#include <glib/ghash.h>

typedef struct _Stream Stream;
typedef struct _StreamClass StreamClass;

#define STREAM_TYPE             (stream_get_type ())
#define STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), STREAM_TYPE, Stream))
#define STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), STREAM_TYPE, StreamClass))
#define IS_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STREAM_TYPE))
#define IS_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), STREAM_TYPE))
#define STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), STREAM_TYPE, StreamClass))

struct _Stream {
    GObject parent;
    /* Private */
    gchar *uri;
};

struct _StreamClass {
    GObjectClass parent;
    gint good_stream_signal_id;
    gint bad_stream_signal_id;
    gint new_tag_signal_id;
    void (*bad_stream) (Stream *self);
};

GType stream_get_type (void);

Stream * stream_new (void);

gboolean stream_set_uri (Stream *self, const gchar *uri);

/* Use only if you know what you're doing */
Stream * stream_new_with_uri (const gchar *uri);

const gchar * stream_get_tag (Stream *self, const gchar *key);

void stream_set_tag (Stream *self, const gchar *key, const gchar *value);

#endif


