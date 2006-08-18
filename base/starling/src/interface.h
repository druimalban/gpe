/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include "starling.h"

enum { 
    COL_TITLE,
    /* COL_TIME, */
    COL_FONT_WEIGHT,
    COL_POINTER,
    COL_NUMCOLS
};

void player_init (Starling *st);

void interface_init (Starling *st);

#endif
