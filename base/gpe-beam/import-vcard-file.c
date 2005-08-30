#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libintl.h>

#include <mimedir/mimedir-vcard.h>
#include <gpe/vcard.h>

#include <sqlite.h>

#define _(x) gettext (x)

#define DB_NAME "/.gpe/contacts"

sqlite *db;

int 
db_open(void) 
{
  /* open persistent connection */
  char *errmsg;
  char *buf;
  size_t len;
  const char *home = g_get_home_dir ();
  
  len = strlen (home) + strlen (DB_NAME) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, DB_NAME);
  
  db = sqlite_open (buf, 0, &errmsg);
  g_free (buf);
  
  if (db == NULL) 
    {
      fprintf (stderr, "%s", errmsg);
      free (errmsg);
      return -1;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  MIMEDirVCard *vcard;
  MIMEDirProfile *profile;
  gchar *str;
  GError *err = NULL;
	gchar *content;
	gchar **lines = NULL;
	gint length;
	int i = 0;

  g_type_init ();

  if (db_open ())
    exit (1);

 	if (!g_file_get_contents (argv[1], &content, &length, &err))
 	{
		fprintf (stderr, "Could not access file: %s.\n", argv[1]);
		exit(1);
	}
	lines = g_strsplit (content, "\n\n", 2048);
	g_free (content);

  while(lines[i])
  {
    if (!strstr(lines[i],"END:VCARD"))
    {
      lines[i]=realloc(lines[i], strlen(lines[i])+9);
      sprintf(lines[i]+strlen(lines[i]),"%s","END:VCARD");
    }
    profile = mimedir_profile_new(NULL);
    mimedir_profile_parse(profile, lines[i], &err);
	if (!err)
      vcard = mimedir_vcard_new_from_profile (profile, &err);
	
    if (vcard == NULL)
      {
        fprintf (stderr, "Err: %s\n", err->message);
        g_error_free (&err);
		err = NULL;
        i++;
        continue;
      }

    str = mimedir_vcard_get_as_string (vcard);
    if (str) 
    {
      free(str);
    }
    gpe_import_vcard (db, vcard);
    
	g_object_unref(profile);
	g_object_unref(vcard);
    i++;
  }

  exit (0);
}
