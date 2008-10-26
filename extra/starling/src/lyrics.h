/* lyrics.h - Lyrics interface.
   Copyright (C) 2008 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro <skyhusker@handhelds.org>

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

#ifndef LYRICS_H
#define LYRICS_H

#include <glib/gtypes.h>
#include <gtk/gtktextview.h>
#include <libsoup/soup.h>
#include <stdbool.h>

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

void lyrics_display (const gchar *artist, const gchar *title,
		     GtkTextView *view,
		     bool try_to_download, bool force_download);

void lyrics_display_with_uri (const gchar *uri,
			      const char *artist, const char *title,
			      GtkTextView *view,
			      bool try_to_download,
			      bool force_download);

int lyrics_select (const gchar *uri, char **content);

void lyrics_store (const gchar *uri, const gchar *text);

gchar * lyrics_cook_uri (const gchar *artist, const gchar *title);

#endif
