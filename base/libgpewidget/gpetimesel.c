/*
 * Copyright (C) 2003, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@gnu.org>
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

#define GTK_TYPE_NUM_CELL_RENDERER (num_cell_renderer_get_type ())
#define NUM_CELL_RENDERER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_NUM_CELL_RENDERER, NumCellRenderer))
#define NUM_CELL_RENDERER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                            GTK_TYPE_NUM_CELL_RENDERER, NumCellRendererClass))
#define NUM_CELL_RENDERER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              GTK_TYPE_NUM_CELL_RENDERER, NumCellRendererClass))

typedef struct
{
  GtkCellRendererClass cell_renderer;
} NumCellRendererClass;

static GObjectClass *num_parent_class;

typedef struct
{
  GtkCellRenderer cell_renderer;
  char *fmt;
  char *num;
  gboolean bold;
} NumCellRenderer;

static void num_cell_renderer_class_init (gpointer klass, gpointer data);

static GType
num_cell_renderer_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (NumCellRendererClass),
        NULL,
        NULL,
        num_cell_renderer_class_init,
        NULL,
        NULL,
        sizeof (NumCellRenderer),
        0,
        NULL
      };

      type = g_type_register_static (gtk_cell_renderer_get_type (),
				     "NumCellRenderer", &info, 0);
    }

  return type;
}

static void num_cell_renderer_finalize (GObject *object);
static void num_cell_renderer_render (GtkCellRenderer *cell,
				      GdkWindow *window,
				      GtkWidget *widget,
				      GdkRectangle *background_area,
				      GdkRectangle *cell_area,
				      GdkRectangle *expose_area,
				      GtkCellRendererState flags);
static void num_cell_renderer_get_size (GtkCellRenderer *cell,
					GtkWidget *widget,
					GdkRectangle *cell_area,
					gint *x_offset,
					gint *y_offset,
					gint *width,
					gint *height);

static void
num_cell_renderer_class_init (gpointer klass, gpointer data)
{
  num_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = num_cell_renderer_finalize;
  
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);
  cell_class->get_size = num_cell_renderer_get_size;
  cell_class->render = num_cell_renderer_render;
}

static void
num_cell_renderer_finalize (GObject *object)
{
  NumCellRenderer *renderer = NUM_CELL_RENDERER (object);

  if (renderer->fmt)
    {
      g_free (renderer->num);
      g_free (renderer->fmt);
    }

  (* G_OBJECT_CLASS (num_parent_class)->finalize) (object);
}

static PangoLayout*
get_layout (NumCellRenderer *cell,
            GtkWidget *widget,
	    int force_bold)
{
  PangoLayout *layout;

  if (force_bold || cell->bold)
    {
      char *buf = g_strdup_printf ("<b>%s</b>", cell->num);
      layout = gtk_widget_create_pango_layout (widget, NULL);
      pango_layout_set_markup (layout, buf, -1);
      g_free (buf);
    }
  else
    layout = gtk_widget_create_pango_layout (widget, cell->num);

  return layout;
}

static void
num_cell_renderer_get_size (GtkCellRenderer *cell,
			    GtkWidget *widget,
			    GdkRectangle *cell_area,
			    gint *x_offset,
			    gint *y_offset,
			    gint *width,
			    gint *height)
{
  NumCellRenderer *num_cell_renderer = NUM_CELL_RENDERER (cell);

  PangoLayout *pl = get_layout (num_cell_renderer, widget, 1);
  PangoRectangle rect;
  pango_layout_get_pixel_extents (pl, NULL, &rect);
  g_object_unref (pl);

  if (height)
    *height = 2 + rect.height;
  if (width)
    *width = 2 + rect.width;
}

static void
num_cell_renderer_render (GtkCellRenderer *cell,
			  GdkWindow *window,
			  GtkWidget *widget,
			  GdkRectangle *background_area,
			  GdkRectangle *cell_area,
			  GdkRectangle *expose_area,
			  GtkCellRendererState flags)
{
  PangoLayout *pl = get_layout (NUM_CELL_RENDERER (cell), widget, 0);

  gtk_paint_layout (widget->style, window, GTK_WIDGET_STATE (widget),
                    TRUE, expose_area, widget,
                    "cellrenderertext",
                    cell_area->x + 1,
                    cell_area->y + 1,
                    pl);

  g_object_unref (pl);
}

static GtkCellRenderer *
num_cell_renderer_new (void)
{
  return g_object_new (GTK_TYPE_NUM_CELL_RENDERER, NULL);
}

static void
num_cell_data_func (GtkCellLayout *cell_layout,
		    GtkCellRenderer *cell_renderer,
		    GtkTreeModel *model,
		    GtkTreeIter *iter,
		    gpointer data)
{
  NumCellRenderer *cell = NUM_CELL_RENDERER (cell_renderer);
  char *num;

  gtk_tree_model_get (model, iter, 0, &num, -1);
  if (cell->fmt)
    {
      g_free (cell->num);
      cell->num = g_strdup_printf (cell->fmt, num);
    }
  else
    cell->num = num;

  GtkTreeIter active_iter;
  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (data), &active_iter))
    {
      char *active_num;
      gtk_tree_model_get (model, &active_iter, 0, &active_num, -1);
      cell->bold = strcmp (num, active_num) == 0;
    }
  else
    cell->bold = 0;
}

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

static Window
find_deepest_window (Display * dpy, Window grandfather, Window parent,
		     int x, int y, int *rx, int *ry)
{
  int dest_x, dest_y;
  Window child;

  while (1)
    {
      XTranslateCoordinates (dpy, grandfather, parent, x, y,
			     &dest_x, &dest_y, &child);

      if (child == None)
	{
	  *rx = dest_x;
	  *ry = dest_y;

	  return parent;
	}

      grandfather = parent;
      parent = child;
      x = dest_x;
      y = dest_y;
    }
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
adj_change (GObject *obj, GpeTimeSel *sel)
{
  char buf[3];
  snprintf (buf, sizeof (buf), "%d",
	    (int)GTK_ADJUSTMENT (sel->hour_adj)->value);
  gtk_entry_set_text (GTK_ENTRY (GTK_BIN (sel->hour_edit)->child), buf);

  snprintf (buf, sizeof (buf), "%02d",
	    (int)GTK_ADJUSTMENT (sel->minute_adj)->value);
  gtk_entry_set_text (GTK_ENTRY (GTK_BIN (sel->minute_edit)->child), buf);
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

static void
do_popup (GtkWidget *button, GpeTimeSel *sel)
{
  if (sel->popup)
    {
      gtk_widget_hide (sel->popup);
      gtk_widget_destroy (sel->popup);
      sel->popup = NULL;

      sel->hour_adj = NULL;
      sel->minute_adj = NULL;
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

      sel->hour_adj = gtk_adjustment_new (0, 0, 23, 1, 15, 15);
      sel->minute_adj = gtk_adjustment_new (0, 0, 59, 1, 15, 15);

      int hour = atoi (gtk_entry_get_text
		       (GTK_ENTRY (GTK_BIN (sel->hour_edit)->child)));
      int minute = atoi (gtk_entry_get_text
			 (GTK_ENTRY (GTK_BIN (sel->minute_edit)->child)));
      gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->hour_adj), hour);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->minute_adj), minute);

      g_signal_connect (G_OBJECT (sel->hour_adj),
			"value-changed", G_CALLBACK (adj_change), sel);
      g_signal_connect (G_OBJECT (sel->minute_adj),
			"value-changed", G_CALLBACK (adj_change), sel);

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
combo_entry_change (GObject *obj, GpeTimeSel *sel)
{
  if (!sel->changing_time)
    gpe_time_sel_emit_changed (sel);
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

static int
entry_key_press_event (GtkWidget *widget, GdkEventKey *k)
{
  /* Translate common handwriting errors.  */
  if (k->keyval == 'l')
    k->keyval = '1';
  if (k->keyval == 'o' || k->keyval == 'O')
    k->keyval = '0';

  return FALSE;
}

static GtkListStore *hour_model;
static GtkListStore *minute_model;

static void
squash_pointer (gpointer data, GObject *object)
{
  g_assert (* (GObject **) data == object);
  * (GObject **) data = NULL;
}

static void
gpe_time_sel_init (GpeTimeSel *sel)
{
  int hour_model_deref = 0;
  if (! hour_model)
    /* Create the model.  */
    {
      hour_model = gtk_list_store_new (1, G_TYPE_STRING);

      int i;
      for (i = 0; i < 24; i ++)
	{
	  GtkTreeIter iter;
	  gtk_list_store_append (hour_model, &iter);
	  gtk_list_store_set (hour_model, &iter,
			      0, g_strdup_printf ("%d", i), -1);
	}

      g_object_weak_ref (G_OBJECT (hour_model), squash_pointer, &hour_model);
      hour_model_deref = 1;
    }

  int minute_model_deref = 0;
  if (! minute_model)
    /* Create the model.  */
    {
      minute_model = gtk_list_store_new (1, G_TYPE_STRING);

      int i;
      for (i = 0; i < 60; i += 5)
	{
	  GtkTreeIter iter;
	  gtk_list_store_append (minute_model, &iter);
	  gtk_list_store_set (minute_model, &iter,
			      0, g_strdup_printf ("%02d", i), -1);
	}

      g_object_weak_ref (G_OBJECT (minute_model), squash_pointer,
			 &minute_model);
      minute_model_deref = 1;
    }

  /* The hour. */
  sel->hour_edit
    = gtk_combo_box_entry_new_with_model (GTK_TREE_MODEL (hour_model), 0);
  if (hour_model_deref)
    g_object_unref (hour_model);
  gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (sel->hour_edit), 12);
  gtk_entry_set_width_chars (GTK_ENTRY (GTK_BIN (sel->hour_edit)->child), 3);
  gtk_entry_set_max_length (GTK_ENTRY (GTK_BIN (sel->hour_edit)->child), 2);
  gtk_entry_set_alignment (GTK_ENTRY (GTK_BIN (sel->hour_edit)->child), 1);

  /* Set our custom renderer.  */
  GtkCellRenderer *renderer = num_cell_renderer_new ();
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (sel->hour_edit));
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (sel->hour_edit),
			      renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (sel->hour_edit),
				      renderer,
				      num_cell_data_func, sel->hour_edit,
				      NULL);
  g_signal_connect (G_OBJECT (sel->hour_edit), "changed",
		    G_CALLBACK (combo_entry_change), sel);
  g_signal_connect (G_OBJECT (GTK_BIN (sel->hour_edit)->child),
		    "insert_text", G_CALLBACK (insert_text_handler), sel);
  g_signal_connect (G_OBJECT (GTK_BIN (sel->hour_edit)->child),
		    "key-press-event", G_CALLBACK (entry_key_press_event),
		    NULL);

  gtk_box_pack_start (GTK_BOX (sel), sel->hour_edit, FALSE, FALSE, 0);
  gtk_widget_show (sel->hour_edit);

  /* The `:'.  */
  sel->label = gtk_label_new (":");
  gtk_widget_show (sel->label);
  gtk_box_pack_start (GTK_BOX (sel), sel->label, FALSE, FALSE, 0);

  /* The minute.  */
  sel->minute_edit
    = gtk_combo_box_entry_new_with_model (GTK_TREE_MODEL (minute_model), 0);
  if (minute_model_deref)
    g_object_unref (minute_model);
  gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (sel->minute_edit), 3);
  gtk_entry_set_width_chars (GTK_ENTRY (GTK_BIN (sel->minute_edit)->child), 3);
  gtk_entry_set_max_length (GTK_ENTRY (GTK_BIN (sel->minute_edit)->child), 2);
  gtk_entry_set_alignment (GTK_ENTRY (GTK_BIN (sel->minute_edit)->child), 0);

  /* Set our custom renderer.  */
  renderer = num_cell_renderer_new ();
  NUM_CELL_RENDERER (renderer)->fmt = g_strdup (":%s");
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (sel->minute_edit));
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (sel->minute_edit),
			      renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (sel->minute_edit),
				      renderer,
				      num_cell_data_func, sel->minute_edit,
				      NULL);
  g_signal_connect (G_OBJECT (sel->minute_edit), "changed",
		    G_CALLBACK (combo_entry_change), sel);
  g_signal_connect (G_OBJECT (GTK_BIN (sel->minute_edit)->child),
		    "insert_text", G_CALLBACK (insert_text_handler), sel);
  g_signal_connect (G_OBJECT (GTK_BIN (sel->minute_edit)->child),
		    "key-press-event", G_CALLBACK (entry_key_press_event),
		    NULL);

  gtk_box_pack_start (GTK_BOX (sel), sel->minute_edit, FALSE, FALSE, 0);
  gtk_widget_show (sel->minute_edit);


  sel->button = gtk_button_new ();

  if (popup_button == NULL)
    popup_button = gpe_try_find_icon ("clock-popup", NULL);
  if (popup_button)
    {
      GtkWidget *image = gtk_image_new_from_pixbuf (popup_button);
      gtk_container_add (GTK_CONTAINER (sel->button), image);
    }
  g_signal_connect (G_OBJECT (sel->button), "clicked",
		    G_CALLBACK (do_popup), sel);
  gtk_box_pack_start (GTK_BOX (sel), sel->button, FALSE, FALSE, 0);
}

static void
gpe_time_sel_class_init (GpeTimeSelClass * klass)
{
  GObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_hbox_get_type ());
  oclass = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  my_signals[0]
    = g_signal_new ("changed",
		    G_OBJECT_CLASS_TYPE (klass),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (struct _GpeTimeSelClass, changed),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__VOID,
		    G_TYPE_NONE,
		    0);
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
    *hour = CLAMP (atoi (gtk_entry_get_text
			 (GTK_ENTRY (GTK_BIN (sel->hour_edit)->child))),
		   0, 23);
  if (minute)
    *minute = CLAMP (atoi (gtk_entry_get_text
			   (GTK_ENTRY (GTK_BIN (sel->minute_edit)->child))),
		     0, 60);
}

void
gpe_time_sel_set_time (GpeTimeSel *sel, guint hour, guint minute)
{
  g_return_if_fail (hour <= 23);
  g_return_if_fail (minute <= 59);

  sel->changing_time = TRUE;
  if (sel->hour_adj)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->hour_adj), hour);
  if (sel->minute_adj)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (sel->minute_adj), minute);

  gtk_combo_box_set_active (GTK_COMBO_BOX (sel->hour_edit), hour);
  if (minute % 5 == 0)
    gtk_combo_box_set_active (GTK_COMBO_BOX (sel->minute_edit), minute / 5);
  else
    {
      char m[3];
      sprintf (m, "%02d", minute);
      gtk_entry_set_text (GTK_ENTRY (GTK_BIN (sel->minute_edit)->child), m);
    }

  sel->changing_time = FALSE;
}

GtkWidget *
gpe_time_sel_new (void)
{
  return GTK_WIDGET (g_object_new (gpe_time_sel_get_type (), NULL));
}
