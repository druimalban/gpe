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
#include <stdio.h>  //remove(file)
#include <time.h>   //time --> filename

#include "files.h"
#include "gpe-sketchbook.h"
#include "sketchpad.h"

gchar * file_new_fullpath_filename(){
  gchar * fullpath_filename;

  GTimeVal current_time; //struct GTimeVal{  glong tv_sec;  glong tv_usec;};
  struct tm * formated_time;

  g_get_current_time(&current_time); //gettimeofday
  formated_time = localtime ((time_t *) &current_time);
  fullpath_filename = g_strdup_printf(
           "%s%04d-%02d-%02d_%02d-%02d-%02d.png",
           sketchbook.save_dir,
           formated_time->tm_year + 1900, //tm_year: number of years from 1900 (!)
           formated_time->tm_mon  + 1,    //tm_mon: range 0 to 11 (!)
           formated_time->tm_mday,
           formated_time->tm_hour,
           formated_time->tm_min,
           formated_time->tm_sec);
  return fullpath_filename;
}

gboolean file_delete(const gchar * fullpath_filename){
  int res;
  //**/g_printerr("deleting: %s\n", fullpath_filename);
  res = remove(fullpath_filename);
  if(res == 0) return TRUE;
  else return FALSE;
}

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
