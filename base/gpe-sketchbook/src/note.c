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

#include "note.h"
#include <gtk/gtk.h>
#include <stdlib.h> //malloc/free

Note * note_new(){
  Note * note;
  note = (Note *) malloc(sizeof(Note));
  note->fullpath_filename = NULL;
  note->thumbnail         = NULL;
  note->icon_widget       = NULL;
  return note;
}

void note_delete(Note * note){
  if(!note) return;
  if(note->fullpath_filename) free(note->fullpath_filename);
  if(note->thumbnail)   gdk_pixbuf_unref(note->thumbnail);
  if(note->icon_widget) gtk_widget_destroy(note->icon_widget);
  free(note);
}

void note_destroy(gpointer note){
  note_delete((Note *) note);
}

