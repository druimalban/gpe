/*
 * Copyright (C) 2003, 2006 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "gpetimesel.h"
#include "gpeclockface.h"
#include "pixmaps.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(x) gettext(x)
#else
#define _(x) (x)
#endif

struct _GpeTimeSelClass 
{
  GtkHBoxClass parent_class;

  void (*changed)           (struct _GpeTimeSel *);
};

static GtkHBoxClass *parent_class;

static GdkPixbuf *popup_button;

static guint my_signals[1];

static void
gpe_time_sel_emit_changed (GpeTimeSel *sel)
{
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static void
note_change (GObject *obj, GpeTimeSel *sel)
{
  gchar *buf;

  if (!sel->editing)
    {
      buf = g_strdup_printf ("%02d", (int)GTK_ADJUSTMENT (sel->hour_adj)->value);
      gtk_entry_set_text (GTK_ENTRY (sel->hour_edit), buf);
      g_free (buf);
      buf = g_strdup_printf ("%02d", (int)GTK_ADJUSTMENT (sel->minute_adj)->value);
      gtk_entry_set_text (GTK_ENTRY (sel->minute_edit), buf);
      g_free (buf);
    }
  
  if (!sel->changing_time)
    gpe_time_sel_emit_changed (sel);
}

static gint
spin_button_output (GtkSpinButton *spin_button)
{
  gchar *buf = g_strdup_printf ("%02d", (int)spin_button->adjustment->value);

  if (strcmp (buf, gtk_entry_get_text (GTK_ENTRY (spin_button))))
    gtk_entry_set_text (GTK_ENTRY (spin_button), buf);
  g_free (buf);

  return TRUE;
}

static Window
find_deepest_window (Display * dpy, Window grandfather, Window parent,
		     int x, int y, int *rx, int *ry)
{
  int dest_x, dest_y;
  Window child;

  XTranslateCoordinates (dpy, grandfather, parent, x, y,
			 &dest_x, &dest_y, &child);

  if (child == None)
    {
      *rx = dest_x;
      *ry = dest_y;

      return parent;
    }

  return find_deepest_window (dpy, parent, child, dest_x, dest_y, rx, ry);
}

static void
propagate_button_event (GtkWidget *old_widget, GdkEventButton *b)
{
  Window w;
  Display *dpy;
  GdkDisplay *gdisplay;
  GtkWidget *event_widget;
  int x, y;
  GdkWindow *old_window;
  
  dpy = GDK_WINDOW_XDISPLAY (b->window);
  gdisplay = gdk_x11_lookup_xdisplay (dpy);

  w = find_deepest_window (dpy, DefaultRootWindow (dpy), DefaultRootWindow (dpy),
			   b->x_root, b->y_root, &x, &y);

  old_window = b->window;
  b->window = gdk_window_lookup_for_display (gdisplay, w);
  b->x = x;
  b->y = y;
  event_widget = gtk_get_event_widget ((GdkEvent *)b);
  
  g_object_ref (b->window);
  g_object_unref (old_window);

  gtk_propagate_event (event_widget, (GdkEvent *)b);
}

static gboolean
button_press (GtkWidget *w, GdkEventButton *b, GpeTimeSel *sel)
{
  GdkRectangle rect;

  gdk_window_get_frame_extents (w->window, &rect);

  if (b->x < 0 || b->y < 0 || b->x > rect.width || b->y > rect.height)
    {
      gdk_pointer_ungrab (b->time);
      gtk_grab_remove (sel->popup);
      gtk_widget_hide (sel->popup);
      gtk_widget_destroy (sel->popup);
      sel->popup = NULL;

      return TRUE;
    }

  if (b->x >= sel->p_hbox->allocation.x
      && b->y >= sel->p_hbox->allocation.y
      && b->x < sel->p_hbox->allocation.x + sel->p_hbox->allocation.width
      && b->y < sel->p_hbox->allocation.y + sel->p_hbox->allocation.height)
    {
      propagate_button_event (w, b);
      return TRUE;
    }

  gtk_grab_add (sel->clock);
  gdk_pointer_grab (sel->clock->window, FALSE, 
		    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
		    sel->clock->window, NULL, b->time);
  sel->dragging = TRUE;
  b->x -= sel->clock->allocation.x;
  b->y -= sel->clock->allocation.y;
  gtk_widget_event (sel->clock, (GdkEvent *)b);

  return FALSE;
}

static gboolean
button_release (GtkWidget *w, GdkEventButton *b, GpeTimeSel *sel)
{
  if (sel->dragging)
    {
      gtk_grab_remove (sel->clock);

      gdk_pointer_grab (sel->popup->window, FALSE, 
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, b->time);
    }

  if (b->x >= sel->p_hbox->allocation.x
      && b->y >= sel->p_hbox->allocation.y
      && b->x < sel->p_hbox->allocation.x + sel->p_hbox->allocation.width
      && b->y < sel->p_hbox->allocation.y + sel->p_hbox->allocation.height)
    {
      propagate_button_event (w, b);
      return TRUE;
    }

  return FALSE;
}

static void
do_popup (GtkWidget *button, GpeTimeSel *sel)
{
  if (sel->popup)
    {
      gtk_widget_hide (sel->popup);
      gtk_widget_destroy (sel->popup);
      sel->popup = NULL;
    }
  else
    {
      GtkRequisition requisition;
      gint x, y, w, h;
      gint screen_width;
      gint screen_height;
      GdkBitmap *bitmap, *clock_bitmap;
      GtkWidget *vbox;
      GtkWidget *label;
      GtkWidget *hbox;
      GtkWidget *hbox2;
      GdkGC *zero_gc, *one_gc;
      GdkColor zero, one;

      sel->hour_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sel->hour_adj), 1, 0);
      sel->minute_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sel->minute_adj), 1, 0);

      gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (sel->hour_spin), TRUE);
      gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (sel->minute_spin), TRUE);
  
      g_signal_connect (G_OBJECT (sel->hour_spin), "output", G_CALLBACK (spin_button_output), NULL);
      g_signal_connect (G_OBJECT (sel->minute_spin), "output", G_CALLBACK (spin_button_output), NULL);
      
      sel->popup = gtk_window_new (GTK_WINDOW_POPUP);

      sel->clock = gpe_clock_face_new (GTK_ADJUSTMENT (sel->hour_adj), 
				  GTK_ADJUSTMENT (sel->minute_adj),
				  NULL);

      gpe_clock_face_set_do_grabs (GPE_CLOCK_FACE (sel->clock), FALSE);

      hbox = gtk_hbox_new (FALSE, 0);
      hbox2 = gtk_hbox_new (FALSE, 0);
      sel->p_hbox = hbox2;

      label = gtk_label_new (":");

      gtk_box_pack_start (GTK_BOX (hbox), sel->hour_spin, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), sel->minute_spin, FALSE, FALSE, 0);

      gtk_box_pack_start (GTK_BOX (hbox2), hbox, TRUE, FALSE, 0);

      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), sel->clock, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);

      gtk_container_add (GTK_CONTAINER (sel->popup), vbox);

      gdk_window_get_pointer (NULL, &x, &y, NULL);
      gtk_widget_size_request (sel->clock, &requisition);

      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();
      
      x = CLAMP (x - 2, 0, MAX (0, screen_width - requisition.width));
      y = CLAMP (y + 4, 0, MAX (0, screen_height - requisition.height));
      
      gtk_widget_set_uposition (sel->popup, MAX (x, 0), MAX (y, 0));

      g_signal_connect (G_OBJECT (sel->popup), "button_press_event", 
			G_CALLBACK (button_press), sel);
      g_signal_connect (G_OBJECT (sel->popup), "button_release_event", 
			G_CALLBACK (button_release), sel);
      g_signal_connect (G_OBJECT (sel->clock), "button_release_event", 
			G_CALLBACK (button_release), sel);
      
      gtk_widget_realize (sel->popup);
      gtk_widget_show_all (sel->popup);

      bitmap = gdk_pixmap_new (sel->popup->window,
			       sel->popup->allocation.width,
			       sel->popup->allocation.height,
			       1);

      zero_gc = gdk_gc_new (bitmap);
      one_gc = gdk_gc_new (bitmap);

      zero.pixel = 0;
      one.pixel = 1;

      gdk_gc_set_foreground (zero_gc, &zero);
      gdk_gc_set_foreground (one_gc, &one);

      clock_bitmap = gpe_clock_face_get_shape (GPE_CLOCK_FACE (sel->clock));

      gdk_draw_rectangle (GDK_DRAWABLE (bitmap), zero_gc,
			  TRUE, 0, 0, 
			  sel->popup->allocation.width,
			  sel->popup->allocation.height);

      gdk_draw_rectangle (GDK_DRAWABLE (bitmap), one_gc,
			  TRUE, 
			  hbox->allocation.x, 
			  hbox->allocation.y,
			  hbox->allocation.width,
			  hbox->allocation.height);

      gdk_drawable_get_size (clock_bitmap, &w, &h);
      gdk_draw_drawable (GDK_DRAWABLE (bitmap), zero_gc,
			 clock_bitmap, 0, 0, 0, 0,
			 w, h);

      g_object_unref (clock_bitmap);

      g_object_unref (zero_gc);
      g_object_unref (one_gc);

      gtk_widget_shape_combine_mask (sel->popup, bitmap, 0, 0);

      g_object_unref (bitmap);

      sel->dragging = FALSE;

      gdk_pointer_grab (sel->popup->window, FALSE, 
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, GDK_CURRENT_TIME);

      gtk_grab_add (sel->popup);
    }
}

static void
insert_text_handler (GtkEditable *editable,
                     const gchar *text,
                     gint         length,
                     gint        *position,
                     gpointer     data)
{
  int i;
  gboolean ok = TRUE;

  for (i = 0; i < length; i++)
    {
      if (! isdigit (text[i]))
	ok = FALSE;
    }

  if (ok)
    {
      g_signal_handlers_block_by_func (editable,
				       (gpointer) insert_text_handler, data);
      gtk_editable_insert_text (editable, text, length, position);
      g_signal_handlers_unblock_by_func (editable,
					 (gpointer) insert_text_handler, data);
    }

  g_signal_stop_emission_by_name (editable, "insert_text"); 
}

static void
update_from_entry (GtkWidget *w, GtkAdjustment *adj)
{
  const char *p;
  int n;
  GpeTimeSel *sel;

  sel = g_object_get_data (G_OBJECT (w), "sel");

  p = gtk_entry_get_text (GTK_ENTRY (w));
  if (p[0])
    {
      n = atoi (p);
      sel->editing = TRUE;
      gtk_adjustment_set_value (adj, n);
      sel->editing = FALSE;
    }
}

static void
gpe_time_sel_init (GpeTimeSel *sel)
{
  sel->hour_adj = gtk_adjustment_new (0, 0, 23, 1, 15, 15);
  sel->minute_adj = gtk_adjustment_new (0, 0, 59, 1, 15, 15);
  
  g_object_ref (sel->hour_adj);
  g_object_ref (sel->minute_adj);

  g_signal_connect (G_OBJECT (sel->hour_adj), "value-changed", G_CALLBACK (note_change), sel);
  g_signal_connect (G_OBJECT (sel->minute_adj), "value-changed", G_CALLBACK (note_change), sel);

  sel->hour_edit = gtk_entry_new ();
  sel->minute_edit = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (sel->hour_edit), 2);
  gtk_entry_set_width_chars (GTK_ENTRY (sel->minute_edit), 2);

  g_object_set_data (G_OBJECT (sel->hour_edit), "sel", sel);
  g_object_set_data (G_OBJECT (sel->minute_edit), "sel", sel);
  g_signal_connect (G_OBJECT (sel->hour_edit), "changed", G_CALLBACK (update_from_entry), sel->hour_adj);
  g_signal_connect (G_OBJECT (sel->minute_edit), "changed", G_CALLBACK (update_from_entry), sel->minute_adj);
  g_signal_connect (G_OBJECT (sel->hour_edit), "insert_text", G_CALLBACK (insert_text_handler), sel);
  g_signal_connect (G_OBJECT (sel->minute_edit), "insert_text", G_CALLBACK (insert_text_handler), sel);

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
  sel->dragging = sel->editing = FALSE;

  gtk_box_pack_start (GTK_BOX (sel), sel->hour_edit, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sel), sel->label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sel), sel->minute_edit, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sel), sel->button, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (sel->button), "clicked", G_CALLBACK (do_popup), sel);
}

static void
gpe_time_sel_show (GtkWidget *widget)
{
  GpeTimeSel *sel;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GPE_IS_TIME_SEL (widget));

  sel = GPE_TIME_SEL (widget);

  gtk_widget_show (sel->hour_edit);
  gtk_widget_show (sel->minute_edit);
  gtk_widget_show (sel->label);

  GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
gpe_time_sel_class_init (GpeTimeSelClass * klass)
{
  GObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_hbox_get_type ());
  oclass = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  widget_class->show = gpe_time_sel_show;

  my_signals[0] = g_signal_new ("changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GpeTimeSelClass, changed),
				NULL, NULL,
				gtk_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
}

GtkType
gpe_time_sel_get_type (void)
{
  static GType time_sel_type = 0;

  if (! time_sel_type)
    {
      static const GTypeInfo time_sel_info =
      {
        sizeof (GpeTimeSelClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpe_time_sel_class_init,
        (GClassFinalizeFunc) NULL,
        NULL /* class_data */,
        sizeof (GpeTimeSel),
        0 /* n_preallocs */,
        (GInstanceInitFunc) gpe_time_sel_init,
      };

      time_sel_type = g_type_register_static (GTK_TYPE_HBOX, "GpeTimeSel", &time_sel_info, (GTypeFlags)0);
    }
  return time_sel_type;
}

void
gpe_time_sel_get_time (GpeTimeSel *sel, guint *hour, guint *minute)
{
  if (hour)
    *hour = (guint) gtk_adjustment_get_value (GTK_ADJUSTMENT (sel->hour_adj));
  if (minute)
    *minute = (guint) gtk_adjustment_get_value (GTK_ADJUSTMENT (sel->minute_adj));
}

void
gpe_time_sel_set_time (GpeTimeSel *sel, guint hour, guint minute)
{
  sel->changing_time = TRUE;
  gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->hour_adj), hour);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->minute_adj), minute);
  sel->changing_time = FALSE;
}

GtkWidget *
gpe_time_sel_new (void)
{
  return GTK_WIDGET (g_object_new (gpe_time_sel_get_type (), NULL));
}
