/*
 * Copyright (C) 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef ABOUT_H
#define ABOUT_H

extern void gpe_about (gchar * app_name,
               gchar * app_version, //optional, may be NULL
               gchar * app_icon,
               gchar * app_short_description,
               gchar * minihelp_text,
               gchar * legal_text);
#endif
