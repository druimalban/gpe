/* Preferences handling for GPE
 *
 * Copyright (C) 2003 Luc Pionchon
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


/*
   TODO:
   - serious error/report management, with i18ned error string
   - use libgpedb (wraper usqld/sqlite)
   - impleme: load all into data struct (GData ?), set/get, save all/changed (?)
   - check  : need of "delete key" ? "clean all prefs" ?
   - check  : charset possible issues 
*/

#include <glib.h>
#include <sqlite.h> //backend
#include <stdlib.h> //free() atoi() atof()

#include "gpepreferences.h"

#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...) 
#endif


sqlite * prefs_db = NULL;

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

  //--Open DB
  db_name = g_strdup_printf("%s/.gpe/prefs_%s.db", g_get_home_dir(), prog_name);

  TRACE("Opening: %s", db_name);
  prefs_db = sqlite_open(db_name, mode, &errmsg);
  g_free(db_name);
  if( prefs_db == NULL ){
    TRACE("ERROR> Can't open database: %s", errmsg);
    return GPE_PREFS_ERROR;
  }

  //--Create table
  //if already exists, will return an error.
  error = sqlite_exec (prefs_db, db_schema, NULL, NULL, &errmsg);
  TRACE("%s", db_schema);
  if(error){
    g_printerr("ERROR> #%d %s\n", error, errmsg);
    free (errmsg);
    return GPE_PREFS_ERROR;
  }

  return GPE_PREFS_OK;
}

void gpe_prefs_exit(){
  if(prefs_db) sqlite_close(prefs_db);
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
  gchar * _pvalue = NULL;

  if(!prefs_db) return GPE_PREFS_ERROR;
  
  result = sqlite_exec_printf (prefs_db,
                               "SELECT value FROM prefs WHERE key='%q'",
                               _get_single_value , &_pvalue, &errmsg,
                               key);
  if(result != SQLITE_OK){
    TRACE("ERROR> #%d : %s", result, errmsg);
    free(errmsg);
    return GPE_PREFS_ERROR;
  }

  if(_pvalue == NULL){ _pvalue = g_strdup("");}
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
  gchar * errmsg;

  if(!prefs_db) return GPE_PREFS_ERROR;
  
  result = sqlite_exec_printf (prefs_db,
                               "SELECT key FROM prefs WHERE key='%q'",
                               _key_exists , &exists, &errmsg,
                               key);
  if(exists){
    switch(type){
      case G_TYPE_INT:
        result = sqlite_exec_printf (prefs_db,
                                     "UPDATE prefs SET value='%d' WHERE key='%q'",
                                     NULL, NULL, &errmsg,
                                     *(gint *)pvalue, key);
        TRACE("update> %s = %d", key,*(gint *)pvalue);
        break;
      case G_TYPE_FLOAT:
        result = sqlite_exec_printf (prefs_db,
                                     "UPDATE prefs SET value='%g' WHERE key='%q'",
                                     NULL, NULL, &errmsg,
                                     *(gfloat *)pvalue, key);
        TRACE("update> %s = %g", key,*(gfloat *)pvalue);
        break;
      case G_TYPE_STRING:
        result = sqlite_exec_printf (prefs_db,
                                     "UPDATE prefs SET value='%q' WHERE key='%q'",
                                     NULL, NULL, &errmsg,
                                     *(gchar **)pvalue, key);
        TRACE("update> %s = %s", key,*(gchar **)pvalue);
        break;
      default:
        //ERROR: Unhandled type
    }
  }
  else{
    switch(type){
      case G_TYPE_INT:
        result = sqlite_exec_printf (prefs_db,
                                     "INSERT INTO prefs VALUES('%q', %d, %d)",
                                     NULL, NULL, &errmsg,
                                     key, type, * (gint *)pvalue);
        TRACE("insert> %s = %d", key,*(gint *)pvalue);
        break;
      case G_TYPE_FLOAT:
        result = sqlite_exec_printf (prefs_db,
                                     "INSERT INTO prefs VALUES('%q', %d, %g)",
                                     NULL, NULL, &errmsg,
                                     key, type, * (gfloat *)pvalue);
        TRACE("insert> %s = %g", key,*(gfloat *)pvalue);
        break;
      case G_TYPE_STRING:
        result = sqlite_exec_printf (prefs_db,
                                     "INSERT INTO prefs VALUES('%q', %d, '%q')",
                                     NULL, NULL, &errmsg,
                                     key, type, * (gchar **)pvalue);
        TRACE("insert> %s = %s", key,*(gchar **)pvalue);
        break;
      default:
        //ERROR: Unhandled type
    }
  }

  if(result != SQLITE_OK){
    TRACE("ERROR> #%d : %s", result, errmsg);
    free(errmsg);
    return GPE_PREFS_ERROR;
  }

  return GPE_PREFS_OK;
}
