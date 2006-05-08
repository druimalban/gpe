/* handoff.c - Handoff Implementation.
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

#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "handoff.h"

// #define DEBUG
#ifdef DEBUG
#define D(x) x
#else
#define D(x) do { } while (0)
#endif

struct conn
{
  Handoff *handoff;
  GIOChannel *channel;
  guint source;

  char *pending_data;
  int pending_data_len;
  char *pending_command;
};

struct _Handoff
{
  GObject object;
  /* A list of struct conn.  */
  GList *conns;

  GIOChannel *channel;
  guint source;

  gboolean display_relevant;

  char *(*serialize) (Handoff *);
  void (*exit) (Handoff *);
};

struct _HandoffClass
{
  GObjectClass gobject_class;

  HandoffCallback handoff;

  guint handoff_signal;
};

static void handoff_class_init (gpointer klass, gpointer klass_data);
static void handoff_dispose (GObject *obj);
static void handoff_finalize (GObject *object);
static void handoff_init (GTypeInstance *instance, gpointer klass);

static GObjectClass *handoff_parent_class;

GType
handoff_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (HandoffClass),
	NULL,
	NULL,
	handoff_class_init,
	NULL,
	NULL,
	sizeof (struct _Handoff),
	0,
	handoff_init
      };

      type = g_type_register_static (G_TYPE_OBJECT, "Handoff", &info, 0);
    }

  return type;
}

static void
handoff_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  HandoffClass *handoff_class;

  handoff_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = handoff_finalize;
  object_class->dispose = handoff_dispose;

  handoff_class = HANDOFF_CLASS (klass);
  handoff_class->handoff_signal
    = g_signal_new ("handoff",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (HandoffClass, handoff),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);
}

static void
handoff_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (handoff_parent_class)->dispose (obj);
}

static void
conn_destroy_simple (struct conn *conn)
{
  g_free (conn->pending_data);
  g_free (conn->pending_command);
  g_source_remove (conn->source);
  close (g_io_channel_unix_get_fd (conn->channel));
  g_io_channel_unref (conn->channel);
  g_free (conn);
}

static void
conn_destroy (struct conn *conn)
{
  conn->handoff->conns = g_list_remove (conn->handoff->conns, conn);
  conn_destroy_simple (conn);
}

static void
handoff_finalize (GObject *object)
{
  Handoff *handoff = HANDOFF (object);

  GList *i;
  for (i = handoff->conns; i; i = i->next)
    conn_destroy_simple (i->data);
  g_list_free (handoff->conns);

  g_source_remove (handoff->source);
  close (g_io_channel_unix_get_fd (handoff->channel));
  g_io_channel_unref (handoff->channel);

  G_OBJECT_CLASS (handoff_parent_class)->finalize (object);
}


static void
handoff_init (GTypeInstance *instance, gpointer klass)
{
  Handoff *handoff = HANDOFF (instance);
}

static void
handoff_signal (Handoff *handoff, char *data)
{
  D (printf ("%s: %s\n", __func__, data));

  GValue args[2];
  GValue rv;

  args[0].g_type = 0;
  g_value_init (&args[0], G_TYPE_FROM_INSTANCE (G_OBJECT (handoff)));
  g_value_set_instance (&args[0], handoff);
        
  args[1].g_type = 0;
  g_value_init (&args[1], G_TYPE_POINTER);
  g_value_set_pointer (&args[1], data);

  g_signal_emitv (args,
		  HANDOFF_GET_CLASS (handoff)->handoff_signal,
		  0, &rv);
}

/* All commands are case sensitive and followed by any arguments and
   then a newline character.  */

/* Client commands:  */

/* Takes a single argument: the end of data command.  Followed by
   binary data which is sent along with the signal and freely
   interpreted by the program.  It must be terminated with a newline
   character and then the argument command followed by a newline.  */
#define CMD_HANDOFF "HANDOFF"

/* Server replies.  */

/* The server replies with this indicating that it is willing to take
   over for the client: the client may now exit.  */
#define CMD_OK "OK"

/* The server replies with this indicating that the client should take
   off and that it will exit.  */
#define CMD_PUNT "PUNT"

#define WRITE(fd, s) \
  do \
    { \
      const char *p_ = (s); \
      while (strlen (p_) > 0) \
        { \
          int n = write (fd, p_, strlen (p_)); \
          if (n == 0) \
            break; \
          p_ += n; \
        } \
    } \
  while (0)

/* Reads the next command from fd.  If looking for a simple command
   (i.e. without a data payload), set EXTENDED to false (TERMINATOR is
   then a don't care).  Exactly one line of NUL terminated input will
   be returned excluding the trailing newline in *DATA and 0 is
   returned.  Otherwise, data is read until a line contains TERMINATOR
   by itself and the data is returned in *DATA, again NUL terminated
   and excluding the line containing TERMINATOR and the single newline
   immediately preceding it and 0 is returned.  If no command is
   ready, returns 0 and sets *DATA to NULL.  If an EOF is detected,
   returns 1.  */
static int
read_command (char **pending_data_p, int *pending_data_len_p,
	      int fd, int block_time,
	      gboolean extended, const char *terminator,
	      char **data)
{
  D (printf ("%s: pending_data (%d): `", __func__, *pending_data_len_p);
     fwrite (*pending_data_p, 1, *pending_data_len_p, stdout);
     printf ("' extended: %d, terminator: `%s'\n", extended, terminator));

  char *check (void)
    {
      D (printf ("%s: pending_data (%d): `", __func__, *pending_data_len_p);
	 fwrite (*pending_data_p, 1, *pending_data_len_p, stdout);
	 fwrite ("'\n", 2, 1, stdout));

      char *p = *pending_data_p;
      int p_len = *pending_data_len_p;
      if (! extended)
	/* Ignore empty commands.  */
	while (p_len > 0 && *p == '\n')
	  {
	    p_len --;
	    p ++;
	  }

      char *data_end = p;
      int data_len = 0;
      char *trailing = p;
      int trailing_len = p_len;

      while (1)
	{
	  if (extended)
	    /* Looking for an extended chunk terminated by TERMINATOR.  */
	    {
	      if (trailing_len >= strlen (terminator) + 1
		  && memcmp (trailing, terminator, strlen (terminator)) == 0
		  && trailing[strlen (terminator)] == '\n')
		/* Found the end of the extended chunk!  */
		break;
	    }

	  data_end = memchr (trailing, '\n', trailing_len);
	  if (! data_end)
	    return NULL;

	  data_len = data_end - p + 1;
	  trailing = data_end + 1;
	  trailing_len = p_len - data_len;

	  if (! extended)
	    break;
	}

      if (extended)
	{
	  trailing += strlen (terminator) + 1;
	  trailing_len -= strlen (terminator) + 1;
	}

      char *data = g_malloc (data_len);
      memcpy (data, p, data_len - 1);
      data[data_len - 1] = 0;

      D (printf ("%s: got command: `%s'\n", __func__, data));

      if (trailing_len == 0)
	{
	  g_free (*pending_data_p);
	  *pending_data_p = NULL;
	  *pending_data_len_p = 0;
	}
      else
	{
	  char *old = *pending_data_p;
	  *pending_data_p = g_malloc (trailing_len);
	  *pending_data_len_p = trailing_len;
	  memcpy (*pending_data_p, trailing, trailing_len);
	  g_free (old);
	}

      D (printf ("%s: remaining data (%d): `", __func__, *pending_data_len_p);
	 fwrite (*pending_data_p, 1, *pending_data_len_p, stdout);
	 fwrite ("'\n", 2, 1, stdout));

      return data;
    }

  if (*pending_data_p)
    {
      char *cmd = check ();
      if (cmd)
	{
	  *data = cmd;
	  return 0;
	}
    }

  int size = *pending_data_len_p;

  fd_set read_set;
  FD_ZERO (&read_set);
  FD_SET (fd, &read_set);

  /* If the server takes too long to respond, we assume that it
     died.  */
  struct timeval timeout;
  timeout.tv_sec = block_time;
  timeout.tv_usec = 0;

  fd_set set;
  for (set = read_set;
       select (FD_SETSIZE, &set, NULL, NULL,
	       block_time > 0 ? &timeout : NULL) > 0;
       set = read_set)
    {
      if (! FD_ISSET (fd, &set))
	/* Komisch.  */
	continue;

      if (size == *pending_data_len_p)
	{
	  size = MAX (size * 2, 64);
	  *pending_data_p = g_realloc (*pending_data_p, size);
	}

      ssize_t n = read (fd, *pending_data_p + *pending_data_len_p,
			size - *pending_data_len_p);
      if (n == 0)
	/* EOF.  */
	return 1;

      *pending_data_len_p += n;

      char *cmd = check ();
      if (cmd)
	{
	  *data = cmd;
	  return 0;
	}
    }

  *data = NULL;
  return 0;
}

static gboolean
handoff_server_read (GIOChannel *channel, GIOCondition condition,
		     gpointer user_data)
{
  struct conn *conn = user_data;

  if ((condition & G_IO_HUP))
    /* Client hung up.  */
    {
      conn_destroy (conn);
      return FALSE;
    }

  g_assert (condition == G_IO_IN);

  if (!conn->pending_command)
    {
      int eof = read_command (&conn->pending_data, &conn->pending_data_len,
			      g_io_channel_unix_get_fd (channel), 0,
			      FALSE, NULL, &conn->pending_command);
      if (eof)
	{
	  conn_destroy (conn);
	  return FALSE;
	}
    }

  if (conn->pending_command)
    {
      char *cmd = conn->pending_command;
      if (strncmp (CMD_HANDOFF, cmd, strlen (CMD_HANDOFF)) == 0)
	{
	  char *terminator = cmd + strlen (CMD_HANDOFF);
	  while (*terminator == ' ')
	    terminator ++;
	  if (strlen (terminator) == 0)
	    g_warning ("%s: terminator has 0 length!", __func__);

	  char *data;
	  int eof
	    = read_command (&conn->pending_data, &conn->pending_data_len,
			    g_io_channel_unix_get_fd (channel), 0,
			    TRUE, terminator, &data);
	  if (eof)
	    {
	      conn_destroy (conn);
	      return FALSE;
	    }

	  if (data)
	    {
	      char *display = getenv ("DISPLAY");
	      if (strncmp (data, "DISPLAY=", strlen ("DISPLAY=")) == 0)
		/* The display is relevant for the sender.  */
		{
		  if (strncmp (data + strlen ("DISPLAY="), display,
			       strlen (display)) == 0
		      && data[strlen ("DISPLAY=") + strlen (display)] == '\n')
		    /* The displays match, we continue running.  */
		    {
		      WRITE (g_io_channel_unix_get_fd (channel), CMD_OK "\n");
		      handoff_signal (conn->handoff,
				      data + strlen ("DISPLAY=")
				      + strlen (display) + 1);
		    }
		  else
		    /* The displays don't match: display migration.  */
		    {
		      char *state = NULL;
		      if (conn->handoff->serialize)
			state = conn->handoff->serialize (conn->handoff);

		      WRITE (g_io_channel_unix_get_fd (channel),
			     CMD_PUNT " EOD\n");
		      if (state)
			WRITE (g_io_channel_unix_get_fd (channel), state);
		      WRITE (g_io_channel_unix_get_fd (channel), "EOD\n");

		      if (conn->handoff->exit)
			conn->handoff->exit (conn->handoff);
		      else
			exit (0);
		    }
		}
	      else
		/* The display is not relevant for the sender: simply
		   accept the sender's state.  */
		{
		  WRITE (g_io_channel_unix_get_fd (channel), CMD_OK "\n");
		  handoff_signal (conn->handoff, data);
		}

	      g_free (data);
	      g_free (conn->pending_command);
	      conn->pending_command = NULL;
	    }
	}
      else
	{
	  g_warning ("%s: unknown command: \"%s\", ignoring", __func__, cmd);
	  g_free (cmd);
	  conn->pending_command = NULL;
	}
    }

  return TRUE;
}

static gboolean
handoff_server_connect (GIOChannel *channel, GIOCondition condition,
			gpointer data)
{
  Handoff *handoff = HANDOFF (data);

  int s = g_io_channel_unix_get_fd (channel);
  int fd = accept (s, NULL, NULL);
  if (fd == -1)
    {
      g_critical ("%s: accept: %s", __func__, strerror (errno));
      return FALSE;
    }

  struct conn *conn = g_malloc0 (sizeof (*conn));
  conn->handoff = handoff;
  conn->channel = g_io_channel_unix_new (fd);
  conn->source = g_io_add_watch (conn->channel, G_IO_IN,
				 handoff_server_read, conn);
  handoff->conns = g_list_prepend (handoff->conns, conn);

  return TRUE;
}

/* Another instance has contacted us.  Create a connection and
   negotiate a state handoff.  */
static gboolean
handoff_server (Handoff *handoff, const char *rendez_vous)
{
  int err;
  int s;
  struct sockaddr_un addr;

  g_return_val_if_fail (rendez_vous, -1);
  g_return_val_if_fail (*rendez_vous, -1);
  g_return_val_if_fail (strlen (rendez_vous) < sizeof (addr.sun_path), -1);

  s = socket (PF_LOCAL, SOCK_STREAM, 0);
  if (s == -1)
    {
      g_critical ("%s: Failed to create socket: %s",
		  __func__, strerror (errno));
      return FALSE;
    }

  unlink (rendez_vous);

  memset (&addr, 0, sizeof (addr));
  addr.sun_family = AF_UNIX;
  strcpy (&addr.sun_path[0], rendez_vous);

  err = bind (s, (struct sockaddr *) &addr, SUN_LEN (&addr));
  if (err)
    {
      g_critical ("%s: bind: %s", __func__, strerror (errno));
      close (s);
      return FALSE;
    }

  if (listen (s, 5) < 0)
    {
      g_critical ("%s: listen: %s", __func__, strerror (errno));
      close (s);
      return FALSE;
    }

  handoff->channel = g_io_channel_unix_new (s);
  handoff->source = g_io_add_watch (handoff->channel, G_IO_IN,
				    handoff_server_connect, handoff);

  return TRUE;
}

gboolean
handoff_handoff (Handoff *handoff, const char *rendezvous,
		 const char *data, gboolean display_relevant,
		 char *(*serialize) (Handoff *), void (*exit)(Handoff *))
{
  int did_handoff = FALSE;
  handoff->display_relevant = display_relevant;

  handoff->serialize = serialize;
  handoff->exit = exit;

  int fd;
  fd = socket (PF_LOCAL, SOCK_STREAM, 0);
  if (fd == -1)
    goto run_server;

  struct sockaddr_un addr;
  memset (&addr, 0, sizeof (addr));
  addr.sun_family = AF_UNIX;
  strcpy (&addr.sun_path[0], rendezvous);

  int err = connect (fd, (struct sockaddr *) &addr, SUN_LEN (&addr));
  if (err < 0)
    {
      close (fd);
      goto run_server;
    }

  /* Issue a handoff command.  */
  /* XXX: Need to check that the string "\nEOD\n" doesn't occur in DATA.  */
  WRITE (fd, CMD_HANDOFF " EOD\n");
  if (handoff->display_relevant)
    {
      const char *display = getenv ("DISPLAY");
      if (display)
	{
	  WRITE (fd, "DISPLAY=");
	  WRITE (fd, display);
	  WRITE (fd, "\n");
	}
    }
  if (data)
    WRITE (fd, data);
  WRITE (fd, "\nEOD\n");

  char *pending_data = NULL;
  int pending_data_len = 0;
  char *reply;
  int eof = read_command (&pending_data, &pending_data_len,
			  fd, 3, FALSE, 0, &reply);
  if (eof)
    goto out;

  if (reply)
    {
      if (strcmp (reply, CMD_OK) == 0)
	/* Peer will take over.  */
	{
	  did_handoff = TRUE;
	  goto out;
	}
      else if (strncmp (reply, CMD_PUNT, strlen (CMD_PUNT)) == 0)
	/* Peer punts to us.  */
	{
	  char *terminator = reply + strlen (CMD_PUNT);
	  while (*terminator == ' ')
	    terminator ++;
	  if (strlen (terminator) == 0)
	    g_warning ("%s: terminator has 0 length!", __func__);

	  char *data;
	  int eof = read_command (&pending_data, &pending_data_len,
				  fd, 3, TRUE, terminator, &data);
	  if (! eof && data)
	    {
	      handoff_signal (handoff, data);
	      g_free (data);
	    }
	}
    }

 out:
  g_free (pending_data);
  g_free (reply);

 run_server:
  if (fd >= 0)
    close (fd);

  if (did_handoff)
    return TRUE;

  handoff_server (handoff, rendezvous);
  return FALSE;
}

Handoff *
handoff_new (void)
{
  return HANDOFF (g_object_new (handoff_get_type (), NULL));
}
