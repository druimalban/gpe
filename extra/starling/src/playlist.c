/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib/glist.h>
#include <glib/gstrfuncs.h>
#include <glib/gstdio.h>
#include <glib/gstring.h>
#include <glib/gstdio.h>

#include <gst/gst.h>
#include <gst/audio/gstaudiosink.h>

#include "marshal.h"

#include "playlist.h"

/* Private stuff */

typedef struct _PlayListPrivate PlayListPrivate;

struct _PlayListPrivate {
    GList *tracks;
    gint current;
    gboolean random;
    GstElement *playbin;
    GstState last_state;
};

static gboolean initialized = FALSE;

#define PLAY_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), PLAY_LIST_TYPE, PlayListPrivate))

static void
play_list_deleted_stream_default_handler (GObject *obj, Stream *stream, 
        gint pos, gpointer data)
{
    g_object_unref (stream);
}

static gboolean
play_list_bus_cb (GstBus *bus, GstMessage *message, gpointer data)
{
    PlayList *self = PLAY_LIST (data);
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (data);
    GstState newstate;

    switch (GST_MESSAGE_TYPE (message)) {
        case GST_MESSAGE_STATE_CHANGED:
            gst_message_parse_state_changed (message, NULL, &newstate, NULL);
            /* XXX: Create a default handler
             * for this
             */
            priv->last_state = newstate;
            //g_debug ("State changed to %d\n", newstate);
            g_signal_emit (self, 
                    PLAY_LIST_GET_CLASS (self)->state_changed_signal_id,
                    0, newstate);
            break;
        case GST_MESSAGE_EOS:
            g_signal_emit (self, PLAY_LIST_GET_CLASS (self)->eos_signal_id, 0);
        default:
            break;
    }

    return TRUE;
}

static void
play_list_class_init (PlayListClass *klass)
{
    GClosure *deleted_stream_closure;
    GType deleted_stream_types[2];
    
    g_type_class_add_private (klass, sizeof (PlayListPrivate));

    klass->new_stream_signal_id = g_signal_new ("new-stream",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST,
                                    0, NULL, NULL,
                                    g_cclosure_user_marshal_VOID__OBJECT_INT,
                                    G_TYPE_NONE, 2,
                                    STREAM_TYPE,
                                    G_TYPE_INT);

    deleted_stream_closure = g_cclosure_new (G_CALLBACK
            (play_list_deleted_stream_default_handler), NULL, NULL);

    deleted_stream_types[0] = STREAM_TYPE;
    deleted_stream_types[1] = G_TYPE_INT;

    klass->deleted_stream_signal_id = g_signal_newv ("deleted-stream",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    deleted_stream_closure,
                                    NULL,
                                    NULL,
                                    g_cclosure_user_marshal_VOID__OBJECT_INT,
                                    G_TYPE_NONE, 2,
                                    deleted_stream_types);
    
    klass->cleared_signal_id = g_signal_new ("cleared",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST,
                                    0, NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);
    
    klass->swap_signal_id = g_signal_new ("swap",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST,
                                    0, NULL, NULL,
                                    g_cclosure_user_marshal_VOID__INT_INT,
                                    G_TYPE_NONE, 2,
                                    G_TYPE_INT,
                                    G_TYPE_INT);
    
    klass->state_changed_signal_id = g_signal_new ("state-changed",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST,
                                    0, NULL, NULL,
                                    g_cclosure_marshal_VOID__ENUM,
                                    G_TYPE_NONE, 1,
                                    G_TYPE_INT);
    
    klass->eos_signal_id = g_signal_new ("eos",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST,
                                    0, NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);

}

static void
play_list_init_object (GTypeInstance *instance, gpointer g_class)
{
    PlayList *self = PLAY_LIST (instance);
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (instance);

    priv->tracks = NULL;
    priv->random = FALSE;
    priv->current = 0;
    priv->playbin = gst_element_factory_make ("playbin", "play");
    gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (priv->playbin)),
            play_list_bus_cb, self);

}

static void
play_list_clear_helper (gpointer data, gpointer dummy)
{
    g_object_unref (G_OBJECT (data));
}

static void
play_list_put_current_stream (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    void *data = g_list_nth_data (priv->tracks, priv->current);
    GstState last_state = priv->last_state;

    if (data && IS_STREAM (data)) {
        /* Try to make this work
         * with GST_STATE_READY
         */
        gst_element_set_state (priv->playbin, GST_STATE_NULL);
        g_object_set (G_OBJECT (priv->playbin), "uri", 
                STREAM (data)->uri, NULL);
        gst_element_set_state (priv->playbin, last_state);
    } else {
        g_debug ("play_list_put_currrent() called without valid data: %d\n", priv->current);
        g_debug ("PlayList lenght: %d\n", play_list_length (self));
    }
}

static void
play_list_good_stream_cb (Stream *stream, gpointer data)
{
    PlayList *self = PLAY_LIST (data);
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    GList *found;
    gint pos;

    found = g_list_find (priv->tracks, stream);

    g_return_if_fail (found != NULL);

    pos = g_list_position (priv->tracks, found);

    g_signal_emit (self, PLAY_LIST_GET_CLASS (self)->new_stream_signal_id,
                    0, stream, pos);
}

static void
play_list_bad_stream_cb (Stream *stream, gpointer data)
{
    PlayList *self = PLAY_LIST (data);
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    
    priv->tracks = g_list_remove (priv->tracks, stream);
}

static gint
play_list_random_next (PlayList *self)
{
    guint rand;
    FILE *fd;

    fd = fopen ("/dev/urandom", "r");

    fread (&rand, sizeof (rand), 1, fd);

    fclose (fd);

    return rand % play_list_length (self);
}
    

/* Public */

void
play_list_init (int *argc, char **argv[])
{
    gst_init (argc, argv);
    
    //g_thread_init (NULL);

    initialized = TRUE;
}

gboolean
play_list_add_uri (PlayList *self, const gchar *uri, gint pos)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    Stream *str;

    str = stream_new ();
    g_signal_connect (G_OBJECT (str), "good-stream", 
                    G_CALLBACK (play_list_good_stream_cb), self);
    g_signal_connect (G_OBJECT (str), "bad-stream",
                    G_CALLBACK (play_list_bad_stream_cb), self);

    priv->tracks = g_list_insert (priv->tracks, str, pos);
    /* Set the uri *only after* connecting the callback
     * and putting the stream on the list.
     * Otherwise we could miss the signal or don't 
     * find the stream in the list.
     */
    if (!stream_set_uri (str, uri)) {
        g_object_unref (str);
        return FALSE;
    }

    return TRUE;
}

void
play_list_add_m3u (PlayList *self, const gchar *path) 
{
    gchar *content;
    gchar **lines;
    gint ii;
    gsize size;
    GError *error;

    if (!g_file_get_contents (path, &content, &size, &error)) {
        g_fprintf (stderr, "Error reading file %s: %s\n", path,
                error->message);
        g_error_free (error);
        
        return;
    }

    lines = g_strsplit (content, "\n", 2048); /* This should be enough */ 

    g_free (content);

    for (ii = 0; lines[ii]; ii++)
        play_list_add_file (self, lines[ii]);

    g_strfreev (lines);
}

void
play_list_save_m3u (PlayList *self, const gchar *path)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    GList *cur;
    GString *string = NULL;

    if (g_list_length (priv->tracks) == 0) {
        /* Remove the previous playlist */
        g_unlink (path);
        return;
    }

    string = g_string_new ("");

    //g_debug ("PlayList length: %d\n", g_list_length (priv->tracks));

    //play_list_dump (self);
    
    for (cur = priv->tracks; cur; cur = cur->next) {
        Stream *s = cur->data;
        string = g_string_append (string, s->uri + 7);
        string = g_string_append (string, "\n");
    }

    g_file_set_contents (path, string->str, string->len, NULL);

    g_string_free (string, TRUE);
}

void
play_list_add_file (PlayList *self, const gchar *path)
{
    gchar *uri;

    if (path && path[0] != '\0') {
        uri = g_strdup_printf ("file://%s", path);
        play_list_add_uri (self, uri, -1);

        g_free (uri);
    }
}

void
play_list_add_recursive (PlayList *self, const gchar *path)
{
    GDir *dir;
    gchar *file;
    gchar *filename;

    if (!g_file_test (path, G_FILE_TEST_EXISTS))
        return;
    
    if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
        dir = g_dir_open (path, 0, NULL);
        if (!dir)
            return;
        while (( file = (gchar *) g_dir_read_name( dir ) )) {
            filename = g_strdup_printf ("%s/%s", path, file);
            play_list_add_recursive (self, filename);
            g_free (filename);
        }
        
        g_dir_close (dir);
        return;
    }
    
    if (g_str_has_suffix (path, ".m3u")) {
        play_list_add_m3u (self, path);
        return;
    }

    play_list_add_file (self, path);
}
    
void
play_list_set_random (PlayList *self, gboolean random)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);

    priv->random = random;
}

gboolean
play_list_get_random (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);

    return priv->random;
}

GType
play_list_get_type (void)
{
    static GType play_list_type = 0;

    if (G_UNLIKELY (0 == play_list_type)) {
        static const GTypeInfo play_list_info = {
            sizeof(PlayListClass),
            NULL,
            NULL,
            (GClassInitFunc) play_list_class_init,
            NULL,
            NULL,
            sizeof(PlayList),
            0,
            play_list_init_object
        };

        play_list_type = g_type_register_static (G_TYPE_OBJECT, 
                                                "PlayList",
                                                &play_list_info, 0);
    }

    return play_list_type;
}

PlayList *
play_list_new (void)
{
    if (G_UNLIKELY (!initialized)) {
        g_critical ("PlayList has not been initilized\n");

        /* This will likely make the application crash.
         * I think it's the best way to tell the developer
         * he/she's doing something bad :)
         */
        return NULL;
    }

    return g_object_new (PLAY_LIST_TYPE, NULL);
}

void
play_list_remove_pos (PlayList *self, gint pos)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    GList *cur = g_list_nth (priv->tracks, pos);

    if (cur && cur->data && IS_STREAM (cur->data)) {
        /*
         * g_object_unref (G_OBJECT (cur->data));
         * The object will be unreferenced in the
         * default handler
         */
        g_debug ("Deleting at position %d\n", pos);
        g_signal_emit (self, 
                    PLAY_LIST_GET_CLASS (self)->deleted_stream_signal_id,
                    0, STREAM (cur->data), pos);
        priv->tracks = g_list_remove (priv->tracks, cur->data);
    }
}

void
play_list_swap_pos (PlayList *self, gint left, gint right)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    
    GList *l = g_list_nth (priv->tracks, left);
    GList *r = g_list_nth (priv->tracks, right);

    if (!l || !r)
        return;

    gpointer tmp = l->data;

    l->data = r->data;
    r->data = tmp;

    if (priv->current == left) {
        priv->current = right;
    } else if (priv->current == right) {
        priv->current = left;
    }

    g_signal_emit (self, PLAY_LIST_GET_CLASS (self)->swap_signal_id,
                    0, left, right);
}

gint
play_list_length (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);

    return g_list_length (priv->tracks);
}

void
play_list_clear (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);

    if (g_list_length (priv->tracks) < 1)
        return;

    g_list_foreach (priv->tracks, play_list_clear_helper, NULL);
    g_list_free (priv->tracks);
    priv->tracks = NULL;
    priv->current = -1;
    
    g_signal_emit (self, PLAY_LIST_GET_CLASS (self)->cleared_signal_id, 0);
}

void
play_list_set_state (PlayList *self, GstState state)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    
    gst_element_set_state (GST_ELEMENT (priv->playbin), state);
}

GstState
play_list_get_last_state (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);

    return priv->last_state;
}

gint
play_list_get_current (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    
    return priv->current;
}

void
play_list_set_current (PlayList *self, gint pos)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);

    if (pos < 0) {
        pos = 0;
    } else if (pos > g_list_length (priv->tracks) - 1) {
        pos = g_list_length (priv->tracks) - 1;
    }

    priv->current = pos;

    play_list_put_current_stream (self);
}

void
play_list_next (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);

    if (priv->random) {
        priv->current = play_list_random_next (self);
    } else {
        priv->current++;
        if (priv->current >= g_list_length (priv->tracks))
            priv->current = 0;
    }

    play_list_put_current_stream (self);
}

void
play_list_prev (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    
    if (priv->random) {
        priv->current = play_list_random_next (self);
    } else {
        priv->current--;
        if (priv->current < 0)
            priv->current = g_list_length (priv->tracks) - 1;
    }

    play_list_put_current_stream (self);
}

gboolean
play_list_seek (PlayList *self, GstFormat format, gint64 pos)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    
    return gst_element_seek (GST_ELEMENT (priv->playbin), 1.0, format,
                                 GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
                                 pos, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

gboolean
play_list_query_position (PlayList *self, GstFormat *fmt, gint64 *pos)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);

    return gst_element_query_position (priv->playbin, fmt, pos); 
}

gboolean
play_list_query_duration (PlayList *self, GstFormat *fmt, gint64 *pos, gint n)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    
    if (n < 0) {
        return gst_element_query_duration (priv->playbin, fmt, pos);
    }
}

gboolean
play_list_set_sink (PlayList *self, const gchar *sink)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    GstElement *audiosink = NULL;

    audiosink = gst_element_factory_make (sink, "sink");
    if (!GST_IS_AUDIO_SINK (audiosink)) {
        if (audiosink) {
            gst_object_unref (audiosink);
        }

        return FALSE;
    }

    g_object_set (G_OBJECT (priv->playbin), "audio-sink", audiosink, NULL);
    
    return TRUE;
}

Stream *
play_list_get_stream (PlayList *self, gint pos)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    gint requested;
    Stream *stream;

    if(pos >= 0) {
        requested = pos;
    } else {
        requested = priv->current;
    }

    stream = g_list_nth_data (priv->tracks, requested);

    return (IS_STREAM (stream) ? stream : NULL);
}
    

void 
play_list_dump (PlayList *self)
{
    PlayListPrivate *priv = PLAY_LIST_GET_PRIVATE (self);
    GList *cur;

    for( cur = priv->tracks; cur; cur = cur->next) {
        Stream *s = cur->data;
        g_debug ("%s\n", s->uri);
    }
}


