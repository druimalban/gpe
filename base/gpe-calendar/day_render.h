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

#ifndef DAY_RENDER_H
#define DAY_RENDER_H

#include <glib.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gpe/event-db.h>
#include "day_view.h"

#define day_render_update_offset( dr )  ((dr)->offset.x = (dr)->page->time_width)
/* Day Caption */
typedef struct caption
{
  GtkWidget *draw;		/* Drawable */
  guint day;			/* Day number */
  guint width;			/* Caption Width */
  guint height;			/* Caption Height */
  GdkPoint offset;		/* Offset in the drawable */
  GdkGC *gc;			/* background Color  */
  GdkPixbuf *bell_pb;		/* Bell Pixbuf */
  PangoLayout *pl;		/* Pango layout */

  /* Other stuff here... */

} *caption_t;


struct day_render
{
  caption_t capt;
  GdkDrawable *draw;
  GtkWidget *widget;
  day_page_t page;
  GdkGC *normal_gc;		/*Normal event color */
  GdkGC *ol_gc;			/* Overlapping areas color */
  guint width;			/* rectangle width */
  guint height;			/* rectangle height */
  guint cols;			/* Number of columns, i.e. how many hours in a row. */
  guint rows;
  guint gap;			/* Define gap... */
  time_t date;			/* Date of this day */
  guint dx;
  guint dy;
  GdkPoint offset;
  guint hours;
  GSList *events;		/* Events associated to this day. */
  GSList *event_rectangles;
};

typedef struct row
{
  GdkPoint point;
  guint row_num;

} *row_t;

typedef struct ev_rec
{
  guint width;
  guint height;
  guint x;
  guint y;
  event_t event;
} *ev_rec_t;

struct day_render *day_render_new (GtkWidget * widget,
				   day_page_t page,
				   GdkGC * app_gc,
				   GdkGC * overl_gc,
				   time_t date,
				   guint cols, guint gap, guint hours,
				   GSList * events);

void day_render_delete (struct day_render *dr);

void day_render_show (struct day_render *);

void day_render_show_big (struct day_render *);

GSList *day_render_find_overlapping (GSList * events);

GNode *find_overlapping_sets (GSList * events);

ev_rec_t
day_render_event_show_big (struct day_render *, event_t event, GdkGC * gc,
			   GSList * ol_period);

void day_render_resize (struct day_render *dr, guint width, guint height);

void day_render_set_event_rectangles (struct day_render *dr);

void draw_appointments (struct day_render *dr);

GdkGC *pen_new (GtkWidget * widget, guint red, guint green, guint blue);

#endif /* DAY_RENDER_H */
