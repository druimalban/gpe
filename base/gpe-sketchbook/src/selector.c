/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002, 2003, 2004, 2005 Luc Pionchon
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

#include <errno.h>  //errors
#include <stdio.h>  //perror
#include <stdlib.h> //free
#include <string.h> //tokens
#include <sys/stat.h> //mkdir
#include <sys/types.h>//mkdir
#include <asm/errno.h>//mkdir
#include <time.h>   //localtime

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

void selector_init(){
  GtkListStore * store;
  
  store = gtk_list_store_new (NUM_ENTRIES,
                              /*0*/G_TYPE_STRING,  //title
                              /*1*/G_TYPE_INT,     //id
                              /*2*/G_TYPE_INT,     //creation timestamp
                              /*3*/G_TYPE_STRING,  //creation timestamp label
                              /*4*/G_TYPE_INT,     //update timestamp
                              /*5*/G_TYPE_STRING,  //update timestamp label
                              /*6*/G_TYPE_STRING,  //note's url
                              /*7*/GDK_TYPE_PIXBUF,//thumbnail
                              /*8*/G_TYPE_OBJECT   //GPEIconListItem
                              );
  selector.listmodel = GTK_TREE_MODEL(store);

  selector.thumbnails_notloaded = TRUE;

  sketch_list_size = 0;
  current_sketch   = SKETCH_NEW;

}//selector_init()


void selector_add_note(gint id,
                       gchar * title,
                       gchar * url,
                       gint created,
                       gint updated,
                       GdkPixbuf * thumbnail){
  GtkListStore * store;
  GtkTreeIter iter;
  gchar *created_label, *updated_label;

  created_label = get_time_label(created);
  updated_label = get_time_label(updated);
  
  store = GTK_LIST_STORE(selector.listmodel);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      ENTRY_ID,          id,
                      ENTRY_TITLE,       title,
                      ENTRY_CREATED,     created_label,
                      ENTRY_CREATED_VAL, created,
                      ENTRY_UPDATED,     updated_label,
                      ENTRY_UPDATED_VAL, updated,
                      ENTRY_URL,         url,
                      ENTRY_THUMBNAIL,   thumbnail,
                      -1);

  //free items, liststore takes a copy
  g_free(url);  
  g_free(title);
  g_free(created_label);
  g_free(updated_label);

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

/** Formats a timestamp on 5 characters.
    The time displayed is relative to the current time.

    hh:mm    17:53   -> today
    dd/mm    18/01   -> this year
     YYYY     2003   -> previous year
    xxxxx    xxxxx   -> future date  (litteral "xxxxx")

    Returned string is newly allocated, must be freed.
*/
gchar * get_time_label(glong timestamp){
  gchar * s;
  GTimeVal current_time; //struct GTimeVal{  glong tv_sec;  glong tv_usec;};
  GTimeVal stamped_time; //struct GTimeVal{  glong tv_sec;  glong tv_usec;};
  struct tm * formated_current;
  struct tm * formated_stamped;
  struct tm   aformated_stamped;

  g_get_current_time(&current_time);
  stamped_time.tv_sec = timestamp;
  
  formated_current = localtime ((time_t *) &current_time);
  localtime_r((time_t *) &stamped_time, &aformated_stamped);
  formated_stamped = &aformated_stamped;

  if(stamped_time.tv_sec > current_time.tv_sec){
    s = g_strdup_printf("xxxxx"); //future
  }
  else if(formated_stamped->tm_year < formated_current->tm_year){
    s = g_strdup_printf(" %4d", formated_stamped->tm_year +1900);
  }
  else if(formated_stamped->tm_mon  <= formated_current->tm_mon &&
          formated_stamped->tm_mday <  formated_current->tm_mday){
    s = g_strdup_printf("%02d/%02d",
                        formated_stamped->tm_mday,
                        formated_stamped->tm_mon+1); //FIXME: needs localization !!!
  }
  else{
    s = g_strdup_printf("%02d:%02d",
                        formated_stamped->tm_hour,
                        formated_stamped->tm_min);
  }

  return s;
}
