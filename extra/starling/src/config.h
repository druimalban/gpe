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

#ifndef _STARLING_CONFIG_H
#define _STARLING_CONFIG_H

#include <libintl.h>
#define _(String)   String
#define gettext_noop(String)    String
#define N_(String)  gettext_noop(String)


#define CONFIGDIR ".starling"

#endif
