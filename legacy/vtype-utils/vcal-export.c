#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libintl.h>

#include <mimedir/mimedir-vcal.h>

#include <sqlite.h>

#include <gpe/vevent.h>

#define _(x) gettext (x)

#define DB_NAME "/.gpe/calendar"

sqlite *db;

MIMEDirVCal *vcal;

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
read_tags (void *data, int argc, char *argv[], char *ignore[])
{
  GSList **tags = (GSList **)data;

  *tags = gpe_tag_list_prepend (*tags, argv[0], g_strdup (argv[1]));

  return 0;
}

void
export_one_vcal (int uid)
{
  MIMEDirVEvent *vevent;
  char *err = NULL;
  GSList *tags = NULL;

  if (sqlite_exec_printf (db, "select tag,value from calendar where uid=%d", read_tags, &tags, &err,
			  uid) != SQLITE_OK)
    {
      printf ("%s\n", err);
      exit (1);
    }

  vevent = vevent_from_tags (tags);

  mimedir_vcal_add_component (vcal, MIMEDIR_VCOMPONENT (vevent));
}

int
read_one (void *ignore, int argc, char *argv[], void *data)
{
  int uid = atoi (argv[0]);
  export_one_vcal (uid);
  return 0;
}

int
main (int argc, char *argv[])
{
  gchar *str;

  g_type_init ();

  if (db_open ())
    exit (1);

  vcal = mimedir_vcal_new ();

  if (argc > 1)
    {
      guint uid;
      uid = atoi (argv[1]);
      export_one_vcal (uid);
    }
  else
    sqlite_exec (db, "select uid from calendar_urn", read_one, NULL, NULL);

  str = mimedir_vcal_write_to_string (vcal);

  fprintf (stdout, "%s\n", str);

  exit (0);
}
