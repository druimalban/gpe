/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
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

#include "render.h"
#include "pixmaps.h"
#include "picturebutton.h"

GtkWidget *
gpe_picture_button (GtkStyle *style, gchar *text, gchar *icon)
{
  GtkWidget *button = gtk_button_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *hbox2 = gtk_hbox_new (FALSE, 0);

  if (icon[0] == '!')
    {
      GtkWidget *pw = gtk_image_new_from_stock (icon + 1, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (hbox), pw, FALSE, FALSE, 0);
      gtk_widget_show (pw);
    }
  else
    {
      GdkPixbuf *p = gpe_try_find_icon (icon, NULL);
      if (p)
	{
	  GtkWidget *pw = gpe_render_icon (style, p);
	  gtk_box_pack_start (GTK_BOX (hbox), pw, FALSE, FALSE, 0);
	  gtk_widget_show (pw);
	}
    }

  if (text)
    {
      GtkWidget *label = gtk_label_new (text);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_widget_show (label);
    }

  gtk_widget_show (hbox2);
  gtk_widget_show (hbox);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox2), hbox, TRUE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (button), hbox2);

  return button;
}


GtkWidget *
gpe_button_new_from_stock (const gchar *stock_id, int type)
{
  GtkWidget *button, *hbox, *image, *label, *align;
  GtkStockItem item;

  button = gtk_button_new ();
  
  if (gtk_stock_lookup (stock_id, &item) == TRUE)
    {
      hbox = gtk_hbox_new (FALSE, 2);
      align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      
      if (type == GPE_BUTTON_TYPE_ICON || type == GPE_BUTTON_TYPE_BOTH)
	{
	  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
	  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	}

      if (type == GPE_BUTTON_TYPE_LABEL || type == GPE_BUTTON_TYPE_BOTH)
	{
	  label = gtk_label_new_with_mnemonic (item.label);
	  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	}
      
      gtk_container_add (GTK_CONTAINER (button), align);
      gtk_container_add (GTK_CONTAINER (align), hbox);
      gtk_widget_show_all (align);
    }

  return button;
}
