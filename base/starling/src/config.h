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

#ifndef CONFIG_H
#define CONFIG_H

#include <glib/gtypes.h>

#include "starling.h"

#define CONFIGDIR ".gpe/starling"
#define CONFIG_PL "starling.m3u"
#define CONFIG_FILE "starlingrc"

void config_init (Starling *st);

void config_load (Starling *st);

void config_save (Starling *st);

void config_store_lastm (const gchar *user, const gchar *passwd, Starling *st);

#endif
