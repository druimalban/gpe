/* gpe-go, a GO board for GPE
 *
 * $Id$
 *
 * Copyright (C) 2003-2004 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef GPE_GO_H
#define GPE_GO_H

#include <gtk/gtk.h>

void status_update_fmt(const char * format, ...);
void status_update_current();
void update_capture_label();

#endif
