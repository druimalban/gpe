/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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

#include <gtk/gtk.h>

#include "globals.h"
#include "event-db.h"
#include "event-ui.h"
#include "day_popup.h"
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>

static void
destroy_popup(GtkWidget *widget,
	      GtkWidget *parent)
{
  gtk_object_set_data (GTK_OBJECT (parent), "popup-handle", NULL);
  pop_window=NULL;
}

static void
close_window(GtkWidget *widget,
	     GtkWidget *w)
{
  gtk_widget_destroy (w);
}

static void
day_clicked(GtkWidget *widget,
	    GtkWidget *w)
{
  struct day_popup *p = gtk_object_get_data (GTK_OBJECT (w), "popup-data");
  struct tm tm;
  localtime_r (&viewtime, &tm);
  tm.tm_year = p->year - 1900;
  tm.tm_mon = p->month;
  tm.tm_mday = p->day;
  viewtime = mktime (&tm);
  gtk_widget_destroy (w);
  set_day_view ();
}

static void 
selection_made( GtkWidget      *clist,
		gint            row,
		gint            column,
		GdkEventButton *event,
		GtkWidget      *widget)
{
  event_t ev;
    
  if (event->type == GDK_2BUTTON_PRESS)
    {
      ev = gtk_clist_get_row_data (GTK_CLIST (clist), row);

      gtk_widget_show (edit_event (ev));

      gtk_widget_destroy (widget);
    }
}

GtkObject *
day_popup(GtkWidget *parent, struct day_popup *p)
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
  GtkWidget *label;
  GtkWidget *contents;
  char buf[256];
  struct tm tm;

  gtk_widget_realize (window);
  day_button = gpe_picture_button (window->style, NULL, "day_view");
  close_button = gpe_picture_button (window->style, NULL, "cancel");

  memset (&tm, 0, sizeof (tm));
  tm.tm_year = p->year - 1900;
  tm.tm_mon = p->month;
  tm.tm_mday = p->day;
  strftime(buf, sizeof (buf), "%a %d %B", &tm);
  label = gtk_label_new (buf);

  if (gtk_object_get_data (GTK_OBJECT (parent), "popup-handle"))
    return NULL;

  if (p->events)
    {
      GSList *events = p->events;
      guint row = 0;
      contents = gtk_clist_new (1);

      while (events)
	{
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

	  row++;
	  events = events->next;
	}

      gtk_signal_connect (GTK_OBJECT (contents), "select_row",
			  GTK_SIGNAL_FUNC (selection_made),
			  window);
    }
  else
    {
      contents = gtk_label_new ("No appointments");
    }

  gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (day_button), GTK_RELIEF_NONE);

  gtk_signal_connect (GTK_OBJECT (close_button), "clicked",
		      GTK_SIGNAL_FUNC (close_window), window);

  gtk_signal_connect (GTK_OBJECT (day_button), "clicked",
		      GTK_SIGNAL_FUNC (day_clicked), window);

  gtk_object_set_data (GTK_OBJECT (window), "popup-data",
                       (gpointer) p);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), day_button, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (hbox), close_button, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), contents, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));

  gdk_window_get_pointer (NULL, &x, &y, NULL);

  gtk_widget_show_all (vbox);

  gtk_widget_realize (window);
  gtk_widget_size_request (window, &requisition);
  
  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();

  x = CLAMP (x - 2, 0, MAX (0, screen_width - requisition.width));
  y = CLAMP (y + 4, 0, MAX (0, screen_height - requisition.height));
  
  gtk_widget_set_uposition (window, MAX (x, 0), MAX (y, 0));

  gtk_object_set_data (GTK_OBJECT (parent), "popup-handle", window);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      destroy_popup, parent);

  gtk_widget_show (window);

  return GTK_OBJECT (window);
}
