/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 * Original code by Matthew Allum <mallum@handhelds.org>
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
#include <sys/types.h>
#include <dirent.h>

#include "update_mime_db.h"
#include "hash.h"
#include "mime-sql.h"
#include "mime-programs-sql.h"

static int
_parse_ini_entry (IniFile *ini);

/*
static char *
trim (char *str)
{
char *result = str;

  while (isspace(*result))
    result++;

  while(isspace(*(result+strlen(result))))
    *(result+strlen(result)) = '\0';

  return result;
}
*/

static int
_parse_ini_entry (IniFile *ini)
{
  FILE *fp;
  char data[256];
  const char delim[] = "=";
  char *key = NULL, *val = NULL, *str = NULL;
  
  if (!(fp = fopen(ini->filename, "r"))) return INI_ERROR_FILE_OPEN_FAILED;

  if (fgets(data,256,fp) != NULL)
    {
      if (strncasecmp ("[mime type]", data, 11) == 0)
	{
	  ini->type = strdup ("Mime Type");
	}
      else if (strncasecmp ("[mime program association]", data, 26) == 0)
	{
	  ini->type = strdup ("Mime Program Association");
	}
      else
	{
	  fprintf(stderr, "Doesn't look like any type of mime entry? %s\n", data);
	  return INI_ERROR_NOT_MIME_FILE;
	}
    } else return INI_ERROR_NOT_MIME_FILE;

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
		if (ini->lang == NULL) goto END; /* Ignore C or no locale*/
		if (!strcmp(ini->lang, locale)) /* Check for match */
		  key = new_key;
		else goto END;
	      }
	    if (val[strlen(val)-1] == '\n') val[strlen(val)-1] = '\0';
	    hash_add(ini->h, key, val);
	  }
    END:
      free(str);
    }
  
  fclose(fp);

  return INI_SUCCESS;
}

IniFile *
ini_new_from_file (const char *filename, char *lang)
{
  IniFile *ini;
  
  ini = malloc (sizeof (IniFile));
  ini->filename = strdup(filename);

  if (lang == NULL)
    {
      lang = setlocale (LC_MESSAGES, NULL);
      if (lang != NULL && !strcmp(lang, "C"))
	ini->lang = NULL;
      else
	ini->lang = strdup(lang);
    }
  else ini->lang = strdup(lang);

  ini->h = hash_new(51);

  if (_parse_ini_entry(ini) == INI_SUCCESS)
    return ini;
  else
    {
      ini_free(ini);
      return NULL;
    }
}

unsigned char *
ini_get (IniFile *ini, char *field)
{
  struct nlist *n;
  n = hash_lookup(ini->h, field);
  if (n)
    return (unsigned char *)n->value;
  else
    return "";
}

void
ini_set (IniFile *ini, const char *field, const unsigned char *value)
{
  ;
}

void
ini_free (IniFile *ini)
{
  free(ini->filename);
  free(ini->lang);

  hash_destroy(ini->h);

  free(ini);
}

static gchar *
get_file_extension (gchar *filename)
{
  int i;
  gchar *extension;

  for (i = strlen (filename); i > 0; i--)
  {
    if (filename[i] == '.')
      break;
  }

  if (i == strlen (filename))
  {
    return NULL;
  }
  else
  {
    extension = g_malloc (strlen (filename) - i);
    extension = g_strdup (filename + i + 1);
    return extension;
  }
}

int
main (int argc, char **argv)
{
  IniFile *ini = NULL;
  const char *lang;
  gchar *filename;
  gchar **extensions;
  int i = 0;
  struct dirent *d;
  DIR *dir;

  int utf8_mode = 0;
  char *s;

  if (!setlocale(LC_CTYPE, "")) 
    {
      fprintf(stderr, "Locale not specified. Check LANG, LC_CTYPE, LC_ALL.");
    }

  if ((s = getenv("LC_ALL")) 
      || (s = getenv("LC_CTYPE")) 
      || (s = getenv ("LANG"))) 

    {
      printf("got a locale %s %s %s %s\n", getenv("LC_ALL"), 
	     getenv("LC_CTYPE"), getenv ("LANG"), s );
      if (strstr(s, "UTF-8"))
	{
	  printf("have utf8 locale\n");
	  utf8_mode = 1;
	}
    }

  if (sql_start ())
    exit (1);

  if (programs_sql_start ())
    exit (1);

  lang = setlocale (LC_MESSAGES, NULL);
  if (lang != NULL && !strcmp(lang, "C"))
    lang = NULL;

  dir = opendir ("/etc/mime-handlers");

  if (dir)
  {
    del_mime_all ();
    del_mime_program_all ();

    while (d = readdir (dir), d != NULL)
    {
      if (strcmp (get_file_extension (d->d_name), "mime") == 0)
      {
	filename = g_strdup_printf ("/etc/mime-handlers/%s", d->d_name);
	ini = ini_new_from_file (filename, lang);

        if (strcmp (ini->type, "Mime Type") == 0)
        {
	  printf ("---------------------------\n");
          printf ("name => %s \n", ini_get (ini, "Name"));
          printf ("type => %s \n", ini_get (ini, "Type"));
          printf ("extension => %s \n", ini_get (ini, "Extension"));
          printf ("icon => %s \n", ini_get (ini, "Icon"));

	  extensions = g_strsplit (ini_get (ini, "Extension"), " ", 0);

	  while (1)
	  {
	    if (extensions[i])
	      new_mime_type (ini_get (ini, "Type"), ini_get (ini, "Name"), extensions[i], ini_get (ini, "Icon"));
            else
	      break;

	    i++;
	  }
	  i = 0;
	}
        else if (strcmp (ini->type, "Mime Program Association") == 0)
        {
	  printf ("---------------------------\n");
          printf ("name => %s \n", ini_get (ini, "Name"));
          printf ("mime association => %s \n", ini_get (ini, "Association"));
          printf ("command => %s \n", ini_get (ini, "Command"));

          new_mime_program (ini_get (ini, "Name"), ini_get (ini, "Association"), ini_get (ini, "Command"));
	}

	ini_free (ini);
      }
    }
  }
}



