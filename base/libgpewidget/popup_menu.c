/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <gtk/gtk.h>

void
popup_menu_close (GtkWidget *parent_button)
{
  GtkWidget *parent_arrow;

  parent_arrow = g_object_get_data (G_OBJECT (parent_button), "arrow");

  gtk_widget_destroy (g_object_get_data (G_OBJECT (parent_button), "window"));
  g_object_set_data (G_OBJECT (parent_button), "active", FALSE);
  gtk_arrow_set (GTK_ARROW (parent_arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
}

static void
toggle_popup_menu (GtkWidget *parent_button, GtkWidget *(construct_func)())
{
  GtkWidget *popup_window, *frame, *child_widget;
  GtkWidget *parent_arrow;
  GtkRequisition frame_requisition, parent_button_requisition;
  gint x, y;
  gint screen_width;
  gint screen_height;

  parent_arrow = g_object_get_data (G_OBJECT (parent_button), "arrow");

  if ((gboolean) g_object_get_data (G_OBJECT (parent_button), "active") == TRUE)
  {
    popup_menu_close (parent_button);
  }
  else
  {
    g_object_set_data (G_OBJECT (parent_button), "active", (gpointer) TRUE);
    gtk_arrow_set (GTK_ARROW (parent_arrow), GTK_ARROW_UP, GTK_SHADOW_NONE);

    popup_window = gtk_window_new (GTK_WINDOW_POPUP);
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

    child_widget = construct_func (parent_button);

    gtk_container_add (GTK_CONTAINER (frame), child_widget);
    gtk_container_add (GTK_CONTAINER (popup_window), frame);

    gtk_widget_size_request (parent_button, &parent_button_requisition);
    gtk_widget_size_request (frame, &frame_requisition);

    gdk_window_get_position (gtk_widget_get_parent_window (parent_button), &x, &y);

    screen_width = gdk_screen_width ();
    screen_height = gdk_screen_height ();
      
    x = CLAMP (x + parent_button->allocation.x, 0, MAX (0, screen_width - frame_requisition.width));
    y += parent_button->allocation.y;
    y += parent_button_requisition.height;

    gtk_widget_set_uposition (popup_window, x, y);
      
    g_object_set_data (G_OBJECT (parent_button), "window", popup_window);

    gtk_widget_show_all (popup_window);
  }
}

GtkWidget *
popup_menu_button_new (GtkWidget *child, GtkWidget *(construct_func)(), void *(callback_func)())
{
  GtkWidget *button, *arrow, *hbox;
  GtkRequisition requisition;
  gint width = 0, height;

  button = gtk_button_new ();
  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  hbox = gtk_hbox_new (FALSE, 0);

  gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);

  g_object_set_data (G_OBJECT (button), "active", FALSE);
  g_object_set_data (G_OBJECT (button), "hbox", hbox);
  g_object_set_data (G_OBJECT (button), "child", child);
  g_object_set_data (G_OBJECT (button), "arrow", arrow);
  g_object_set_data (G_OBJECT (button), "callback", callback_func);

  gtk_box_pack_start (GTK_BOX (hbox), child, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), arrow, FALSE, FALSE, 0);

  gtk_widget_size_request (child, &requisition);
  width = width + requisition.width;
  height = requisition.height;

  gtk_widget_size_request (arrow, &requisition);
  width = (width + requisition.width) - 5;

  gtk_signal_connect (GTK_OBJECT (button), "pressed",
		      GTK_SIGNAL_FUNC (toggle_popup_menu), (gpointer) construct_func);

  gtk_widget_set_size_request (hbox, width, height);
  gtk_container_add (GTK_CONTAINER (button), hbox);
  gtk_widget_show_all (button);

  return button;
}

GtkWidget *
popup_menu_button_new_from_stock (const gchar *stock_id, GtkWidget *(construct_func)(), void *(callback_func)())
{
  GtkWidget *image;

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_SMALL_TOOLBAR);

  return popup_menu_button_new (image, construct_func, callback_func);
}

static int
construct_font_popup_cmp_families (const void *a, const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);
  
  return g_utf8_collate (a_name, b_name);
}

static GtkWidget *
construct_font_popup (GtkWidget *parent_button)
{
  GtkWidget *vbox, *button, *alignment, *scrolled_window, *button_label;
  GtkRcStyle *button_label_rc_style;
  PangoFontFamily **families;
  gint n_families, i;

  vbox = gtk_vbox_new (FALSE, 0);
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  pango_context_list_families (gtk_widget_get_pango_context (GTK_WIDGET (parent_button)), &families, &n_families);
  qsort (families, n_families, sizeof (PangoFontFamily *), construct_font_popup_cmp_families);

  for (i=0; i<n_families; i++)
  {
    const gchar *font_name = pango_font_family_get_name (families[i]);

    button_label = gtk_label_new (font_name);
    button_label_rc_style = gtk_rc_style_new ();
    button_label_rc_style->font_desc = pango_font_description_from_string (g_strdup_printf ("%s 9", font_name));
    gtk_widget_modify_style (button_label, button_label_rc_style);

    alignment = gtk_alignment_new (0, 0, 0, 0);
    button = gtk_button_new ();
    g_signal_connect (G_OBJECT (button), "clicked",
    		      GTK_SIGNAL_FUNC (void *(g_object_get_data (G_OBJECT (button), "callback"))()), families[i]);
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    g_object_set_data (G_OBJECT (button), "parent_button", parent_button);
    gtk_container_add (GTK_CONTAINER (alignment), button_label);
    gtk_container_add (GTK_CONTAINER (button), alignment);
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  }

  g_free (families);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox);

  return scrolled_window;
}

GtkWidget *
popup_menu_button_new_type_font (void *(callback_func)())
{
  return popup_menu_button_new_from_stock (GTK_STOCK_SELECT_FONT, construct_font_popup, callback_func);
}
