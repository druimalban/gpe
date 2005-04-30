/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <libintl.h>

#include "smallbox.h"
#include "picturebutton.h"
#include "spacing.h"

#define _(x) dgettext(PACKAGE, x)

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

/**
 * smallbox_x:
 * @title: Box title.
 * @d: Array of box content descriptions (Query text and default value).
 *
 * Small and simple text input box to query for text input.
 * 
 * Returns: TRUE if a value was entered and confirmed, FALSE otherwise.
 */
gboolean
smallbox_x (gchar *title, struct box_desc *d)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *table;
  GtkWidget **entry;
  gboolean destroyed = FALSE;
  guint i = 0;
  struct box_desc *di = d;
  GtkWidget *buttonok, *buttoncancel;
  guint gpe_boxspacing = gpe_get_boxspacing ();

  gtk_widget_realize (window);
  buttonok  = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  buttoncancel  = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

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

      gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i + 1, 0, 0, gpe_boxspacing, 1);
      gtk_table_attach_defaults (GTK_TABLE (table), entry[i], 1, 2, i, i + 1);

      i++;
      di++;
    }
  
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), table, 
		      TRUE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
		      buttoncancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
		      buttonok, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked", 
		      GTK_SIGNAL_FUNC (smallbox_click_ok), NULL);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked", 
		      GTK_SIGNAL_FUNC (smallbox_click_cancel), window);

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (smallbox_note_destruction), &destroyed);

  gtk_window_set_default_size (GTK_WINDOW (window), 200, 100);

  gtk_widget_show_all (window);
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

static GtkWidget *
make_combo (GList *options)
{
  GtkWidget *w = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (w), options);
  return w;
}

/**
 * smallbox_x2:
 * @title: Box title.
 * @d: Aray of box content descriptions containing query text, default value and
 *     a list of predefined values.
 *
 * Small and simple text input box to query for text input offering a defined
 * set of values to select from or to enter own text.
 * 
 * Returns: TRUE if a value was entered and confirmed, FALSE otherwise.
 */
gboolean
smallbox_x2 (gchar *title, struct box_desc2 *d)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *buttonok  = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  GtkWidget *buttoncancel  = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);
  GtkWidget *table;
  GtkWidget **entry;
  gboolean destroyed = FALSE;
  guint i = 0;
  struct box_desc2 *di = d;
  guint gpe_boxspacing = gpe_get_boxspacing ();

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
      if (di->suggestions)
	{
	  GtkWidget *combo = make_combo (di->suggestions);
	  entry[i] = GTK_COMBO (combo)->entry;
	  gtk_table_attach_defaults (GTK_TABLE (table), combo, 1, 2, i, i + 1);
	}
      else
	{
	  entry[i] = gtk_entry_new ();
	  gtk_table_attach_defaults (GTK_TABLE (table), entry[i], 1, 2, i, i + 1);
	}

      if (di->value)
	gtk_entry_set_text (GTK_ENTRY (entry[i]), di->value);

      gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i + 1, 0, 0, gpe_boxspacing, 1);

      i++;
      di++;
    }
  
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), table, 
		      TRUE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
		      buttoncancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), 
		      buttonok, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked", 
		      GTK_SIGNAL_FUNC (smallbox_click_ok), NULL);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked", 
		      GTK_SIGNAL_FUNC (smallbox_click_cancel), window);

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (smallbox_note_destruction), &destroyed);

  gtk_window_set_default_size (GTK_WINDOW (window), 200, 100);

  gtk_widget_show_all (window);
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
