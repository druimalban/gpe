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

#include <sqlite.h>

#include <gtk/gtklabel.h>

#include <libsoup/soup.h>

#include "config.h"
#include "errorbox.h"
#include "starling.h"
#include "utils.h"
#include <time.h>

#define TABLE_NAME "lastfm"

#define CREATE_STATEMENT "CREATE TABLE " TABLE_NAME " " \
                        "(artist VARCHAR(50), " \
                        "title VARCHAR(50), " \
                        "length INT, time DATE);"
#define COUNT_STATEMENT "SELECT COUNT(rowid) FROM " TABLE_NAME ";"

#define LASTFM_MAX_ITEMS_PER_SUBMISSION 10

#define SELECT_STATEMENT "SELECT rowid, artist, title, length, strftime(\"%s\",time) " \
                        "FROM " TABLE_NAME " LIMIT 10;"

#define STORE_STATEMENT "INSERT INTO " TABLE_NAME " (artist, title, length, " \
                        "time) VALUES (?, ?, ?, DATETIME('NOW'));"

#define DELETE_STATEMENT "DELETE FROM " TABLE_NAME " WHERE rowid <= ?;"

#define HANDSHAKE_URI "http://post.audioscrobbler.com/" \
                        "?hs=true&p=1.2&c=gpe&v=0.1&u=%s&t=%lu&a=%s"


typedef struct {
  gchar *sessionid;
  gchar *nowplaying_uri;
  gchar *post_uri;
  GtkWidget *label;
} lastfm_data;

typedef struct {
    gint n;
    GString *str;
} lastfm_helper_data;

static sqlite *db = NULL;

static gint last_id_sent = 0;

/* Forward declarations */
static void lastfm_show_count (GtkLabel *label, gint count);
static void lastfm_delete_by_id (gint id);
static gint lastfm_submit_real_helper (gpointer data, gint argc, gchar **argv, gchar **col);
static gboolean lastfm_submit_real (lastfm_data *data);
static void got_handshake_response (SoupMessage *msg, gpointer arg);

static void
lastfm_show_count (GtkLabel *label, gint count)
{
    gchar *text;

    text = g_strdup_printf (_("%d tracks pending"), count);

    gtk_label_set_text (label, text);

    g_free (text);
}

static void
lastfm_delete_by_id (gint id)
{
    sqlite_vm *vm;
    gint ret;

    g_return_if_fail (db != NULL);

    sqlite_compile (db, DELETE_STATEMENT, NULL, &vm, NULL);
    sqlite_bind_int (vm, 1, id);

    ret = sqlite_step (vm, NULL, NULL, NULL);

    if (ret != SQLITE_DONE) {
        g_warning ("Error removing track %d from the queue.\n", id);
    }

    sqlite_finalize (vm, NULL);
}

gint lastfm_create_table (sqlite *db)
{
  gint ret;
  sqlite_vm *vm;

  sqlite_compile (db, CREATE_STATEMENT, NULL, &vm, NULL);

  ret = sqlite_step (vm, NULL, NULL, NULL);

  sqlite_finalize (vm, NULL); 
  
  return ret;
}

gboolean
lastfm_init (Starling *st)
{
    gchar *dbpath;
    gint ret;

    dbpath = g_strdup_printf ("%s/%s/%s", g_get_home_dir(), CONFIGDIR,
            "starling.db");

    db = sqlite_open (dbpath, 0, NULL);

    g_free (dbpath);

    g_return_val_if_fail (db != NULL, FALSE);

    if (!has_db_table (db, TABLE_NAME)) {
      ret = lastfm_create_table(db);
                    
      if (SQLITE_DONE != ret)
	return FALSE;
    }

    //lastfm_show_count (GTK_LABEL (st->web_count), lastfm_count());

    return TRUE;
}


void
lastfm_enqueue (const gchar *artist, const gchar *title, gint length, Starling *st)
{
    sqlite_vm *vm;
    gint ret;

    if (! (artist && title && db))
      return;

    sqlite_compile (db, STORE_STATEMENT, NULL, &vm, NULL);
    sqlite_bind (vm, 1, artist, -1, 0);
    sqlite_bind (vm, 2, title, -1, 0);
    sqlite_bind_int (vm, 3, length);

    ret = sqlite_step (vm, NULL, NULL, NULL);
    /* XXX: Check ret */

    sqlite_finalize (vm, NULL);
    
#if 0
    lastfm_show_count (GTK_LABEL (st->web_count), lastfm_count());
#endif
}

gint
lastfm_count (void)
{
    sqlite_vm *vm;
    const gchar **sqlout;
    gint count;

    g_return_val_if_fail (db != NULL, 0);

    sqlite_compile (db, COUNT_STATEMENT, NULL, &vm, NULL);

    sqlite_step (vm, NULL, &sqlout, NULL);

    count = atoi (sqlout[0]);

    sqlite_finalize (vm, NULL);

    return count;
}

static gint
lastfm_submit_real_helper (gpointer data, gint argc, gchar **argv, gchar **col)
{
    gchar *chunk;
    lastfm_helper_data *d = data;

    gchar *artist = soup_uri_encode(argv[1],"&+; ");
    gchar *title = soup_uri_encode(argv[2],"&+; ");
    chunk = g_strdup_printf ("&a[%d]=%s&t[%d]=%s&b[%d]=&m[%d]=&l[%d]=%s&i[%d]=%s&n[%d]=&r[%d]=&o[%d]=P",
        d->n, artist, d->n, title, d->n, d->n, d->n, argv[3], d->n, argv[4], d->n, d->n, d->n);
    g_free(artist);
    g_free(title);

    //g_message("Chunk = %s\n", chunk);

    d->n++;

    last_id_sent = atoi (argv[0]);

    g_string_append (d->str, chunk);

    g_free (chunk);
    
    return 0;
}

static void
got_submission_response (SoupMessage *msg, gpointer arg)
{
    gchar **lines;
    gchar *command;
    lastfm_data *data = arg;

    lines = g_strsplit (msg->response.body, "\n", 2);
    
    command = lines[0];

    if (g_str_equal (command, "OK")) {
        lastfm_delete_by_id (last_id_sent);
        last_id_sent = 0;

        if (lastfm_count()) {
            /* There are more tracks to send */

            g_timeout_add (1000, (GSourceFunc) lastfm_submit_real,
                    data);

            g_strfreev (lines);
            return;
        } 
    } else { /* command is FAILED <reason> */
        starling_error_box_fmt (_("The submission failed with the "
                "following reason: %s."), strstr (command, " "));
    }

    /* If we reach this point, we're done with the submission */
    lastfm_show_count(GTK_LABEL (data->label), lastfm_count());
    
    g_free (data->sessionid);
    g_free (data->post_uri);
    g_free (data->nowplaying_uri);
    g_free (data);
    g_strfreev (lines);
}

static gboolean
lastfm_submit_real (lastfm_data *data)
{
    SoupSession *session;
    SoupMessage *msg;
    GString *str;
    gchar *auth;
    lastfm_helper_data *d;

    str = g_string_new ("");
    d = g_new0 (lastfm_helper_data, 1);
    d->str = str;
    d->n = 0;

    auth = g_strdup_printf ("s=%s", data->sessionid);
    g_string_append (str, auth);
    g_free (auth);

    sqlite_exec (db, SELECT_STATEMENT, lastfm_submit_real_helper, d, NULL);

    g_free (d);
    //g_message("last.fm submission = %s\n",str->str);

    msg = soup_message_new (SOUP_METHOD_POST, data->post_uri);

    msg->request.body = str->str;
    msg->request.length = str->len;
    msg->request.owner = SOUP_BUFFER_SYSTEM_OWNED;
    g_string_free (str, FALSE);

    soup_message_add_header (msg->request_headers, "Content-Type", 
            "application/x-www-form-urlencoded");

    session = soup_session_async_new ();
    soup_session_queue_message (session, msg, got_submission_response, data);

    /* Don't call me again */
    return FALSE;
}

static void
got_handshake_response (SoupMessage *msg, gpointer arg)
{
    gchar *command;
    gchar **lines;
    lastfm_data *data = arg;

    g_return_if_fail (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code));

    //g_message("Handshake response = %s\n", msg->response.body);

    lines = g_strsplit (msg->response.body, "\n", 5);

    command = lines[0];

    if (g_str_equal (command, "OK")) {
      data->sessionid = g_strdup(lines[1]);
      data->nowplaying_uri = g_strdup(lines[2]);
      data->post_uri = g_strdup(lines[3]);
      //g_message("last.fm post URI = %s\n",data->post_uri);

      g_timeout_add (1000, (GSourceFunc) lastfm_submit_real, data);
    } else if (g_str_equal (command, "BADAUTH")) {
        starling_error_box (_("Your last.fm password is wrong."));
    } else if (g_str_equal (command, "BANNED")) {
        starling_error_box (_("This version of this software has been banned. Please upgrade to a newer version."));
    } else if (g_str_equal (command, "BADTIME")) {
        starling_error_box (_("The system clock is too inaccurate.  Please correct the system time."));
    } else { /* command is FAILED <reason> */
        starling_error_box_fmt (_("The submission failed with the "
                "following reason: %s."), strstr (command, " "));
    }

    g_strfreev (lines);
}

static gchar * lastfm_md5(const gchar *string)
{
  /* From Audioscrobbler spec:
     The md5() function takes a string and returns the 32-byte ASCII hexadecimal 
     representation of the MD5 hash, using lower case characters for the hex values.
  */

  return g_compute_checksum_for_data (G_CHECKSUM_MD5,
				      (guint8 *) string, strlen (string));
}

void
lastfm_submit (const gchar *username, const gchar *passwd, Starling *st)
{
    SoupSession *session;
    SoupMessage *msg;
    lastfm_data *data;
    gchar *uri;
    gchar *md5pwd, *md5andtime, *token;
    time_t t = time(NULL);

    if (lastfm_count() == 0)
      {
      starling_error_box (_("No tracks pending to be sent."));
      return;
      }

    /* Create protocol V1.2 authentication token
       token := md5(md5(password) + timestamp) */
    md5pwd = lastfm_md5(passwd);
    md5andtime = g_strdup_printf("%s%lu", md5pwd, (unsigned long)t);
    g_free(md5pwd);
    token = lastfm_md5(md5andtime);
    g_free(md5andtime);    

    data = g_new0 (lastfm_data, 1);
    uri = g_strdup_printf (HANDSHAKE_URI, username, (unsigned long)t, token);
    g_free(token);

    session = soup_session_async_new ();
    msg = soup_message_new (SOUP_METHOD_GET, uri);

#if 0
    data->label = st->web_count;
#endif

    soup_session_queue_message (session, msg, got_handshake_response, (gpointer) data);

    g_free (uri);
}


        
