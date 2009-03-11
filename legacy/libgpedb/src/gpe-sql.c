/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE
 * as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 * 
 * (c) 2003 Florian Boor <florian.boor@kernelconcepts.de>
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <gpe/errorbox.h>

#include "../include/gpe-sql.h"


int 
sql_list_tables(t_sql_handle* sqlh, char*** resvec)
{
	char *err;
	int ret, nrow,ncolumn;
	ret = sql_get_table(sqlh,"SELECT name FROM sqlite_master WHERE type='table'",resvec,&nrow,&ncolumn,&err);
	if (ret) {
		fprintf(stderr,"err gpe_acontrol_list_tables: %s",err);
		free(err);
		return 0;
	}
	return nrow;
}


t_sql_handle*
sql_open (const char *dbtoopen)
{
  static const char *create_control_str = "create table _acontrol (
	atable	TEXT NOT NULL,
	auser	TEXT NOT NULL,
	amask	INTEGER NOT NULL,	
	aowner	INTEGER	
  );
  ";

  char *err;
  char *buf = NULL;
  t_sql_handle *sqlh;
#ifdef USE_USQLD
  sqlh = usqld_connect ("localhost", dbtoopen, &err);
#else
  if (strcmp(USQLD_CFGDB,dbtoopen))
  {
    const char *home = getenv ("HOME");
    size_t len;
    if (home == NULL)
      home = "";
    len = strlen (home) + strlen (dbpath) + strlen (dbtoopen) + 1;
    buf = g_malloc (len);
    strcpy (buf, home);
    strcat (buf, dbpath);
    strcat (buf, dbtoopen);
    sqlh = sqlite_open (buf, 0, &err);
  }
  else
	sqlh = sqlite_open (dbtoopen, 0, &err);
#endif
  if (sqlh == NULL)
    {
      gpe_error_box (err);
      free (err);
      if (buf) g_free (buf);
      return NULL;
    }

#ifndef USE_USQLD
  // create control table if it doesn't exist
  // we just want to be shure :-)
  sql_exec (sqlh, create_control_str, NULL, NULL, &err);
#endif
  return sqlh;
}


void
sql_close (t_sql_handle* sqlh)
{
#ifdef USE_USQLD
  usqld_disconnect (sqlh);
#else
  sqlite_close (sqlh);
#endif
}
