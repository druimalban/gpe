/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

  gtk_widget_realize (window);
  buttonok  = gpe_picture_button (window->style, _("OK"), "ok");
  buttoncancel  = gpe_picture_button (window->style, _("Cancel"), "cancel");

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

static GtkWidget *
make_combo (GList *options)
{
  GtkWidget *w = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (w), options);
  return w;
}

gboolean
smallbox_x2 (gchar *title, struct box_desc2 *d)
{
  GtkWidget *window = gtk_dialog_new ();
  GtkWidget *buttonok  = gpe_picture_button (window->style, _("OK"), "ok");
  GtkWidget *buttoncancel  = gpe_picture_button (window->style, _("Cancel"), "cancel");
  GtkWidget *table;
  GtkWidget **entry;
  gboolean destroyed = FALSE;
  guint i = 0;
  struct box_desc2 *di = d;

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
      if (di->suggestions)
	{
	  GtkWidget *combo = make_combo (di->suggestions);
	  entry[i] = GTK_COMBO (combo)->entry;
	  gtk_widget_show (combo);
	  gtk_table_attach_defaults (GTK_TABLE (table), combo, 1, 2, i, i + 1);
	}
      else
	{
	  entry[i] = gtk_entry_new ();
	  gtk_widget_show (entry[i]);
	  gtk_table_attach_defaults (GTK_TABLE (table), entry[i], 1, 2, i, i + 1);
	}

      if (di->value)
	gtk_entry_set_text (GTK_ENTRY (entry[i]), di->value);

      gtk_widget_show (label);

      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, i, i + 1);

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
