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

#include <dirent.h> //filesystem acces
#include <errno.h>  //errors
#include <stdio.h>  //perror
#include <stdlib.h> //free
#include <string.h> //tokens
#include <sys/stat.h> //mkdir
#include <sys/types.h>//mkdir
#include <asm/errno.h>//mkdir
#include <stdlib.h>   //system()

#include "gpe/errorbox.h"
#include "gpe/gpe-iconlist.h"

#include "selector.h"
#include "selector-gui.h"
#include "note.h"
#include "files.h"
#include "sketchpad.h"
#include "gpe-sketchbook.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

Selector selector;

GdkColor bg_color;//alternate color for list cells

gint sketch_list_size;
gint current_sketch;
gboolean is_current_sketch_selected;

int _direntry_selector(const struct dirent * direntry);
void _clist_update_alternate_colors_from(GtkCList * clist, gint index);

void selector_init(){
  sketch_list_size = 0;
  current_sketch   = SKETCH_NEW;
}//selector_init()

void window_selector_init(GtkWidget * window_selector){
  struct dirent ** direntries;
  int scandir_nb_entries;
  int scandir_errno;
  GdkColormap * colormap;

  //--alternate bg color for list items
  colormap = gdk_colormap_get_system();
  bg_color.red   = 65535;
  bg_color.green = 65535;
  bg_color.blue  = 40000;
  gdk_colormap_alloc_color(colormap, &bg_color, FALSE, TRUE);

  //--Clist init
  gtk_clist_column_titles_hide(GTK_CLIST(selector.textlistview));//no title (single column)

  //--fill CList
  scandir_nb_entries = scandir (sketchbook.save_dir, &direntries, _direntry_selector, alphasort);//FIXME: --> file.c
  scandir_errno = errno;
  if (scandir_nb_entries == -1){//scandir error
    //might not exist, try to create it:
    if( mkdir(sketchbook.save_dir, S_IRWXU) == -1){
      switch(errno){
        case EEXIST: //already exists
          gpe_error_box_fmt(_("Cannot read the sketchbook directory: %s. Exit."),
                            strerror(scandir_errno));
          break;
        case EACCES: //write permission is denied
        case ENOSPC: //file system doesn't have enough room
        default:
          /* TRANSLATORS: %s are: [folder name] [error message]*/
          gpe_error_box_fmt(_("Cannot create %s: %s. Exit."), sketchbook.save_dir, strerror(errno));
      }
      app_quit();
    }
    {//then insert a dummy sketch (so the list does not appear empty)
      gchar * command;
      command = g_strdup_printf("cp -f %s/share/gpe/pixmaps/default/gpe-sketchbook/welcome.png %s/2003-00-00_00-00-00.png", PREFIX, sketchbook.save_dir);
      system (command);
      g_free(command);
      scandir_nb_entries = scandir (sketchbook.save_dir,
                                    &direntries,
                                    _direntry_selector,
                                    alphasort);//FIXME: --> file.c
      /**/g_printerr("inserted %d\n", scandir_nb_entries);
    }
  }
  /*else*/{ //if empty, added a dummy sketch so do it in any case
    gchar * fullpath_filename;
    gchar * line_text[1];
    int line;
    int i;
    
    for(i = 0; i < scandir_nb_entries; i++){
      Note * note;

      line_text[0] = make_label_from_filename(direntries[i]->d_name);
      fullpath_filename = g_strdup_printf("%s%s", sketchbook.save_dir, direntries[i]->d_name);

      line = gtk_clist_append(GTK_CLIST(selector.textlistview), line_text);
      note = note_new();
      note->fullpath_filename = fullpath_filename;

      {//--update thumbnail
        GdkPixbuf * pixbuf;
        GdkPixbuf * pixbuf_scaled;
        pixbuf = gdk_pixbuf_new_from_file(note->fullpath_filename, NULL); //GError **error
        pixbuf_scaled = gdk_pixbuf_scale_simple (pixbuf, //FIXME: let iconlist do that!
                                                 THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                                 GDK_INTERP_BILINEAR);
        gdk_pixbuf_unref(pixbuf);
        note->thumbnail = pixbuf_scaled;
      }
      gpe_iconlist_add_item_pixbuf (GPE_ICONLIST(selector.iconlist),
                                    NULL,//"Sketch",
                                    note->thumbnail,//icon
                                    note);//udata
      gtk_clist_set_row_data_full(GTK_CLIST(selector.textlistview), line, note, note_destroy);
      //do NOT g_free(fullpath_filename) now! :)
      if(i%2) gtk_clist_set_background(GTK_CLIST(selector.textlistview), i, &bg_color);
      sketch_list_size++;

      g_free(line_text[0]);
    }

    free(direntries);
  }//else
}//window_selector_init()
  
void selector_refresh_list(){}

gchar * make_label_from_filename(const gchar * filename){
  //filename:  yyyy-mm-dd_hh-mm-ss.png
  //label   : "yyyy mm dd  at  hh:mm:ss"
  gchar * label;

  G_CONST_RETURN gchar * file_basename_ref;
  gchar * file_basename;
  char * date_token;
  char * time_token;

  file_basename_ref = g_basename(filename);
  file_basename     = g_strdup(file_basename_ref);

  //NOTE: May use: g_strsplit()
  //gchar** g_strsplit(const gchar *string, const gchar *delimiter, gint max_tokens)
  //Returns :	a newly-allocated array of strings. Use g_strfreev() to free it. 
  date_token = strtok(file_basename, "_");
  if(!date_token) return g_strdup(filename);

  time_token = strtok(NULL, ".");
  if(!time_token) return g_strdup_printf("%s", g_strdelimit(date_token, "-", ' '));

  /* TRANSLATORS: default sketch label based on [date] and [time]
     example: "2003 10 12  at  18:48:51" */
  label = g_strdup_printf(_("%s  at  %s"),
                          g_strdelimit(date_token, "-", ' '), //FIXME: localize date/time
                          g_strdelimit(time_token, "-", ':'));
  free(file_basename);
  return label;
}//make_label_from_filename()

int _direntry_selector (const struct dirent * direntry){
  //struct dirent{ char d_name[]; ...};
  gchar * s;
  s = (gchar *) direntry->d_name; //WARNING: do not modify s!

  /* no hidden file */
  if(s[0] == '.') return 0;

  /* only .png files */
  g_strreverse(s);
  if(g_strncasecmp(s,"gnp.", 4)) return 0;
  g_strreverse(s);

  /* yyyy-mm-dd_hh-mm-ss */
  // ...

  return 1;
}

void delete_current_sketch(){
  Note * note;
  gboolean is_deleted = FALSE;
  
  note = gtk_clist_get_row_data(GTK_CLIST(selector.textlistview), current_sketch);
  is_deleted = file_delete(note->fullpath_filename);

  if(is_deleted){
    gpe_iconlist_remove_item_with_udata(GPE_ICONLIST(selector.iconlist), note);

    _clist_update_alternate_colors_from(GTK_CLIST(selector.textlistview), current_sketch);
    gtk_clist_remove(GTK_CLIST(selector.textlistview), current_sketch);

    if(is_current_sketch_last) current_sketch--;
    sketch_list_size--;

    if(is_sketch_list_empty){
      current_sketch = SKETCH_NEW;
      sketchpad_new_sketch();//... //FIXME: only with PAGE_SKETCHPAD
    }
    else{
      open_indexed_sketch(current_sketch);//... //FIXME: only with PAGE_SKETCHPAD
    }

  }
}

void open_indexed_sketch(gint index){
  Note * note;
  note = gtk_clist_get_row_data(GTK_CLIST(selector.textlistview), index);
  sketchpad_open_file(note->fullpath_filename);
}

void _clist_update_alternate_colors_from(GtkCList * clist, gint index){
  gint i;
  for(i = index; i < sketch_list_size; i++){
    if(i%2) gtk_clist_set_background(clist, i, &white);
    else    gtk_clist_set_background(clist, i, &bg_color);
  }
}
