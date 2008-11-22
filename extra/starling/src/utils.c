/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <string.h>

#include <glib/gstrfuncs.h>
#include <glib/gstring.h>

#include <gst/gsttaglist.h>

#include "starling.h"
#include "utils.h"

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>

gboolean
has_db_table (sqlite *db, const gchar *name)
{
  int have_table = FALSE;

  int callback (void *arg, int argc, char **argv, char **names)
  {
    have_table = TRUE;
    return 0;
  }

  char *err = NULL;
  sqlite_exec_printf (db,
		      "SELECT name FROM sqlite_master WHERE " \
		      " type='table' AND name='%q';",
		      callback, NULL, &err, name);
  if (err)
    {
      g_warning ("%s:%d: selecting: %s", __func__, __LINE__, err);
      sqlite_freemem (err);
    }

  return have_table;
}

char *
uri_escape_string (const char *string)
{
  /* We prefer g_uri_escape_string if it is available, as it is more
     robust than just escaping %.  */
#ifdef HAVE_G_URI_ESCAPE_STRING
  return g_uri_escape_string (string,
			      G_URI_RESERVED_CHARS_ALLOWED_IN_PATH,
			      1);
#else
  struct obstack escaped;
  obstack_init (&escaped);

  const char *s = string;
  while (*s)
    {
      int len = strcspn (s, "%");
      obstack_grow (&escaped, s, len);
      s += len;

      while (*s == '%')
	{
	  obstack_grow (&escaped, "%25", 3);
	  s ++;
	}
    }

  obstack_1grow (&escaped, 0);
  char *result = g_strdup (obstack_finish (&escaped));
  obstack_free (&escaped, NULL);
  return result;
#endif
}
