#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <glib.h>
#include "update_mime_db.h"
#include "hash.h"
#include "mime-sql.h"
#include "mime-programs-sql.h"

/*  
    TODO:
    
    o Parse initial [Desktop Entry] style lines
      - support actions

    o Basic parsing of Exec substitutions ( eg %f etc )

    o add more functionaility from desktop spec for stnadard 
      required keys, such as type ( can point to an enum ) etc

    o Parse .directorys too XXX handled now by vfolder
      - see standard catogaries
        http://www.freedesktop.org/standards/VFolderDesktops.txt

    o parse true/false to non-null/NULL

    o ignore space around ='s

    o vfolder 'OnlyShowIn' key for dock apps 
      - eg OnlyShowIn=X-MB-DOCK ?
      - or maybe just set an X-MB_DOCKAPP: true key ?
      - or both ?
      - Catogarie=Applet ?

    o check ENV(DESKTOP_FILE_PATH), /usr/(local)/share/applications,
      .applications

*/

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
  int type;
  char *key = NULL, *val = NULL, *str = NULL;
  
  if (!(fp = fopen(ini->filename, "r"))) return INI_ERROR_FILE_OPEN_FAILED;

  if (fgets(data,256,fp) != NULL)
    {
      if (strncasecmp ("[mime type]", data, 11) == 0)
	{
	  ini->type = strdup ("Mime Type");
	}
      else if (strncasecmp ("[mime program association]", data, 26))
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
	    if (scanf (key, "%64[^[][%16[^][]]", new_key, locale) == 2)
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
    return NULL;
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



int
main (int argc, char **argv)
{
  IniFile *ini = NULL;
  struct nlist *n;
  const char *lang;

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

  ini = ini_new_from_file (argv[1], lang);

  if (strcmp (ini->type, "Mime Type") == 0)
    {
      struct mime_type *e = g_malloc (sizeof (struct mime_type));

      printf("name => %s \n", ini_get (ini, "Name"));
      printf("type => %s \n", ini_get (ini, "Type"));
      printf("extension => %s \n", ini_get (ini, "Extension"));
      printf("icon => %s \n", ini_get (ini, "Icon"));

      e = new_mime_type (ini_get (ini, "Type"), ini_get (ini, "Name"), ini_get (ini, "Extension"), ini_get (ini, "Icon"));
    }
}
