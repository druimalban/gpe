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
#include <stdio.h>  //perror
#include <stdlib.h> //free
#include <string.h> //tokens
#include <sys/stat.h> //mkdir
#include <sys/types.h>//mkdir
#include <asm/errno.h>//mkdir

#include "selector.h"
#include "files.h"
#include "sketchpad.h"
#include "gpe-sketchbook.h"
#include "_support.h" //lookup_widget

GtkWidget * window_selector;
GtkCList  * selector_clist; 
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
  int n;
  GdkColormap * colormap;

  //--alternate bg color for list items
  colormap = gdk_colormap_get_system();
  bg_color.red   = 65535;
  bg_color.green = 65535;
  bg_color.blue  = 40000;
  gdk_colormap_alloc_color(colormap, &bg_color, FALSE, TRUE);

  //--Clist init
  selector_clist = (GtkCList *) lookup_widget(window_selector, "clist_selector");
  gtk_clist_column_titles_hide(selector_clist);//no title (single column)

  //--fill CList
  n = scandir (sketchdir, &direntries, _direntry_selector, alphasort);//FIXME: --> file.c
  if (n == -1){
    int res = 0;
    perror ("Couldn't open the directory");
    //might not exist, try to create it:
    res = mkdir(sketchdir, S_IRWXU);
    switch(res){
      case EACCES: //write permission is denied
      case EEXIST: //already exists
      case ENOSPC: //file system doesn't have enough room
        g_printerr("Can not create %s, exit.\n", sketchdir);
        app_quit();
    }
  }
  else{
    gchar * fullpath_filename;
    gchar * line_text[1];
    int line;
    int i;
    
    for(i = 0; i < n; i++){
      line_text[0] = make_label_from_filename(direntries[i]->d_name);
      line         = gtk_clist_append(selector_clist, line_text);
      g_free(line_text[0]);
      fullpath_filename = g_strdup_printf("%s%s", sketchdir, direntries[i]->d_name);
      gtk_clist_set_row_data_full(selector_clist, line, fullpath_filename, g_free);
      //do NOT g_free(fullpath_filename) now! :)
      if(i%2) gtk_clist_set_background(selector_clist, i, &bg_color);
      sketch_list_size++;
    }
    free(direntries);
  }//else
}//window_selector_init()
  
void selector_refresh_list(){}

gchar * make_label_from_filename(const gchar * filename){
  //filename:  yyyy-mm-dd_hh-mm-ss.xpm[.gz]
  //label   : "yyyy mm dd  at  hh:mm:ss"
  gchar * label;

  char * s;
  char * date_token;
  char * time_token;

  s = g_basename((char *) g_strdup(filename));

  //NOTE: May use: g_strsplit()
  //gchar** g_strsplit(const gchar *string, const gchar *delimiter, gint max_tokens)
  //Returns :	a newly-allocated array of strings. Use g_strfreev() to free it. 
  date_token = strtok(s, "_");
  if(!date_token) return g_strdup(filename);

  time_token = strtok(NULL, ".");
  if(!time_token) return g_strdup_printf("%s", g_strdelimit(date_token, "-", ' '));

  label = g_strdup_printf("%s  at  %s",
                          g_strdelimit(date_token, "-", ' '),
                          g_strdelimit(time_token, "-", ':'));
  free(s);
  return label;
}//make_label_from_filename()

int _direntry_selector (const struct dirent * direntry){
  //struct dirent{ char d_name[]; ...};
  gchar * s;
  s = (gchar *) direntry->d_name; //WARNING: do not modify s!

  /* no hidden file */
  if(s[0] == '.') return 0;

  /* only .xpm[.gz] files */
  g_strreverse(s);
  if(g_strncasecmp(s,"mpx.", 4) && g_strncasecmp(s,"zg.mpx.", 7)) return 0;
  g_strreverse(s);

  /* yyyy-mm-dd_hh-mm-ss.xpm[.gz] */
  // ...

  return 1;
}

void delete_current_sketch(){
  gchar    * fullpath_filename;
  gboolean is_deleted = FALSE;
  
  fullpath_filename = gtk_clist_get_row_data(selector_clist, current_sketch);
  is_deleted = file_delete(fullpath_filename);

  if(is_deleted){
    _clist_update_alternate_colors_from(selector_clist, current_sketch);
    gtk_clist_remove(selector_clist, current_sketch);
    if(is_current_sketch_last) current_sketch--;
    sketch_list_size--;

    if(is_sketch_list_empty){
      current_sketch = SKETCH_NEW;
      sketchpad_new_sketch();//...
    }
    else{
      open_indexed_sketch(current_sketch);//...
    }

  }
}

void open_indexed_sketch(gint index){
  gchar * fullpath_filename;
  gchar * title;

  fullpath_filename = (gchar *) gtk_clist_get_row_data(selector_clist, index);
  gtk_clist_get_text(selector_clist, index, 0, &title);
  sketchpad_open_file(fullpath_filename , title);
}

void _clist_update_alternate_colors_from(GtkCList * clist, gint index){
  gint i;
  for(i = index; i < sketch_list_size; i++){
    if(i%2) gtk_clist_set_background(clist, i, &white);
    else    gtk_clist_set_background(clist, i, &bg_color);
  }
}
