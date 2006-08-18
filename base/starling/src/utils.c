/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <string.h>

#include <glib/gstrfuncs.h>
#include <glib/gstring.h>

#include <gst/gsttaglist.h>

#include "starling.h"
#include "utils.h"

gint
gtk_tree_view_get_position (GtkTreeView *view)
{
    GtkTreePath *path = NULL;
    gchar *pathstr;
    gint pos;

    gtk_tree_view_get_cursor (view, &path, NULL);

    if (!path)
        return -1;

    pathstr = gtk_tree_path_to_string (path);         
    pos = g_strtod (pathstr, NULL);
    gtk_tree_path_free (path);
    g_free (pathstr);
    return pos;
}

gboolean
gtk_tree_model_get_iter_from_int (GtkTreeModel *model,
        GtkTreeIter *iter, gint pos)
{
    GtkTreePath *path;
    gboolean ret;

    path = gtk_tree_path_new_from_indices (pos, -1);
    ret = gtk_tree_model_get_iter (model, iter, path);
    
    gtk_tree_path_free (path);
    return ret;
}

gchar *
pretty_stream_name (Stream *stream)
{
    gchar *name;
    const gchar *artist;
    const gchar *title;
    
    artist = stream_get_tag (stream, GST_TAG_ARTIST);
    title = stream_get_tag (stream, GST_TAG_TITLE);
    
    if (artist && title) {
        name = g_strdup_printf ("%s - %s", artist, title);
    } else {
        name = g_strdup (g_basename (stream->uri));
    }

    return name;
}

gchar *
escape_spaces (gchar *orig)
{
    GString *str;
    gchar *retval;
    gint ii;

    str = g_string_new ("");

    for(ii = 0; ii < strlen (orig); ii++) {
        if (orig[ii] == ' ')
            g_string_append (str, "%20");
        else
            g_string_append_c (str, orig[ii]);
    }

    retval = str->str;
    
    g_string_free (str, FALSE);

    return retval;
}

