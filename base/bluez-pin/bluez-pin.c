/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/picturebutton.h>

#define BT_ICON PREFIX "/share/pixmaps/bt-logo.png"

#define _(x) gettext(x)

static char *address;
static char *name = "";
static GtkWidget *check;
static sqlite *sqliteh;

static const char *fname = "/etc/bluetooth/pin.db";

int
sql_start (void)
{
  char *err;
  sqliteh = sqlite_open (fname, 0, &err);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      return -1;
    }

  sqlite_exec (sqliteh, "create table btdevice (address text NOT NULL, pin text, name text)", NULL, NULL, &err);

  return 0;
}

int
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
click_cancel(GtkWidget *widget,
	     GtkWidget *window)
{
  abandon ();
}

static void
click_ok(GtkWidget *widget,
	 GtkWidget *entry)
{
  const char *pin = gtk_entry_get_text (GTK_ENTRY (entry));
  gboolean save = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));

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

  printf ("PIN:%s\n", pin);
  exit (0);
}

void
ask_user (int outgoing, const char *address)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *logo = NULL;
  GtkWidget *text1, *text2, *text3, *hbox, *vbox;
  GtkWidget *vbox_top;
  GtkWidget *hbox_pin;
  GtkWidget *pin_label, *entry;
  GtkWidget *buttonok, *buttoncancel, *hbox_but;
  GdkPixbuf *pixbuf;

  const char *dir = outgoing ? _("Outgoing connection to") 
    : _("Incoming connection from");
 
  gtk_widget_realize (window);

  pixbuf = gdk_pixbuf_new_from_file (BT_ICON, NULL);
  if (pixbuf)
    logo = gpe_render_icon (window->style, pixbuf);

  pin_label = gtk_label_new (_("PIN:"));
  entry = gtk_entry_new ();

  check = gtk_check_button_new_with_label (_("Save in database"));

  hbox = gtk_hbox_new (FALSE, 4);
  vbox = gtk_vbox_new (FALSE, 0);

  vbox_top = gtk_vbox_new (FALSE, 2);
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

  hbox_but = gtk_hbox_new (FALSE, 0);

#ifdef USE_LIBGPEWIDGET
  gtk_widget_realize (window);
  buttonok = gpe_picture_button (window->style, _("OK"), "ok");
  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");
#else
  buttonok = gtk_button_new_with_label (_("OK"));
  buttoncancel = gtk_button_new_with_label (_("Cancel"));
#endif

  gtk_box_pack_start (GTK_BOX (hbox_but), buttonok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_but), buttoncancel, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
		      GTK_SIGNAL_FUNC (click_ok), entry);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		      GTK_SIGNAL_FUNC (click_cancel), entry);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		     GTK_SIGNAL_FUNC (click_ok), entry);

  gtk_box_pack_start (GTK_BOX (vbox_top), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), hbox_pin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), check, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), hbox_but, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox_top);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);

  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_widget_show_all (window);

  gtk_widget_grab_focus (entry);

  gtk_main ();
}

void
usage(char *argv[])
{
  fprintf (stderr, _("Usage: %s <in|out> <address>\n"), argv[0]);
  exit (1);
}

int 
main(int argc, char *argv[])
{
  int outgoing = 0;
  char *pin;
  gboolean gui_started;
  char *dpy = getenv (dpy);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (dpy == NULL)
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

  exit (0);
}
