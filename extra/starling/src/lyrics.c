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

#include <libsoup/soup.h>
#include <stdbool.h>

#include <string.h>

#include <sqlite.h>

#include <glib/gstrfuncs.h>
#include <glib/gstdio.h>

#include <libsoup/soup.h>

#include <gtk/gtktextbuffer.h>

#include "config.h"
#include "lyrics.h"
#include "utils.h"

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>

static sqlite *db;
static SoupSession *session;
static char *artist;
static char *title;
static GtkTextView *view;
static int extant;

/* Abort any outstanding requests.  */
static void
abort_extant (void)
{
  if (session)
    {
      extant = 0;
      soup_session_abort (session);
      g_free (artist);
      g_free (title);
      artist = NULL;
      title = NULL;
      view = NULL;
    }
}

/* Lyrics providers */

/* lyrc.com.ar */
static gchar *
lyrcar_cook (const gchar *artist, const gchar *title)
{
  char *a = uri_escape_string (artist);
  char *t = uri_escape_string (title);
    
  char *uri = g_strdup_printf ("http://lyrc.com.ar/en/tema1en.php?"
			       "artist=%s&songname=%s", a, t);

  g_free (a);
  g_free (t);

  return uri;
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
  /* We must have all words with initial upper case letters.  */
  char *a = g_strdup (artist);
  int i;
  for (i = 0; a[i]; i ++)
    if (i == 0 || a[i - 1] == ' ')
      a[i] = g_ascii_toupper (a[i]);

  char *a2 = uri_escape_string (a);
  g_free (a);

  char *t = g_strdup (title);
  for (i = 0; t[i]; i ++)
    if (i == 0 || t[i - 1] == ' ')
      t[i] = g_ascii_toupper (t[i]);

  char *t2 = uri_escape_string (t);
  g_free (t);

  char *uri = g_strdup_printf ("http://lyricwiki.org/%s:%s", a2, t2);

  g_free (a2);
  g_free (t2);

  return uri;
}

static gchar *
lyricwiki_parse (SoupMessage *msg)
{
  const gchar *div = "<div class='lyricbox' >";

  char *start = strstr (msg->response.body, div);
  if (! start)
    return NULL;

  start += strlen (div);

  char *end = strstr (start, "</div>");
  if (! end || end == start)
    return NULL;

  struct obstack text;
  obstack_init (&text);

  while (start < end)
    {
      char *delim = "<br />";
      char *br = strstr (start, delim);
      if (br > end || ! br)
	br = end;

      obstack_grow (&text, start, br - start);
      obstack_1grow (&text, '\n');

      start = br + strlen (delim);
    }

  obstack_1grow (&text, 0);

  char *result = g_strdup (obstack_finish (&text));
  obstack_free (&text, NULL);

  return result;
}

struct
{
    gchar * (*cook) (const gchar *, const gchar *);
    gchar * (*parse) (SoupMessage *);
} providers[] = { 
  {.cook = lyrcar_cook, .parse = lyrcar_parse},
  {.cook = lyricwiki_cook, .parse = lyricwiki_parse}
};

gboolean
lyrics_init (void)
{
  gchar *dbpath;
  dbpath = g_strdup_printf ("%s/%s/%s",
			    g_get_home_dir(), CONFIGDIR, "lyrics");

  char *err = NULL;
  db = sqlite_open (dbpath, 0, &err);

  g_free (dbpath);

  if (err)
    {
      g_warning ("%s: %d: Cannot open the database: %s",
		 __func__, __LINE__, err);
      sqlite_freemem (err);
      return FALSE;
    }

  if (!has_db_table (db, "lyrics"))
    {
      /* If there is an error, it is likely that the table already
	 exists.  We can ignore that.  */
      char *err = NULL;
      sqlite_exec (db,
		   "CREATE TABLE lyrics "
		   "  (artist TEXT, title TEXT, content TEXT, "
		   "   time DATE);",
		   NULL, NULL, &err);
      if (err)
	{
	  g_warning ("%s:%d: creating table: %s", __func__, __LINE__, err);
	  sqlite_freemem (err);
	}
    }

  return TRUE;
}

void
lyrics_finalize (void)
{
  if (db)
    sqlite_close (db);
}

static char *
lyrics_select (const char *artist, const char *title)
{
  if (!db)
    return 0;

  char *lyrics = NULL;
  int callback (void *arg, int argc, char **argv, char **names)
  {
    lyrics = g_strdup (argv[0]);
    return 0;
  }

  char *err = NULL;
  sqlite_exec_printf (db,
		      "select content from lyrics "
		      " where artist = lower ('%q') "
		      "  and title = lower ('%q');",
		      callback, NULL, &err, artist, title);
  if (err)
    {
      g_warning ("%s:%d: selecting: %s", __func__, __LINE__, err);
      sqlite_freemem (err);
    }

  return lyrics;
}

static void
lyrics_write_textview (GtkTextView *view, const gchar *content)
{
    GtkTextBuffer *buffer;
        
    buffer = gtk_text_view_get_buffer (view);
    gtk_text_buffer_set_text (buffer,
			      content && *content
			      ? content : _("No lyrics found."),
			      -1);
}

static void
lyrics_store (const gchar *lyrics)
{
  if (! db)
    return;

  char *err = NULL;
  sqlite_exec_printf (db,
		      "INSERT OR REPLACE INTO lyrics "
		      "  (artist, title, content, time) "
		      " VALUES (lower ('%q'), lower ('%q'),"
		      "         '%q', DATETIME('NOW'));",
		      NULL, NULL, &err,
		      artist, title, lyrics ?: "");
  if (err)
    {
      g_warning ("%s:%d: storing: %s", __func__, __LINE__, err);
      sqlite_freemem (err);
    }
}

static void
got_lyrics (SoupMessage *msg, gpointer provider)
{
  if (msg->status_code == SOUP_STATUS_CANCELLED)
    return;

  gchar *lyrics = NULL;
  if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
    lyrics = providers[(int) provider].parse (msg);

  g_debug ("%d (%d extant): Returned %.20s%s (status: %d)\n",
	   (int) provider, extant,
	   lyrics ?: "nothing!", lyrics ? "..." : "",
	   msg->status_code);

  if (lyrics || extant == 1)
    {
      lyrics_write_textview (GTK_TEXT_VIEW (view), lyrics);
    
      lyrics_store (lyrics);

      abort_extant ();
    }
  else
    extant --;

  g_free (lyrics);
}

void
lyrics_display (const gchar *a, const gchar *t, GtkTextView *v,
		bool try_to_download, bool force_download)
{
  abort_extant ();

  if (! a || ! t)
    /* We need both an artist and a title.  */
    return;

  gchar *content;
  if (! force_download && (content = lyrics_select (a, t)))
    {
      lyrics_write_textview (v, content);
        
      g_free (content);

      return;
    }

  if (! try_to_download)
    {
      char *message = g_strdup_printf (_("%s by %s not in database."),
				       t, a);
      lyrics_write_textview (v, message);
      g_free (message);
      return;
    }

  char *message = g_strdup_printf (_("Downloading lyrics for %s by %s..."),
				   t, a);
  lyrics_write_textview (v, message);
  g_free (message);

  artist = g_strdup (a);
  title = g_strdup (t);
  view = v;

  if (! session)
    session = soup_session_async_new ();

  int i;
  for (i = 0; i < sizeof (providers) / sizeof (providers[0]); i ++)
    {
      char *uri = providers[i].cook (artist, title);
      if (uri)
	{
	  g_debug ("%d: Fetching %s\n", i, uri);

	  SoupMessage *msg = soup_message_new (SOUP_METHOD_GET, uri);
	  g_free (uri);

	  soup_session_queue_message (session, msg, got_lyrics, (gpointer) i);

	  extant ++;
	}
    }
}
