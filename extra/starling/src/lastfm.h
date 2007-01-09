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

#ifndef LASTFM_H
#define LASTFM_H

#include <glib/gtypes.h>

#include "startling.h"

gboolean lastfm_init (Starling *st);

void lastfm_enqueue (const gchar *artist, const gchar *title, gint length, Starling *st);

void lastfm_submit (const gchar *username, const gchar *passwd, Starling *st);

gint lastfm_count (void);

#endif
