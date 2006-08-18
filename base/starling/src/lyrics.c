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

#include <string.h>

#include <sqlite3.h>

#include <glib/gstrfuncs.h>
#include <glib/gstdio.h>

#include <libsoup/soup.h>

#include <gtk/gtktextbuffer.h>

#include "config.h"
#include "lyrics.h"
#include "utils.h"

static sqlite3 *db = NULL;
static provider_t provider = PROVIDER_LYRCAR;

#define TABLE_CREATION "CREATE TABLE IF NOT EXISTS lyrics " \
                         "(uri TEXT PRIMARY KEY, content TEXT, " \
                        "time DATE);"
#define SELECT_STATEMENT "SELECT content FROM lyrics WHERE " \
                        "uri = ?;"
#define STORE_STATEMENT "INSERT INTO lyrics (uri, content, time) " \
                        "VALUES (?, ?, DATETIME('NOW'));"

gboolean
lyrics_init (void)
{
    gchar *dbpath;
    gchar *error;
    sqlite3_stmt *stmt;
    gint ret;

    dbpath = g_strdup_printf ("%s/%s/%s", g_get_home_dir(), CONFIGDIR,
            "starling.db");

    ret = sqlite3_open (dbpath, &db);

    g_free (dbpath);

    if (SQLITE_OK != ret) {
        g_fprintf (stderr, "Cannot open the database,"
                    "lyrics won't be cached\n");
        sqlite3_close (db);

        db = NULL;

        return FALSE;
    }

    sqlite3_prepare (db, TABLE_CREATION, -1, &stmt, NULL);

    ret = sqlite3_step (stmt);

    sqlite3_finalize (stmt); 
                    
    if (SQLITE_DONE != ret)
        return FALSE;

    return TRUE;
}

void
lyrics_set_provider (provider_t prov)
{
    provider = prov;
}

provider_t
lyrics_get_provider (void)
{
    return provider;
}

gchar *
lyrics_select (const gchar *uri)
{
    sqlite3_stmt *stmt;
    gchar *content;
    gchar *retval;
    gint ret;

    if (!db) {
        return NULL;
    }

    sqlite3_prepare (db, SELECT_STATEMENT, -1, &stmt, NULL);

    sqlite3_bind_text (stmt, 1, uri, -1, NULL);

    ret = sqlite3_step (stmt);
    
    if (ret != SQLITE_ROW) {
        sqlite3_finalize (stmt);
        return NULL;
    }

    /* Definitely, this can't be freed */
    content = (gchar*) sqlite3_column_text (stmt, 0);


    sqlite3_finalize (stmt);

    retval = g_locale_to_utf8 (content, -1, NULL, NULL, NULL);

    return retval;
}

static void
lyrics_write_textview (GtkTextView *view, const gchar *content)
{
    GtkTextBuffer *buffer;
        
    buffer = gtk_text_view_get_buffer (view);
    gtk_text_buffer_set_text (buffer, content, -1);
}

static void
got_lyrics (SoupMessage *msg, gpointer view)
{
    const gchar *uri;
    gchar **lines;
    GString *str;
    gint ii;
    gchar *pos;

    if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
        return;
    
    str = g_string_new ("");
    lines = g_strsplit (msg->response.body, "\n", 4096);
    /* XXX: This is for lyrc.com.ar. This needs reworking when
     * new providers are added.
     */

    /* First line */
    pos = strstr (lines[110], "</font><br>");
    
    if (!pos) {
        /* This is an error page, no lyrics */
        lyrics_write_textview (GTK_TEXT_VIEW (view), _("No lyrics found"));
        return;
    }

    pos += 11; /* strlen("</font><br>) */
    g_string_append_len (str, pos, strlen (pos) - 7);
    g_string_append (str, "\n"); 

    /* Lyrics start at line 111 */
    for (ii = 111; lines[ii]; ii++) {
        if ((pos = strstr (lines[ii], "<p>"))) {
            g_string_append_len (str, lines[ii], pos - lines[ii]);
            break;
        }
        /* Skip <br /> at eol */
        g_string_append_len (str, lines[ii], strlen(lines[ii]) - 7); 
        g_string_append (str, "\n"); 
    }
        
    g_strfreev (lines);    

        
    lyrics_write_textview (GTK_TEXT_VIEW (view), str->str);
    
    uri = soup_uri_to_string (soup_message_get_uri (msg), FALSE);
    lyrics_store (uri, str->str);

    g_string_free (str, TRUE);
}

void
lyrics_display (const gchar *artist, const gchar *title, GtkTextView *view)
{
    gchar *uri;

    uri = lyrics_cook_uri (artist, title);

    g_return_if_fail (uri != NULL);

    lyrics_display_with_uri (uri, view);

    g_free (uri);
}

void
lyrics_display_with_uri (const gchar *uri, GtkTextView *view)
{
    gchar *content;
    SoupSession *session;
    SoupMessage *msg;
    
    content = lyrics_select (uri);
    
    if (content) {
        lyrics_write_textview (view, content);
        
        g_free (content);
        return;
    }

    session = soup_session_async_new ();
    msg = soup_message_new (SOUP_METHOD_GET, uri);

    soup_session_queue_message (session, msg, got_lyrics, view);
}
    
void
lyrics_store (const gchar *uri, const gchar *text)
{
    sqlite3_stmt *stmt;
    gint ret;
    gchar *locale_text;

    if (!db) {
        return;
    }

    sqlite3_prepare (db, STORE_STATEMENT, -1, &stmt, NULL);

    ret = sqlite3_bind_text (stmt, 1, uri, -1, NULL);
    if (ret) {
        return;
    }

    locale_text = g_locale_from_utf8 (text, -1, NULL, NULL, NULL);

    ret = sqlite3_bind_text (stmt, 2, locale_text, -1, NULL);

    g_free (locale_text);
    if (ret) {
        return;
    }

    ret = sqlite3_step (stmt);

    if (ret != SQLITE_DONE) {
        printf("Error storing lyrics: %d\n", ret);
    }

    sqlite3_finalize (stmt);
}

gchar *
lyrics_cook_uri (const gchar *artist, const gchar *title)
{
    gchar *uri;
    gchar *escaped;

    switch (provider) {
        case PROVIDER_LYRCAR:
            uri = g_strdup_printf ("http://lyrc.com.ar/en/tema1en.php?"
                    "artist=%s&songname=%s", artist, title);
            break;
        default:
            uri = NULL;
    }

    escaped = escape_spaces (uri);

    g_free (uri);

    return escaped;
}

