/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define _XOPEN_SOURCE

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sqlite.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>

#include <gpe/errorbox.h>

#include "mime-sql.h"

static sqlite *sqliteh;

static const char *fname = "/.gpe/mime-types";

GSList *mime_types;

gint rows = 0;

static struct mime_type *
new_mime_type_internal (int id, const char *mime_name, const char *description, const char *extension, const char *icon)
{
  struct mime_type *e = g_malloc (sizeof (struct mime_type));

  e->mime_name = mime_name;
  e->description = description;
  e->extension = extension;
  e->icon = icon;

  mime_types = g_slist_append (mime_types, e);

  return e;
}

struct mime_type *
new_mime_type (const char *mime_name, const char *description, const char *extension, const char *icon)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "insert into mime_types values (NULL, '%q', '%q', '%q', '%q')",
			  NULL, NULL, &err, mime_name, description, extension, icon))
    {
      gpe_error_box (err);
      free (err);
      return NULL;
    }

  return new_mime_type_internal (sqlite_last_insert_rowid (sqliteh), mime_name, description, extension, icon);
}

void
del_mime_extension (struct mime_type *e)
{
  char *err;

  if (sqlite_exec_printf (sqliteh, "delete from mime_types where uid=%d",
			  NULL, NULL, &err,
			  e->id))
    {
      gpe_error_box (err);
      free (err);
    }

  mime_types = g_slist_remove (mime_types, e);
}

static int
mime_type_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 5 && argv[0] && argv[1])
    new_mime_type_internal (atoi (argv[0]), g_strdup (argv[1]), g_strdup (argv[2]), g_strdup (argv[3]), g_strdup (argv[4]));
  rows++;
  return 0;
}

/* --- */

static void
add_mime_types (void)
{
  struct mime_type *e = g_malloc (sizeof (struct mime_type));

  e = new_mime_type ("application/pdf", "Adobe PDF Document", "pdf", "application-pdf");
  e = new_mime_type ("application/x-compress", "Gzip Compressed File", "gz", "application-x-gzip");
  e = new_mime_type ("application/zip", "Zip Compressed File", "zip", "application-x-compress");
  e = new_mime_type ("application/x-ipk", "Ipkg File", "ipk", "application-x-ipk");
  e = new_mime_type ("audio/x-gsm", "GSM Audio File", "gsm", "audio-x-gsm");
  e = new_mime_type ("audio/mpeg", "MP3 Audio File", "mp3", "audio-mpeg");
  e = new_mime_type ("application/x-ogg", "OGG Vorbis Audio File", "ogg", "application-x-ogg");
  e = new_mime_type ("audio/x-wav", "WAV Audio File", "wav", "audio-x-wav");
  e = new_mime_type ("font/ttf", "True Type Font", "ttf", "font-ttf");
  e = new_mime_type ("image/jpeg", "JPEG Image File", "jpeg", "image-jpeg");
  e = new_mime_type ("image/jpeg", "JPEG Image File", "jpg", "image-jpeg");
  e = new_mime_type ("image/jpeg", "JPEG Image File", "jpe", "image-jpeg");
  e = new_mime_type ("image/png", "Portabe Image Graphics File", "png", "image-png");
  e = new_mime_type ("image/gif", "GIF Image File", "gif", "image-gif");
  e = new_mime_type ("image/xpm", "XPM Image File", "xpm", "image-xpm");
  e = new_mime_type ("text/html", "HTML Text Document", "html", "text-html");
  e = new_mime_type ("text/x-python", "Python Text Document", "py", "text-x-python");
  e = new_mime_type ("video/mpeg", "MPEG Video File", "mpg", "video-mpeg");
  e = new_mime_type ("video/mpeg", "MPEG Video File", "mpeg", "video-mpeg");
  e = new_mime_type ("video/avi", "AVI Video File", "avi", "video-x-msvideo");
}

int
sql_start (void)
{
  static const char *schema1_str = 
    "create table mime_types (uid INTEGER PRIMARY KEY, name TEXT, description TEXT, extension TEXT, icon TEXT)";

  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;
  if (home == NULL) 
    home = "";
  len = strlen (home) + strlen (fname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, fname);
  sqliteh = sqlite_open (buf, 0, &err);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      g_free (buf);
      return -1;
    }

  sqlite_exec (sqliteh, schema1_str, NULL, NULL, &err);

  if (sqlite_exec (sqliteh, "select uid,name,description,extension,icon from mime_types",
		   mime_type_callback, NULL, &err))
    {
      gpe_error_box (err);
      free (err);
      return -1;
    }

  //if (rows == 0)
  //  add_mime_types ();

  return 0;
}

void
sql_close (void)
{
  sqlite_close (sqliteh);
}
