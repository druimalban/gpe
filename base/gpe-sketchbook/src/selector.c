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
#include "gpe/gpeiconlistview.h"
#include "gpe/gpeiconlistitem.h"

#include "selector.h"
#include "selector-gui.h"
#include "files.h"
#include "sketchpad.h"
#include "gpe-sketchbook.h"
#include "db.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

#define DEBUG
#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...) 
#endif

Selector selector;

gint sketch_list_size;

gint current_sketch;
gboolean is_current_sketch_selected;

int _direntry_selector(const struct dirent * direntry);

void selector_init(){
  GtkListStore * store;
  
  store = gtk_list_store_new (NUM_ENTRIES,
                              /*0*/G_TYPE_STRING,  //title
                              /*1*/G_TYPE_INT,     //id
                              /*2*/G_TYPE_STRING,  //note's url
                              /*3*/GDK_TYPE_PIXBUF,//thumbnail
                              /*4*/G_TYPE_OBJECT   //GPEIconListItem
                              );
  selector.listmodel = GTK_TREE_MODEL(store);

  selector.thumbnails_notloaded = TRUE;

  sketch_list_size = 0;
  current_sketch   = SKETCH_NEW;

}//selector_init()


void selector_add_note(gint id, gchar * title, gchar * url, GdkPixbuf * thumbnail){
  GtkListStore * store;
  GtkTreeIter iter;

  store = GTK_LIST_STORE(selector.listmodel);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      ENTRY_ID,        id,
                      ENTRY_TITLE,     title,
                      ENTRY_URL,       url,
                      ENTRY_THUMBNAIL, thumbnail,
                      -1);
  g_free(url);  //liststore take a copy
  g_free(title);//liststore take a copy

  //gpe-iconlist is not linked to the model, so update it
  if(thumbnail){
    GObject * item;
    item = gpe_icon_list_view_add_item_pixbuf (GPE_ICON_LIST_VIEW(selector.iconlist),
                                         NULL, //title
                                         thumbnail,//icon
                                         gtk_tree_iter_copy(&iter));//udata
    gtk_list_store_set (store, &iter,
                        ENTRY_ICONLISTITEM, item,
                        -1);  
  }
}


void window_selector_init(GtkWidget * window_selector){

  if( mkdir(sketchbook.save_dir, S_IRWXU) == -1){
    if(errno != EEXIST){
      switch(errno){
        case EACCES: //write permission is denied
        case ENOSPC: //file system doesn't have enough room
        default:
          /* TRANSLATORS: %s are: [folder name] [error message]*/
          gpe_error_box_fmt(_("Cannot create %s: %s. Exit."), sketchbook.save_dir, strerror(errno));
          app_quit();
          return;
      }
    }
  }

  db_load_notes();

}//window_selector_init()

void load_thumbnails(){
  GtkTreeModel * model;
  GtkTreeIter iter;
  gboolean is_node;

  gchar     * filename;
  GdkPixbuf * pixbuf;
  GdkPixbuf * thumbnail;
  GObject   * item;
 

  model = GTK_TREE_MODEL(selector.listmodel);

  is_node = gtk_tree_model_get_iter_first(model, &iter);

  while (is_node) {
    gtk_tree_model_get(model, &iter, ENTRY_URL, &filename, -1);

    /**/g_printerr("loading %s...", filename);

    pixbuf    = gdk_pixbuf_new_from_file(filename, NULL); //GError **error
    thumbnail = gdk_pixbuf_scale_simple (pixbuf, THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                         GDK_INTERP_BILINEAR);
    gdk_pixbuf_unref(pixbuf);
    gtk_list_store_set (GTK_LIST_STORE(model), &iter, ENTRY_THUMBNAIL, thumbnail, -1);

    item = gpe_icon_list_view_add_item_pixbuf (GPE_ICON_LIST_VIEW(selector.iconlist),
                                         NULL, //title
                                         thumbnail,//icon
                                         gtk_tree_iter_copy(&iter));//udata
    gtk_list_store_set (GTK_LIST_STORE(model), &iter, ENTRY_ICONLISTITEM, item, -1);  

    while (gtk_events_pending ()) gtk_main_iteration ();

    /**/g_printerr("done.\n");

    is_node = gtk_tree_model_iter_next(model, &iter);
  }

  selector.thumbnails_notloaded = FALSE;
}

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
  gboolean is_deleted = FALSE;
  
  GtkTreeModel * model;
  GtkTreeIter iter;
  gboolean got_it;
  GtkTreePath * path;
  gchar * fullpath_filename;
  gint    id;

  model = GTK_TREE_MODEL(selector.listmodel);

  path = gtk_tree_path_new_from_indices (current_sketch, -1);

  got_it = gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_path_free(path);
  if(!got_it) return;

  gtk_tree_model_get(model, &iter,
                     ENTRY_URL, &fullpath_filename,
                     ENTRY_ID,  &id,
                     -1);
  
  //--Delete the selected sketch
  is_deleted = file_delete(fullpath_filename);

  if(is_deleted){
    GObject * iconlist_item;

    db_delete_note(id);

    gtk_tree_model_get(model, &iter,
                       ENTRY_ICONLISTITEM, &iconlist_item,
                       -1);

    gpe_icon_list_view_remove_item(GPE_ICON_LIST_VIEW(selector.iconlist), iconlist_item);

    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

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
  GtkTreeModel * model;
  GtkTreeIter iter;
  gboolean got_it;
  GtkTreePath * path;
  gchar * fullpath_filename;
  
  model = GTK_TREE_MODEL(selector.listmodel);

  path = gtk_tree_path_new_from_indices (index, -1);

  got_it = gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_path_free(path);
  if(!got_it) return;

  gtk_tree_model_get(model, &iter, ENTRY_URL, &fullpath_filename, -1);
  sketchpad_open_file(fullpath_filename);
  g_free(fullpath_filename);
}
