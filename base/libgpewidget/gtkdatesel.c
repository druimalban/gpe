/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkDateSel widget for GTK+
 * Copyright (C) 2001 Philip Blundell
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#include <gtk/gtk.h>
#include "gtkdatesel.h"

#define ARROW_SIZE                         11
#define PAD				   4

static void gtk_date_sel_class_init     (GtkDateSelClass *klass);
static void gtk_date_sel_init           (GtkDateSel      *spin_button);
static void gtk_date_sel_realize        (GtkWidget          *widget);
static void gtk_date_sel_size_request   (GtkWidget          *widget,
					    GtkRequisition     *requisition);
static void gtk_date_sel_size_allocate  (GtkWidget          *widget,
					    GtkAllocation      *allocation);
static void gtk_date_sel_paint          (GtkWidget          *widget,
					    GdkRectangle       *area);
static void gtk_date_sel_draw           (GtkWidget          *widget,
					    GdkRectangle       *area);
static gint gtk_date_sel_expose         (GtkWidget          *widget,
					    GdkEventExpose     *event);
static gint gtk_date_sel_button_press   (GtkWidget          *widget,
					    GdkEventButton     *event);
static gint gtk_date_sel_button_release (GtkWidget          *widget,
					    GdkEventButton     *event);


static GtkWidgetClass *parent_class = NULL;

static guint my_signals[1];

struct _GtkDateSel
{
  GtkWidget widget;

  guint day_width, month_width, year_width, week_width;
  gint day_clamped;

  time_t time;

  guint mode;
};

struct _GtkDateSelClass
{
  GtkWidgetClass parent_class;

  void (* changed)      (GtkDateSel *sel);
};

GtkType
gtk_date_sel_get_type (void)
{
  static guint date_sel_type = 0;

  if (!date_sel_type)
    {
      static const GtkTypeInfo date_sel_info =
      {
	"GtkDateSel",
	sizeof (GtkDateSel),
	sizeof (GtkDateSelClass),
	(GtkClassInitFunc) gtk_date_sel_class_init,
	(GtkObjectInitFunc) gtk_date_sel_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      date_sel_type = gtk_type_unique (GTK_TYPE_WIDGET, &date_sel_info);
    }
  return date_sel_type;
}

static void
gtk_date_sel_class_init (GtkDateSelClass *class)
{
  GtkObjectClass   *object_class;
  GtkWidgetClass   *widget_class;

  object_class   = (GtkObjectClass*)   class;
  widget_class   = (GtkWidgetClass*)   class;

  parent_class = gtk_type_class (GTK_TYPE_WIDGET);

  my_signals[0] = gtk_signal_new ("changed",
				  GTK_RUN_LAST,
				  object_class->type,
				  GTK_SIGNAL_OFFSET (GtkDateSelClass, changed),
				  gtk_marshal_NONE__NONE,
				  GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, my_signals, 1);

  widget_class->realize = gtk_date_sel_realize;
  widget_class->size_request = gtk_date_sel_size_request;
  widget_class->size_allocate = gtk_date_sel_size_allocate;
  widget_class->draw = gtk_date_sel_draw;
  widget_class->expose_event = gtk_date_sel_expose;
  widget_class->button_press_event = gtk_date_sel_button_press;
  widget_class->button_release_event = gtk_date_sel_button_release;
}

static void
gtk_date_sel_init (GtkDateSel *sel)
{
  time(&sel->time);
  sel->day_clamped = -1;
}

static void
gtk_date_sel_realize (GtkWidget *widget)
{
  GtkDateSel *sel;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DATE_SEL (widget));
  
  sel = GTK_DATE_SEL (widget);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK 
    | GDK_BUTTON_RELEASE_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), 
				   &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static void
gtk_date_sel_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
  GtkDateSel *sel;
  struct tm tm;
  guint i, w;
  char buf[128];

  g_return_if_fail (widget != NULL);
  g_return_if_fail (requisition != NULL);
  g_return_if_fail (GTK_IS_DATE_SEL (widget));

  sel = GTK_DATE_SEL (widget);

  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
  
  sel->week_width = gdk_string_width (widget->style->font, "Week 88");

  for (i = 0; i < 7; i++)
    {
      tm.tm_wday = i;
      strftime (buf, sizeof (buf), "%a", &tm);
      w = gdk_string_width (widget->style->font, buf);
      if (w > sel->day_width)
	sel->day_width = w;
    }
  sel->day_width += gdk_string_width (widget->style->font, ", 88");
  
  for (i = 0; i < 11; i++)
    {
      tm.tm_mon = i;
      strftime (buf, sizeof (buf), "%B", &tm);
      w = gdk_string_width (widget->style->font, buf);
      if (w > sel->month_width)
	sel->month_width = w;
    }

  sel->year_width = gdk_string_width (widget->style->font, "8888");

  requisition->height = widget->style->font->ascent + 
    widget->style->font->descent;

  switch (sel->mode)
    {
    case GTKDATESEL_YEAR:
      requisition->width = sel->year_width + 2 * ARROW_SIZE + 2 * PAD;
      break;
    case GTKDATESEL_WEEK:
      requisition->width = sel->week_width + sel->year_width + 4 * ARROW_SIZE + 6 * PAD;
      break;
    case GTKDATESEL_MONTH:
      requisition->width = sel->month_width + sel->year_width + 4 * ARROW_SIZE + 6 * PAD;
      break;
    case GTKDATESEL_FULL:
      requisition->width = sel->day_width + sel->month_width + sel->year_width
	+ 6 * ARROW_SIZE + 9 * PAD;
      break;
    }
}

static void
gtk_date_sel_size_allocate (GtkWidget *widget,
			    GtkAllocation *allocation)
{
  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      allocation->x,
			      allocation->y,
			      allocation->width,
			      allocation->height);
    }
}

static void
draw_arrows(GtkWidget *widget, guint left, guint right)
{
  gtk_draw_arrow (widget->style, widget->window,
		  GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		  GTK_ARROW_LEFT,
		  TRUE, 
		  left, 2, 
		  ARROW_SIZE, ARROW_SIZE);

  gtk_draw_arrow (widget->style, widget->window,
		  GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		  GTK_ARROW_RIGHT,
		  TRUE, 
		  right, 2, 
		  ARROW_SIZE, ARROW_SIZE);
}

static void
gtk_date_sel_paint (GtkWidget    *widget,
		    GdkRectangle *area)
{
  GtkDateSel *sel;
  gint leftspace;
  guint l, r;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DATE_SEL (widget));

  sel = GTK_DATE_SEL (widget);

  leftspace = (widget->allocation.width - widget->requisition.width) / 2;

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      char buf[128];
      struct tm tm;
      guint fy = widget->style->font->ascent;
      guint w;

      gdk_window_clear_area (widget->window,
			     area->x, area->y,
			     area->width, area->height);
      gdk_gc_set_clip_rectangle (widget->style->black_gc, area);

      localtime_r (&sel->time, &tm);

      switch (sel->mode)
	{
	case GTKDATESEL_YEAR:
	  l = leftspace;
	  r = l + sel->year_width + ARROW_SIZE + PAD + PAD;
	  draw_arrows(widget, l, r);
	  strftime(buf, sizeof(buf), "%Y", &tm);
	  w = gdk_string_width(widget->style->font, buf);
	  gdk_draw_string(widget->window, widget->style->font,
			  widget->style->black_gc,
			  l + ARROW_SIZE + PAD + ((sel->year_width - w) / 2), 
			  fy, buf);
	  break;

	case GTKDATESEL_WEEK:
	  l = leftspace;
	  r = l + sel->week_width + ARROW_SIZE + PAD + PAD;
	  draw_arrows(widget, l, r);
	  strftime(buf, sizeof(buf), "Week %W", &tm);
	  w = gdk_string_width(widget->style->font, buf);
	  gdk_draw_string(widget->window, widget->style->font,
			  widget->style->black_gc,
			  l + ARROW_SIZE + PAD + ((sel->week_width - w) / 2), fy,
			  buf);      

	  l = r + ARROW_SIZE + PAD;
	  r = l + sel->year_width + ARROW_SIZE + PAD + PAD;
	  draw_arrows(widget, l, r);

	  strftime(buf, sizeof(buf), "%Y", &tm);
	  w = gdk_string_width(widget->style->font, buf);
	  gdk_draw_string(widget->window, widget->style->font,
			  widget->style->black_gc,
			  l + ARROW_SIZE + PAD + ((sel->year_width - w) / 2), fy,
			  buf);
	  break;

	case GTKDATESEL_MONTH:
	  l = leftspace;
	  r = l + sel->month_width + ARROW_SIZE + PAD + PAD;
	  draw_arrows(widget, l, r);
	  strftime(buf, sizeof(buf), "%B", &tm);
	  w = gdk_string_width(widget->style->font, buf);
	  gdk_draw_string(widget->window, widget->style->font,
			  widget->style->black_gc,
			  l + ARROW_SIZE + PAD + ((sel->month_width - w) / 2), fy,
			  buf);      

	  l = r + ARROW_SIZE + PAD;
	  r = l + sel->year_width + ARROW_SIZE + PAD + PAD;
	  draw_arrows(widget, l, r);

	  strftime(buf, sizeof(buf), "%Y", &tm);
	  w = gdk_string_width(widget->style->font, buf);
	  gdk_draw_string(widget->window, widget->style->font,
			  widget->style->black_gc,
			  l + ARROW_SIZE + PAD + ((sel->year_width - w) / 2), fy,
			  buf);
	  break;

	case GTKDATESEL_FULL:
	  l = leftspace;
	  r = l + sel->day_width + ARROW_SIZE + PAD + PAD;
	  draw_arrows(widget, l, r);
	  strftime(buf, sizeof(buf), "%a, %d", &tm);
	  w = gdk_string_width(widget->style->font, buf);
	  gdk_draw_string(widget->window, widget->style->font,
			  widget->style->black_gc,
			  l + ARROW_SIZE + PAD + ((sel->day_width - w) / 2), fy,
			  buf);      
	  l = r + ARROW_SIZE + PAD;
	  r = l + sel->month_width + ARROW_SIZE + PAD + PAD;
	  draw_arrows(widget, l, r);
	  strftime(buf, sizeof(buf), "%B", &tm);
	  w = gdk_string_width(widget->style->font, buf);
	  gdk_draw_string(widget->window, widget->style->font,
			  widget->style->black_gc,
			  l + ARROW_SIZE + PAD + ((sel->month_width - w) / 2), fy,
			  buf);
	  l = r + ARROW_SIZE + PAD;
	  r = l + sel->year_width + ARROW_SIZE + PAD + PAD;
	  draw_arrows(widget, l, r);
	  strftime(buf, sizeof(buf), "%Y", &tm);
	  w = gdk_string_width(widget->style->font, buf);
	  gdk_draw_string(widget->window, widget->style->font,
			  widget->style->black_gc,
			  l + ARROW_SIZE + PAD + ((sel->year_width - w) / 2), fy,
			  buf);
	  break;
	}
      gdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);
    }
}

static void
gtk_date_sel_draw (GtkWidget    *widget,
		      GdkRectangle *area)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_DATE_SEL (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    gtk_date_sel_paint (widget, area);
}

static gint
gtk_date_sel_expose (GtkWidget      *widget,
			GdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DATE_SEL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget))
    gtk_date_sel_paint (widget, &event->area);

  return FALSE;
}

static void
day_click(GtkDateSel *sel, int d)
{
  sel->time += d * 60 * 60 * 24;
  gtk_widget_draw (GTK_WIDGET (sel), NULL);
  sel->day_clamped = -1;
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

static void
week_click(GtkDateSel *sel, int d)
{
  sel->time += d * 60 * 60 * 24 * 7;
  gtk_widget_draw (GTK_WIDGET (sel), NULL);
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

static int
days_in_month(struct tm *tm)
{
  if (tm->tm_mon == 1)
    {
      guint y = 1900 + tm->tm_year;
      if ((y % 4) == 0
	  && (((y % 100) != 0) || (y % 400) == 0))
	return 29;
      else
	return 28;
    }

  switch (tm->tm_mon)
    {
    case 3:
    case 5:
    case 8:
    case 10:
      return 30;
    default:
      return 31;
    }
}

static void
month_click(GtkDateSel *sel, int d)
{
  struct tm tm;
  guint mdays;
  localtime_r (&sel->time, &tm);
  if (d < 0)
    {
      if (tm.tm_mon)
	tm.tm_mon--;
      else
	{
	  tm.tm_year--;
	  tm.tm_mon = 11;
	}
    }
  else
    {
      if (tm.tm_mon < 11)
	tm.tm_mon++;
      else
	{
	  tm.tm_year++;
	  tm.tm_mon = 0;
	}
    }
  mdays = days_in_month(&tm);
  if (tm.tm_mday > mdays)
    {
      sel->day_clamped = tm.tm_mday;
      tm.tm_mday = mdays;
    }
  else
    {
      if (sel->day_clamped != -1)
	{
	  tm.tm_mday = mdays;
	  if (tm.tm_mday > sel->day_clamped)
	    tm.tm_mday = sel->day_clamped;
	}
    }
  sel->time = mktime (&tm);
  gtk_widget_draw (GTK_WIDGET (sel), NULL);
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

static void
year_click(GtkDateSel *sel, int d)
{
  struct tm tm;
  guint mdays;
  localtime_r (&sel->time, &tm);
  tm.tm_year += d;
  mdays = days_in_month(&tm);
  if (tm.tm_mday > mdays)
    {
      sel->day_clamped = tm.tm_mday;
      tm.tm_mday = mdays;
    }
  else
    {
      if (sel->day_clamped != -1)
	{
	  tm.tm_mday = mdays;
	  if (tm.tm_mday > sel->day_clamped)
	    tm.tm_mday = sel->day_clamped;
	}
    }
  sel->time = mktime (&tm);
  gtk_widget_draw (GTK_WIDGET (sel), NULL);
  gtk_signal_emit (GTK_OBJECT (sel), my_signals[0]);
}

static gint
gtk_date_sel_button_press (GtkWidget      *widget,
			   GdkEventButton *event)
{
  GtkDateSel *sel;
  gint leftspace;
  guint x;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DATE_SEL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  sel = GTK_DATE_SEL (widget);

  leftspace = (widget->allocation.width - widget->requisition.width) / 2;
  x = event->x - leftspace;

  switch (sel->mode)
    {
    case GTKDATESEL_YEAR:
      if (x < ARROW_SIZE)
	year_click(sel, -1);
      else if (x > (ARROW_SIZE + PAD + sel->year_width + PAD)
	       && x < (ARROW_SIZE + PAD + sel->year_width + PAD + ARROW_SIZE))
	year_click(sel, 1);
      break;

    case GTKDATESEL_WEEK:
      if (x < ARROW_SIZE)
	week_click(sel, -1);
      else if (x > (ARROW_SIZE + PAD + sel->week_width + PAD)
	       && x < (ARROW_SIZE + PAD + sel->week_width + PAD + ARROW_SIZE))
	week_click(sel, 1);
      else if (x > (ARROW_SIZE + PAD + sel->week_width + PAD + ARROW_SIZE + PAD)
	       && x < (ARROW_SIZE + PAD + sel->week_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE))
	year_click(sel, -1);
      else if (x > (ARROW_SIZE + PAD + sel->week_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->year_width + PAD)
	       && x < (ARROW_SIZE + PAD + sel->week_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->year_width + PAD + ARROW_SIZE))
	year_click(sel, 1);
      break;

    case GTKDATESEL_MONTH:
      if (x < ARROW_SIZE)
	month_click(sel, -1);
      else if (x > (ARROW_SIZE + PAD + sel->month_width + PAD)
	       && x < (ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE))
	month_click(sel, 1);
      else if (x > (ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE + PAD)
	       && x < (ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE))
	year_click(sel, -1);
      else if (x > (ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->year_width + PAD)
	       && x < (ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->year_width + PAD + ARROW_SIZE))
	year_click(sel, 1);
      break;

    case GTKDATESEL_FULL:
      if (x < ARROW_SIZE)
	day_click(sel, -1);
      else if (x > (ARROW_SIZE + PAD + sel->day_width + PAD)
	       && x < (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE))
	day_click(sel, 1);
      else if (x > (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE + PAD)
	       && x < (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE))
	month_click(sel, -1);
      else if (x > (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->month_width + PAD)
	       && x < (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE))
	month_click(sel, 1);
      else if (x > (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE + PAD)
	       && x < (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE))
	year_click(sel, -1);
      else if (x > (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->year_width + PAD)
	       && x < (ARROW_SIZE + PAD + sel->day_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->month_width + PAD + ARROW_SIZE + PAD + ARROW_SIZE + PAD + sel->year_width + PAD + ARROW_SIZE))
	year_click(sel, 1);
      break;
    }

  gtk_grab_add (widget);

  return FALSE;
}

static gint
gtk_date_sel_button_release (GtkWidget      *widget,
			     GdkEventButton *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_DATE_SEL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  gtk_grab_remove (widget);

  return FALSE;
}

void 
gtk_date_sel_update (GtkDateSel *spin_button)
{
}

/***********************************************************
 ***********************************************************
 ***                  Public interface                   ***
 ***********************************************************
 ***********************************************************/


GtkWidget *
gtk_date_sel_new (guint mode)
{
  GtkDateSel *ds;

  ds = gtk_type_new (GTK_TYPE_DATE_SEL);

  ds->mode = mode;

  return GTK_WIDGET (ds);
}

time_t
gtk_date_sel_get_time (GtkDateSel *sel)
{
  return sel->time;
}

void
gtk_date_sel_set_time (GtkDateSel *sel, time_t time)
{
  sel->time = time;
}
