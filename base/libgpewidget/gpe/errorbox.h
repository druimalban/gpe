/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef ERRORBOX_H
#define ERRORBOX_H

extern void gpe_error_box (char *text);
extern void gpe_perror_box (char *text);
extern void gpe_error_box_fmt (const char *format, ...);

#endif
