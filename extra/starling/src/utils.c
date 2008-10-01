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

#define CHECK_TBL_STATEMENT "SELECT name FROM sqlite_master WHERE " \
                        " type='table' AND name=?;"

gchar *
escape_spaces (const gchar *orig, const gchar *replace)
{
    GString *str;
    gchar *retval;
    gint ii;

    str = g_string_new ("");

    for(ii = 0; ii < strlen (orig); ii++) {
        if (orig[ii] == ' ')
            g_string_append (str, replace);
        else
            g_string_append_c (str, orig[ii]);
    }

    retval = str->str;
    
    g_string_free (str, FALSE);

    return retval;
}

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
