/* starling.h - Starling interface.
   Copyright 2008 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro
        <skyhusker@handhelds.org>

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

#ifndef STARLING_H
#define STARLING_H

#include <glib-object.h>
#include <stdbool.h>

typedef struct _Starling Starling;

/** starling_new:

  Create a new starling.  */
extern Starling *starling_run (void);

/* Return whether random mode is enabled.  */
extern bool starling_random (Starling *st);

/* Set whether random mode is enabled to VALUE.  */
extern void starling_random_set (Starling *st, bool enabled);

/* Start playing.  */
extern gboolean starling_play (Starling *st);

/* Go to the next track.  */
extern void starling_next (Starling *st);

/* Go to the previos track.  */
extern void starling_prev (Starling *st);

/* Scroll the main view to the currently playing track.  */
extern void starling_scroll_to_playing (Starling *st);

/* Set the audio sink.  */
extern void starling_set_sink (Starling *st, char *sink);

#endif
