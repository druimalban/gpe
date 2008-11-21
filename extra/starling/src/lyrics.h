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

#include <gtk/gtktextview.h>
#include <glib/gtypes.h>

gboolean lyrics_init (void);

void lyrics_display (const gchar *artist, const gchar *title,
		     GtkTextView *view,
		     bool try_to_download, bool force_download);

#endif
