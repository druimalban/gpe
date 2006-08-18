/*
 * This file is part of Starling
 *
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <unistd.h>

#include <string.h>

#include <glib/gstrfuncs.h>
#include <glib/gutils.h>
#include <glib/gstdio.h>

#include <gtk/gtkentry.h>

#include "starling.h"
#include "config.h"

#define KEYVERSION "version"
#define KEYLOADPLAYLIST "load-playlist"
#define KEYLASTPATH "last-path"
#define KEYSINK "sink"
#define KEYLFMUSER "lastfm-user"
#define KEYLFMPASSWD "lastfm-password"

/* default values */
#define DEFKEYVERSION 1
#define DEFKEYLOADPLAYLIST TRUE
#define DEFKEYLASTPATH ""
#define DEFKEYSINK "esdsink"

#define GROUP "main"

void
config_init (Starling *st)
{
    gchar *path;

    path = g_strdup_printf ("%s/%s", g_get_home_dir(), CONFIGDIR);
    g_mkdir (path, 0755);
    
    g_free (path);

    path = g_strdup_printf ("%s/%s/%s", g_get_home_dir(),
            CONFIGDIR, CONFIG_FILE);

    st->keyfile = g_key_file_new ();
    
    /* Ignore return value, we will check the default
     * settings later.
     */

    g_key_file_load_from_file (st->keyfile, path, 0, NULL);
/*
    if (!g_key_file_load_from_file (st->keyfile, path, 0, NULL)) {
        g_key_file_set_integer (st->keyfile, GROUP,
                KEYVERSION, 1);
        g_key_file_set_boolean (st->keyfile, GROUP,
                KEYLOADPLAYLIST, TRUE);
        g_key_file_set_string (st->keyfile, GROUP,
                KEYLASTPATH, "");
        g_key_file_set_string (st->keyfile, GROUP,
                KEYSINK, "esdsink");
    } else {   
*/      
        if (!g_key_file_has_key (st->keyfile, GROUP, KEYVERSION, NULL)) {
            g_key_file_set_integer (st->keyfile, GROUP,
                    KEYVERSION, DEFKEYVERSION);
        }
        
        if (!g_key_file_has_key (st->keyfile, GROUP, KEYLOADPLAYLIST, NULL)) {
            g_key_file_set_boolean (st->keyfile, GROUP,
                    KEYLOADPLAYLIST, DEFKEYLOADPLAYLIST);
        }
        if (!g_key_file_has_key (st->keyfile, GROUP, KEYLASTPATH, NULL)) {
            g_key_file_set_string (st->keyfile, GROUP,
                    KEYLASTPATH, DEFKEYLASTPATH);
        }
        if (!g_key_file_has_key (st->keyfile, GROUP, KEYSINK, NULL)) {
            g_key_file_set_string (st->keyfile, GROUP,
                    KEYSINK, DEFKEYSINK);
        }
//    }

    g_free (path);
}

void
config_load (Starling *st)
{
    gchar *path;
    gchar *value;

    if (!st->keyfile) {
        config_init (st);
    }

    value = g_key_file_get_string (st->keyfile, GROUP, KEYSINK, NULL);
    play_list_set_sink (st->pl, value);

    if (g_key_file_has_key (st->keyfile, GROUP, KEYLFMUSER, NULL)) {
        value = g_key_file_get_string (st->keyfile, GROUP, KEYLFMUSER, NULL);
        gtk_entry_set_text (GTK_ENTRY (st->webuser_entry), value);
    }
    
    if (g_key_file_has_key (st->keyfile, GROUP, KEYLFMPASSWD, NULL)) {
        value = g_key_file_get_string (st->keyfile, GROUP, KEYLFMPASSWD, NULL);
        gtk_entry_set_text (GTK_ENTRY (st->webpasswd_entry), value);
    }

    g_free (value);

    /* Don't care for errors, config_init() should take care of them */
    if (g_key_file_get_boolean (st->keyfile, GROUP, KEYLOADPLAYLIST, NULL)) {
        path = g_strdup_printf ("%s/%s/%s", g_get_home_dir(), 
                CONFIGDIR, CONFIG_PL);

        if (g_access (path, R_OK) == 0)
            play_list_add_m3u (st->pl, path);

        g_free (path);
    }

    path = g_key_file_get_string (st->keyfile, GROUP, KEYLASTPATH, NULL);

    if (path && strlen (path) > 0) {
        st->fs_last_path = path;
    } else {
        st->fs_last_path = NULL;
    }
}

void
config_save (Starling *st)
{
    gchar *path;
    gchar *data;
    gsize length;
    
    path = g_strdup_printf ("%s/%s/%s", g_get_home_dir(), 
            CONFIGDIR, CONFIG_PL);

    play_list_save_m3u (st->pl, path);

    g_free (path);

    /* Grab settings from memory */
    g_key_file_set_string (st->keyfile, GROUP, KEYLASTPATH, st->fs_last_path);

    data = g_key_file_to_data (st->keyfile, &length, NULL);

    path = g_strdup_printf ("%s/%s/%s", g_get_home_dir(),
            CONFIGDIR, CONFIG_FILE);
    g_file_set_contents (path, data, length, NULL);

    g_free (path);

    st->keyfile = NULL;
}

void
config_store_lastfm (const gchar *user, const gchar *passwd, Starling *st)
{
    g_key_file_set_string (st->keyfile, GROUP, KEYLFMUSER, user);
    g_key_file_set_string (st->keyfile, GROUP, KEYLFMPASSWD, passwd);
}

