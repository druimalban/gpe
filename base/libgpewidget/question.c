/*
 * Copyright (C) 2002 Robert Mibus <mibus@handhelds.org>
 *                    Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "question.h"
#include "render.h"
#include "picturebutton.h"
#include "pixmaps.h"

#define _(x) dgettext(PACKAGE, x)

int button_pressed=0;

void
on_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data)
{
  button_pressed=2;
  gtk_main_quit ();
}

void
on_no_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy (user_data);
  button_pressed=1;
  gtk_main_quit();
}

void
on_yes_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy (user_data);
  button_pressed=0;
  gtk_main_quit();
}

int
gpe_question_ask_yn (char *qn)
{
  GtkWidget *window, *fakeparentwindow, *hbox, *label, *icon;
  GtkWidget *buttonok, *buttoncancel;
  GdkPixbuf *p;

  fakeparentwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakeparentwindow);

  window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(window), _("Question"));
  gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(fakeparentwindow));
  gtk_widget_realize (window);
 
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (qn);

  gtk_widget_show (label);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  p = gpe_find_icon ("question");
  icon = gpe_render_icon (GTK_DIALOG(window)->vbox->style, p);
  gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  gtk_widget_realize (window);

  buttonok = gpe_picture_button (window->style, _("Yes"), "ok");
  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
                      GTK_SIGNAL_FUNC (on_yes_button_clicked),
                      window);

  buttoncancel = gpe_picture_button (window->style, _("No"), "cancel");
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                      GTK_SIGNAL_FUNC (on_no_button_clicked),
                      window);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                     buttoncancel);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                      buttonok);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), hbox);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (on_window_destroy),
                      NULL);

  gtk_widget_show_all (window);

  button_pressed=0;

  gtk_main ();

  return button_pressed;
}

/*
void
gpe_error_box_fmt(const char *format, ...)
{
  va_list ap;
  char *str;

  va_start (ap, format);
  vasprintf (&str, format, ap);
  va_end (ap);
  gpe_error_box (str);
  g_free (str);
}
*/
