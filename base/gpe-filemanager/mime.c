/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <glib.h>
#include <dirent.h>

#include "mime.h"

static int
_parse_desktop_entry(DotDesktop *dd, gchar *section)
{
  FILE *fp;
  char data[256];
  const char delim[] = "=";
  char *key = NULL, *val = NULL, *str = NULL;
  
  if (!(fp = fopen(dd->filename, "r"))) return DD_ERROR_FILE_OPEN_FAILED;

  if (fgets(data,256,fp) != NULL)
    {
      if (strncasecmp(section, data, strlen (section)))
	{
	  fprintf(stderr, "This dosn't look like a desktop entry? %s\n", data);
	  return DD_ERROR_NOT_DESKTOP_FILE;
	}
    } else return DD_ERROR_NOT_DESKTOP_FILE;

  while(fgets(data,256,fp) != NULL)
    {
      if (data[0] == '#') 
	continue;
      str = strdup(data);
      if ((key = strsep (&str, delim)) != NULL)
	if ((val = strsep (&str, delim)) != NULL)
	  {
	    char new_key[64], locale[16]; 
	    if (sscanf (key, "%64[^[][%16[^][]]", new_key, locale) == 2)
	      {
		if (dd->lang == NULL) goto END; /* Ignore C or no locale*/
		if (!strcmp(dd->lang, locale)) /* Check for match */
		  key = new_key;
		else goto END;
	      }
	    if (val[strlen(val)-1] == '\n') val[strlen(val)-1] = '\0';
	    g_hash_table_insert (dd->hash,(gpointer *)  g_strdup (key), (gpointer *) g_strdup (val));
	  }
    END:
      free(str);
    }
  
  fclose(fp);
  
  return DD_SUCCESS;
}
