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
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <png.h>

#include "files.h"
#include "files-png.h"

#include "gpe-sketchbook.h"
#include "sketchpad.h"

gint file_save_png(const gchar * fullpath_filename){
    GdkPixbuf   * pixbuf;
    gboolean success;

    //--retrieve image data
    pixbuf = sketchpad_get_current_sketch_pixbuf();
    success = gdk_pixbuf_save (pixbuf, fullpath_filename, "png", NULL /*&error*/, NULL);
    if(!success){
      return 0;
    }

    gdk_pixbuf_unref(pixbuf);

    return 1;
}//file_save_png()

gint file_load_png(const gchar * fullpath_filename){
  GdkPixbuf * pixbuf = NULL;

  pixbuf = gdk_pixbuf_new_from_file(fullpath_filename, NULL); //GError **error
  if(pixbuf == NULL){
    return 0;
  }

  sketchpad_set_current_sketch_from_pixbuf(pixbuf);
  gdk_pixbuf_unref(pixbuf);

  return 1; 
}//file_load_png()


