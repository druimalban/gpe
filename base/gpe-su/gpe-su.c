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

#include "gpe/pixmaps.h"
#include "gpe/init.h"
#include "gpe/render.h"
#include "gpe/picturebutton.h"
#include "gpe/errorbox.h"

static struct gpe_icon my_icons[] = {
  { "lock", PREFIX "/share/pixmaps/gpe-su.png" },
  { NULL, NULL }
};

static GtkWidget *entry;
static GtkWidget *window;

static char *command = "x-terminal-emulator";

#define _(x) gettext(x)

#define SU "/bin/su"

void
do_su (void)
{
  gchar *password = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  int pty;
  pid_t pid;
  size_t pwlen = strlen (password);
  char buf[256];
  size_t rlen = 256;
  char *result, *rp;
  gboolean newline = FALSE;
  char cr = 10;
  int n;
  struct termios tp;

  result = g_malloc (rlen);
  rp = result;

  gtk_entry_set_text (GTK_ENTRY (entry), "");
  gdk_window_withdraw (window->window);

  gtk_main_iteration ();

  pid = forkpty (&pty, NULL, NULL, NULL);
  
  if (pid == 0)
    {
      execl (SU, SU, "-c", command, NULL);
      exit (1);
    }

  sleep (1);
  if (tcgetattr (pty, &tp))
    perror ("tcgetattr");
  cfmakeraw (&tp);
  if (tcsetattr (pty, TCSANOW, &tp))
    perror ("tcsetattr");
  write (pty, password, pwlen);
  write (pty, &cr, 1);
  memset (password, 0, pwlen);
  g_free (password);

  while (n = read (pty, buf, sizeof (buf)), n > 0)
    {
      guint i;
      for (i = 0; i < n; i++)
	{
	  if (buf[i] == '\n')
	    newline = TRUE;
	  else
	    {
	      if (newline)
		{
		  rp = result;
		  newline = FALSE;
		}
	      *rp++ = buf[i];
	      if ((rp - result) == rlen)
		{
		  size_t roff = rp - result;
		  rlen *= 2;
		  result = g_realloc (result, rlen);
		  rp = result + roff;
		}
	    }
	}
    }
  *rp = 0;

  if (strncmp (result, "Password:", 9))
    {
      gpe_error_box (result);
      kill (pid, 15);
      gdk_window_show (window->window);
    }
  else
    gtk_main_quit ();
}

int
main (int argc, char *argv[])
{
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkWidget *hbox, *vbox;
  GtkWidget *label;
  GtkWidget *buttonok, *buttoncancel;
  int i;
  gboolean flag_c = FALSE;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  for (i = 1; i < argc; i++)
    {
      if (flag_c)
	{
	  command = argv[i];
	  flag_c = FALSE;
	}
      else if (!strcmp (argv[i], "-c"))
	flag_c = TRUE;
    }

  window = gtk_dialog_new ();
  gtk_widget_realize (window);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  label = gtk_label_new (_("Enter password:"));
  gtk_widget_show (label);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  p = gpe_find_icon ("lock");
  pw = gpe_render_icon (window->style, p);
  gtk_widget_show (pw);
  gtk_box_pack_start (GTK_BOX (hbox), pw, FALSE, FALSE, 4);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
		      hbox, FALSE, FALSE, 0);

  gtk_widget_realize (window);

  buttonok = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  buttoncancel = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
		      buttoncancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
		      buttonok, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (window), "destroy", 
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  gtk_signal_connect (GTK_OBJECT (entry), "activate", do_su, NULL);
  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked", do_su, NULL);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked", gtk_main_quit, NULL);

  gtk_widget_show (window);

  gtk_widget_grab_focus (entry);

  gtk_main ();

  exit (0);
}
