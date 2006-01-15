/*
 * Copyright (C) 2002, 2003, 2006 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>

#include "progress.h"

static gboolean
handle_progress_expose (GtkWidget *w, GdkEventExpose *ev, GtkWidget *child)
{
  gdk_window_clear (w->window);

  gdk_draw_rectangle (w->window, w->style->black_gc, FALSE, 0, 0, 
		      w->allocation.width - 1, w->allocation.height - 1);
  
  /* Draw the contents */
  gtk_container_propagate_expose (GTK_CONTAINER (w), child, ev);

  return TRUE;
}

static gboolean
handle_progress_size_allocate (GtkWidget *w, GtkAllocation *a, GtkWidget *child)
{
  /* Redraw the whole widget since the borders will be wrong now.  */
  gtk_widget_queue_draw (w);
}

GtkWidget *
bt_progress_dialog (gchar *text, GdkPixbuf *pixbuf)
{
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *hbox;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  hbox = gtk_hbox_new (FALSE, 0);
  image = gtk_image_new_from_pixbuf (pixbuf);
  label = gtk_label_new (text);

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);

  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
  
  gtk_container_set_border_width (GTK_CONTAINER (hbox), gpe_get_border ());
	
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), hbox);

  g_object_set_data (G_OBJECT (window), "label", label);

  g_signal_connect (G_OBJECT (window), "expose-event", G_CALLBACK (handle_progress_expose), hbox);
  g_signal_connect (G_OBJECT (window), "size-allocate", G_CALLBACK (handle_progress_size_allocate), NULL);

  return window;
}

void
bt_progress_dialog_update (GtkWidget *w, gchar *new_text)
{
  GtkWidget *label;

  label = GTK_WIDGET (g_object_get_data (G_OBJECT (w), "label"));

  gtk_label_set_text (GTK_LABEL (label), new_text);
}

