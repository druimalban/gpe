/*
 *  Copyright (C) 2004 Luca De Cicco <ldecicco@gmx.net> 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *
 */

#include <gpe/event-db.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <langinfo.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gpe/pixmaps.h>

#include "day_render.h"
#include "day_view.h"

#define event_ends(event) ((event)->start + (event)->duration)


GSList *found_node;

/**
 * Make new set used by overlapping set 
 */
GNode *
make_set (event_t e)
{
  GSList *set = NULL;
  set = g_slist_append (set, e);

  return g_node_new (set);

}

gboolean
events_overlap (event_t ev1, event_t ev2)
{				
  /* Event overlaps if and only if they intersect */
  if (ev1->duration == 0 && ev2->duration == 0)
    {
      return TRUE;
    }
  if (ev1 == ev2)
    {
      return FALSE;
    }
  if (ev1->start == ev2->start && event_ends (ev1) == event_ends (ev2))
    {
      return TRUE;
    }
  if ((ev1->start <= ev2->start && event_ends (ev1) < ev2->start) ||
      (ev1->start >= ev2->start && event_ends (ev2) > ev1->start))
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}


gboolean
find_set_in_list (GNode * node, gpointer data)
{
  GSList *iter, *ol_set;
  event_t ev = (event_t) data;
  event_t ev2;


  if (node == NULL)
    {
      found_node = NULL;
      return TRUE;
    }

  ol_set = node->data;

  for (iter = ol_set; iter; iter = iter->next)
    {
      ev2 = (event_t) iter->data;
      if (ev2 == ev)
	continue;		/* Ignore us */

      if (events_overlap (ev, ev2))
	{
	  found_node = ol_set;
	  return TRUE;
	}
    }
  found_node = NULL;
  return FALSE;
}

/* Returns the overlapping set */
GSList *
find_set (GNode * root, event_t ev)
{
  found_node = NULL;

  g_node_traverse (root, G_IN_ORDER, G_TRAVERSE_ALL, -1, find_set_in_list, ev);

  return found_node;


}


GSList *
union_set (GSList * set, const event_t ev)
{
  return (g_slist_append (set, ev));
}

GNode *
find_overlapping_sets (GSList * events)
{
  GNode *root;
  GSList *iter = NULL;
  GSList *set = NULL;

  root = g_node_new (NULL);


  for (iter = events; iter; iter = iter->next)
    {

      event_t ev = (event_t) iter->data;
      set = find_set (root, ev);

      if (set == NULL)		/* the event doesn't overlap */
	{
	  g_node_insert (root, -1, make_set (ev));
	}
      else
	{
	  set = union_set (set, ev);
	}
    }

  return root;
}

void
show_event (const struct day_render *dr, const ev_rec_t event_rectangle,
	    GdkGC * gc)
{
  guint length;

  event_details_t evd;
  gchar buf[250], *buffer;
  PangoLayout *pl;
  GdkRectangle gr;
  gint arc_size;
  gint offsetx, width = 0, height = 0;
  PangoRectangle pr;

  pl = gtk_widget_create_pango_layout (dr->widget, NULL);


  evd = event_db_get_details (event_rectangle->event);
  snprintf (buf, sizeof (buf), "<span size='small'>%s</span>", evd->summary);
  buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
  if (!buffer)
      buffer = g_strdup(buf);

  length = event_rectangle->height - 1;
  /* Rectangle used to write appointment summary */

  gr.width = event_rectangle->width - dr->gap - 2;
  gr.height = length - 1;
  gr.x = event_rectangle->x + dr->gap + 2;
  gr.y = event_rectangle->y + 1;
  offsetx = event_rectangle->x + dr->gap;

  width = event_rectangle->width - dr->gap - 1;
  height = length;

  pango_layout_set_markup (pl, buffer, strlen (buffer));
  pango_layout_get_pixel_extents (pl, &pr, NULL);
  pango_layout_set_width (pl, PANGO_SCALE * gr.width);
  pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
  arc_size = dr->page->height/24/2 - 1;
  if (arc_size < 7)
  {
	  /* Lower bound */
	  arc_size = 7;
  }

  /* Drawing appointment rectangle */
  gdk_draw_rectangle (dr->draw, dr->normal_gc, TRUE,
		      offsetx, event_rectangle->y+arc_size, width, height-2*arc_size);
  gdk_draw_rectangle (dr->draw, dr->normal_gc, TRUE,
		      offsetx+arc_size, event_rectangle->y, width-2*arc_size, height);
  
  gdk_draw_line (dr->draw, dr->widget->style->black_gc, offsetx+arc_size,
			     event_rectangle->y,offsetx+width-arc_size, event_rectangle->y);
		      
  gdk_draw_line (dr->draw, dr->widget->style->black_gc, offsetx+arc_size,
			     event_rectangle->y+height,offsetx+width-arc_size, 
				 event_rectangle->y+height);
  gdk_draw_line (dr->draw, dr->widget->style->black_gc,
				 offsetx,arc_size+event_rectangle->y,
				 offsetx, event_rectangle->y+height-arc_size);
  gdk_draw_line (dr->draw, dr->widget->style->black_gc,
				 offsetx+width,arc_size+event_rectangle->y,
				 offsetx+width, event_rectangle->y+height-arc_size);				 
/* North-west corner */
  gdk_draw_arc (dr->draw, dr->normal_gc, TRUE, offsetx, 
		event_rectangle->y,arc_size*2,arc_size*2,90*64,90*64);
  gdk_draw_arc (dr->draw, dr->widget->style->black_gc, FALSE, offsetx, 
		event_rectangle->y,arc_size*2,arc_size*2,90*64,90*64);
  /* North-east corner */
  gdk_draw_arc (dr->draw, dr->normal_gc, TRUE, offsetx+width-arc_size*2, 
		event_rectangle->y,arc_size*2,arc_size*2,0*64,90*64);
  gdk_draw_arc (dr->draw, dr->widget->style->black_gc, FALSE, offsetx+width-arc_size*2, 
		event_rectangle->y,arc_size*2,arc_size*2,0*64,90*64);
  /* South-west corner */
  gdk_draw_arc (dr->draw, dr->normal_gc, TRUE, offsetx, 
		event_rectangle->y+height-2*arc_size,arc_size*2,arc_size*2,180*64,90*64);
  gdk_draw_arc (dr->draw, dr->widget->style->black_gc, FALSE, offsetx, 
		event_rectangle->y+height-2*arc_size,arc_size*2,arc_size*2,180*64,90*64);
  /* South-east corner */
  gdk_draw_arc (dr->draw, dr->normal_gc, TRUE, offsetx+width-arc_size*2, 
		event_rectangle->y+height-2*arc_size,arc_size*2,arc_size*2,270*64,90*64);
  gdk_draw_arc (dr->draw, dr->widget->style->black_gc, FALSE, offsetx+width-2*arc_size, 
		event_rectangle->y+height-2*arc_size,arc_size*2,arc_size*2,270*64,90*64);	
  /* Write summary... */
  
  gtk_paint_layout (dr->widget->style,
		    dr->draw,
		    GTK_WIDGET_STATE (dr->widget),
		    FALSE, &gr, dr->widget, "label", gr.x, gr.y, pl);
  
		
  		
		
  if (event_rectangle->event->flags & FLAG_ALARM)
    {
      if (dr->capt->bell_pb)
        {
          gint width_pix, height_pix;
          width_pix = gdk_pixbuf_get_width (dr->capt->bell_pb);
          height_pix = gdk_pixbuf_get_height (dr->capt->bell_pb);
    
          gdk_draw_pixbuf (dr->draw,
                   dr->normal_gc,
                   dr->capt->bell_pb,
                   0, 0,
                   offsetx + width - width_pix - 1,
                   event_rectangle->y + 1, MIN (width_pix, width - 1),
                   MIN (height_pix, height - 1),
                   GDK_RGB_DITHER_NORMAL, 0, 0);
        }
    }
  g_free (buffer);
}



void
draw_appointments (struct day_render *dr)
{
  GSList *iter;

  iter = dr->event_rectangles;
  while (iter)
    {
      show_event (dr, (ev_rec_t) (iter->data), dr->normal_gc);
      iter = iter->next;
    }
}



/*
 * Caption functions.
 */

/* Caption Constructor*/
#define caption_new(day, width, height, offset,gc,widget,bell) caption_with_pango_new((day),(width),(height),(offset),(gc),(widget),bell ,NULL)

/** 
 * Caption constructor specifing pango layout
 * widget is the widget where you want pango layout will be drawn
 */
caption_t
caption_with_pango_new (guint day, guint width, guint height, GdkPoint offset,
			GdkGC * gc, GtkWidget * widget, GdkPixbuf * bell,
			PangoLayout * pl)
{
  caption_t this;

  this = (struct caption *) g_malloc (sizeof (struct caption));

  if (this == NULL)
    return NULL;

  if (pl != NULL)
    {
      this->pl = pl;
    }
  else
    {
      if (widget == NULL)
	{
	  g_free (this);
	  return NULL;
	}

      this->pl = gtk_widget_create_pango_layout (widget, NULL);
    }
  this->draw = widget;
  this->day = day;
  this->height = height;
  this->width = width;
  this->offset = offset;
  this->bell_pb = bell;
  this->gc = gc;
  return this;
}

void
caption_set_pango (caption_t this, PangoLayout * pl)
{
  this->pl = pl;
}

void
caption_show (caption_t this)
{
  char buf[40], *buffer;
  PangoRectangle pr;
  GdkRectangle gr;

  buffer = (char *) g_malloc (sizeof (char) * 256);

  /* Displays day number */
  snprintf (buf, sizeof (buf), "<span size='%d'>%d</span>",
	    this->height * 800, this->day);


  buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);

  pango_layout_set_markup (this->pl, buffer, strlen (buffer));
  pango_layout_get_pixel_extents (this->pl, &pr, NULL);

  gr.width = pr.width;
  gr.height = pr.height * 2;
  gr.x = this->offset.x;
  gr.y = this->offset.y;
  /* Draw day number in the upper left corner */
  gtk_paint_layout (this->draw->style,
		    this->draw->window,
		    GTK_WIDGET_STATE (this->draw),
		    FALSE,
		    &gr,
		    this->draw,
		    "label", this->offset.x, this->offset.y, this->pl);

  if (this->day == 19)
    gdk_draw_rectangle (this->draw->window, this->gc, TRUE,
			this->offset.x + gr.width * 1.2,
			this->offset.y + this->height * .5,
			this->height * 0.8, this->height * 0.8);

  g_free (buffer);
}

/* 
 * Returns true if event started today.
 *
 * ev: Event to check
 */
gboolean
event_starts_today (const struct day_render *dr, const event_t ev)
{
  time_t today, tomorrow;
  struct tm td;

  today = dr->date;
  localtime_r (&today, &td);
  td.tm_hour = 0;
  td.tm_min = 0;
  td.tm_sec = 0;

  today = mktime (&td);

  tomorrow = today + 60 * 60 * 24;
  if (ev->start <= tomorrow && ev->start >= today)
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



ev_rec_t
event_rect_new (const struct day_render * dr, const event_t event,
		gint column, gint columns)
{
  GdkPoint offset;
  gint len_t, length;
  ev_rec_t ev_rect = (struct ev_rec *) g_malloc (sizeof (struct ev_rec));

  offset.x = 0;
  if (!event_starts_today (dr, event))
    {
      len_t = event->start + event->duration - dr->date;
      length =
	(guint) ((float) len_t / (3600.0 * 24.0) * (float) dr->page->height);
      offset.y = 0;		/* Event starts at 00:00 */
    }
  else
    {
      struct tm start_tm;
      guint duration;

      localtime_r (&event->start, &start_tm);

      if (event->duration < 3600 * 24)
	{
	  duration = event->duration;
	}
      else
	{
	  duration = 3600 * 24;
	}

      len_t = duration;
      offset.y =
	(float) (start_tm.tm_hour * 3600 +
		 start_tm.tm_min * 60) / (float) (3600 * 24) *
	dr->page->height;

      length =
	(guint) ((float) len_t / (3600.0 * 24.0) * (float) dr->page->height);
    }

  offset.x = column * (dr->page->width - dr->offset.x) / columns;

  ev_rect->x = offset.x + dr->offset.x;
  ev_rect->y = offset.y;
  ev_rect->width = (dr->page->width - dr->offset.x) / columns;
  ev_rect->height = MAX (length, dr->page->height / dr->hours);
  ev_rect->event = event;
  if (event->duration == 0)	/* This is a reminder */
    {
      ev_rect->y = 0;
    }
  return (ev_rect);
}

void
event_rect_delete (ev_rec_t ev)
{
  g_free (ev);
}




GSList *
ol_sets_to_rectangles (const struct day_render *dr, GNode * node)
{
  GSList *iter, *ev_rects = NULL;
  GNode *n;
  gint column = 0;
  event_t ev;
  gint columns;

  ev_rec_t event_rect;
  n = node->children;
  while (n)
    {

      column = 0;

      columns = g_slist_length (n->data);
      iter = n->data;
      while (iter)
	{
	  ev = (event_t) iter->data;

	  event_rect = event_rect_new (dr, ev, column, columns);
	  ++column;
	  ev_rects = g_slist_append (ev_rects, event_rect);
	  iter = g_slist_next (iter);

	}
      n = n->next;
    }
  n = node->children;

  return ev_rects;
}


/*
 * Day render functions
 */

void
day_render_set_event_rectangles (struct day_render *dr)
{
  GNode *olset;
  olset = find_overlapping_sets (dr->events);
  dr->event_rectangles = ol_sets_to_rectangles (dr, olset);

  gtk_widget_add_events (GTK_WIDGET (dr->widget),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
}

void
page_height_set (day_page_t page, gint height)
{
  page->height = MAX (page->height_min, height);
}

void
day_render_resize (struct day_render *dr, guint width, guint height)
{
  gint tmp;
  tmp = height / dr->hours;

  dr->page->width = width;
  page_height_set (dr->page, tmp * dr->hours);


  dr->dy = (height) / (dr->rows + 2);
  day_render_update_offset (dr);
}


/* 
 * Constructor of day_render object
 *
 * app_gc: Appointment color
 * overl_gc: Color of overlapping zones
 * date: date of the day
 * width: Width of the drawing area
 * height: Height of the drawing area
 * cols: Number of columns
 * gap: Gap between lines
 * offset: Offset from the (0,0) of the drawing area
 * events: List of events of the day
 */
struct day_render *
day_render_new (GtkWidget * widget,
		day_page_t page,
		GdkGC * app_gc,
		GdkGC * overl_gc,
		time_t date,
		guint cols, guint gap, guint hours, GSList * events)
{
  struct tm time;
  struct day_render *dr;

  GdkPixbuf *pbuf;



  if (cols <= 0 || widget == NULL || page == NULL)
    {
      return NULL;
    }
  localtime_r (&date, &time);

  time.tm_hour = 0;
  time.tm_min = 0;

  dr = (struct day_render *) g_malloc (sizeof (struct day_render));
  dr->hours = hours;
  dr->page = page;
  dr->widget = widget;
  dr->draw = widget->window;
  dr->date = mktime (&time);
  dr->cols = cols;
  dr->rows = dr->hours / cols;
  dr->gap = gap;
  dr->events = events;
  dr->dx = page->width / cols;
  dr->dy = (page->height) / (dr->rows + 2);
  dr->normal_gc = app_gc;
  dr->ol_gc = overl_gc;
  dr->offset.x = page->time_width;
  dr->offset.y = 0;

  pbuf = gpe_find_icon ("bell");


  dr->capt =
    caption_new (time.tm_mday, dr->page->width, dr->dy * 2.5, dr->offset,
		 overl_gc, widget, pbuf);

  day_render_set_event_rectangles (dr);

  g_signal_connect (G_OBJECT (dr->widget), "button-press-event",
		    G_CALLBACK (day_view_button_press), dr);
  return dr;

}



void
day_render_delete (struct day_render *dr)
{
  if (dr != NULL)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (dr->widget),
					    day_view_button_press, dr);
      GSList *iter;
      iter = dr->event_rectangles;
      while (iter)
       {
         event_rect_delete (iter->data);
         iter = iter->next;
       }

      g_slist_free (dr->event_rectangles);
      g_slist_free (dr->events);
      g_free (dr->capt);
      g_free (dr);
    }
}



/* 
 * Returns true if event started today.
 *
 * ev: Event to check
 */
gboolean
event_ends_today (const struct day_render *dr, const event_t ev)
{
  GTime today, tomorrow;
  today = dr->date;
  tomorrow = today + 60 * 60 * 24;

  if (ev->start + ev->duration <= tomorrow)
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

/**
 * Given events list it returns the overlapping intervals in a GSList
 * 
 */
GSList *
day_render_find_overlapping (GSList * events)
{
  GSList *iter1, *iter2;
  GSList *ol_list = NULL;
  event_t ol_event, ev1, ev2;

  for (iter1 = events; iter1; iter1 = iter1->next)
    {
      for (iter2 = iter1->next; iter2; iter2 = iter2->next)
	{
	  ev1 = iter1->data;
	  ev2 = iter2->data;
	  if (ev2->start >= ev1->start
	      && ev2->start <= ev1->start + ev1->duration)
	    {
	      struct tm a;
	      if (event_ends (ev2) < event_ends (ev1))
		{

		  localtime_r (&(ev2->start), &a);

		  ol_list = g_slist_append (ol_list, ev2);

		}
	      else
		{
		  gint duration;
		  localtime_r (&(ev2->start), &a);
		  duration = event_ends (ev1) - (guint) (ev2->start);
		  if (duration)
		    {
		      ol_event = (event_t) g_malloc (sizeof (event_t));
		      ol_event->start = ev2->start;
		      ol_event->duration = duration;
		      ol_list = g_slist_append (ol_list, ol_event);
		    }
		}

	    }
	}

    }
  return ol_list;
}


/*
 * Returns where the event starts (x,y and row number)
 *
 */
row_t
day_render_event_starts (struct day_render * dr, event_t event)
{
  row_t start;
  start = (row_t) malloc (sizeof (row_t));
  if (event_starts_today (dr, event))
    {
      struct tm time;
      GdkPoint point;

      localtime_r (&(event->start), &time);
      point.x =
	((guint) ((float) time.tm_hour + (float) time.tm_min / 60.0) %
	 dr->cols) * dr->dx + dr->gap;
      point.y =
	((guint) ((float) time.tm_hour + (float) time.tm_min / 60.0) /
	 dr->cols) * dr->dy + dr->capt->height * 1.2;
      start->row_num =
	(guint) ((float) time.tm_hour +
		 (float) time.tm_min / 60.0) / dr->cols;
      start->point = point;

    }
  else
    {
      /* Event doesn't start today, so for today it starts since 00:00 */
      (start->point).x = 0;
      (start->point).y = dr->capt->height * 1.2;
      start->row_num = 0;
    }
  return start;
}


/* Renders an event with gc color. Note that also an overlapping event
 * is rendered with this function. 
 */
void
day_render_event_show (struct day_render *dr, event_t event, GdkGC * gc)
{
  row_t start;
  guint height;
  GdkPoint offset = dr->offset;
  guint length;
  height = dr->dy;
  start = day_render_event_starts (dr, event);


  if (!event_starts_today (dr, event))
    {
      length = (guint) (((float) event->start + (float) event->duration -
               (float) dr->date) / 3600.0 * (float) dr->dx);
    }
  else
    {
      length = (guint) (((float) event->duration) / 3600.0 * (float) dr->dx);
    }

  if (length + (start->point).x < dr->page->width)
    {
      /*It fits in a row... */
      gdk_draw_rectangle (dr->draw, gc, TRUE, (start->point).x + offset.x,
			  (start->point).y + offset.y + dr->gap, length,
			  dr->dy - dr->gap);
    }
  else
    {
      /* Draw first rectangle */
      int i = start->row_num + 1;
      guint reminder = 0;
      gdk_draw_rectangle (dr->draw, gc, TRUE, (start->point).x + offset.x,
			  (start->point).y + offset.y + dr->gap,
			  dr->page->width - (start->point).x,
			  dr->dy - dr->gap);
      reminder = length - (dr->page->width - (start->point).x);

      while ((reminder > dr->page->width) && reminder > 0)
        {
          gdk_draw_rectangle (dr->draw, gc, TRUE, offset.x,
                      (start->point).y + offset.y + dr->gap 
                        + (i - start->row_num - 1) * dr->dy, 
                      dr->page->width, dr->dy - dr->gap);
          reminder -= dr->page->width;
    
          ++i;
          if (i > dr->rows)
            {
              break;
            }
        }
      if (i <= dr->rows)
        {
          gdk_draw_rectangle (dr->draw, gc, TRUE, offset.x,
                      (start->point).y + offset.y + dr->gap + (i -
                                           start->
                                           row_num
                                           -
                                           1) *
                      dr->dy, reminder, dr->dy - dr->gap);
        }
    }
}

GdkGC *
pen_new (GtkWidget * widget, guint red, guint green, guint blue)
{
  GdkColormap *colormap;
  GdkGC *pen_color_gc;
  GdkColor pen_color;

  colormap = gdk_window_get_colormap (widget->window);
  pen_color_gc = gdk_gc_new (widget->window);
  gdk_gc_copy (pen_color_gc, widget->style->black_gc);
  pen_color.red = red;
  pen_color.green = green;
  pen_color.blue = blue;
  gdk_colormap_alloc_color (colormap, &pen_color, FALSE, TRUE);
  gdk_gc_set_foreground (pen_color_gc, &pen_color);

  return pen_color_gc;
}
