/* gpe-sketchbook -- a notebook to sketch your notes
 *
 * Copyright (C) 2002, 2003, 2004, 2005 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <glib.h>
#include <sqlite.h>
#include <stdlib.h> //free()
#include <unistd.h> //access()

#include <gpe/errorbox.h>

#include "db.h"
#include "selector.h"

#define DEBUG
#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...) 
#endif


#define DB_NAME "sketchbook.db"
#define DB_VERSION "0.1" //FIXME: use it

static const gchar *db_schema_meta  = "CREATE TABLE metadata (" //FIXME: use it
                                      " key   VARCHAR(30) PRIMARY KEY,"
                                      " value TEXT"
                                      ");";

static const gchar *db_schema_notes = "CREATE TABLE notes ("
                                      " type    INTEGER,"
                                      " title   TEXT,"
                                      " created INTEGER,"
                                      " updated INTEGER,"
                                      " content TEXT" //url...
                                      ");";

static sqlite *db;

static void report_error(const int errno, gchar * errmsg){
    TRACE("ERROR> #%d : %s", errno, errmsg);
    //gpe_error_box("SQL error #%d: %s", );
    free(errmsg);
}

static int load_note_cb(void *pArg, int argc, char **argv, char **columnNames){
//  gint i;
//
//  TRACE("GOT> %d cols", argc);
//  for (i=0; i< argc; i++){
//    TRACE("GOT> [%d] %s:%s", i, columnNames[i], argv[i]);
//  }

//GOT> 6 cols
//GOT> [0] ROWID:3
//GOT> [1] type:1
//GOT> [2] title:2005 01 23  at  21:55:12
//GOT> [3] created:1106510113
//GOT> [4] updated:1106510113
//GOT> [5] content:/home/luccio/.gpe/sketchbook/2005-01-23_21-55-12.png


  if(argc == 6){
    Note note;

    note.id      =     atoi(argv[0]);
    note.type    =     atoi(argv[1]);//unused yet
    note.title   = g_strdup(argv[2]);
    note.created =     atoi(argv[3]);//unused yet
    note.updated =     atoi(argv[4]);//unused yet
    note.url     = g_strdup(argv[5]);

//    TRACE("[%30s] created %s updated %s",
//          note.title,
//          _get_time_label(note.created),
//          _get_time_label(note.updated));

    selector_add_note(note.id, note.title, note.url, note.created, note.updated, NULL);
    sketch_list_size++;
  }
  return 0;
}


int db_load_notes(){
  gint result;
  gchar *errmsg;

  result = sqlite_exec_printf (db, "SELECT ROWID,* FROM notes",
                               load_note_cb, NULL,
                               &errmsg);

  if(result != SQLITE_OK){
    report_error(result, errmsg);
  }

  return 0;
}


void db_update_timestamp (const gint id, const gint timestamp){
  gint   result;
  gchar *errmsg;

  result = sqlite_exec_printf (db, "UPDATE notes SET updated='%d' WHERE ROWID='%d'",
                               NULL, NULL,
                               &errmsg,
                               timestamp,
                               id);

  TRACE("SQL> [%d] update timestamp >>%d<<", id, timestamp);

  if(result != SQLITE_OK){
    report_error(result, errmsg);
  }  
}

void db_update_title(const gint id, const gchar *title){
  gint result;
  gchar *errmsg;

  result = sqlite_exec_printf (db, "UPDATE notes SET title='%q' WHERE ROWID='%d'",
                               NULL, NULL,
                               &errmsg,
                               title,
                               id);

  TRACE("SQL> [%d] update title >>%s<<", id, title);

  if(result != SQLITE_OK){
    report_error(result, errmsg);
  }
}

int db_insert_note(const Note *note){
  //gboolean exists = FALSE;
  gint    result;
  gchar * errmsg;

  if(!db) return -1;
  
  result = sqlite_exec_printf (db,"INSERT INTO notes VALUES(%d, '%q', %d, %d, '%q')",
                               NULL, NULL, //no callback function
                               &errmsg,
                               note->type,
                               note->title,
                               note->created,
                               note->updated,
                               note->url);

  if(result != SQLITE_OK){
    report_error(result, errmsg);
    return -1;
  }

  TRACE("SQL> inserted NOTE: %s", note->title);

  return sqlite_last_insert_rowid(db);
}

void db_delete_note(const gint id){ //FIXME: use it!
  gint result;
  gchar *errmsg;

  result = sqlite_exec_printf (db, "DELETE FROM notes WHERE ROWID='%d'",
                               NULL, NULL,
                               &errmsg,
                               id);
  if(result != SQLITE_OK){
    report_error(result, errmsg);
    return;
  }
}

void db_close(){
  if(db) sqlite_close(db);
}

int db_open(){
  gchar * db_name;
  gint    error;
  gchar * errmsg;
  gint mode = 0;// rw/r mode, ignored by sqlite (2.8.5)
  gint dbexists = 0;

  //--Open DB
  db_name = g_strdup_printf("%s/.gpe/%s", g_get_home_dir(), DB_NAME);
  TRACE("Opening: %s", db_name);

  dbexists = !access(db_name, F_OK); //access() in unistd.h

  db = sqlite_open(db_name, mode, &errmsg);
  g_free(db_name);

  if( db == NULL ){
    TRACE("ERROR> Can't open database: %s", errmsg);
    gpe_error_box (errmsg);
    free(errmsg);
    return -1;
  }

  //--Create table if necessary, we assume table exists if database is present  
  if (!dbexists){
    error = sqlite_exec (db, db_schema_meta,  NULL, NULL, &errmsg);
    error = sqlite_exec (db, db_schema_notes, NULL, NULL, &errmsg);

    TRACE("SQL> %s", db_schema_meta);
    TRACE("SQL> %s", db_schema_notes);

    if(error){
      report_error(error, errmsg);
      return -1;
    }
  }

  //FIXME: check DB version (!)

  //gpe_pim_categories_init ();

  return 0;
}
