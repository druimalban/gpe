/* lastfm.h - lastfm interface.
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

#ifndef LASTFM_H
#define LASTFM_H

#include <glib/gtypes.h>
#include <gtk/gtklabel.h>

/* If auto_submit is not zero, then when at least AUTO_SUBMIT tracks
   are in the queue, they will automatically be submitted.  Otherwise,
   submissions will only occur when triggered by a call to
   lastfm_submit.  */
extern gboolean lastfm_init (const char *user, const char *password,
			     int auto_submit, GtkLabel *status);

/* Enqueue a track for submission.  */
void lastfm_enqueue (const gchar *artist, const gchar *title, gint length);

/* Cause any pending tracks to be submitted now.  */
void lastfm_submit (void);

void lastfm_user_data_set (const char *user, const char *password,
			   int auto_submit);

#endif
