/*
 * This file is part of Starling
 *
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 * Copyright (C) 2007 Neal H. Walfield <neal@walfield.org>
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
#define KEYRANDOM "random"
#define KEYLASTPATH "last-path"
#define KEYSINK "sink"
#define KEYLFMUSER "lastfm-user"
#define KEYLFMPASSWD "lastfm-password"
#define KEY_WIDTH "width"
#define KEY_HEIGHT "height"

/* default values */
#define DEFKEYVERSION 1
#define DEFKEYRANDOM FALSE
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
        if (!g_key_file_has_key (st->keyfile, GROUP, KEYRANDOM, NULL)) {
            g_key_file_set_boolean (st->keyfile, GROUP,
                    KEYRANDOM, DEFKEYRANDOM);
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
    g_free (value);

    if (g_key_file_get_boolean (st->keyfile, GROUP, KEYRANDOM, NULL)) {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (st->random), TRUE);
    }


    if (g_key_file_has_key (st->keyfile, GROUP, KEYLFMUSER, NULL)) {
        value = g_key_file_get_string (st->keyfile, GROUP, KEYLFMUSER, NULL);
        gtk_entry_set_text (GTK_ENTRY (st->webuser_entry), value);
	g_free (value);
    }
    
    if (g_key_file_has_key (st->keyfile, GROUP, KEYLFMPASSWD, NULL)) {
        value = g_key_file_get_string (st->keyfile, GROUP, KEYLFMPASSWD, NULL);
        gtk_entry_set_text (GTK_ENTRY (st->webpasswd_entry), value);
	g_free (value);
    }

    int w = -1;
    int h = -1;
    if (g_key_file_has_key (st->keyfile, GROUP, KEY_WIDTH, NULL))
      {
        value = g_key_file_get_string (st->keyfile, GROUP, KEY_WIDTH, NULL);
	if (value)
	  w = MAX (100, atoi (value));
	g_free (value);
      }
    if (g_key_file_has_key (st->keyfile, GROUP, KEY_HEIGHT, NULL))
      {
        value = g_key_file_get_string (st->keyfile, GROUP, KEY_HEIGHT, NULL);
	if (value)
	  h = MAX (100, atoi (value));
	g_free (value);
      }
    gtk_window_set_default_size (GTK_WINDOW (st->window), w, h);

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
    if (st->fs_last_path)
      g_key_file_set_string (st->keyfile, GROUP, KEYLASTPATH,
			     st->fs_last_path);

    g_key_file_set_boolean (st->keyfile, GROUP, KEYRANDOM,
			    play_list_get_random (st->pl)); 

    int w, h;
    gtk_window_get_size (GTK_WINDOW (st->window), &w, &h);
    g_key_file_set_integer (st->keyfile, GROUP, KEY_WIDTH, w);
    g_key_file_set_integer (st->keyfile, GROUP, KEY_HEIGHT, h);

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

