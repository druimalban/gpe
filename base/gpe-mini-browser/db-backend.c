/*
 * gpe-mini-browser v0.16
 *
 * Basic web browser based on gtk-webcore 
 * 
 * db-backend.c Backend for sqlite to store bookmarks.
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <glib.h>

#include <sqlite.h>

#include <gpe/errorbox.h>

#include "gpe-mini-browser.h"

#define DB_NAME "/.gpe/bookmarks"


static sqlite *db = NULL; 
extern GSList *booklist;

/* initialize db, return 0 if successful */
int start_db (void)
{
	static const char *create_str = "create table bookmarks (bookmark TEXT NOT NULL);";

	const char *home = getenv ("HOME");
  	char *buf;
  	char *err;
  	size_t len;
  
	if (home == NULL) 
    		home = "";
	len = strlen (home) + strlen (DB_NAME) + 1;
 	buf = g_malloc (len);
 	strcpy (buf, home);
  	strcat (buf, DB_NAME);
  	db = sqlite_open (buf, 0, &err);
  	if (db == NULL)
    	{
     	 gpe_error_box (err);
     	 free (err);
     	 g_free (buf);
     	 return -1;
    	}

	sqlite_exec (db, create_str, NULL, NULL, &err);
	g_free (buf);

	return 0;
}

void stop_db (void)
{
	GSList *iter;

  	for (iter = booklist; iter; iter = g_slist_next (iter))
  	//  todo_db_destroy_item (iter->data);
 	g_slist_free (booklist);
 	booklist = NULL;
	
	sqlite_close (db);
}

int insert_new_bookmark (char *bookmark)
{
	char *err;
	const char *insert = "insert into bookmarks values ";
	char *cmd = NULL;
  
  	strcpy (cmd, insert);
  	strcat (cmd, bookmark);
  	if (sqlite_exec (db, cmd, NULL, NULL, &err))
  	{
    	 g_free(err);
    	 g_free(cmd);
   	 return 1;
 	 }

  	booklist = g_slist_append (booklist, bookmark);
  	g_free(err);
 	g_free(cmd);

 	 return 0;

}	

int remove_bookmark (char *bookmark)
{
	char *err;
	const char *remove = "delete from bookmarks where bookmark=";
	char *cmd = NULL;

	strcpy (cmd, remove);
	strcat (cmd, bookmark);
	if (sqlite_exec (db, cmd, NULL, NULL, &err))
	{
	 g_free(err);
	 g_free(cmd);
    	 return 1;
 	}
	
	booklist = g_slist_remove (booklist, bookmark);
	g_free(err);
        g_free(cmd);

	return 0;
}
