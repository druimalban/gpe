/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "gtktimesel.h"
#include "gpeclockface.h"
#include "pixmaps.h"

#define _(x) gettext(x)

struct _GtkTimeSelClass 
{
  GtkHBoxClass parent_class;
};

static GtkHBoxClass *parent_class;

static GdkPixbuf *popup_button;

static gboolean
button_press (GtkWidget *w, GdkEventButton *b, GtkTimeSel *sel)
{
  GdkRectangle rect;

  gdk_window_get_frame_extents (w->window, &rect);

  if (b->x < 0 || b->y < 0 || b->x > rect.width || b->y > rect.height)
    {
      gdk_pointer_ungrab (b->time);

      gtk_widget_hide (sel->popup);
      gtk_widget_destroy (sel->popup);
      sel->popup = NULL;

      return TRUE;
    }

  gdk_pointer_grab (w->window, FALSE, 
		    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
		    w->window, NULL, b->time);

  return FALSE;
}

static gboolean
button_release (GtkWidget *w, GdkEventButton *b, GtkTimeSel *sel)
{
  gdk_pointer_grab (w->window, FALSE, 
		    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, b->time);

  return FALSE;
}

static void
do_popup (GtkWidget *button, GtkTimeSel *sel)
{
  if (sel->popup)
    {
      gtk_widget_hide (sel->popup);
      gtk_widget_destroy (sel->popup);
      sel->popup = NULL;
    }
  else
    {
      GtkWidget *clock;
      GtkRequisition requisition;
      gint x, y;
      gint screen_width;
      gint screen_height;
      
      sel->popup = gtk_window_new (GTK_WINDOW_POPUP);

      clock = gpe_clock_face_new (GTK_ADJUSTMENT (sel->hour_adj), 
				  GTK_ADJUSTMENT (sel->minute_adj));

      gpe_clock_face_set_do_grabs (GPE_CLOCK_FACE (clock), FALSE);

      gtk_container_add (GTK_CONTAINER (sel->popup), clock);

      gdk_window_get_pointer (NULL, &x, &y, NULL);
      gtk_widget_size_request (clock, &requisition);

      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();
      
      x = CLAMP (x - 2, 0, MAX (0, screen_width - requisition.width));
      y = CLAMP (y + 4, 0, MAX (0, screen_height - requisition.height));
      
      gtk_widget_set_uposition (sel->popup, MAX (x, 0), MAX (y, 0));

      g_signal_connect (G_OBJECT (clock), "button_press_event", 
			G_CALLBACK (button_press), sel);
      g_signal_connect (G_OBJECT (clock), "button_release_event", 
			G_CALLBACK (button_release), sel);

      gtk_widget_show_all (sel->popup);

      gdk_pointer_grab (clock->window, FALSE, 
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, GDK_CURRENT_TIME);
    }
}

static void
gtk_time_sel_init (GtkTimeSel *sel)
{
  sel->hour_adj = gtk_adjustment_new (0, 0, 23, 1, 15, 15);
  sel->minute_adj = gtk_adjustment_new (0, 0, 59, 1, 15, 15);

  sel->hour_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sel->hour_adj), 1, 0);
  sel->minute_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sel->minute_adj), 1, 0);

  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (sel->hour_spin), TRUE);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (sel->minute_spin), TRUE);

  sel->label = gtk_label_new (":");

  if (popup_button == NULL)
    popup_button = gpe_try_find_icon ("clock-popup", NULL);

  sel->button = gtk_button_new ();

  if (popup_button)
    {
      GtkWidget *image = gtk_image_new_from_pixbuf (popup_button);
      gtk_container_add (GTK_CONTAINER (sel->button), image);
    }

  sel->popup = NULL;

  gtk_box_pack_start (GTK_BOX (sel), sel->hour_spin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (sel), sel->label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sel), sel->minute_spin, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (sel), sel->button, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (sel->button), "clicked", G_CALLBACK (do_popup), sel);
}

static void
gtk_time_sel_show (GtkWidget *widget)
{
  GtkTimeSel *sel;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TIME_SEL (widget));

  sel = GTK_TIME_SEL (widget);

  gtk_widget_show (sel->hour_spin);
  gtk_widget_show (sel->minute_spin);
  gtk_widget_show (sel->label);

  GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
gtk_time_sel_class_init (GtkTimeSelClass * klass)
{
  GObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_hbox_get_type ());
  oclass = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  widget_class->show = gtk_time_sel_show;
}

GtkType
gtk_time_sel_get_type (void)
{
  static GType time_sel_type = 0;

  if (! time_sel_type)
    {
      static const GTypeInfo time_sel_info =
      {
	sizeof (GtkTimeSelClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gtk_time_sel_class_init,
	(GClassFinalizeFunc) NULL,
	NULL /* class_data */,
	sizeof (GtkTimeSel),
	0 /* n_preallocs */,
	(GInstanceInitFunc) gtk_time_sel_init,
      };

      time_sel_type = g_type_register_static (GTK_TYPE_HBOX, "GtkTimeSel", &time_sel_info, (GTypeFlags)0);
    }
  return time_sel_type;
}

void
gtk_time_sel_get_time (GtkTimeSel *sel, guint *hour, guint *minute)
{
  if (hour)
    *hour = (guint) gtk_adjustment_get_value (GTK_ADJUSTMENT (sel->hour_adj));
  if (minute)
    *minute = (guint) gtk_adjustment_get_value (GTK_ADJUSTMENT (sel->minute_adj));
}

void
gtk_time_set_set_time (GtkTimeSel *sel, guint hour, guint minute)
{
  gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->hour_adj), hour);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->minute_adj), minute);
}

GtkWidget *
gtk_time_sel_new (void)
{
  return GTK_WIDGET (g_object_new (gtk_time_sel_get_type (), NULL));
}
