/* GPE Life
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include "l-view.h"
#include <math.h>

#define L_VIEW_WIDTH 500
#define L_VIEW_HEIGHT 500

struct _LView
{
  GtkWidget *drawing_area;
  GdkPixmap *pixmap;
  GdkPixmap *background_pixmap;
  gint cell_size;
  gint last_mx;
  gint last_my;
  LModel *model;
};
typedef struct _LView LView;

void
l_view_destroy (gpointer data)
{
  if (data)
    {
      LView *view = (LView *) data;

      l_model_destroy (view->model);
      if (view->drawing_area)
        g_object_unref (view->drawing_area);
      if (view->background_pixmap)
        g_object_unref (view->background_pixmap);
      if (view->pixmap)
        g_object_unref (view->pixmap);
      g_free (view);
    }
}

void
l_view_to_model_coords (GtkWidget *widget, gint vx, gint vy, gint *mx, gint *my)
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));
  gint cell_size = view->cell_size;
  gint width = widget->allocation.width;
  gint height = widget->allocation.height;

  (*mx) = floor ((gdouble)(vx - width/2) / (gdouble)(cell_size + 2));
  (*my) = floor ((gdouble)(vy - height/2) / (gdouble)(cell_size + 2));

  /*g_print ("(%d, %d) to model: (%d, %d)\n", vx, vy, *mx, *my);*/
}

/* Redraw the screen from the backing pixmap */
static gboolean expose_event( GtkWidget      *widget,
                              GdkEventExpose *event )
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));

  gdk_draw_drawable (widget->window,
		     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		     view->pixmap,
		     event->area.x, event->area.y,
		     event->area.x, event->area.y,
		     event->area.width, event->area.height);

  /* g_print ("expose: %dx%d\n", event->area.width, event->area.height);
   */
  return FALSE;
}

/* Draw a rectangle on the screen */
void
draw_cell (GtkWidget *widget, gint x, gint y)
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));
  gint cell_size = view->cell_size;
  gint width = widget->allocation.width;
  gint height = widget->allocation.height;
  GdkRectangle cell_rect;

  cell_rect.x = (x * (cell_size + 2) +1) + (width / 2);
  cell_rect.y = (y * (cell_size + 2) +1) + (height / 2);
  cell_rect.width = cell_size;
  cell_rect.height = cell_size;
  /*g_print ("width: %d; height: %d; cell (%d, %d): (%d,%d) (%d,%d)\n", width, height, x, y, cell_rect.x, cell_rect.y, cell_rect.x+cell_size, cell_rect.y+cell_size);*/
  gdk_draw_rectangle (view->pixmap,
		      widget->style->black_gc,
		      TRUE,
		      cell_rect.x, cell_rect.y,
		      cell_rect.width, cell_rect.height);
  gtk_widget_queue_draw_area (widget, 
		              cell_rect.x, cell_rect.y,
		              cell_rect.width, cell_rect.height);
}

void
queue_redraw_cell (GtkWidget *widget, gint x, gint y)
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));
  gint cell_size = view->cell_size;
  gint width = widget->allocation.width;
  gint height = widget->allocation.height;
  GdkRectangle cell_rect;

  cell_rect.x = (x * (cell_size + 2) +1) + (width / 2);
  cell_rect.y = (y * (cell_size + 2) +1) + (height / 2);
  cell_rect.width = cell_size;
  cell_rect.height = cell_size;
  /*g_print ("width: %d; height: %d; cell (%d, %d): (%d,%d) (%d,%d)\n", width, height, x, y, cell_rect.x, cell_rect.y, cell_rect.x+cell_size, cell_rect.y+cell_size);*/
  gtk_widget_queue_draw_area (widget, 
		              cell_rect.x, cell_rect.y,
		              cell_rect.width, cell_rect.height);
}


gboolean
button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));

  if (event->button == 1 && view->pixmap != NULL && view->model)
    {
      gint x, y;

      l_view_to_model_coords (widget, (int) event->x, (int) event->y, &x, &y);
      l_model_toggle (view->model, x, y);
      view->last_mx = x;
      view->last_my = y;
      queue_redraw_cell (widget, x, y);
    }

  return TRUE;
}

static gboolean
motion_notify_event( GtkWidget *widget,
                     GdkEventMotion *event )
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));
  gint x, y;
  GdkModifierType state;

  if (event->is_hint)
    gdk_window_get_pointer (event->window, &x, &y, &state);
  else
    {
      x = event->x;
      y = event->y;
      state = event->state;
    }
    
  if (state & GDK_BUTTON1_MASK && view->pixmap != NULL && view->model)
    {
      gint mx, my;

      l_view_to_model_coords (widget, x, y, &mx, &my);
      if (mx != view->last_mx || my != view->last_my)
        {
          l_model_toggle (view->model, mx, my);
	  view->last_mx = mx;
	  view->last_my = my;
          queue_redraw_cell (widget, mx, my);
	}
    }
  
  return TRUE;
}

void
draw_cell_func (gint x, gint y, gpointer data)
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (data), "l_view"));

  draw_cell (GTK_WIDGET (view->drawing_area), x, y);
}

void
redraw_cell_func (gint x, gint y, gpointer data)
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (data), "l_view"));

  queue_redraw_cell (GTK_WIDGET (view->drawing_area), x, y);
}

void
l_view_update (gpointer data)
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (data), "l_view"));
  GdkPixmap *pixmap;
  gint width = 0, height = 0;

  l_model_get_dimension (view->model, &width, &height);
  /*g_print ("size: %dx%d\n", width, height);*/

  /* too expensive */
  /* gtk_widget_set_size_request (GTK_WIDGET (view->drawing_area), width * (view->cell_size + 2) *1.3, height * (view->cell_size + 2) * 1.3);
   */

  pixmap = gdk_pixmap_new (view->drawing_area->window,
			   view->drawing_area->allocation.width,
			   view->drawing_area->allocation.height,
			   -1);
  if (view->pixmap)
    g_object_unref (view->pixmap);
  view->pixmap = pixmap;

  /* white background. */
  gdk_draw_rectangle (view->pixmap,
		      view->drawing_area->style->white_gc,
		      TRUE,
		      0, 0,
		      view->drawing_area->allocation.width,
		      view->drawing_area->allocation.height);

  /* draw all live cells */
  l_model_foreach (view->model, draw_cell_func, view->drawing_area);

  /* clear all dead cells */
  l_model_foreach_zombie (view->model, redraw_cell_func, view->drawing_area);

}

void
l_view_refresh (GtkWidget *widget)
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));

  gint width = widget->allocation.width;
  gint height = widget->allocation.height;

  gdk_draw_drawable (widget->window,
		     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		     view->pixmap,
		     0, 0,
		     0, 0,
		     width, height);
  gtk_widget_queue_draw_area (widget, 
		              0, 0,
		              width, height);

  /* this is somewhat redundant.  not sure why it's necessary.  */
  l_view_update (widget);
}

/* Create a new backing pixmap of the appropriate size */
static gboolean configure_event( GtkWidget         *widget,
                                 GdkEventConfigure *event )
{
  LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));

  l_view_update (widget);

  return TRUE;
}

void
l_view_set_model (GtkWidget *widget, LModel *model)
{
  if (widget && model)
    {
      LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));

      if (view)
        {
	  /* testing only */
	  /*if (view->model && view->model->grid)
	    g_print ("%s\n", l_grid_serialize (view->model->grid));
	  */

          l_model_destroy (view->model);
	  view->model = model;
          l_view_update (widget);
          gtk_widget_queue_draw_area (view->drawing_area, 
                                      0, 0,
                                      view->drawing_area->allocation.width,
				      view->drawing_area->allocation.height);

          gtk_widget_set_size_request (GTK_WIDGET (view->drawing_area), L_VIEW_WIDTH, L_VIEW_HEIGHT);
	  l_model_add_notify (model, l_view_update, widget);
        }
    }
}

LModel *
l_view_get_model (GtkWidget *widget)
{
  if (widget)
    {
      LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));

      if (view)
        return view->model;
    }
  return NULL;
}

gboolean
l_view_inc_cell_size (GtkWidget *widget)
{
  if (widget)
    {
      LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));

      if (view)
        {
          view->cell_size += 2;
	  l_view_refresh (widget);
	  return TRUE;
	}
    }
  return FALSE;
}

gboolean
l_view_dec_cell_size (GtkWidget *widget)
{
  if (widget)
    {
      LView *view = (LView *) (g_object_get_data (G_OBJECT (widget), "l_view"));

      if (view && view->cell_size > 2)
        {
          view->cell_size -= 2;
	  l_view_refresh (widget);
	  if (view->cell_size > 2)
	    return TRUE;
	  else
	    return FALSE;
	}
    }
  return FALSE;
}

GtkWidget *
l_view_new (void)
{
  LView *view = g_new0 (LView, 1);
  GtkWidget *drawing_area = gtk_drawing_area_new ();

  view->drawing_area = drawing_area;
  view->cell_size = 12;
  g_object_set_data_full (G_OBJECT (drawing_area), "l_view", view, l_view_destroy);

  /* Signals used to handle backing pixmap */
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
		    G_CALLBACK (expose_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area),"configure_event",
		    G_CALLBACK (configure_event), NULL);

  /* Event signals */
  g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event",
		    G_CALLBACK (motion_notify_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
		    G_CALLBACK (button_press_event), NULL);

  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK);

  return drawing_area;
}

