/*
 * This file is part of Starling
 *
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef LYRICS_H
#define LYRICS_H

#include <glib/gtypes.h>
#include <gtk/gtktextview.h>
#include <libsoup/soup.h>

struct _Provider
{
    gchar * (*cook) (const gchar *, const gchar *);
    gchar * (*parse) (SoupMessage *);
};

typedef struct _Provider Provider;

typedef enum {
    PROVIDER_LYRCAR,
    PROVIDER_LYRICWIKI,
    PROVIDER_INVALID
} provider_t;

gboolean lyrics_init (void);

void lyrics_set_provider (provider_t prov);

provider_t lyrics_get_provider (void);

void lyrics_display (const gchar *artist, const gchar *title, GtkTextView *view);

void lyrics_display_with_uri (const gchar *uri, GtkTextView *view);

gchar * lyrics_select (const gchar *uri);

void lyrics_store (const gchar *uri, const gchar *text);

gchar * lyrics_cook_uri (const gchar *artist, const gchar *title);

#endif
