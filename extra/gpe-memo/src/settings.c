/*
* This file is part of GPE-Memo
*
* Copyright (C) 2007 Alberto García Hierro
*	<skyhusker@rm-fr.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
*/

#include <unistd.h>
#include <sys/stat.h>
#include <glib/gkeyfile.h>
#include <glib/gstrfuncs.h>
#include <glib/gutils.h>
#include <glib/gmem.h>
#include <glib/gfileutils.h>

#include "settings.h"

#define CONFIG_FILE
#define GROUP "main"
#define RCFILE "gpe-memorc"

static GKeyFile *keyfile = NULL;

static void settings_init (void) __attribute__ ((constructor));
 static void settings_commit (void) __attribute__ ((destructor));

static void
settings_init (void)
{
    gchar *filename;

    if (!keyfile) {
        filename = g_strdup_printf ("%s/.gpe/%s", g_get_home_dir(), RCFILE);
        keyfile = g_key_file_new ();
        g_key_file_load_from_file (keyfile, filename, 0, NULL);
        g_free (filename);

        if (!g_key_file_has_key (keyfile, GROUP, "memo-dir", NULL)) {
            filename = g_strdup_printf ("%s/%s", g_get_home_dir(), "Memo");
            if (g_access(filename, F_OK))
               g_mkdir(filename, S_IRWXU | S_IRWXG );
            g_key_file_set_string (keyfile, GROUP, "memo-dir", filename);
            g_free (filename);
        }
    }

}

void
settings_commit (void)
{
    gchar *filename;
    gchar *data;
    gsize len;

    data = g_key_file_to_data (keyfile, &len, NULL);
    filename = g_strdup_printf ("%s/.gpe/%s", g_get_home_dir(), RCFILE);

    g_file_set_contents (filename, data, len, NULL);
}

gchar *
settings_get_memo_dir (void)
{
    return g_key_file_get_string (keyfile, GROUP, "memo-dir", NULL);
}

void
settings_set_memo_dir (const gchar *dirname) 
{
    g_key_file_set_string (keyfile, GROUP, "memo-dir", dirname);
    settings_commit ();
}

