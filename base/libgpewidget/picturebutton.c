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

#include "pixmaps.h"
#include "picturebutton.h"

GtkWidget *
gpe_picture_button (GtkStyle *style, gchar *text, gchar *icon)
{
  return gpe_picture_button_aligned (style, text, icon, GPE_POS_CENTER);
}

GtkWidget *
gpe_picture_button_aligned (GtkStyle *style, gchar *text, gchar *icon, GpePositionType pos)
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
	  GtkWidget *pw = gtk_image_new_from_pixbuf (p);
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

  if (pos != GPE_POS_CENTER)
    {
      GtkWidget * alignment;
      switch(pos)
	{
	case GPE_POS_LEFT:
	  alignment = gtk_alignment_new (0.0, 0.5, 0, 0);
	  break;
	case GPE_POS_RIGHT:
	  alignment = gtk_alignment_new (1.0, 0.5, 0, 0);
	  break;
	default:
	  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
	}

      gtk_container_add (GTK_CONTAINER (alignment), hbox2);
      gtk_container_add (GTK_CONTAINER (button), alignment);
    }
  else
    {
      gtk_container_add (GTK_CONTAINER (button), hbox2);  
    }
  return button;
}


GtkWidget *
gpe_button_new_from_stock (const gchar *stock_id, int type)
{
  GtkWidget *button;
  GtkStockItem item;

  if (type == GPE_BUTTON_TYPE_BOTH)
    return gtk_button_new_from_stock (stock_id);

  button = gtk_button_new ();
  
  if (gtk_stock_lookup (stock_id, &item) == TRUE)
    {
      GtkWidget *widget = NULL, *align;

      align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

      switch (type)
	{
	case GPE_BUTTON_TYPE_ICON:
	  widget = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
	  gtk_container_add (GTK_CONTAINER (align), widget);
	  break;

	case GPE_BUTTON_TYPE_LABEL:
	  widget = gtk_label_new_with_mnemonic (item.label);
	  gtk_container_add (GTK_CONTAINER (align), widget);
	  break;

	default:
	  break;
	}

      gtk_container_add (GTK_CONTAINER (button), align);
      gtk_widget_show_all (align);
    }

  return button;
}
