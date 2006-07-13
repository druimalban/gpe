/* pannedwindow.c - Panned window implementation.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include "pannedwindow.h"

#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkmarshal.h>
#include <gdk/gdkx.h>

#include <stdlib.h>

typedef struct _PannedWindowPrivate PannedWindowPrivate;

#define PANNED_WINDOW_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_PANNED_WINDOW, \
                                PannedWindowPrivate))

struct _PannedWindowPrivate
{
  GdkWindow *original_window;

  /* Whether we should consider motion for panning.  */
  gboolean consider_motion;

  /* Whether we are currently panning.  */
  gboolean panning;

  /* Where the drag started (relative to the root).  */
  gdouble x_origin;
  gdouble y_origin;
  /* The location of the last processed "drag-motion" signal
     position (relative to the root).  */
  gdouble x;
  gdouble y;

  gdouble motion_timer;

  /* When we hit the edge.  */
  guint32 at_edge;

  /* The distance travelled since the last edge flip.  */
  int x_distance;
  int y_distance;
};

enum {
  EDGE_FLIP,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];

static void panned_window_class_init (gpointer klass,
					  gpointer klass_data);
static void panned_window_init (GTypeInstance *instance,
				    gpointer klass);
static void panned_window_dispose (GObject *obj);
static void panned_window_finalize (GObject *object);
static gboolean button_press_event (GtkWidget *widget,
				    GdkEventButton *event);
static gboolean button_release_event (GtkWidget *widget,
				      GdkEventButton *event);
static gboolean motion_notify_event (GtkWidget *widget,
				     GdkEventMotion *event);

GType
panned_window_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (PannedWindowClass),
	NULL,
	NULL,
	panned_window_class_init,
	NULL,
	NULL,
	sizeof (PannedWindowClass),
	0,
	panned_window_init,
      };

      type = g_type_register_static (GTK_TYPE_EVENT_BOX,
				     "PannedWindow", &info, 0);
    }

  return type;
}

static GObjectClass *parent_class;

static void
panned_window_class_init (gpointer klass, gpointer klass_data)
{
  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private (klass, sizeof (PannedWindowPrivate));

  GObjectClass *object_class;
  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = panned_window_finalize;
  object_class->dispose = panned_window_dispose;

  GtkWidgetClass *widget_class;
  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->button_press_event = button_press_event;
  widget_class->button_release_event = button_release_event;
  widget_class->motion_notify_event = motion_notify_event;

  signals[EDGE_FLIP]
    = g_signal_new ("edge-flip",
		    G_TYPE_FROM_CLASS (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (PannedWindowClass, edge_flip),
		    NULL,
		    NULL,
		    gtk_marshal_VOID__UINT,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_UINT);
}

static void
panned_window_init (GTypeInstance *instance, gpointer klass)
{
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (instance), TRUE);
  gtk_widget_add_events (GTK_WIDGET (instance),
GDK_POINTER_MOTION_HINT_MASK |
			 GDK_BUTTON1_MOTION_MASK
			 | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (instance), scrolled_window);
  gtk_widget_show (scrolled_window);
}

static void
panned_window_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
panned_window_finalize (GObject *object)
{
  PannedWindowPrivate *priv = PANNED_WINDOW_GET_PRIVATE (object);

  if (priv->motion_timer)
    g_source_remove (priv->motion_timer);

  if (priv->original_window)
    g_object_remove_weak_pointer (object,
				  (gpointer *) &priv->original_window);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

gboolean
panned_window_is_panning (PannedWindow *pw)
{
  PannedWindowPrivate *priv = PANNED_WINDOW_GET_PRIVATE (pw);

  return priv->panning;
}

static gboolean
button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  if (GTK_WIDGET_CLASS (parent_class)->button_press_event
      && GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event))
    return TRUE;

  if (event->button == 1)
    {
      PannedWindowPrivate *priv = PANNED_WINDOW_GET_PRIVATE (widget);

      if (priv->original_window)
	{
	  g_object_remove_weak_pointer (G_OBJECT (widget),
					(gpointer *) &priv->original_window);
	  priv->original_window = NULL;
	}

      if (event->window != widget->window)
	{
	  g_object_add_weak_pointer (G_OBJECT (widget),
				     (gpointer *) &priv->original_window);
	  priv->original_window = event->window;
	}

      gdk_pointer_grab (widget->window, FALSE,
			GDK_BUTTON1_MOTION_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_ENTER_NOTIFY_MASK
			| GDK_LEAVE_NOTIFY_MASK,
			NULL, NULL,
			event->time);

      priv->consider_motion = TRUE;

      return TRUE;
    }

  return FALSE;
}

static gboolean
button_release_event (GtkWidget *widget, GdkEventButton *event)
{
  PannedWindowPrivate *priv = PANNED_WINDOW_GET_PRIVATE (widget);

  if (priv->consider_motion)
    {
      gboolean ret = FALSE;

      /* We grab at the button press event and only set PRIV->PANNING
	 in the motion event.  */
      gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
				  event->time);

      if (priv->panning)
	{
	  priv->consider_motion = FALSE;
	  g_source_remove (priv->motion_timer);
	  priv->motion_timer = 0;
	}

      if (priv->original_window)
	/* Propagate the button release event.  */
	{
	  g_object_remove_weak_pointer (G_OBJECT (widget),
					(gpointer *) &priv->original_window);

	  if (gdk_window_is_visible (priv->original_window))
	    {
	      gpointer nwidget;
	      gdk_window_get_user_data (priv->original_window, &nwidget);

	      GdkEvent *e = gdk_event_copy ((GdkEvent *) event);

	      g_object_unref (e->button.window);
	      e->button.window = priv->original_window;
	      g_object_ref (e->button.window);
	      priv->original_window = NULL;

	      int x = event->x;
	      int y = event->y;
	      int nx, ny;
	      gtk_widget_translate_coordinates (widget,
						GTK_WIDGET (nwidget),
						x, y, &nx, &ny);

	      e->button.x = nx;
	      e->button.y = ny;

	      ret = gtk_main_do_event ((GdkEvent *) e);
	    }
	  else
	    priv->original_window = NULL;
	}

      if (priv->panning)
	priv->panning = FALSE;

      return ret;
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_release_event)
    return GTK_WIDGET_CLASS (parent_class)
      ->button_release_event (widget, event);

  return FALSE;
}

#define FPS 15

/* Called periodically when the mouse is held down to continue
   scrolling.  */
static gboolean
process_motion (PannedWindow *pw)
{
  PannedWindowPrivate *priv = PANNED_WINDOW_GET_PRIVATE (pw);

  int x, y;
  GdkModifierType mask;
  gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, &mask);

  GtkScrolledWindow *scrolled_window
    = GTK_SCROLLED_WINDOW (GTK_BIN (pw)->child);
  GtkWidget *child = GTK_BIN (scrolled_window)->child;

  guint32 t = gdk_x11_get_server_time (child->window);
  if (t == 0)
    /* We have reserved 0 to mean that we are not at an edge.  We
       could use a boolean but we have 32-bits so this case won't
       happen often and when it does we just use a nearby value.  */
    t = -1;

  int child_x, child_y;
  gtk_widget_get_pointer (child, &child_x, &child_y);

  gboolean inside
    = child_x >= 0 && child_x < child->allocation.width
    && child_y >= 0 && child_y < child->allocation.height;

  GtkAdjustment *hadj
    = gtk_scrolled_window_get_hadjustment (scrolled_window);
  GtkAdjustment *vadj
    = gtk_scrolled_window_get_vadjustment (scrolled_window);

  /* If the mouse movement has caused the an adjustment to bump an
     edge.  */
  gboolean at_edge = FALSE;

  /* Difference between the current cursor position and where the drag
     started.  */
  int dx = priv->x_origin - x;
  int dy = priv->y_origin - y;

  gdouble dx_share = (gdouble) (dx * dx) / (dx * dx + dy * dy);
  gdouble dy_share = (gdouble) (dy * dy) / (dx * dx + dy * dy);

  void scroll_by (GtkAdjustment *adj, gdouble delta, int *acc)
    {
      if (! adj)
	{
	  *acc += delta;
	  at_edge = TRUE;
	  if (! priv->at_edge)
	    priv->at_edge = t;
	  return;
	}

      gdouble value = adj->value + delta;
      if (value < adj->lower || value > adj->upper - adj->page_size)
	{
	  *acc += delta;
	  at_edge = TRUE;
	  if (! priv->at_edge)
	    priv->at_edge = t;
	}
      else
	*acc = 0;

      gtk_adjustment_set_value
	(adj, CLAMP (value, adj->lower, adj->upper - adj->page_size));
    }

  if (inside)
    /* The mouse is inside the child.  Do normal panning.  */
    {
      scroll_by (vadj, - (y - priv->y), &priv->y_distance);
      scroll_by (hadj, - (x - priv->x), &priv->x_distance);

      if (! at_edge)
	goto out;
    }
  else
    /* We are outside the viewport.  We scroll a few pixels
       automatically.  */
    {
      void scroll (GtkAdjustment *adj, int diff,
		   gdouble share, int *acc)
	{
	  if (! (diff < -10 || 10 <= diff))
	    return;

	  /* Scroll by 100 / FPS * share pixels.  */
	  gdouble delta = (100.0 / FPS) * share;
	  if (diff < 0)
	    delta = -delta;

	  scroll_by (adj, delta, acc);
	}

      scroll (hadj, dx, dx_share, &priv->x_distance);
      scroll (vadj, dy, dy_share, &priv->y_distance);
    }


  /* The mouse is either outside of the viewport or we are at an
     edge.  */

  if (! priv->at_edge)
    {
      priv->at_edge = t;
      priv->x_distance = 0;
      priv->y_distance = 0;
    }

  if (at_edge && t - priv->at_edge >= 500)
    /* We've bumped an edge at least half second ago and traveled at
       least 100 ms ago.  */
    {
      enum PannedWindowEdge edge = 0;

      if (dx_share > 0.3 && abs (priv->x_distance) > 100)
	{
	  if (hadj)
	    {
	      if (hadj->value <= hadj->lower)
		edge |= PANNED_WEST;
	      else
		{
		  g_assert (hadj->value >= hadj->upper - hadj->page_size);
		  edge |= PANNED_EAST;
		}
	    }
	  else
	    {
	      if (dx < 0)
		edge |= PANNED_WEST;
	      else
		edge |= PANNED_EAST;
	    }
	}
      if (dy_share > 0.3 && abs (priv->y_distance) > 100)
	{
	  if (vadj)
	    {
	      if (vadj->value <= vadj->lower)
		edge |= PANNED_NORTH;
	      else
		{
		  g_assert (vadj->value >= vadj->upper - vadj->page_size);
		  edge |= PANNED_SOUTH;
		}
	    }
	  else
	    {
	      if (dy < 0)
		edge |= PANNED_NORTH;
	      else
		edge |= PANNED_SOUTH;
	    }
	}

      if (edge)
	{
	  priv->at_edge = t;
	  priv->x_distance = 0;
	  priv->y_distance = 0;

	  g_signal_emit (G_OBJECT (pw), signals[EDGE_FLIP], 0, edge, 0);

	  goto out;
	}
    }
  
 out:
  priv->x = x;
  priv->y = y;

  return TRUE;
}

static gboolean
motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
  gboolean res = FALSE;
  if (GTK_WIDGET_CLASS (parent_class)->motion_notify_event)
    res = GTK_WIDGET_CLASS (parent_class)
      ->motion_notify_event (widget, event);

  PannedWindowPrivate *priv = PANNED_WINDOW_GET_PRIVATE (widget);
  if (priv->consider_motion && ! priv->panning)
    /* This is the start of a drag event.  */
    {
      priv->panning = TRUE;
      priv->at_edge = 0;
      priv->x = priv->x_origin = event->x_root;
      priv->y = priv->y_origin = event->y_root;

      g_assert (! priv->motion_timer);
      priv->motion_timer
	= g_timeout_add (1000 / FPS, (GSourceFunc) process_motion, widget);

      return TRUE;
    }

  return res;
}

GtkWidget *
panned_window_new (void)
{
  GtkWidget *widget = gtk_widget_new (TYPE_PANNED_WINDOW, NULL);

  return widget;
}
