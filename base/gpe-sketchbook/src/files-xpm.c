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
#include <gdk/gdkx.h> //wraping GDK to X
#include <X11/xpm.h>  //XPM read/write

#include "files.h"
#include "files-xpm.h"
#include "gpe-sketchbook.h"
#include "sketchpad.h"

void file_save_xpm(GtkWidget *_drawing_area, const gchar * fullpath_filename){

  GdkImage    * image;
  GdkColormap * colormap;

  XpmAttributes xpm_attributes;
  int res;

  image  = gdk_image_get(drawing_area_pixmap_buffer,
                         0, 0,
                         drawing_area_width,
                         drawing_area_height);

  colormap = gdk_window_get_colormap (_drawing_area->window);

  xpm_attributes.valuemask = (XpmSize | XpmColormap | XpmDepth);
  xpm_attributes.colormap  = GDK_COLORMAP_XCOLORMAP (colormap);
  xpm_attributes.depth     = image->depth;

  res = XpmWriteFileFromImage(
           GDK_WINDOW_XDISPLAY (_drawing_area->window), //Display * xdisplay,
           (char *) fullpath_filename, //char   *filename,
           GDK_IMAGE_XIMAGE(image),    //XImage *image,
           NULL,                       //XImage *shapeimage,
           &xpm_attributes);           //XpmAttributes *attributes);

  if (res != XpmSuccess){
    //FIXME: do something!
  }
  gdk_image_destroy(image);
}//file_save_xpm()

void file_load_xpm(GtkWidget * _drawing_area, const gchar * fullpath_filename){
  
  //FIXME: use libXpm to handle .xpm.gz files!
  drawing_area_pixmap_buffer = gdk_pixmap_create_from_xpm(_drawing_area->window,
                                                          NULL, NULL,
                                                          fullpath_filename);
  //FIXME: update the drawing area size
  //drawing_area_width  = ;
  //drawing_area_height = ;

  sketchpad_refresh_drawing_area(_drawing_area);
}//file_load_xpm()

