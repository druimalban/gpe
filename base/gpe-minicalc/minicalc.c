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
#include <libintl.h>
#include <locale.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "init.h"

#include <gmp.h>

static char buf[256];
static GtkWidget *display;

static mpf_t accum, dv;
static int base = 10;

static gboolean display_volatile;
static gboolean in_error;

void
update_display (void)
{
  gtk_label_set_text (GTK_LABEL (display), buf[0] ? buf : "0");
}

void
error (void)
{
  gtk_label_set_text (GTK_LABEL (display), "Error");
  in_error = TRUE;
}

void
display_to_val (void)
{
  if (mpf_set_str (dv, buf, base))
    error ();
}

void
val_to_display (void)
{
  mp_exp_t exp;
  char *s = mpf_get_str (NULL, &exp, base, sizeof (buf) - 4, dv);
  size_t l = strlen (s);
  if (exp == 0)
    {
      buf[0] = '0';
      if (l)
	{
	  buf[1] = '.';
	  strcpy (buf+2, s);
	}
      else
	buf[1] = 0;
    }
  else if (exp == l)
    {
      strcpy (buf, s);
    }
  else if (exp > 0 && exp < l)
    {
      strncpy (buf, s, exp);
      buf[exp] = '.';
      strcpy (buf + exp + 1, s + exp);
    }
  else if (exp < 0)
    {
      strcpy (buf, "-exp");
      if (exp > -40)
	{
	}
      else
	{
	}
    }
  else
    {
      if (exp <= 40)
	{
	  strcpy (buf, s);
	  strncat (buf, "000000", exp - l);
	}
      else
	{
	  strcpy (buf, "XeY");
	}
    }
  free (s);
  display_volatile = TRUE;
  update_display ();
}

void
push_char (char c)
{
  size_t s;
  if (in_error)
    return;
  if (display_volatile)
    {
      s = 0;
      display_volatile = FALSE;
    }
  else
    s = strlen (buf);
  if (s < sizeof (buf) - 1)
    {
      buf[s] = c;
      buf[s+1] = 0;
      update_display ();
    }
}

void
push_digit (GtkWidget *w, int n)
{
  char c = "0123456789ABCDEF"[n];
  if (buf[0] != 0 || n != 0)
    push_char (c);
}

void
clear (void)
{
  buf[0] = 0;
  update_display ();
}

GtkWidget *
number_button (int n)
{
  GtkWidget *w;
  char buf [2];
  if (n < 10)
    buf[0] = n + '0';
  else
    buf[0] = n + 'A' - 10;
  buf[1] = 0;
  w = gtk_button_new_with_label (buf);
  gtk_widget_set_usize (w, 16, 16);
  gtk_signal_connect (GTK_OBJECT (w), "clicked",
		      GTK_SIGNAL_FUNC (push_digit),
		      (gpointer)n);
  return w;
}

void
set_base (GtkWidget *w, int n)
{
  display_to_val ();
  base = n;
  val_to_display ();
}

void
base_button (GtkWidget *box, char *string, int n)
{
  static GtkWidget *radio_group;
  GtkWidget *w;
  if (radio_group)
    w = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio_group), string);
  else
    {
      w = gtk_radio_button_new_with_label (NULL, string);
      radio_group = w;
    }
  gtk_signal_connect (GTK_OBJECT (w), "clicked", GTK_SIGNAL_FUNC (set_base), n);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *hbox;
  Atom window_type_atom, window_type_dock_atom;
  Display *dpy;
  Window xwindow;
  int x, y;
  GtkWidget *vbox_base;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (window);

  dpy = GDK_WINDOW_XDISPLAY (window->window);
  xwindow = GDK_WINDOW_XWINDOW (window->window);

  window_type_atom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", False);
  window_type_dock_atom = XInternAtom (dpy,
				       "_NET_WM_WINDOW_TYPE_DOCK", False);

  XChangeProperty (dpy, xwindow, 
		   window_type_atom, XA_ATOM, 32, 
		   PropModeReplace, (unsigned char *)
		   &window_type_dock_atom, 1);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  display = gtk_label_new ("");
  gtk_widget_show (display);
  gtk_misc_set_alignment (GTK_MISC (display), 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), display, FALSE, FALSE, 0);
  table = gtk_table_new (4, 4, TRUE);
  gtk_widget_show (table);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);

  for (y = 0; y < 4; y++)
    {
      for (x = 0; x < 4; x++)
	{
	  int n = x + ((3 - y) * 4);
	  GtkWidget *b = number_button (n);
	  gtk_widget_show (b);
	  gtk_table_attach_defaults (GTK_TABLE (table), b, x, x+1, y, y+1);
	}
    }

  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

  vbox_base = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox_base);
  base_button (vbox_base, "dec", 10);
  base_button (vbox_base, "oct", 8);
  base_button (vbox_base, "hex", 16);
  base_button (vbox_base, "bin", 2);
  gtk_box_pack_end (GTK_BOX (hbox), vbox_base, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  clear ();
  mpf_init (dv);
  mpf_init (accum);

  gtk_widget_show (window);
  
  gtk_main ();
  
  exit (0);
}
