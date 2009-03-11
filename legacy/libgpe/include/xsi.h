/* xsi - adapted from xsingleinstance for use in gpe-appmgr

   Copyright 2002 Robert Mibus;
   xsingleinstance Copyright 2000 Merle F. McClelland

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifndef _XSI_H
#define _XSI_H

#include <X11/X.h>

int run_program (char *exec, char *name);

struct window_info
{
	Window xid;
	char *name;
};

void window_info_free (struct window_info *w);

GList *get_windows_with_name (char *name);
void raise_window_default(Window w);

#endif
