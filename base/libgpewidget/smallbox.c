/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <libintl.h>

#include "smallbox.h"

#define _(x) gettext(x)

void
smallbox_click_ok (GtkWidget *widget, gpointer p)
{
  gtk_main_quit ();
}

void
smallbox_click_cancel (GtkWidget *widget, gpointer p)
{
  gtk_widget_destroy (GTK_WIDGET (p));
}

void
smallbox_note_destruction (GtkWidget *widget, gpointer p)
{
  gboolean *b = (gboolean *)p;

  if (*b == FALSE)
    {
      *b = TRUE;
      
      gtk_main_quit ();
    }
}

gboolean
smallbox_x (gchar *title, struct box_desc *d)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *buttonok = gtk_button_new_with_label (_("OK"));
  GtkWidget *buttoncancel = gtk_button_new_with_label (_("Cancel"));
  GtkWidget *table;
  GtkWidget **entry;
  gboolean destroyed = FALSE;
  guint i = 0;
  struct box_desc *di = d;

  gtk_widget_show (buttonok);
  gtk_widget_show (buttoncancel);

  while (di->label)
    {
      i++;
      di++;
    }
  
  table = gtk_table_new (i, 2, FALSE);
  entry = g_malloc (i * sizeof (GtkEntry *));

  i = 0;
  di = d;
  
  while (di->label)
    {
      GtkWidget *label = gtk_label_new (di->label);
      entry[i] = gtk_entry_new ();

      if (di->value)
	gtk_entry_set_text (GTK_ENTRY (entry[i]), di->value);

      gtk_widget_show (entry[i]);
      gtk_widget_show (label);

      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, i, i + 1);
      gtk_table_attach_defaults (GTK_TABLE (table), entry[i], 1, 2, i, i + 1);

      i++;
      di++;
    }
  
  gtk_widget_show (table);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), table, 
		      TRUE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
		      buttonok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
		      buttoncancel, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked", 
		      GTK_SIGNAL_FUNC (smallbox_click_ok), NULL);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked", 
		      GTK_SIGNAL_FUNC (smallbox_click_cancel), window);

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      smallbox_note_destruction, &destroyed);

  gtk_widget_set_usize (window, 200, 100);

  gtk_widget_show (window);
  gtk_widget_grab_focus (entry[0]);

  gtk_main ();

  if (destroyed)
    return FALSE;

  destroyed = TRUE;

  i = 0;
  di = d;
  
  while (di->label)
    {
      if (di->value)
	g_free (di->value);
      di->value = gtk_editable_get_chars (GTK_EDITABLE (entry[i]), 0, -1);
      di++;
      i++;
    }

  gtk_widget_destroy (window);

  return TRUE;
}

gchar *
smallbox (gchar *title, gchar *labeltext, gchar *dval)
{
  struct box_desc d[2];

  d[0].label = labeltext;
  d[0].value = g_strdup (dval);
  d[1].label = NULL;
  d[1].value = NULL;

  if (smallbox_x (title, d) == FALSE)
    {
      g_free (d[0].value);
      return NULL;
    }

  return d[0].value;
}
