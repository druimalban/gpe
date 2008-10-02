/* lyrics.c - Lyrics support.
   Copyright (C) 2008 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro <skyhusker@handhelds.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include <string.h>

#include <sqlite.h>

#include <glib/gstrfuncs.h>
#include <glib/gstdio.h>

#include <libsoup/soup.h>

#include <gtk/gtktextbuffer.h>

#include "config.h"
#include "lyrics.h"
#include "utils.h"

static sqlite *db = NULL;
static provider_t current_provider = PROVIDER_LYRCAR;

#define TABLE_NAME "lyrics"

#define TABLE_CREATION "CREATE TABLE " TABLE_NAME " " \
                         "(uri TEXT PRIMARY KEY, content TEXT, " \
                        "time DATE);"
#define SELECT_STATEMENT "SELECT content FROM lyrics WHERE " \
                        "uri = ?;"
#define STORE_STATEMENT "INSERT INTO lyrics (uri, content, time) " \
                        "VALUES (?, ?, DATETIME('NOW'));"

/* Lyrics providers */

/* lyrc.com.ar */
static gchar *
lyrcar_cook (const gchar *artist, const gchar *title)
{
    gchar *uri;
    gchar *escaped;
    
    uri = g_strdup_printf ("http://lyrc.com.ar/en/tema1en.php?"
        "artist=%s&songname=%s", artist, title);

    escaped = escape_spaces (uri, "%20");

    g_free (uri);

    return escaped;
}

static gchar *
lyrcar_parse (SoupMessage *msg)
{
    gchar *retval = NULL;
    gchar **lines;
    GString *str;
    gint ii;
    gchar *pos;

    lines = g_strsplit (msg->response.body, "\n", 4096);

    int len = g_strv_length (lines);
    if (len < 130)
      goto out;

    /* First line */
    pos = strstr (lines[129], "</script></td></tr></table>");
    
    if (!pos)
      /* This is an error page, no lyrics */
      goto out;

    if (strlen (pos) < 27)
      goto out;

    pos += 27; /* strlen("</script></td></tr></table>") */
    str = g_string_new ("");
    g_string_append_len (str, pos, strlen (pos) - 7);
    g_string_append (str, "\n"); 

    /* Lyrics start at line 130 */
    for (ii = 130; lines[ii]; ii++) {
    	/* Lyrics end with <p> */
        if ((pos = strstr (lines[ii], "<p>"))) {
            g_string_append_len (str, lines[ii], pos - lines[ii]);
            break;
        }
        /* Skip <br /> at eol */
        g_string_append_len (str, lines[ii], strlen(lines[ii]) - 7); 
        g_string_append (str, "\n"); 
    }
        
        
    retval = str->str;

    g_string_free (str, FALSE);

 out:
    g_strfreev (lines);    
    return retval;
}

/* lyricwiki.org */
gchar *
lyricwiki_cook (const gchar *artist, const gchar *title)
{
    gchar *uri;
    gchar *escaped;
    
    uri = g_strdup_printf ("http://lyricwiki.org/%s:%s", artist, title);

    escaped = escape_spaces (uri, "_");

    g_free (uri);

    return escaped;
}

static gchar *
lyricwiki_parse (SoupMessage *msg)
{
    const gchar *delim = "<div id=\"lyric\">";
    gchar **lines;
    gchar *line = NULL;
    gchar *retval;
    GString *str;
    gint ii;

    lines = g_strsplit (msg->response.body, "\n", 4096);

    for (ii = 0; lines[ii]; ii++) {
        if (g_str_has_prefix (lines[ii], delim)) {
            line = g_strdup (lines[ii]);
            break;
        }
    }

    g_strfreev (lines);

    if (!line)
        return NULL;

    lines = g_strsplit (line + strlen (delim), "<br />", 4096);

    g_free (line);

    str = g_string_new ("");

    for (ii = 0; lines[ii]; ii++) {
        g_string_append (str, lines[ii]);
        g_string_append (str, "\n");
    }

    g_strfreev (lines);

    /* Remove ending </div> */
    g_string_erase (str, str->len - 7, -1);

    retval = str->str;

    g_string_free (str, FALSE);

    return retval;
}

Provider providers[] = { 
        {.cook = lyrcar_cook, .parse = lyrcar_parse},
        {.cook = lyricwiki_cook, .parse = lyricwiki_parse}
        };

gboolean
lyrics_init (void)
{
    gchar *dbpath;
    sqlite_vm *vm;
    gint ret;

    dbpath = g_strdup_printf ("%s/%s/%s", g_get_home_dir(), CONFIGDIR,
            "starling.db");

    db = sqlite_open (dbpath, 0, NULL);

    g_free (dbpath);

    if (!db) {
        g_fprintf (stderr, "Cannot open the database,"
                    "lyrics won't be cached\n");
        return FALSE;
    }

    if (!has_db_table (db, TABLE_NAME)) {
        ret = sqlite_compile (db, TABLE_CREATION, NULL, &vm, NULL);

        ret = sqlite_step (vm, NULL, NULL, NULL);

        sqlite_finalize (vm, NULL); 
                    
        if (SQLITE_DONE != ret) {
            return FALSE;
        }
    }

    return TRUE;
}

void
lyrics_finalize (void)
{
    g_return_if_fail (db != NULL);

    sqlite_close (db);
}

void
lyrics_set_provider (provider_t prov)
{
    current_provider = prov;
}

provider_t
lyrics_get_provider (void)
{
    return current_provider;
}

int
lyrics_select (const gchar *uri, char **content)
{
    sqlite_vm *vm;
    gint ret;
    const gchar **sqlout;

    if (!db)
      return 0;

    sqlite_compile (db, SELECT_STATEMENT, NULL, &vm, NULL);

    sqlite_bind (vm, 1, uri, -1, FALSE);

    ret = sqlite_step (vm, NULL, &sqlout, NULL);
    
    if (ret != SQLITE_ROW) {
        sqlite_finalize (vm, NULL);
        return 0;
    }

    if (sqlout[0])
      *content = g_strdup (sqlout[0]);
    else
      *content = NULL;

    sqlite_finalize (vm, NULL);

    return 1;
}

static void
lyrics_write_textview (GtkTextView *view, const gchar *content)
{
    GtkTextBuffer *buffer;
        
    buffer = gtk_text_view_get_buffer (view);
    gtk_text_buffer_set_text (buffer, content ?: _("No lyrics found."), -1);
}

void
lyrics_display (const gchar *artist, const gchar *title, GtkTextView *view,
		bool try_to_download)
{
    gchar *uri;

    uri = lyrics_cook_uri (artist, title);

    g_return_if_fail (uri != NULL);

    lyrics_display_with_uri (uri, view, try_to_download);

    g_free (uri);
}

static void
got_lyrics (SoupMessage *msg, gpointer view)
{
    gchar *lyrics = NULL;

    //write (0, msg->response.body, msg->response.length);

    if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
        lyrics = providers[current_provider].parse (msg);

    lyrics_write_textview (GTK_TEXT_VIEW (view), lyrics);
    
    char *uri = soup_uri_to_string (soup_message_get_uri (msg), FALSE);
    lyrics_store (uri, lyrics);
    g_free (uri);

    g_free (lyrics);
}

void
lyrics_display_with_uri (const gchar *uri, GtkTextView *view,
			 bool try_to_download)
{
    gchar *content;
    SoupSession *session;
    SoupMessage *msg;
    
    if (lyrics_select (uri, &content))
      {
        lyrics_write_textview (view, content);
        
        g_free (content);
        return;
      }

    if (! try_to_download)
      {
        lyrics_write_textview (view, NULL);
	return;
      }

    session = soup_session_async_new ();
    msg = soup_message_new (SOUP_METHOD_GET, uri);

    g_debug ("Fetching %s\n", uri);

    soup_session_queue_message (session, msg, got_lyrics, view);
}
    
void
lyrics_store (const gchar *uri, const gchar *text)
{
    sqlite_vm *vm;
    gint ret;

    if (!db) {
        return;
    }

    sqlite_compile (db, STORE_STATEMENT, NULL, &vm, NULL);

    ret = sqlite_bind (vm, 1, uri, -1, FALSE);
    if (ret)
      goto out;

    ret = sqlite_bind (vm, 2, text, -1, FALSE);
    if (ret)
      goto out;

    ret = sqlite_step (vm, NULL, NULL, NULL);
    
 out:;
    char *err = NULL;
    sqlite_finalize (vm, &err);
    if (err)
      {
        printf ("Error storing lyrics: %d: %s\n", ret, err);
	sqlite_freemem (err);
      }
}

gchar *
lyrics_cook_uri (const gchar *artist, const gchar *title)
{
    if (current_provider >= PROVIDER_INVALID || !artist || !title)
        return NULL;

    return providers[current_provider].cook (artist, title);
}


