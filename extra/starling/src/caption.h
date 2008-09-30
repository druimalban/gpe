/* caption.h - Generate captions.
   Copyright (C) 2008 Neal H. Walfield <neal@walfield.org>

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

#include "musicdb.h"

struct caption;

/* %a - artist
   %A - album
   %t - title
   %T - track
   %g - genre
   %r - rating
   %d - duration
   %c - play count
   %[width][.][precision]c - width and precision.

  Must be freed with caption_free.  */
struct caption *caption_create (const char *fmt);

/* Free CAPTION.  */
void caption_free (struct caption *caption);

/* Create a caption for the track with uid UID.  Caller is responsible
   for freeing it.  */
char *caption_render (struct caption *caption, MusicDB *db, int uid);
