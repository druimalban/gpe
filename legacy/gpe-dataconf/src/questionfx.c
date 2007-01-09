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
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "../include/questionfx.h"
#include "gpe/picturebutton.h"
#include "gpe/pixmaps.h"

#define _(x) dgettext(PACKAGE, x)

static GtkWidget *edit;
static gchar* eBuf;

void
on_qn_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  gint *ret=NULL;
  eBuf = gtk_editable_get_chars(GTK_EDITABLE(edit),0,-1);	
  ret = gtk_object_get_data (GTK_OBJECT (user_data), "return");
  *ret=(gint)gtk_object_get_data (GTK_OBJECT(button), "value");
  gtk_widget_destroy (user_data);
  gtk_main_quit();
}

void
add_button (char *text, char *icon, GtkWidget *dialog, int value)
{
  GtkWidget *btn;
  btn = gpe_picture_button (dialog->style, text, icon);
  gtk_object_set_data (GTK_OBJECT (btn), "value", (gpointer)value);
  gtk_signal_connect (GTK_OBJECT (btn), "clicked",
                      GTK_SIGNAL_FUNC (on_qn_button_clicked),
                      dialog);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area),
                      btn);
}

gint
gpe_question_fx_ask (char *qn, char *title, char *iconname, char **strbuf, ...)
{
  GtkWidget *window, *fakeparentwindow, *hbox, *label, *icon;
  GdkPixbuf *p;
  gint button_pressed=-1;
  int i=0;

  va_list ap;

  /* window gunk */
  fakeparentwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakeparentwindow);

  window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(window), title);
  gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(fakeparentwindow));
  gtk_widget_realize (window);
 
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy),
                      (gpointer)fakeparentwindow);
  gtk_object_set_data (GTK_OBJECT (window), "return", &button_pressed);

  /* icon / label */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), hbox);

  p = gpe_try_find_icon (iconname, NULL);
  if (p != NULL)
  {
    icon = gtk_image_new_from_pixbuf (p);
    gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
  }

  label = gtk_label_new (qn);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  edit = gtk_entry_new_with_max_length(8);
  gtk_box_pack_start (GTK_BOX (hbox), edit, TRUE, TRUE, 5);

  gtk_widget_realize (window);

  /* buttons */
  va_start (ap, strbuf);
  while (TRUE)
  {
    char *btn_lbl=NULL, *btn_icon=NULL;

    btn_lbl = va_arg (ap, char *);
    if (btn_lbl == NULL)
	    break;

    btn_icon = va_arg (ap, char *);

    add_button (btn_lbl, btn_icon, window, i++);
  }
  va_end (ap);

  gtk_widget_show_all (window);

  gtk_main ();
  
  *strbuf = g_strdup(eBuf);
  g_free(eBuf);

  return button_pressed;
}

