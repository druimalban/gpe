/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>

#include <gtk/gtk.h>

#include "todo.h"
#include "pixmaps.h"
#include "errorbox.h"
#include "render.h"
#include "picturebutton.h"

#include "tick.xpm"
#include "box.xpm"

#define _(_x) gettext(_x)

static GdkPixmap *tick_pixmap, *box_pixmap;
static GdkBitmap *tick_bitmap, *box_bitmap;

static guint ystep;
static guint xcol = 18;

GtkWidget *g_draw;

static GtkWidget *g_option;
static GSList *display_items = NULL;

static struct todo_category *selected_category = NULL;

static void lists_menu (void);

static void
set_category (GtkWidget *w, gpointer user_data)
{
  selected_category = user_data;
  gtk_widget_draw (g_draw, NULL);
}

void
categories_menu (void)
{
  GtkWidget *menu = gtk_menu_new ();
  GSList *l;

  for (l = categories; l; l = l->next)
    {
      struct todo_category *t = l->data;
      GtkWidget *i = gtk_menu_item_new_with_label (t->title);
      gtk_menu_append (GTK_MENU (menu), i);
      gtk_signal_connect (GTK_OBJECT (i), "activate", set_category, t);
      gtk_widget_show (i);
    }

  gtk_widget_show (menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (g_option), menu);
}

static void
new_todo_item (GtkWidget *w, gpointer user_data)
{
  GtkWidget *todo = edit_item (NULL);
  gtk_widget_show_all (todo);
}

static void
purge_completed (GtkWidget *w, gpointer list)
{
  GSList *iter = items;
  
  while (iter)
    {
      struct todo_item *i = iter->data;
      GSList *new_iter = iter->next;
      if (i->state == COMPLETED)
	delete_item (i);
      
      iter = new_iter;
    }
  
  gtk_widget_draw (g_draw, NULL);
}

static void
show_hide_completed(GtkWidget *w, gpointer list)
{
  hide = ! hide;
  gtk_widget_draw (g_draw, NULL);
}

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;
  guint max_width;
  guint max_height;
  guint y;
  guint skew = 2;
  GSList *iter;
  GdkFont *font = widget->style->font;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  ystep = font->ascent + font->descent;
  if (ystep < 14)
    ystep = 14;

  if (! tick_pixmap)
    {
      tick_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
						  &tick_bitmap,
						  NULL,
						  tick_xpm);
      box_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
						 &box_bitmap,
						 NULL,
						 box_xpm);
    }
  
  gdk_draw_rectangle(drawable, white_gc, TRUE, 0, 0, max_width, max_height);

  y = skew;

  for (iter = display_items; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;
      
      if (i->state != COMPLETED)
	{
	  gdk_draw_text (drawable, font, black_gc, xcol, y + font->ascent, 
			 i->summary, strlen (i->summary));
	  gdk_draw_pixmap (drawable, black_gc, box_pixmap,
			   2, 0, 2, y - skew, 14, 14);
	  i->pos=y/ystep;
	  y += ystep;
	}
      
    }
  
  if (!hide) 
    {
      for (iter = display_items; iter; iter = iter->next)
	{
	  struct todo_item *i = iter->data;
	  
	  if (i->state == COMPLETED)
	    {
	      gdk_draw_text (drawable, font, black_gc, xcol, y + font->ascent, 
			     i->summary, strlen (i->summary));
	      
	      gdk_draw_line (drawable, black_gc, xcol, 
			     y + (font->ascent + font->descent) / 2, 
			     18 + gdk_string_width (font, i->summary), 
			     y + (font->ascent + font->descent) / 2);
	      gdk_draw_pixmap (drawable, black_gc, tick_pixmap,
			       2, 0, 2, y - skew, 14, 14);
	      i->pos=y/ystep;
	       y += ystep;
	    }
	}
    }

  return TRUE;
}

static void
draw_click_event (GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer user_data)
{
  unsigned int idx = event->y/ystep;

  if (event->type == GDK_BUTTON_PRESS && event->x < xcol)
    {
      GSList *iter;
      for (iter = display_items; iter; iter = iter->next)
	{
	  struct todo_item *ti = iter->data;
	  if (idx == ti->pos)
	    {
	      if (ti->state == COMPLETED)
		ti->state = NOT_STARTED;
	      else
		ti->state = COMPLETED;
	      
	      push_item (ti);
	      gtk_widget_draw (g_draw, NULL);
	      break;
	    }
	}
    }
  else if (event->type == GDK_2BUTTON_PRESS && event->x > 18)
    {
      GSList *iter;
      for (iter = display_items; iter; iter = iter->next)
	{
	  struct todo_item *ti = iter->data;
	  if (idx == ti->pos)
	    {
	      gtk_widget_show_all (edit_item (ti));
	      break;
	    }
	}
    }
}

void
refresh_items (void)
{
  GSList *iter;

  if (display_items)
    {
      g_slist_free (display_items);
      display_items = NULL;
    }

  for (iter = items; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;
      if (selected_category == NULL)
	display_items = g_slist_append (display_items, i);
    }

  gtk_widget_draw (g_draw, NULL);
}

GtkWidget *
top_level (GtkWidget *window)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *sep = gtk_vseparator_new ();
  GtkWidget *toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
  GtkWidget *toolbar2 = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
  GtkWidget *option = gtk_option_menu_new ();
  GtkWidget *pw;
  GtkWidget *draw = gtk_drawing_area_new();
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  
  g_option = option;
  categories_menu ();

  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2), GTK_RELIEF_NONE);

  pw = gpe_render_icon (window->style, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("New"), 
			   _("Add a new item"), 
			   _("Add a new item"),
			   pw, new_todo_item, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("properties"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Configure"), 
			   _("Configure lists"), 
			   _("Configure lists"),
			   pw, configure, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("clean"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Purge completed"), 
			   _("Purge completed"), 
			   _("Purge completed"),
			   pw, purge_completed, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("hide"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Show/Hide completed"), 
			   _("Show/Hide completed"), 
			   _("Show/Hide completed"),
			   pw, show_hide_completed, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("exit"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), 
			   _("Exit"), 
			   _("Exit"), 
			   _("Exit"),
			   pw, gtk_exit, NULL);

  gtk_widget_show (toolbar);
  gtk_widget_show (toolbar2);
  gtk_widget_show (sep);
  gtk_widget_show (option);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), option, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar2, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  gtk_signal_connect (GTK_OBJECT (draw), "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (draw), "button_press_event",
		      GTK_SIGNAL_FUNC (draw_click_event), NULL);

  gtk_widget_add_events (GTK_WIDGET (draw), GDK_BUTTON_PRESS_MASK);

  gtk_widget_show (draw);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					 draw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  gtk_widget_show (vbox);

  g_draw = draw;

  refresh_items ();

  return vbox;
}
