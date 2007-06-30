/*
* This file is part of GPE-Memo
*
* Copyright (C) 2007 Alberto García Hierro
*	<skyhusker@rm-fr.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <glib/gtypes.h>

gchar * settings_get_memo_dir (void);

void settings_set_memo_dir (const gchar *dirname);

#endif
