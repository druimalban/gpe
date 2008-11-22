/* lastfm.c - lastfm implementation.
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

#include <gtk/gtklabel.h>

#include <libsoup/soup.h>

#include "config.h"
#include "errorbox.h"
#include "utils.h"
#include "md5sum.h"
#include <time.h>

#define LASTFM_MAX_ITEMS_PER_SUBMISSION "10"

typedef struct {
  gchar *sessionid;
  gchar *nowplaying_uri;
  gchar *post_uri;
} lastfm_data;

static sqlite *db;
#define TABLE_NAME "lastfm"

static gint last_id_sent;

static SoupSession *session;
static int extant;

static char *user;
static char *password;
static GtkLabel *label;
static int auto_submit;

/* Forward declarations */
static void lastfm_delete_by_id (gint id);
static gint lastfm_submit_real_helper (gpointer data, gint argc, gchar **argv, gchar **col);
static gboolean lastfm_submit_real (lastfm_data *data);
static void got_handshake_response (SoupMessage *msg, gpointer arg);
static void submit (void);
static gint lastfm_count (void) __attribute__ ((pure));

static void
update_status (void)
{
  if (! label)
    return;

  int count = lastfm_count ();
  gchar *text;
  if (extant)
    text = g_strdup_printf (_("Submitting %d tracks..."), count);
  else
    text = g_strdup_printf (_("%d tracks pending"), count);

  gtk_label_set_text (label, text);

  g_free (text);
}

static void
lastfm_delete_by_id (gint id)
{
  if (! db)
    return;

  char *err = NULL;
  sqlite_exec_printf (db,
		      "DELETE FROM " TABLE_NAME " WHERE rowid <= %d;",
		      NULL, NULL, &err, id);
  if (err)
    {
      g_warning ("%s:%d: deleting rows: %s",
		 __func__, __LINE__, err);
      sqlite_freemem (err);
    }
}

static gboolean
lastfm_create_table (sqlite *db)
{
  char *err = NULL;
  sqlite_exec (db,
	       "CREATE TABLE " TABLE_NAME " "
	       " (artist VARCHAR(50),"
	       "  title VARCHAR(50),"
	       "  length INT, time DATE);",
	       NULL, NULL, &err);
  if (err)
    {
      g_warning ("%s:%d: creating table: %s",
		 __func__, __LINE__, err);
      sqlite_freemem (err);
    }

  return err == 0;
}

static gint
lastfm_count (void)
{
  if (! db)
    return 0;

  int count = 0;
  int callback (void *arg, int argc, char **argv, char **names)
  {
    count = atoi (argv[0]);
    return 0;
  }

  char *err = NULL;
  sqlite_exec (db,
	       "SELECT COUNT(*) FROM " TABLE_NAME ";",
	       callback, NULL, &err);
  if (err)
    {
      g_warning ("%s:%d: selecting: %s", __func__, __LINE__, err);
      sqlite_freemem (err);
    }

  return count;
}

static void
submit (void)
{
  if (extant)
    /* Already working...  */
    return;

  /* Create protocol V1.2 authentication token
     token := md5(md5(password) + timestamp) */
  time_t t = time (NULL);
  char *md5pwd = md5sum (password);
  char *md5andtime = g_strdup_printf ("%s%lu",
				      md5pwd, (unsigned long) t);
  g_free (md5pwd);
  char *token = md5sum (md5andtime);
  g_free (md5andtime);

  char *uri = g_strdup_printf ("http://post.audioscrobbler.com/"
			       "?hs=true&p=1.2&c=gpe"
			       "&v=" PACKAGE_VERSION "&u=%s&t=%lu&a=%s",
			       user, (unsigned long) t, token);
  g_free (token);

  SoupMessage *msg = soup_message_new (SOUP_METHOD_GET, uri);
  g_free (uri);

  session = soup_session_async_new ();
  soup_session_queue_message (session, msg, got_handshake_response, NULL);
}

void
lastfm_submit (void)
{
  if (lastfm_count () == 0)
    {
      starling_error_box (_("No tracks pending."));
      return;
    }

  submit ();
}

static void
maybe_submit (void)
{
  if (! extant && auto_submit && lastfm_count () >= auto_submit)
    submit ();
}


void
lastfm_enqueue (const gchar *artist, const gchar *title, gint length)
{
  if (! (artist && title && db))
    return;

  char *err = NULL;
  sqlite_exec_printf (db,
		      "INSERT INTO " TABLE_NAME
		      "  (artist, title, length, time)"
		      "  VALUES ('%q', '%q', %d, DATETIME('NOW'));",
		      NULL, NULL, &err,
		      artist, title, length);
  if (err)
    {
      g_warning ("%s:%d: inserting: %s", __func__, __LINE__, err);
      sqlite_freemem (err);
    }

  update_status ();

  maybe_submit ();
}

static void
got_submission_response (SoupMessage *msg, gpointer arg)
{
  lastfm_data *data = arg;

  if (msg->status_code == SOUP_STATUS_CANCELLED)
    /* We were cancelled.  Retry.  */
    {
      submit ();
      goto out;
    }

  if (! SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
    goto out;

  if (strncmp (msg->response.body, "OK\n", 3) == 0)
    {
      lastfm_delete_by_id (last_id_sent);
      last_id_sent = 0;

      update_status ();

      if (lastfm_count ())
	{
	  /* There are more tracks to send.  */
	  g_timeout_add (1000, (GSourceFunc) lastfm_submit_real, data);
	  return;
        } 
    }
  else
    /* command is FAILED <reason> */
    starling_error_box_fmt (_("Lastfm submission failed: %s"),
			    strchr (msg->response.body, ' ')
			    ?: msg->response.body);

  /* If we reach this point, we're done with the submission */
  extant = 0;

 out:
  update_status ();

  g_free (data->sessionid);
  g_free (data->post_uri);
  g_free (data->nowplaying_uri);
  g_free (data);
}

static gboolean
lastfm_submit_real (lastfm_data *data)
{
  gint n = 0;
  GString *str = g_string_new ("s=");
  g_string_append (str, data->sessionid);

  gint helper (gpointer data, gint argc, gchar **argv, gchar **col)
  {
    char *artist = soup_uri_encode (argv[1],"&+; ");
    char *title = soup_uri_encode (argv[2],"&+; ");
    char *chunk = g_strdup_printf ("&a[%d]=%s&t[%d]=%s&b[%d]=&m[%d]=&l[%d]=%s&i[%d]=%s&n[%d]=&r[%d]=&o[%d]=P",
				   n, artist, n, title, n, n, n,
				   argv[3], n, argv[4], n, n, n);
    g_free(artist);
    g_free(title);

    //g_message("Chunk = %s\n", chunk);

    g_string_append (str, chunk);
    g_free (chunk);

    n++;

    last_id_sent = atoi (argv[0]);

    return 0;
  }

  char *err = NULL;
  sqlite_exec (db,
	       "SELECT rowid, artist, title, length, strftime(\"%s\",time) "
	       " FROM " TABLE_NAME " ORDER BY ROWID"
	       " LIMIT " LASTFM_MAX_ITEMS_PER_SUBMISSION ";",
	       helper, NULL, &err);
  if (err)
    {
      g_warning ("%s:%d: selecting: %s", __func__, __LINE__, err);
      sqlite_freemem (err);
    }

  SoupMessage *msg = soup_message_new (SOUP_METHOD_POST, data->post_uri);

  msg->request.body = str->str;
  msg->request.length = str->len;
  msg->request.owner = SOUP_BUFFER_SYSTEM_OWNED;
  g_string_free (str, FALSE);

  soup_message_add_header (msg->request_headers, "Content-Type", 
			   "application/x-www-form-urlencoded");

  soup_session_queue_message (session, msg, got_submission_response, data);

  /* Don't call me again */
  return FALSE;
}

static void
got_handshake_response (SoupMessage *msg, gpointer arg)
{
  gchar *command;
  gchar **lines;

  g_return_if_fail (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code));

  //g_message("Handshake response = %s\n", msg->response.body);

  lines = g_strsplit (msg->response.body, "\n", 5);

  command = lines[0];

  if (g_str_equal (command, "OK") && lines[1] && lines[2] && lines[3])
    {
      lastfm_data *data = g_new0 (lastfm_data, 1);
      data->sessionid = g_strdup(lines[1]);
      data->nowplaying_uri = g_strdup(lines[2]);
      data->post_uri = g_strdup(lines[3]);
      //g_message("last.fm post URI = %s\n",data->post_uri);

      lastfm_submit_real (data);
    }
  else if (g_str_equal (command, "BADAUTH"))
    starling_error_box (_("Your last.fm password is wrong."));
  else if (g_str_equal (command, "BANNED"))
    starling_error_box (_("This version of this software has been banned. "
			  "Please upgrade to a newer version."));
  else if (g_str_equal (command, "BADTIME"))
    starling_error_box (_("The system clock is too inaccurate.  "
			  "Please correct the system time."));
  else
    /* command is FAILED <reason> */
    starling_error_box_fmt (_("The submission failed with the "
			      "following reason: %s."),
			    strstr (command, " "));

  g_strfreev (lines);
}

void
lastfm_user_data_set (const char *u, const char *p, int a)
{
  if (user)
    g_free (user);
  user = g_strdup (u);
  if (password)
    g_free (password);
  password = g_strdup (p);

  auto_submit = a;

  if (extant)
    /* Abort any outstanding requests.  */
    soup_session_abort (session);
  else
    maybe_submit ();
}

gboolean
lastfm_init (const char *user, const char *password,
	     int auto_submit, GtkLabel *status)
{
  if (! db)
    {
      char *dbpath = g_strdup_printf ("%s/%s/%s",
				      g_get_home_dir(), CONFIGDIR, "lastfm");

      char *err = NULL;
      db = sqlite_open (dbpath, 0, &err);
      if (err)
	{
	  g_warning ("%s:%d: opeing %s: %s",
		     __func__, __LINE__, dbpath, err);
	  sqlite_freemem (err);
	}

      g_free (dbpath);

      if (! db)
	return FALSE;

      if (! has_db_table (db, TABLE_NAME))
	lastfm_create_table (db);
    }

  update_status ();

  lastfm_user_data_set (user, password, auto_submit);
  label = status;
  update_status ();

  return TRUE;
}
