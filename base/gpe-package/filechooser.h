/*
 * Copyright (C) 2004  Nils Faerber <nils.faerber@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef _FILECHOOSER_H
#define _FILECHOOSER_H

int package_choose(GtkWidget *, void (*File_Selected) (char *filename, gpointer data));

#endif
