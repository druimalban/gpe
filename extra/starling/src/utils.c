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

#define CHECK_TBL_STATEMENT "SELECT name FROM sqlite_master WHERE " \
                        " type='table' AND name=?;"

gboolean
has_db_table (sqlite *db, const gchar *name)
{
    sqlite_vm *vm;
    gint ret;

    g_return_val_if_fail (db != NULL, FALSE);

    sqlite_compile (db, CHECK_TBL_STATEMENT, NULL, &vm, NULL);

    sqlite_bind (vm, 1, name, -1, 0);

    ret = sqlite_step (vm, NULL, NULL, NULL);

    sqlite_finalize (vm, NULL);

    if (SQLITE_ROW == ret) {
        return TRUE;
    }

    return FALSE;
}

gint
sqlite_bind_int (sqlite_vm *vm, gint index, gint value)
{
    gchar s[128];

    snprintf (s, sizeof (s), "%d", value);

    return sqlite_bind (vm, index, s, -1, TRUE);
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

  const char *s = source;
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
