/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include "support.h"

#include "smallbox.h"

static void
click_ok (GtkWidget *widget, gpointer p)
{
  gtk_main_quit ();
}

static void
click_cancel (GtkWidget *widget, gpointer p)
{
  gtk_widget_destroy (GTK_WIDGET (p));
}

static void
note_destruction (GtkWidget *widget, gpointer p)
{
  gboolean *b = (gboolean *)p;

  *b = TRUE;

  gtk_main_quit ();
}

gchar *
smallbox (gchar *title, gchar *labeltext, gchar *dval)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *entry = gtk_entry_new ();
  GtkWidget *label = gtk_label_new (labeltext);
  GtkWidget *buttonok = gtk_button_new_with_label (_("OK"));
  GtkWidget *buttoncancel = gtk_button_new_with_label (_("Cancel"));
  gchar *p;
  gboolean destroyed = FALSE;

  gtk_widget_show (buttonok);
  gtk_widget_show (buttoncancel);
  
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 2);
  gtk_widget_show (entry);
  gtk_widget_show (hbox);

  gtk_entry_set_text (GTK_ENTRY (entry), dval);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox, 
		      TRUE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
		      buttonok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
		      buttoncancel, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked", 
		      GTK_SIGNAL_FUNC (click_ok), NULL);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked", 
		      GTK_SIGNAL_FUNC (click_cancel), window);

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      note_destruction, &destroyed);

  gtk_widget_set_usize (window, 200, 100);

  gtk_widget_show (window);
  gtk_widget_grab_focus (entry);

  gtk_main ();

  if (destroyed)
    return NULL;

  gtk_signal_connect (GTK_OBJECT (window), "destroy", NULL, NULL);

  p = gtk_entry_get_text (GTK_ENTRY (entry));

  gtk_widget_destroy (window);

  return p;
}
