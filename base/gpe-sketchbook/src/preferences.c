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

//--GPE libs
#include <gpe/picturebutton.h>
#include <gpe/spacing.h>

//--own headers
#include "gpe-sketchbook.h"
#include "preferences.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext (_x)

/********************************************************/
/********************************************************/

#include <glib.h>
#include <sqlite3.h> //backend
#include <stdlib.h> //free() atoi() atof()
#include <unistd.h> //access

#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...) 
#endif


sqlite3 * prefs_db = NULL;

static const char * db_schema = "CREATE TABLE prefs ("
                                " key   VARCHAR(30) PRIMARY KEY,"
                                " type  INTEGER,"/* unused except for DB export */
                                " value ANYTYPE" /* SQLite is typeless */
                                ");";

GpePrefsResult gpe_prefs_init(gchar * prog_name){
  gchar * db_name;
  gint    error;
  gchar * errmsg;
  gint mode = 0;// rw/r mode, ignored by sqlite (2.8.5)
  gint dbexists;

  //--Open DB
  db_name = g_strdup_printf("%s/.gpe/prefs_%s.db", g_get_home_dir(), prog_name);

  dbexists = !access(db_name, F_OK);
	
  TRACE("Opening: %s", db_name);
  sqlite3_open(db_name, &prefs_db);
  g_free(db_name);
  if( prefs_db == NULL ){
    TRACE("ERROR> Can't open database: %s", errmsg);
    return GPE_PREFS_ERROR;
  }

  //--Create table if necessary, we assume table exists if database is present
  
  if (!dbexists){
    error = sqlite3_exec (prefs_db, db_schema, NULL, NULL, &errmsg);
    TRACE("%s", db_schema);
    if(error){
	  g_printerr("ERROR> #%d %s\n", error, errmsg);
	  free (errmsg);
 	  return GPE_PREFS_ERROR;
    }
  }
  return GPE_PREFS_OK;
}

void gpe_prefs_exit(){
  if(prefs_db) sqlite3_close(prefs_db);
}

int _get_single_value(void * pvalue, int argc, char **argv, char **columnNames){
  if(argc != 1){
    TRACE("ERROR> more than one value");
    return 1;
  }

  * (gchar **) pvalue = g_strdup(*argv);
  return 0;
}

GpePrefsResult gpe_prefs_get (gchar * key, GType type, gpointer pvalue){
  int    result;
  char * errmsg;
  gchar * _pvalue = NULL, *sql;

  if(!prefs_db) return GPE_PREFS_ERROR;
  
  sql = g_strdup_printf("SELECT value FROM prefs WHERE key='%q'", key);
  result = sqlite3_exec (prefs_db, (const char *) &sql,
                         _get_single_value , &_pvalue, &errmsg);
  g_free(sql);
  if(result != SQLITE_OK){
    TRACE("ERROR> #%d : %s", result, errmsg);
    free(errmsg);
    return GPE_PREFS_ERROR;
  }
  
  /* if setting isn't present keep default */
  if(_pvalue == NULL) 
	  return GPE_PREFS_OK;
  //TRACE("GOT   > '%s'",  _pvalue);

  switch(type){
      case G_TYPE_INT:
        *(gint *) pvalue = atoi(_pvalue);
        TRACE("select> %s = %d", key, * (gint *)pvalue);
        break;
      case G_TYPE_FLOAT:
        *(gfloat *) pvalue = atof(_pvalue);
        TRACE("select> %s = %g", key, * (gfloat *)pvalue);
        break;
      case G_TYPE_STRING:
        *(gchar **) pvalue = _pvalue;
        TRACE("select> %s = %s", key, * (gchar **)pvalue);
        break;
      default:
        //ERROR: Unhandled type
        break;
    }

  if(type != G_TYPE_STRING) g_free(_pvalue);

  return GPE_PREFS_OK;
}

int _key_exists(void * pbool, int argc, char **argv, char **columnNames){
  if(argc == 1) * (gboolean *) pbool = TRUE;
  else          * (gboolean *) pbool = FALSE;
  return 0;
}

GpePrefsResult gpe_prefs_set (gchar * key, GType type, gconstpointer pvalue){
  gboolean exists = FALSE;
  gint    result;
  gchar * errmsg, *sql;

  if(!prefs_db) return GPE_PREFS_ERROR;
  
  sql = g_strdup_printf("SELECT key FROM prefs WHERE key='%q'", key);
  result = sqlite3_exec (prefs_db, (const char *) &sql,_key_exists , &exists, &errmsg);
  g_free(sql);

  if(exists){
    switch(type){
      case G_TYPE_INT:
	sql = g_strdup_printf("UPDATE prefs SET value='%d' WHERE key='%q'", *(gint *)pvalue, key);
        result = sqlite3_exec (prefs_db, (const char *) &sql,
                               NULL, NULL, &errmsg);
        g_free(sql);
        TRACE("update> %s = %d", key,*(gint *)pvalue);
        break;
      case G_TYPE_FLOAT:
	sql = g_strdup_printf("UPDATE prefs SET value='%g' WHERE key='%q'", key, *(gfloat *)pvalue);
        result = sqlite3_exec (prefs_db, (const char *)&sql,
                               NULL, NULL, &errmsg);
        g_free(sql);
        TRACE("update> %s = %g", key,*(gfloat *)pvalue);
        break;
      case G_TYPE_STRING:
	sql = g_strdup_printf("UPDATE prefs SET value='%q' WHERE key='%q'", *(gchar **)pvalue, key);
        result = sqlite3_exec (prefs_db, (const char *)&sql,
                               NULL, NULL, &errmsg);
        g_free(sql);
        TRACE("update> %s = %s", key,*(gchar **)pvalue);
        break;
      default:
        //ERROR: Unhandled type
        break;
    }
  }
  else{
    switch(type){
      case G_TYPE_INT:
	sql = g_strdup_printf("INSERT INTO prefs VALUES('%q', %d, %d)", key, type, * (gint *)pvalue);
        result = sqlite3_exec (prefs_db, (const char *) &sql,
                               NULL, NULL, &errmsg);
        g_free(sql);
        TRACE("insert> %s = %d", key,*(gint *)pvalue);
        break;
      case G_TYPE_FLOAT:
	sql = g_strdup_printf("INSERT INTO prefs VALUES('%q', %d, %g)",key, type, * (gfloat *)pvalue);
        result = sqlite3_exec (prefs_db, (const char *) &sql,
                               NULL, NULL, &errmsg);
        g_free(sql);
        TRACE("insert> %s = %g", key,*(gfloat *)pvalue);
        break;
      case G_TYPE_STRING:
	sql = g_strdup_printf("INSERT INTO prefs VALUES('%q', %d, '%q')", key, type, * (gchar **)pvalue);
        result = sqlite3_exec (prefs_db, (const char *) &sql,
                               NULL, NULL, &errmsg);
        g_free(sql);
        TRACE("insert> %s = %s", key,*(gchar **)pvalue);
        break;
      default:
        //ERROR: Unhandled type
        break;
    } 
  }

  if(result != SQLITE_OK){
    TRACE("ERROR> #%d : %s", result, errmsg);
    free(errmsg);
    return GPE_PREFS_ERROR;
  }

  return GPE_PREFS_OK;
}


/********************************************************/
/********************************************************/

void prefs_reset_defaults(){
  sketchbook.prefs.joypad_scroll  = TRUE;
  sketchbook.prefs.grow_on_scroll = TRUE;
  sketchbook.prefs.start_with     = PAGE_SELECTOR_LIST;
}

#ifdef DEBUG
void _print_prefs(){
  g_printerr("Prefs: joypad_scroll  [%s]\n", sketchbook.prefs.joypad_scroll?"on":"off");
  g_printerr("Prefs: grow_on_scroll [%s]\n", sketchbook.prefs.grow_on_scroll?"on":"off");
  g_printerr("Prefs: start_with     [%d]\n", sketchbook.prefs.start_with);
}
#endif /* DEBUG */

void prefs_fetch_settings(){
   GpePrefsResult res;
   res = gpe_prefs_get ("joypad-scroll",  G_TYPE_INT, &(sketchbook.prefs.joypad_scroll));
   res = gpe_prefs_get ("grow-on-scroll", G_TYPE_INT, &(sketchbook.prefs.grow_on_scroll));
   res = gpe_prefs_get ("start-with",     G_TYPE_INT, &(sketchbook.prefs.start_with));
}


void prefs_save_settings(){
   GpePrefsResult res;
   res = gpe_prefs_set ("joypad-scroll",  G_TYPE_INT, &(sketchbook.prefs.joypad_scroll));
   res = gpe_prefs_set ("grow-on-scroll", G_TYPE_INT, &(sketchbook.prefs.grow_on_scroll));
   res = gpe_prefs_set ("start-with",     G_TYPE_INT, &(sketchbook.prefs.start_with));
}

//-----------------------------------------------------------------------------


struct {
  GtkWidget * joypad_scroll;
  GtkWidget * grow_on_scroll;
  GtkWidget * start_with;
  gint        start_with_index;
} _prefs_ui;

void reset_gui_from_prefs(){
  // struct --> ui
  switch(sketchbook.prefs.start_with){
  //WARNING: MUST match gtk_menu_append() order!
  case PAGE_SKETCHPAD:           _prefs_ui.start_with_index = 0; break;
  case PAGE_SELECTOR_LIST:       _prefs_ui.start_with_index = 1; break;
  case PAGE_SELECTOR_ICON_TABLE: _prefs_ui.start_with_index = 2; break;
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(_prefs_ui.start_with), _prefs_ui.start_with_index);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_prefs_ui.joypad_scroll), sketchbook.prefs.joypad_scroll);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_prefs_ui.grow_on_scroll), sketchbook.prefs.grow_on_scroll);
}

void reset_prefs_from_gui(){
  // ui -> struct
  switch(gtk_option_menu_get_history(GTK_OPTION_MENU(_prefs_ui.start_with))){
  //WARNING: MUST match gtk_menu_append() order!
  case 0: sketchbook.prefs.start_with = PAGE_SKETCHPAD;           break;
  case 1: sketchbook.prefs.start_with = PAGE_SELECTOR_LIST;       break;
  case 2: sketchbook.prefs.start_with = PAGE_SELECTOR_ICON_TABLE; break;
  }

  sketchbook.prefs.joypad_scroll = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_prefs_ui.joypad_scroll));
  sketchbook.prefs.grow_on_scroll = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_prefs_ui.grow_on_scroll));
}

void on_button_ok_clicked (GtkToolButton *button, gpointer _unused){
  reset_prefs_from_gui();
  //**/_print_prefs();
  gtk_notebook_set_page(sketchbook.notebook, PAGE_SELECTOR);
}
void on_button_no_clicked (GtkToolButton *button, gpointer _unused){
  reset_gui_from_prefs();
  //**/_print_prefs();
  gtk_notebook_set_page(sketchbook.notebook, PAGE_SELECTOR);
}


GtkWidget * preferences_gui(GtkWidget * window){
  GtkWidget * vbox;
  GtkWidget * separator;
  GtkWidget * hbox;
  GtkWidget * button_no;
  GtkWidget * button_ok;
  gint spacing = gpe_get_boxspacing();

  vbox = gtk_vbox_new(FALSE, spacing);

  //--Preferences

  {//start with
    GtkWidget * label;
    GtkWidget * option_menu;
    GtkWidget * menu;
    GtkWidget * menu_item;
    GtkWidget * hbox;
	  
    label = gtk_label_new(_("Start with"));

    menu = gtk_menu_new();
    menu_item = gtk_menu_item_new_with_label(_("Sketchpad"));
    gtk_menu_append(GTK_MENU(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label(_("Selector - list"));
    gtk_menu_append(GTK_MENU(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label(_("Selector - icons table"));
    gtk_menu_append(GTK_MENU(menu), menu_item);

    option_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    
    hbox = gtk_hbox_new(FALSE, spacing);
    gtk_box_pack_start(GTK_BOX(hbox), label,        FALSE, FALSE, spacing);
    gtk_box_pack_start(GTK_BOX(hbox), option_menu,  TRUE, TRUE, spacing);
    gtk_box_pack_start(GTK_BOX(vbox), hbox,  FALSE, FALSE, spacing);

    _prefs_ui.start_with = option_menu;
  }

  {//joypad scroll
    GtkWidget * check;
    check = gtk_check_button_new_with_label(_("Use joypad scrolling"));
    gtk_box_pack_start(GTK_BOX(vbox), check,      FALSE, FALSE, spacing);
    _prefs_ui.joypad_scroll = check;
  }

  {//grow on scroll
    GtkWidget * check;
    check = gtk_check_button_new_with_label(_("Grow on scroll"));
    gtk_box_pack_start(GTK_BOX(vbox), check,      FALSE, FALSE, spacing);
    _prefs_ui.grow_on_scroll = check;
  }

  //--init gui
  reset_gui_from_prefs();

  //--Buttons
  button_ok = gpe_picture_button(window->style , _("OK")    , "!gtk-ok");
  button_no = gpe_picture_button(window->style , _("Cancel"), "!gtk-cancel");
  g_signal_connect (G_OBJECT (button_ok), "clicked", G_CALLBACK (on_button_ok_clicked), NULL);
  g_signal_connect (G_OBJECT (button_no), "clicked", G_CALLBACK (on_button_no_clicked), NULL);

  hbox = gtk_hbox_new(TRUE, spacing);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), gpe_get_border());
  gtk_box_pack_start (GTK_BOX (hbox), button_no, TRUE, TRUE, spacing);
  gtk_box_pack_start (GTK_BOX (hbox), button_ok, TRUE, TRUE, spacing);

  separator = gtk_hseparator_new();
  gtk_box_pack_end(GTK_BOX(vbox), hbox,      FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(vbox), separator, FALSE, TRUE, 0);

  gtk_widget_show_all(vbox);
  return vbox;
}
