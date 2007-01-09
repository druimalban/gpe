/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib/gstrfuncs.h>
#include <glib/gerror.h>
#include <glib/gstdio.h>

#include <gst/gstobject.h>
#include <gst/gstelement.h>
#include <gst/gstbin.h>
#include <gst/gstpipeline.h>
#include <gst/gststructure.h>
#include <gst/gstcaps.h>
#include <gst/gsttaglist.h>
#include <gst/gstmessage.h>
#include <gst/gstbus.h>
#include <gst/gstutils.h>

#include "errorbox.h"
#include "stream.h"


/* Private */

typedef struct _StreamPrivate StreamPrivate;

struct _StreamPrivate {
    GHashTable *tags;
    gboolean disposed;
    GstElement *source;
    GstElement *pipeline;
    GstElement *decodebin;
};

static gpointer parent_class = NULL;

#define STREAM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), STREAM_TYPE, StreamPrivate))

static void stream_bad_stream_default_handler (Stream *self);
static void stream_new_decoded_pad_cb (GstElement *decodebin, GstPad *pad,
                                    gboolean last, gpointer data);
static void stream_unknown_type_cb (GstElement * decodebin, GstPad * pad, 
                                     GstCaps * caps, gpointer data);

static void
stream_dispose (GObject *obj)
{
    StreamPrivate *priv = STREAM_GET_PRIVATE (obj);

    if (priv->disposed) {
        return;
    }
    
    priv->disposed = TRUE;

    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
stream_finalize (GObject *obj)
{
    Stream *self = STREAM (obj);
    StreamPrivate *priv = STREAM_GET_PRIVATE (self);

    g_hash_table_destroy (priv->tags);
    
    if (priv->pipeline) {
        gst_element_set_state (priv->pipeline, GST_STATE_NULL);
        gst_object_unref (priv->pipeline);
    }

    g_free (self->uri);

    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
stream_class_init (StreamClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = stream_dispose;
    gobject_class->finalize = stream_finalize;

    g_type_class_add_private (klass, sizeof (StreamPrivate));

    parent_class = g_type_class_peek_parent (klass);

    klass->bad_stream = stream_bad_stream_default_handler;

    klass->bad_stream_signal_id = 
            g_signal_new ("bad_stream",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    G_STRUCT_OFFSET (StreamClass, bad_stream), 
                    NULL, 
                    NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);

    klass->good_stream_signal_id = 
            g_signal_new ("good_stream",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    0,
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
    klass->new_tag_signal_id = 
            g_signal_new ("new_tag",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_FIRST,
                    0,
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
stream_init (GTypeInstance *instance, gpointer g_class)
{
    Stream *self = STREAM (instance);
    StreamPrivate *priv = STREAM_GET_PRIVATE (self);

    self->uri = NULL;
    priv->tags = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         NULL, g_free);
    priv->disposed = FALSE;

    priv->source = NULL;
    priv->pipeline = gst_pipeline_new (NULL);
    priv->decodebin = gst_element_factory_make ("decodebin", "decodebin");
    
    if (!GST_IS_ELEMENT (priv->decodebin)) {
        starling_error_box ("Decodebin could not be initialized. Please check your"
                        " GStreamer installation\n");
        /* This is a fatal error. PlayList relies on playbin for playing
         * which relies on decodebin for decoding, so decodebin support
         * is mandatory.
         */


        _Exit (1);
    }

    /* Wait for the decodebin to identify the stream */

    g_signal_connect (G_OBJECT (priv->decodebin), "new-decoded-pad", 
                    G_CALLBACK (stream_new_decoded_pad_cb), self);
    g_signal_connect (G_OBJECT (priv->decodebin), "unknown-type", 
                    G_CALLBACK (stream_unknown_type_cb), self);

}

static void
stream_put_tag (const GstTagList *list, const gchar *tag, gpointer data)
{
    Stream *self = STREAM (data);
    StreamPrivate *priv = STREAM_GET_PRIVATE (self);
    gchar *value;

    if (gst_tag_get_type (tag) == G_TYPE_STRING) {
        gst_tag_list_get_string_index (list, tag, 0, &value);
    } else {
      value =
          g_strdup_value_contents (gst_tag_list_get_value_index (list, tag, 0));
    }

    g_hash_table_insert (priv->tags, (gchar *) tag, value);

    g_signal_emit (self, STREAM_GET_CLASS (self)->new_tag_signal_id, 0, tag);
}

static void 
stream_bad_stream_default_handler (Stream *self)
{
    g_debug ("Autodestroying %s\n", self->uri);
    /* Destroy the GstElements, they are useless now */
    StreamPrivate *priv = STREAM_GET_PRIVATE (self);

    /* Thill will free most of the resources. We will destroy
     * it in the finalize function.
     */
    gst_element_set_state (priv->pipeline, GST_STATE_NULL);
}

static void
stream_new_decoded_pad_cb (GstElement *decodebin, GstPad *pad,
                                    gboolean last, gpointer data)
{

    Stream *self = STREAM (data);
    GstCaps *caps;
    GstStructure *structure;
    const gchar *mimetype;

    caps = gst_pad_get_caps (pad);
    structure = gst_caps_get_structure (caps, 0);
    mimetype = gst_structure_get_name (structure);

    if (g_str_has_prefix (mimetype, "audio/")) {
        g_signal_emit (self, STREAM_GET_CLASS (self)->good_stream_signal_id, 0);
    } else {
         g_signal_emit (self, STREAM_GET_CLASS (self)->bad_stream_signal_id, 0);
    }
    gst_caps_unref (caps);
}
static void
stream_unknown_type_cb (GstElement * decodebin, GstPad * pad, 
                        GstCaps * caps, gpointer data)
{
    Stream *self = STREAM (data);

    g_debug ("Unknown stream\n");

    g_signal_emit (self, STREAM_GET_CLASS (self)->bad_stream_signal_id, 0);
}


static gboolean 
stream_bus_cb (GstBus *bus, GstMessage *message, gpointer data)
{
    //StreamPrivate *priv = STREAM_GET_PRIVATE (data);
    
    GstTagList *tags;
    /*GError *gerror;
    gchar *debug;
    GstState newstate;*/

    switch (GST_MESSAGE_TYPE (message)) {
        case GST_MESSAGE_STATE_CHANGED:
            //gst_message_parse_state_changed (message, NULL, &newstate, NULL);
            //g_debug ("New state in the decodebin: %d\n", newstate);
            break;
        case GST_MESSAGE_EOS:
            g_debug ("EOS in the decodebin\n");
            break;
            
        case GST_MESSAGE_ERROR:
#if 0
            gst_message_parse_error (message, &gerror, &debug);
            gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
            gst_message_unref (message);
            g_error_free (gerror);
            g_free (debug);
            gst_element_set_state (GST_ELEMENT (priv->pipeline), GST_STATE_NULL);
#endif
            break;

        case GST_MESSAGE_TAG:

            gst_message_parse_tag (message, &tags);

            /* bypass set_tag */
            gst_tag_list_foreach (tags, stream_put_tag, data);
            break;

        default:
            break;
    }

    return TRUE;
}

/* Public */

GType
stream_get_type (void)
{
    static GType stream_type = 0;


    if(G_UNLIKELY (0 == stream_type)) {
        static const GTypeInfo stream_info = {
            sizeof(StreamClass),
            NULL,
            NULL,
            (GClassInitFunc) stream_class_init,
            NULL,
            NULL,
            sizeof(Stream),
            0,
            stream_init
        };

        stream_type = g_type_register_static (G_TYPE_OBJECT, 
                                                "Stream",
                                                &stream_info, 0);
    }

    return stream_type;
}

Stream *
stream_new (void)
{
    return g_object_new (STREAM_TYPE, NULL);
}

Stream *
stream_new_with_uri (const gchar *uri)
{
    Stream *str = stream_new ();
    
    if (stream_set_uri (str, uri)) {
        return str;
    }

    g_object_unref (G_OBJECT (str));

    return NULL;
}

gboolean
stream_set_uri (Stream *self, const gchar *uri)
{
    StreamPrivate *p = STREAM_GET_PRIVATE (self);

    g_debug ("Adding uri: %s\n", uri);

    if (g_str_has_prefix (uri, "file://")) {
        self->uri = g_strdup (uri);
        p->source = gst_element_factory_make ("filesrc", "filesrc");
        g_assert (GST_IS_ELEMENT (p->source));

        g_object_set (p->source, "location", uri + 7, NULL); /* omit file:// */

        gst_bin_add_many (GST_BIN (p->pipeline), p->source, p->decodebin, NULL);
        gst_element_link (p->source, p->decodebin);

        gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (p->pipeline)),
                            stream_bus_cb, self);

        gst_element_set_state (GST_ELEMENT (p->pipeline), GST_STATE_PLAYING);
        return TRUE;
    } 

    return FALSE;
}

const gchar *
stream_get_tag (Stream *self, const gchar *key)
{
    StreamPrivate *priv = STREAM_GET_PRIVATE (self);
    
    return g_hash_table_lookup (priv->tags, key);
}

void
stream_set_tag (Stream *self, const gchar *key, const gchar *value)
{
    StreamPrivate *priv = STREAM_GET_PRIVATE (self);
    
    g_hash_table_replace (priv->tags, (gchar*) key, g_strdup (value));
}

