/* calendar-update.c - Calendar update implementation.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <gtk/gtk.h>
#include <string.h>
#include <libsoup/soup.h>
#include "calendar-update.h"
#include "import-vcal.h"
#include "export-vcal.h"
#include "globals.h"

/* So, libmimedir can read from either files or from GIOChannels.  We
   need the ability to pass a buffer.  It would be nice if there was
   native support for providing an in memory buffer as a data source
   (as opposed to a file) a la fmemopen but that doesn't seem to be
   the case.  So here is a cheap implementation.  */
struct GIOBufferChannel
{
  GIOChannel giochannel;

  gint32 pos;
  gint32 length;
  char *buffer;
};
typedef struct GIOBufferChannel GIOBufferChannel;

static GIOStatus
g_io_buffer_read (GIOChannel *channel, gchar *buf, gsize count,
		  gsize *bytes_read, GError **err)
{
  GIOBufferChannel *b = (GIOBufferChannel *) channel;

  if (b->pos + count > b->length)
    count = b->length - b->pos;
  *bytes_read = count;

  if (count == 0)
    return G_IO_STATUS_EOF;

  memcpy (buf, b->buffer + b->pos, count);
  b->pos += count;

  return G_IO_STATUS_NORMAL;
}

static GIOStatus
g_io_buffer_seek (GIOChannel *channel, gint64 offset, GSeekType type,
		  GError **err)
{
  GIOBufferChannel *b = (GIOBufferChannel *) channel;

  switch (type)
    {
    case G_SEEK_SET:
      b->pos = MIN (offset, b->length);
      break;
    case G_SEEK_CUR:
      b->pos = MIN (b->pos + offset, b->length);
      break;
    case G_SEEK_END:
      b->pos = MAX (b->length - offset, 0);
      break;
    default:
      g_assert_not_reached ();
    }

  return G_IO_STATUS_NORMAL;
}

static GIOStatus
g_io_buffer_close (GIOChannel *channel, GError **err)
{
  return G_IO_STATUS_NORMAL;
}

static void 
g_io_buffer_free (GIOChannel *channel)
{
  g_free (channel);
}

static GSource *
g_io_buffer_create_watch (GIOChannel *channel, GIOCondition condition)
{
  /* XXX: Do we need this?  Seems not.  */
  g_assert_not_reached ();
}

static GIOStatus
g_io_buffer_set_flags (GIOChannel *channel, GIOFlags flags, GError **err)
{
  /* Ignore.  */
  return G_IO_STATUS_NORMAL;
}

static GIOFlags
g_io_buffer_get_flags (GIOChannel *channel)
{
  return G_IO_FLAG_IS_SEEKABLE | G_IO_FLAG_IS_READABLE;
}

static GIOChannel *
g_io_channel_from_buffer (char *buffer, int length)
{
  static GIOFuncs buffer_channel_funcs =
    {
      g_io_buffer_read,
      NULL,
      g_io_buffer_seek,
      g_io_buffer_close,
      g_io_buffer_create_watch,
      g_io_buffer_free,
      g_io_buffer_set_flags,
      g_io_buffer_get_flags,
    };

  struct GIOBufferChannel *b = g_malloc0 (sizeof (*b));
  GIOChannel *channel = (GIOChannel *) b;

  g_io_channel_init (channel);
  /* This is private and all but, ah, how else are we supposed to get
     at it?!  */
  channel->funcs = &buffer_channel_funcs;
  channel->is_seekable = TRUE;
  channel->is_readable = TRUE;

  b->buffer = buffer;
  b->length = length;

  return channel;
}

static GtkDialog *info;
static GtkLabel *info_label;

static void
show_info (const char *fmt, ...)
{
  if (! info)
    {
      info = GTK_DIALOG (gtk_dialog_new_with_buttons
			 (_("Calendar Syncronization"),
			  GTK_WINDOW (main_window),
			  GTK_DIALOG_DESTROY_WITH_PARENT,
			  GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			  NULL));
      g_signal_connect (G_OBJECT (info),
			"response", G_CALLBACK (gtk_widget_destroy), NULL);
      g_object_add_weak_pointer (G_OBJECT (info), (gpointer *) &info);

      info_label = GTK_LABEL (gtk_label_new (NULL));
      gtk_label_set_line_wrap (info_label, TRUE);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (info)->vbox),
			  GTK_WIDGET (info_label), FALSE, FALSE, 0);
      gtk_widget_show (GTK_WIDGET (info_label));
    }

  va_list ap;
  va_start (ap, fmt);
  char *message = g_strdup_vprintf (fmt, ap);
  va_end (ap);

  const char *text = gtk_label_get_text (info_label);
  if (text && *text)
    {
      char *s = g_strjoin ("\n", text, message, NULL);
      gtk_label_set_text (info_label, s);
      g_free (s);
    }
  else
    gtk_label_set_text (info_label, message);

  g_free (message);

  gtk_widget_show (GTK_WIDGET (info));
}

static void
authenticate (SoupSession *session, SoupMessage *msg,
	      gchar *auth_type, gchar *auth_realm,
	      char **username, char **password, gpointer user_data) 
{
  EventCalendar *ec = EVENT_CALENDAR (user_data);
  *username = event_calendar_get_username (ec);
  *password = event_calendar_get_password (ec);
}

enum
  {
    BAD_NOTBAD,
    BAD_URL,
    BAD_PARSE,
  };

struct info
{
  /* With a reference.  */
  EventCalendar *ec;

#define UPDATE_HORIZON (5 * 60)
  /* When a calendar which publishes its changes (mode == 2) becomes
     aware that an update is required, we wait some time before
     initiating the update.  This is good as it allows changes to
     coalesce.  0 means no update required, non-zero then the update
     should occur.  */
  time_t update_at;

  /* The last server/libsoup error code: if we keep getting the same
     error message, we only show the user the first time.  */
  guint last_status_code;

  /* Last internal error.  */
  guint last_internal_code;

  /* If non-zero, indicates that the last update ended in failure and
     that this is when we should retry.  */
  time_t retry;
  /* We use exponential back off to figure out when to retry a failed
     message.  First we try again in 60 seconds then keep doubling for
     a maximum interval of 24 hours.  */
#define RETRY_FLOOR 60
#define RETRY_CEILING 24 * 60 * 60
  int last_retry_inc;

  /* If this calendar is currently sync'ing.  */
  gboolean busy;
};

static GSList *calendars;

static struct info *
info_new (EventCalendar *ec)
{
  struct info *info = g_malloc0 (sizeof (*info));
  info->ec = ec;
  g_object_ref (ec);

  calendars = g_slist_prepend (calendars, info);

  return info;
}

static void
info_remove (struct info *info)
{
  calendars = g_slist_remove (calendars, info);
  g_object_unref (info->ec);
  g_free (info);
}

static struct info *
info_find (EventCalendar *ec, int alloc)
{
  GSList *l;
  for (l = calendars; l; l = l->next)
    {
      struct info *info = l->data;
      if (info->ec == ec)
	return info;
    }

  if (alloc)
    return info_new (ec);
  return NULL;
}

static gboolean refresh (gpointer data);

static void
need_retry (struct info *info)
{
  if (! info->retry)
    info->last_retry_inc = RETRY_FLOOR;
  else
    info->last_retry_inc = MIN (RETRY_CEILING, info->last_retry_inc * 2);

  info->retry = time (NULL) + info->last_retry_inc;

  refresh (NULL);
}

static void
pushed (SoupMessage *msg, gpointer data)
{
  struct info *info = data;

  if (! SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
    {
      if (msg->status_code != info->last_status_code)
	{
	  char *url = event_calendar_get_url (info->ec);
	  show_info ("%s: %d %s", url, msg->status_code, msg->reason_phrase);
	  g_free (url);
	  info->last_status_code = msg->status_code;
	}
      need_retry (info);
    }
  else
    {
      event_calendar_set_last_push (info->ec, time (NULL));
      info->update_at = 0;
      info->last_status_code = 0;
      info->last_internal_code = 0;
    }

  g_object_unref (info->ec);
  info->busy = FALSE;
}

static gboolean
build_message (SoupSession **session, SoupMessage **msg,
	       const char *method, struct info *info)
{
  *session = soup_session_async_new ();
  g_signal_connect (G_OBJECT (*session), "authenticate",
		    G_CALLBACK (authenticate), info->ec);

  char *url = event_calendar_get_url (info->ec);
  *msg = soup_message_new (method, url);
  if (! msg)
    {
      if (info->last_internal_code != BAD_URL)
	{
	  char *title = event_calendar_get_title (info->ec);
	  show_info ("%s: Invalid URL: %s", title, url);
	  g_free (title);
	}
      info->last_internal_code = BAD_URL;
      g_free (url);
      need_retry (info);
      return FALSE;
    }
  g_free (url);

  return TRUE;
}

static void
calendar_push_internal (struct info *info)
{
  info->busy = TRUE;

  SoupSession *session;
  SoupMessage *msg;

  if (! build_message (&session, &msg, SOUP_METHOD_PUT, info))
    return;

  g_object_ref (info->ec);

  char *s = export_calendar_as_string (info->ec);
  soup_message_set_request (msg, "text/calendar",
			    SOUP_BUFFER_SYSTEM_OWNED, s, strlen (s));

  soup_session_queue_message (session, msg, pushed, info);
}

void
calendar_push (EventCalendar *ec)
{
  g_return_if_fail (event_calendar_get_mode (ec) == 2);

  struct info *info = info_find (ec, TRUE);
  /* This was user initiated.  If there is an error, show the user
     again.  */
  info->last_status_code = 0;
  info->last_internal_code = 0;
  info->retry = 0;

  calendar_push_internal (info);
}

static void
pulled (SoupMessage *msg, gpointer data)
{
  struct info *info = data;

  if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
    {
      if (msg->status_code != info->last_status_code)
	{
	  char *url = event_calendar_get_url (info->ec);
	  show_info ("%s: %d %s", url, msg->status_code, msg->reason_phrase);
	  g_free (url);
	  info->last_status_code = msg->status_code;
	}
      need_retry (info);
    }
  else
    {
      GIOChannel *channel = g_io_channel_from_buffer (msg->response.body,
						      msg->response.length);
      char *status = import_vcal_from_channel (info->ec, channel);
      g_io_channel_unref (channel);

      if (status && *status)
	{
	  if (info->last_internal_code != BAD_PARSE)
	    {
	      char *title = event_calendar_get_title (info->ec);
	      show_info ("Error updating calendar %s: %s.", title, status);
	      g_free (title);
	      info->last_internal_code = BAD_PARSE;
	    }
	  need_retry (info);
	}
      if (! status)
	{
	  event_calendar_set_last_pull (info->ec, time (NULL));
	  info->last_status_code = 0;
	  info->last_internal_code = 0;
	}
      g_free (status);
    }
  g_object_unref (info->ec);
  info->busy = FALSE;
}

static void
calendar_pull_internal (struct info *info)
{
  info->busy = TRUE;

  SoupSession *session;
  SoupMessage *msg;

  if (! build_message (&session, &msg, SOUP_METHOD_GET, info))
    return;

  g_object_ref (info->ec);
  soup_session_queue_message (session, msg, pulled, info);
}

void
calendar_pull (EventCalendar *ec)
{
  g_return_if_fail (event_calendar_get_mode (ec) == 1);

  struct info *info = info_find (ec, TRUE);
  /* This was user initiated.  If there is an error, show the user
     again.  */
  info->last_status_code = 0;
  info->last_internal_code = 0;
  info->retry = 0;

  calendar_pull_internal (info);
}

static guint refresh_source;

static gboolean
refresh (gpointer data)
{
  if (refresh_source)
    {
      g_source_remove (refresh_source);
      refresh_source = 0;
    }

  time_t now = time (NULL);

  gboolean have_refresh = FALSE;
  time_t earliest_refresh;

  GSList *i;
  for (i = calendars; i; i = i->next)
    {
      struct info *info = i->data;
      EventCalendar *ec = info->ec;

      time_t next_refresh;

      if (info->retry)
	next_refresh = info->retry;
      else
	switch (event_calendar_get_mode (ec))
	  {
	  case 1:
	    next_refresh = event_calendar_get_last_pull (ec)
	      + event_calendar_get_sync_interval (ec);
	    break;
	  case 2:
	    if (info->update_at)
	      /* We are scheduled to do an update at
		 INFO->UPDATE_AT.  */
	      next_refresh = info->update_at;
	    else if (event_calendar_get_last_modification (ec)
		     > event_calendar_get_last_push (ec))
	      /* There are changes which have not yet been uploaded and
		 we are not yet scheduled to do an update.  */
	      next_refresh = now;
	    else
	      continue;
	    break;
	  default:
	    continue;
	  }

      if (! info->busy && next_refresh < now + 60)
	{
	  char *title = event_calendar_get_title (ec);
	  printf ("Updating %s\n", title);
	  g_free (title);

	  if (event_calendar_get_mode (ec) == 1)
	    calendar_pull_internal (info);
	  else
	    calendar_push_internal (info);

	  next_refresh = MAX (now + event_calendar_get_sync_interval (ec),
			      /* Not more than once per minute!  */
			      now + 60);
	}

      if (! have_refresh)
	{
	  have_refresh = TRUE;
	  earliest_refresh = next_refresh;
	}
      else
	earliest_refresh = MIN (earliest_refresh, next_refresh);
    }

  if (have_refresh)
    {
      guint next_update = MIN (G_MAXUINT / 1000 - 1, earliest_refresh - now);
      refresh_source = g_timeout_add (MAX (10000, next_update * 1000),
				      refresh, NULL);
    }

  return FALSE;
}

static void
calendar_new (EventDB *edb, EventCalendar *ec, gpointer *data)
{
  if (event_calendar_get_mode (ec) == 1
      || event_calendar_get_mode (ec) == 2)
    info_new (ec);
}

static void
calendar_deleted (EventDB *edb, EventCalendar *ec, gpointer *data)
{
  GSList *i;
  for (i = calendars; i; i = i->next)
    {
      struct info *info = i->data;
      if (event_calendar_get_uid (info->ec) == event_calendar_get_uid (ec))
	{
	  g_object_unref (info->ec);
	  calendars = g_slist_delete_link (calendars, i);
	  g_free (info);
	  break;
	}
    }
}

static void
calendar_modified (EventDB *edb, EventCalendar *ec, gpointer *data)
{
  struct info *info = info_find (ec, FALSE);

  switch (event_calendar_get_mode (ec))
    {
    case 0:
      if (info)
	/* Synchronization has been turned off.  */
	info_remove (info);
      break;

    case 1:
      /* See if a refresh is needed.  */
      if (! info)
	/* Changed to this node.  */
	info = info_new (ec);
      refresh (NULL);
      break;

    case 2:
      if (! info)
	/* Changed to this node.  */
	info = info_new (ec);

      if (! info->update_at)
	/* No update is scheduled, schedule one in UPDATE_HORIZON
	   seconds (to allow changes to aggregate!).  */
	{
	  info->update_at = time (NULL) + UPDATE_HORIZON;
	  refresh (NULL);
	}
      else if (! info->retry)
	/* An update is scheduled (and there hasn't been a failure).
	   Push the update away.  No need to call refresh that will be
	   done anyways.  */
	info->update_at = time (NULL) + UPDATE_HORIZON;
      break;

    default:
      /* Er... pertend we never saw that.  */
      break;
    }
}

gboolean
calendars_sync_start (void *data)
{
  g_assert (! calendars);

  GSList *l = event_db_list_event_calendars (event_db);
  GSList *i;
  for (i = l; i; i = i->next)
    {
      EventCalendar *ec = EVENT_CALENDAR (i->data);

      if (event_calendar_get_mode (ec) == 1
	  || event_calendar_get_mode (ec) == 2)
	info_new (ec);
      else
	g_object_unref (ec);
    }

  refresh (NULL);

  g_signal_connect (event_db,
		    "calendar-new", G_CALLBACK (calendar_new), NULL);
  g_signal_connect (event_db,
		    "calendar-deleted", G_CALLBACK (calendar_deleted), NULL);
  g_signal_connect (event_db,
		    "calendar-modified", G_CALLBACK (calendar_modified), NULL);

  g_slist_free (l);

  return FALSE;
}
