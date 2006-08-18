/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef UTILS_H
#define UTILS_H

#include <glib/gtypes.h>

#include <gtk/gtktreeview.h>

#include "stream.h"

gint gtk_tree_view_get_position (GtkTreeView *view);

gboolean gtk_tree_model_get_iter_from_int (GtkTreeModel *model,
        GtkTreeIter *iter, gint pos);

gchar * pretty_stream_name (Stream *stream);

gchar *escape_spaces (gchar *orig);

#endif
