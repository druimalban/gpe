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
on_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data)
{
  gtk_exit (0);
}

void
on_cancel_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_exit(0);
}

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
  GtkWidget *window, *fakeparentwindow, *hbox, *label, *icon;
  GtkWidget *buttonok, *buttoncancel;
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

  fakeparentwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakeparentwindow);

  window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(window), _("Confirmation"));
  gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(fakeparentwindow));
  gtk_widget_realize (window);
 
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Are you sure you want to log out?"));

  gtk_widget_show (label);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  p = gpe_find_icon ("logout");
  icon = gpe_render_icon (GTK_DIALOG(window)->vbox->style, p);
  gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  gtk_widget_realize (window);

  buttonok = gpe_picture_button (window->style, _("Log out"), "ok");
  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
                      GTK_SIGNAL_FUNC (on_ok_button_clicked),
                      NULL);

  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                      GTK_SIGNAL_FUNC (on_cancel_button_clicked),
                      NULL);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                     buttoncancel);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                      buttonok);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), hbox);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (on_window_destroy),
                      NULL);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
