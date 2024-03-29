/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef FILES_H
#define FILES_H

#include <gtk/gtk.h>

gchar *  file_new_fullpath_filename();
gboolean file_delete(const gchar * fullpath_filename);

#define  file_save file_save_png
#define  file_load file_load_png

gint file_save_png(const gchar * fullpath_filename);
gint file_load_png(const gchar * fullpath_filename);

#endif
