/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
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


GtkWidget *
day_popup (GtkWidget *parent, struct day_popup *p, gboolean show_items)
{
  GtkRequisition requisition;
  gint x, y;
  gint screen_width;
  gint screen_height;
  GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *day_button;
  GtkWidget *close_button;
  GtkWidget *new_button = NULL;
  GtkWidget *label;
  GtkWidget *contents = NULL;
  GtkWidget *frame;
  char buf[256];
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
  strftime (buf, sizeof (buf), "%a %d %B", &tm);

  gs = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
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
          char *p = buf;
          size_t s = sizeof (buf), l;
          gchar *lineinfo[1];
    
          localtime_r (&ev->start, &tm);
          l = strftime (p, s, TIMEFMT, &tm);
          s -= l;
          p += l;
    
          snprintf (p, s - 1, " %s", evd->summary);
          p[s - 1] = 0;
          
          lineinfo[0] = buf;
          
          gtk_clist_append (GTK_CLIST (contents), lineinfo);
    
          gtk_clist_set_row_data (GTK_CLIST (contents), row, ev);
          
          if ((ev->flags & FLAG_ALARM) && ev->recur)
            {
              if (gpe_find_icon_pixmap ("bell_recur", &pmap, &bmap))
            gtk_clist_set_pixtext (GTK_CLIST (contents), row, 0, buf, 5,
                           pmap, bmap);
            }
          else if (ev->flags & FLAG_ALARM)
            { 
              if (gpe_find_icon_pixmap ("bell", &pmap, &bmap))
            gtk_clist_set_pixtext (GTK_CLIST (contents), row, 0, buf, 5,
                           pmap, bmap);
            }
          else if (ev->recur)
            {
              if (gpe_find_icon_pixmap ("recur", &pmap, &bmap))
            gtk_clist_set_pixtext (GTK_CLIST (contents), row, 0, buf, 5,
                           pmap, bmap);
            }
            
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
  else  /* !show_items */
    {
      new_button = 
        gpe_button_new_from_stock (GTK_STOCK_NEW, GPE_BUTTON_TYPE_BOTH);
      gtk_button_set_relief (GTK_BUTTON (new_button), GTK_RELIEF_NONE);
      g_signal_connect (G_OBJECT (new_button), "clicked",
                    G_CALLBACK (do_new_event), p);
    }

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
  if (new_button)
    gtk_box_pack_start (GTK_BOX (vbox), new_button, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_add (GTK_CONTAINER (window), frame);

  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));

  gdk_window_get_pointer (NULL, &x, &y, NULL);

  gtk_widget_realize (window);
  gtk_widget_size_request (window, &requisition);
  
  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();

  x = CLAMP (x - 2, 0, MAX (0, screen_width - requisition.width));
  y = CLAMP (y + 4, 0, MAX (0, screen_height - requisition.height));
  
  gtk_widget_set_uposition (window, MAX (x, 0), MAX (y, 0));

  g_object_set_data (G_OBJECT (parent), "popup-handle", window);
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (destroy_popup), parent);

  gtk_widget_show_all (window);

  return window;
}
