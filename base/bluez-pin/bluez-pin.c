/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sqlite.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <bluetooth/bluetooth.h>

#define BT_ICON PREFIX "/share/pixmaps/bt-logo.png"

#define _(x) gettext(x)

static char *address;
static char *name = "";
static GtkWidget *check;
static sqlite *sqliteh;

static gboolean dbus_mode;

struct req_context;

static void send_reply (struct req_context *ctx, const char *pin);

static int
sql_start (void)
{
  char *err;
  char *fname = g_strdup_printf ("%s/.bluez-pin", g_get_home_dir ());
  sqliteh = sqlite_open (fname, 0, &err);
  g_free (fname);
  if (sqliteh == NULL)
    {
      fprintf (stderr, "Error: %s\n", err);
      free (err);
      return -1;
    }

  sqlite_exec (sqliteh, "create table btdevice (address text NOT NULL, pin text, name text)", NULL, NULL, &err);

  return 0;
}

static int
lookup_in_list (int outgoing, const char *address, char **pin)
{
  char *err;
  int nrow, ncol;
  char **results;

  if (sql_start ())
    return 0;
  
  if (sqlite_get_table_printf (sqliteh, "select pin from btdevice where address='%q'", &results, &nrow, &ncol, &err, address))
    return 0;

  if (nrow == 0)
    return 0;
  
  *pin = g_strdup (results[ncol]);
  
  sqlite_free_table (results);

  return 1;
}

void
abandon (void)
{
  printf ("ERR\n");
  exit (0);
}

static void
click_cancel (GtkWidget *widget, GtkWidget *window)
{
  struct req_context *req = g_object_get_data (G_OBJECT (window), "context");
  if (req)
    send_reply (req, NULL);
  else
    abandon ();

  gtk_widget_destroy (window);
}

static void
click_ok (GtkWidget *widget, GtkWidget *window)
{
  gboolean save = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  struct req_context *req = g_object_get_data (G_OBJECT (window), "context");
  GtkWidget *entry;
  const char *pin;

  entry = g_object_get_data (G_OBJECT (window), "entry");
  pin = gtk_entry_get_text (GTK_ENTRY (entry));

  if (save && sqliteh)
    {
      char *err;
      if (sqlite_exec_printf (sqliteh, "insert into btdevice values('%q','%q','%q')", NULL, NULL, &err, address, pin, name))
	{
	  fprintf (stderr, "%s\n", err);
	  free(err);
	}

      sqlite_close (sqliteh);
    }

  if (req)
    {
      send_reply (req, pin);
    }
  else
    {
      printf ("PIN:%s\n", pin);
      exit (0);
    }

  gtk_widget_destroy (window);
}

static void
ask_user_dialog (int outgoing, const char *address, struct req_context *ctx)
{
  GtkWidget *window;
  GtkWidget *logo = NULL;
  GtkWidget *text1, *text2, *text3, *hbox, *vbox;
  GtkWidget *hbox_pin;
  GtkWidget *pin_label, *entry;
  GtkWidget *buttonok, *buttoncancel;
  GdkPixbuf *pixbuf;

  const char *dir = outgoing ? _("Outgoing connection to") 
    : _("Incoming connection from");

  window = gtk_dialog_new ();
 
  pixbuf = gdk_pixbuf_new_from_file (BT_ICON, NULL);
  if (pixbuf)
    logo = gtk_image_new_from_pixbuf (pixbuf);

  pin_label = gtk_label_new (_("PIN:"));
  entry = gtk_entry_new ();

  check = gtk_check_button_new_with_label (_("Save in database"));

  hbox = gtk_hbox_new (FALSE, 4);
  vbox = gtk_vbox_new (FALSE, 0);

  hbox_pin = gtk_hbox_new (FALSE, 0);

  text1 = gtk_label_new (dir);
  text2 = gtk_label_new (address);

  gtk_box_pack_start (GTK_BOX (vbox), text1, TRUE, TRUE, 0);
  if (name[0])
    {
      text3 = gtk_label_new (name);
      gtk_box_pack_start (GTK_BOX (vbox), text3, TRUE, TRUE, 0);
    }
  gtk_box_pack_start (GTK_BOX (vbox), text2, TRUE, TRUE, 0);

  if (logo)
    gtk_box_pack_start (GTK_BOX (hbox), logo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox_pin), pin_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_pin), entry, TRUE, TRUE, 0);

  buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
  buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), buttonok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), buttoncancel, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (buttonok), "clicked",
		    G_CALLBACK (click_ok), window);
  g_signal_connect (G_OBJECT (buttoncancel), "clicked",
		    G_CALLBACK (click_cancel), window);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (click_ok), window);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox_pin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), check, TRUE, TRUE, 0);

  if (!sqliteh)
    gtk_widget_set_sensitive (check, FALSE);

  g_object_set_data (G_OBJECT (window), "entry", entry);
  g_object_set_data (G_OBJECT (window), "context", ctx);

  gtk_window_set_title (GTK_WINDOW (window), _("Bluetooth PIN"));

  gtk_widget_show_all (window);

  gtk_widget_grab_focus (entry);
}

static void
ask_user (int outgoing, const char *address)
{
  ask_user_dialog (outgoing, address, NULL);

  gtk_main ();
}

struct req_context
{
  DBusConnection *connection;
  DBusMessage *message;
};

static void
send_reply (struct req_context *ctx, const char *pin)
{
  DBusMessageIter iter;

  dbus_message_append_iter_init (ctx->message, &iter);
  if (pin)
    dbus_message_iter_append_string (&iter, pin);

  dbus_connection_send (ctx->connection, ctx->message, NULL);
  dbus_connection_flush (ctx->connection);

  g_free (ctx);
}

static void
handle_request (DBusConnection *connection, DBusMessage *message)
{
  DBusMessageIter iter;
  gboolean out;
  bdaddr_t bdaddr, sbdaddr;
  int type;
  int i;
  char *address;
  DBusMessage *reply;
  struct req_context *ctx;

  dbus_message_iter_init (message, &iter);
  
  type = dbus_message_iter_get_arg_type (&iter);
  if (type != DBUS_TYPE_BOOLEAN)
    {
      fprintf (stderr, "wrong type for boolean\n");
      goto error;
    }
  out = dbus_message_iter_get_boolean (&iter);

  for (i = 0; i < sizeof (bdaddr); i++)
    {
      unsigned char *p = (unsigned char *)&bdaddr;

      if (! dbus_message_iter_next (&iter))
	goto error;

      type = dbus_message_iter_get_arg_type (&iter);
      if (type != DBUS_TYPE_BYTE)
	{
	  fprintf (stderr, "wrong type for byte %d\n", i);
	  goto error;
	}
      p[i] = dbus_message_iter_get_byte (&iter);
    }

  reply = dbus_message_new_reply (message);

  ctx = g_malloc (sizeof (*ctx));
  ctx->message = reply;
  ctx->connection = connection;

  baswap (&sbdaddr, &bdaddr);
  address = batostr (&sbdaddr);
  ask_user_dialog (out, address, ctx);
  return;

 error:
  reply = dbus_message_new_error_reply (message, NULL, NULL);
  dbus_connection_send (connection, reply, NULL);
}

static DBusHandlerResult
handler_func (DBusMessageHandler *handler,
 	      DBusConnection     *connection,
	      DBusMessage        *message,
	      void               *user_data)
{
  if (dbus_message_has_name (message, "org.handhelds.gpe.bluez.pin-request"))
    handle_request (connection, message);
  
  if (dbus_message_has_name (message, DBUS_MESSAGE_LOCAL_DISCONNECT))
    exit (0);
  
  return DBUS_HANDLER_RESULT_ALLOW_MORE_HANDLERS;
}

static void
dbus_server_run (void)
{
  DBusConnection *connection;
  DBusError error;
  DBusMessageHandler *handler;

  dbus_error_init (&error);
  connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (connection == NULL)
    {
      fprintf (stderr, "Failed to open connection to system message bus: %s\n",
               error.message);
      dbus_error_free (&error);
      exit (1);
    }

  dbus_connection_setup_with_g_main (connection, NULL);

  handler = dbus_message_handler_new (handler_func, NULL, NULL);
  dbus_connection_add_filter (connection, handler);
}

#ifdef MAIN

void
usage (char *argv[])
{
  fprintf (stderr, _("Usage: %s <in|out> <address>\n"), argv[0]);
  exit (1);
}

int 
main (int argc, char *argv[])
{
  char *pin;
  gboolean gui_started;
  char *dpy = getenv (dpy);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (argc > 1 && !strcmp (argv[1], "--dbus"))
    dbus_mode = TRUE;

  if (dbus_mode == FALSE && dpy == NULL)
    {
      char *auth = NULL;
      FILE *fp;
      dpy = ":0";
      fp = popen ("/bin/ps --format args --no-headers -C X -C XFree86", "r");
      if (fp)
	{
	  char buf[1024];
	  while (fgets (buf, sizeof (buf), fp))
	    {
	      char *p = strtok (buf, " ");
	      int authf = 0;
	      while (p)
		{
		  if (authf)
		    {
		      auth = strdup (p);
		      authf = 0;
		    }
		  else if (p[0] == ':')
		    dpy = strdup (p);
		  else if (!strcmp (p, "-auth"))
		    authf = 1;
		  p = strtok (NULL, " ");
		}
	    }
	  pclose (fp);
	}
      setenv ("DISPLAY", dpy, 0);
      if (auth)
	setenv ("XAUTHORITY", auth, 0);
    }

  gtk_set_locale ();
  gui_started = gtk_init_check (&argc, &argv);

  if (dbus_mode)
    {
      dbus_server_run ();

      gtk_main ();
    }
  else
    {
      int outgoing = 0;

      if (argc < 3)
	usage (argv);

      if (strcmp (argv[1], "in") == 0)
	outgoing = 0;
      else if (strcmp (argv[1], "out") == 0)
	outgoing = 1;
      else
	usage (argv);

      address = argv[2];
      if (argc == 4)
	name = argv[3];

      if (lookup_in_list (outgoing, address, &pin))
	{
	  printf ("PIN:%s\n", pin);
	  exit (0);
	}

      if (gui_started)
	ask_user (outgoing, address);
      else
	printf("ERR\n");
    }

  exit (0);
}

#endif

