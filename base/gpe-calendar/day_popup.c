/*
 * Copyright (C) 2002, 2003, 2006 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <langinfo.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/event-db.h>

#include "globals.h"
#include "event-ui.h"
#include "day_popup.h"

#define _(x) gettext(x)

static void
do_new_event (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *w;
  struct day_popup *p = user_data;
    
  struct tm tm;
  localtime_r (&viewtime, &tm);
  tm.tm_year = p->year;
  tm.tm_mon = p->month;
  tm.tm_mday = p->day;
  gtk_widget_destroy(gtk_widget_get_toplevel(widget));
  
  w = new_event (mktime (&tm), 1);
  gtk_widget_show (w);
}

static void
destroy_popup (GtkWidget *widget, GtkWidget *parent)
{
  gtk_grab_remove (widget);
  g_object_set_data (G_OBJECT (parent), "popup-handle", NULL);
  pop_window = NULL;
}

static void
close_window (GtkWidget *widget, GtkWidget *w)
{
  gtk_widget_destroy (w);
}

static void
day_clicked (GtkWidget *widget, GtkWidget *w)
{
  struct day_popup *p = g_object_get_data (G_OBJECT (w), "popup-data");
  struct tm tm;
  time_t selected_time;
  localtime_r (&viewtime, &tm);
  tm.tm_year = p->year;
  tm.tm_mon = p->month;
  tm.tm_mday = p->day;
  selected_time = mktime (&tm);
  gtk_widget_destroy (w);
  set_time_and_day_view (selected_time);
}

static void 
selection_made (GtkWidget *clist, gint row, gint column,
		GdkEventButton *event, GtkWidget *widget)
{
  event_t ev;
    
  if (event->type == GDK_2BUTTON_PRESS)
    {
     
      ev = gtk_clist_get_row_data (GTK_CLIST (clist), row);
      
      gtk_widget_show (edit_event (ev));

      gtk_widget_destroy (widget);
    }
}

static gboolean
key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *window)
{
  if (k->keyval == GDK_Escape) 
    {
      gtk_widget_destroy(window);
      return TRUE;
    }
  return FALSE;
}

static gboolean
handle_expose_event (GtkWidget *w, GdkEventExpose *ev, GtkWidget *child)
{
  gdk_window_clear (w->window);

  gdk_draw_rectangle (w->window, w->style->black_gc, FALSE, 0, 0, 
		      w->allocation.width - 1, w->allocation.height - 1);
  
  /* Draw the contents */
  gtk_container_propagate_expose (GTK_CONTAINER (w), child, ev);

  return TRUE;
}

static gboolean
handle_configure_event (GtkWidget *window, GdkEventConfigure *ev, gpointer user_data)
{
  gint x, y;
  gint screen_width, screen_height;

  /* Adjust the window's position to try to keep it on the screen.  */
  x = (gint)g_object_get_data (G_OBJECT (window), "x");
  y = (gint)g_object_get_data (G_OBJECT (window), "y");

  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();

  x = CLAMP (x, 0, MAX (0, screen_width - ev->width));
  y = CLAMP (y, 0, MAX (0, screen_height - ev->height));

  gtk_widget_set_uposition (window, MAX (x, 0), MAX (y, 0));

  return FALSE;
}

static gint
button_press_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GtkWidget *child;

  child = gtk_get_event_widget (event);

  /* We don't ask for button press events on the grab widget, so
   *  if an event is reported directly to the grab widget, it must
   *  be on a window outside the application (and thus we remove
   *  the popup window). Otherwise, we check if the widget is a child
   *  of the grab widget, and only remove the popup window if it
   *  is not.
   */
  if (child != widget)
    {
      while (child)
	{
	  if (child == widget)
	    return FALSE;

	  child = child->parent;
	}
    }
  
  gtk_widget_destroy (widget);
  
  return TRUE;
}

GtkWidget *
day_popup (GtkWidget *parent, struct day_popup *p, gboolean show_items)
{
  gint x, y;
  GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *day_button;
  GtkWidget *close_button;
  GtkWidget *new_button = NULL;
  GtkWidget *label;
  GtkWidget *contents = NULL;
  struct tm tm;
  gchar *gs;

  gtk_widget_realize (window);
  day_button = gpe_picture_button (window->style, NULL, "day_view");
  close_button = gpe_button_new_from_stock (GTK_STOCK_CLOSE, GPE_BUTTON_TYPE_ICON);

  memset (&tm, 0, sizeof (tm));
  tm.tm_year = p->year;
  tm.tm_mon = p->month;
  tm.tm_mday = p->day;
  mktime (&tm);
  
  gs = strftime_strdup_utf8_locale ("%a %d %B", &tm);
  label = gtk_label_new (gs);
  g_free (gs);
  
  if (g_object_get_data (G_OBJECT (parent), "popup-handle"))
    return NULL;

  if  (show_items)
    {
      if (p->events)
        {
          GSList *events = p->events;
          guint row = 0;
          contents = gtk_clist_new (1);
    
          gtk_clist_freeze (GTK_CLIST (contents));
          gtk_clist_clear (GTK_CLIST (contents));
      
          while (events)
	    {
	      GdkPixmap *pmap;
	      GdkBitmap *bmap;
	      event_t ev = events->data;
	      event_details_t evd = event_db_get_details (ev);
	      gchar *lineinfo[1];
	      gchar *timestr;

	      localtime_r (&ev->start, &tm);
	      timestr = strftime_strdup_utf8_locale (TIMEFMT, &tm);
	      lineinfo[0] = g_strdup_printf ("%s %s", timestr, evd->summary);
	      
	      gtk_clist_append (GTK_CLIST (contents), lineinfo);

	      g_free (timestr);
	      
	      gtk_clist_set_row_data (GTK_CLIST (contents), row, ev);
	      
	      if ((ev->flags & FLAG_ALARM) && ev->recur)
		{
		  if (gpe_find_icon_pixmap ("bell_recur", &pmap, &bmap))
		    gtk_clist_set_pixtext (GTK_CLIST (contents), row, 0, lineinfo[0], 5,
					   pmap, bmap);
		}
	      else if (ev->flags & FLAG_ALARM)
		{ 
		  if (gpe_find_icon_pixmap ("bell", &pmap, &bmap))
		    gtk_clist_set_pixtext (GTK_CLIST (contents), row, 0, lineinfo[0], 5,
					   pmap, bmap);
		}
	      else if (ev->recur)
		{
		  if (gpe_find_icon_pixmap ("recur", &pmap, &bmap))
		    gtk_clist_set_pixtext (GTK_CLIST (contents), row, 0, lineinfo[0], 5,
					   pmap, bmap);
		}
	      
	      g_free (lineinfo[0]);

	      row++;
	      events = events->next;
	    }
	  
          gtk_clist_thaw (GTK_CLIST (contents));
          g_signal_connect (G_OBJECT (contents), "select_row",
                            G_CALLBACK (selection_made), window);
        }
      else
        {
          contents = gtk_label_new (_("No appointments"));
        }
    }

  new_button = 
    gpe_button_new_from_stock (GTK_STOCK_NEW, GPE_BUTTON_TYPE_BOTH);
  gtk_button_set_relief (GTK_BUTTON (new_button), GTK_RELIEF_NONE);
  g_signal_connect (G_OBJECT (new_button), "clicked",
                    G_CALLBACK (do_new_event), p);

  gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (day_button), GTK_RELIEF_NONE);

  g_signal_connect (G_OBJECT (close_button), "clicked",
                    G_CALLBACK (close_window), window);

  g_signal_connect (G_OBJECT (day_button), "clicked",
                    G_CALLBACK (day_clicked), window);

  g_signal_connect (G_OBJECT (window), "key_press_event", 
		    G_CALLBACK (key_press_event), window);
  gtk_widget_add_events (GTK_WIDGET (window), GDK_KEY_PRESS_MASK);
    
  g_object_set_data (G_OBJECT (window), "popup-data", (gpointer) p);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), day_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  if (contents)
    gtk_box_pack_start (GTK_BOX (vbox), contents, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), new_button, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));

  gdk_window_get_pointer (NULL, &x, &y, NULL);

  x -= 2;
  y += 4;

  g_object_set_data (G_OBJECT (window), "x", (gpointer)x);
  g_object_set_data (G_OBJECT (window), "y", (gpointer)y);

  gtk_widget_set_uposition (window, x, y);

  g_object_set_data (G_OBJECT (parent), "popup-handle", window);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy_popup), parent);
  g_signal_connect (G_OBJECT (window), "configure_event", G_CALLBACK (handle_configure_event), NULL);
  g_signal_connect (G_OBJECT (window), "expose_event", G_CALLBACK (handle_expose_event), vbox);
  g_signal_connect (G_OBJECT (window), "button_press_event", G_CALLBACK (button_press_event), NULL);

  gtk_widget_show_all (window);

  gdk_pointer_grab (window->window, TRUE,
		    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, gtk_get_current_event_time ());
  gtk_grab_add (window);

  return window;
}
