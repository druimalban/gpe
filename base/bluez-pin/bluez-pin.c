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

#include <gtk/gtk.h>
#include <gdk_imlib.h>

#define BT_ICON "/usr/share/pixmaps/bt-logo.png"

char *address;
GtkWidget *check;
sqlite *sqliteh;

const char *dname = "/.gpe/bluez-pin";

int
sql_start (void)
{
  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;
  if (home == NULL) 
    home = "";
  len = strlen (home) + strlen (dname);
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, dname);
  sqliteh = sqlite_open (buf, 0, &err);
  if (sqliteh == NULL)
    {
      fprintf (stderr, "%s\n", err);
      free (err);
      return -1;
    }

  sqlite_exec (sqliteh, "create table device_pin (address text NOT NULL, pin text)", NULL, NULL, &err);

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
  
  if (sqlite_get_table_printf (sqliteh, "select pin from device_pin where address='%q'", &results, &nrow, &ncol, &err, address))
    return 0;

  if (nrow == 0)
    return 0;
  
  *pin = results[ncol];

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
  char *pin = gtk_entry_get_text (GTK_ENTRY (entry));
  gboolean save = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));

  if (save && sqliteh)
    {
      char *err;
      if (sqlite_exec_printf (sqliteh, "insert into device_pin values('%q','%q')", NULL, NULL, &err, address, pin))
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
  GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);
  GdkBitmap *logo_mask;
  GdkPixmap *logo_pixmap;
  GtkWidget *logo = NULL;
  GtkWidget *text1, *text2, *hbox, *vbox;
  GtkWidget *vbox_top;
  GtkWidget *hbox_pin;
  GtkWidget *pin_label, *entry;
  GtkWidget *buttonok, *buttoncancel, *hbox_but;

  const char *dir = outgoing ? "Outgoing connection to" 
    : "Incoming connection from";
 
  gtk_widget_realize (window);

  if (gdk_imlib_load_file_to_pixmap (BT_ICON, &logo_pixmap, &logo_mask))
    logo = gtk_pixmap_new (logo_pixmap, logo_mask);

  pin_label = gtk_label_new ("PIN:");
  entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);

  check = gtk_check_button_new_with_label ("Save in database");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);

  hbox = gtk_hbox_new (FALSE, 4);
  vbox = gtk_vbox_new (FALSE, 0);

  vbox_top = gtk_vbox_new (FALSE, 2);
  hbox_pin = gtk_hbox_new (FALSE, 0);

  text1 = gtk_label_new (dir);
  text2 = gtk_label_new (address);

  gtk_box_pack_start (GTK_BOX (vbox), text1, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), text2, TRUE, TRUE, 0);

  if (logo)
    gtk_box_pack_start (GTK_BOX (hbox), logo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox_pin), pin_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_pin), entry, TRUE, TRUE, 0);

  hbox_but = gtk_hbox_new (FALSE, 0);
  buttonok = gtk_button_new_with_label ("OK");
  buttoncancel = gtk_button_new_with_label ("Cancel");

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

  gtk_widget_grab_focus (entry);

  gtk_widget_show_all (window);
  gtk_main ();
}

void
usage(char *argv[])
{
  fprintf (stderr, "Usage: %s <in|out> <address>\n", argv[0]);
  exit (1);
}

int 
main(int argc, char *argv[])
{
  int outgoing = 0;
  char *pin;

  setenv ("DISPLAY", ":0", 0);

  gtk_init (&argc, &argv);
  gdk_imlib_init ();

  if (argc != 3)
    usage (argv);

  if (strcmp (argv[1], "in") == 0)
    outgoing = 0;
  else if (strcmp (argv[1], "out") == 0)
    outgoing = 1;
  else
    usage (argv);

  address = argv[2];

  if (lookup_in_list (outgoing, address, &pin))
    {
      printf ("PIN:%s\n", pin);
      exit (0);
    }

  ask_user (outgoing, address);

  exit (0);
}
