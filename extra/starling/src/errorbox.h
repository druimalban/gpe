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

#ifndef ERRORBOX_H
#define ERRORBOX_H

#include <glib/gtypes.h>

void starling_error_box (const gchar *text);

void starling_error_box_fmt (const gchar *fmt, ...);

#endif
