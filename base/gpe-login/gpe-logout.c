/*
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <fcntl.h>
#include <libintl.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/render.h>

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "logout", PREFIX "/share/pixmaps/gpe-logout.png" },
  { NULL, NULL }
};

#define _(x) gettext(x)

void
on_ok_button_clicked                (GtkButton       *button,
				     gpointer         user_data)
{
  execlp ("/usr/bin/mbcontrol", "mbcontrol", "-exit", NULL);
  gtk_exit(0);
}

int
main(int argc, char *argv[])
{
  GtkWidget *window, *hbox, *label, *icon, *vbox, *buttonhbox;
  GtkWidget *buttonok, *buttoncancel, *frame;
  GdkPixbuf *p;

  if (gpe_application_init (&argc, &argv) == FALSE)
    {
      exit (1);
    }

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    {
      gtk_exit (1);
    }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_widget_realize (window);
 
  /* set no window title in accordance to the Gnome 2 HIG */
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
  gtk_window_set_title (GTK_WINDOW (window), "");

  gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_DIALOG);

  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);
  buttonhbox = gtk_hbox_new (FALSE, 0);

  label = gtk_label_new (_("Are you sure you want to log out?"));

  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  p = gpe_find_icon ("logout");
  icon = gpe_render_icon (vbox->style, p);
  gtk_misc_set_alignment (GTK_MISC (icon), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
  /* FIXME: do not hardcode the spacing here, but use a global GPE constant [CM] */
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 6);

  gtk_widget_realize (window);

  buttonok = gpe_picture_button (window->style, _("Log out"), "ok");
  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
                      GTK_SIGNAL_FUNC (on_ok_button_clicked),
                      NULL);

  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  gtk_container_add (GTK_CONTAINER (buttonhbox),
		     buttonok);
  gtk_container_add (GTK_CONTAINER (buttonhbox),
                     buttoncancel);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), buttonhbox, FALSE, FALSE, 0);

  /* FIXME: do not hardcode the border width here, but use a global GPE constant [CM] */
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_add (GTK_CONTAINER (window), frame);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
