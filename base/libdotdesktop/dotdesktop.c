#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <glib.h>
#include <dirent.h>
#include "hash.h"
#include "dotdesktop.h"

static int
_parse_desktop_entry(DotDesktop *dd, char *section)
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
	  fprintf(stderr, "hmm dont look like a desktop entry? %s\n", data);
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
	    hash_add(dd->h, key, val);
	  }
    END:
      free(str);
    }
  
  fclose(fp);
  
  return DD_SUCCESS;
}

DotDesktop *
dotdesktop_new_from_file(const char *filename, char *lang, char *section)
{
  DotDesktop *dd;
  
  dd = malloc(sizeof(DotDesktop));
  dd->filename = strdup(filename);

  if (lang == NULL)
    {
      lang = setlocale (LC_MESSAGES, NULL);
      if (lang != NULL && !strcmp(lang, "C"))
	dd->lang = NULL;
      else
	dd->lang = strdup(lang);
    }
  else dd->lang = strdup(lang);

  dd->h = hash_new(51);

  if (_parse_desktop_entry(dd, section) == DD_SUCCESS)
    return dd;
  else
    {
      dotdesktop_free(dd);
      return NULL;
    }
}

unsigned char *
dotdesktop_get(DotDesktop *dd, char *field)
{
  struct nlist *n;
  n = hash_lookup(dd->h, field);
  if (n)
    return (unsigned char *)n->value;
  else
    return NULL;
}

void
dotdesktop_set(DotDesktop *dd, const char *field, const unsigned char *value)
{
  ;
}

void
dotdesktop_free(DotDesktop *dd)
{
  free(dd->filename);
  free(dd->lang);

  hash_destroy(dd->h);

  free(dd);
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

GList *
get_mime_types ()
{
  gchar *filename;
  gchar **extensions;
  GList *mime_types;
  struct dirent *d;
  struct MimeType *mime_type = NULL;
  struct DotDesktop *dd_entry = NULL, *dd_mime_type = NULL;
  DIR *dir;

  int utf8_mode = 0;
  char *s;
  const char *lang;

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


  lang = setlocale (LC_MESSAGES, NULL);
  if (lang != NULL && !strcmp(lang, "C"))
    lang = NULL;

  dir = opendir ("/usr/share/applications");

  if (dir)
  {
    while (d = readdir (dir), d != NULL)
    {
      if (strcmp (get_file_extension (d->d_name), "desktop") == 0)
      {
	filename = g_strdup_printf ("/usr/share/applications/%s", d->d_name);

	dd_entry = dotdesktop_new_from_file (filename, lang, "desktop entry");
	mime_type->exec = g_strdup (dotdesktop_get (dd_entry, "Exec"));
	dotdesktop_free (dd_entry);

	dd_mime_type = dotdesktop_new_from_file (filename, lang, "mime_type");
	mime_type->name = g_strdup (dotdesktop_get (dd_mime_type, "Name"));
	mime_type->type = g_strdup (dotdesktop_get (dd_mime_type, "Type"));
	mime_type->icon = g_strdup (dotdesktop_get (dd_mime_type, "Icon"));
	extensions = g_strsplit (dotdesktop_get (dd_entry, "Extension"), " ", 0);
	mime_type->extensions = g_strdup (extensions);
	dotdesktop_free (dd_mime_type);

	mime_types = g_list_append (mime_types, mime_type);
	mime_type = NULL;
      }
    }
  }
  return mime_types;
}

  /*
int
main(int argc, char **argv)
{
  struct hash *h;
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


  lang = setlocale (LC_MESSAGES, NULL);
  if (lang != NULL && !strcmp(lang, "C"))
    lang = NULL;

  h = hash_new(101);
  parse_desktop_entry(h, argv[1], lang);
  
  n = hash_lookup(h, "Icon");
  printf("icon => %s \n", n->value);

  hash_destroy(h);

}
  */
