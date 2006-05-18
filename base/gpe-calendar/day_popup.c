/*
 * Copyright (C) 2002, 2003, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@gnu.org>
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
day_clicked (GtkWidget *widget, gpointer data)
{
  struct day_popup *p = data;
  struct tm tm;

  memset (&tm, 0, sizeof (tm));
  tm.tm_year = p->year;
  tm.tm_mon = p->month;
  tm.tm_mday = p->day;
  tm.tm_hour = 12;
  tm.tm_isdst = -1;

  set_time_and_day_view (mktime (&tm));
}

static void
new_clicked (GtkWidget *widget, gpointer data)
{
  struct day_popup *p = data;
  struct tm tm;
    
  memset (&tm, 0, sizeof (tm));
  tm.tm_year = p->year;
  tm.tm_mon = p->month;
  tm.tm_mday = p->day;
  tm.tm_hour = 12;
  tm.tm_isdst = -1;
  
  GtkWidget *w = new_event (mktime (&tm), 1);
  gtk_widget_show (w);
}

static void
event_clicked (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = edit_event (EVENT (d));
  gtk_widget_show (w);
}

static void
menu_destroy (GtkWidget *widget, gpointer d)
{
  gtk_widget_destroy (widget);
}

static void
squash_pointer (gpointer data, GObject *object)
{
  g_assert (* (GObject **) data == object);
  * (GObject **) data = NULL;
}

static GtkWidget *bell_recur;
static GtkWidget *bell;
static GtkWidget *recur;

GtkMenu *
day_popup (struct day_popup *p, gboolean show_events)
{
  GtkMenu *menu = GTK_MENU (gtk_menu_new ());
  int row = 0;

  struct tm tm;
  memset (&tm, 0, sizeof (tm));
  tm.tm_year = p->year;
  tm.tm_mon = p->month;
  tm.tm_mday = p->day;
  tm.tm_isdst = -1;
  mktime (&tm);
  
  char *s = strftime_strdup_utf8_locale ("%a %d %B", &tm);
  GtkWidget *item = gtk_image_menu_item_new_with_label (s);
  g_free (s);
  GdkPixbuf *pixbuf = gpe_find_icon ("day_view");
  GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (day_clicked), p);
  gtk_widget_show (item);
  gtk_menu_attach (menu, item, 0, 1, row, row + 1);
  row ++;

  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (new_clicked), p);
  gtk_widget_show (item);
  gtk_menu_attach (menu, item, 0, 1, row, row + 1);
  row++;

  if (show_events)
    {
      GSList *l;
      for (l = p->events; l; l = l->next)
	{
	  Event *ev = EVENT (l->data);

	  char *timestr = NULL;
	  if (! is_reminder (ev))
	    {
	      time_t t = event_get_start (ev);
	      localtime_r (&t, &tm);
	      if (tm.tm_mday == p->day && tm.tm_mon == p->month
		  && tm.tm_year == p->year)
		timestr = strftime_strdup_utf8_locale (TIMEFMT, &tm);
	      else
		{
		  time_t end = event_get_start (ev) + event_get_duration (ev);
		  localtime_r (&end, &tm);
		  if (tm.tm_mday == p->day && tm.tm_mon == p->month
		      && tm.tm_year == p->year)
		    {
		      char *b;
		      b = strftime_strdup_utf8_locale (TIMEFMT, &tm);
		      timestr = g_strdup_printf (_("Until %s"), b);
		      g_free (b);
		    }
		}
	    }
	  char summary[60];

	  snprintf (summary, sizeof (summary) - 1,
		    "%s %s", timestr ? timestr : "", event_get_summary (ev));
	  summary[60] = 0;
	  if (timestr)
	    g_free (timestr);

	  /* Convert new lines to spaces.  */
	  char *s = summary;
	  while ((s = strchr (s, '\n')))
	    *s = ' ';

	  GtkWidget *image = NULL;
	  if ((event_get_alarm (ev)) && event_is_recurrence (ev))
	    {
	      if (! bell_recur)
		{
		  GdkPixbuf *pixbuf = gpe_find_icon ("bell_recur");
		  bell_recur = gtk_image_new_from_pixbuf (pixbuf);
		  g_object_weak_ref (G_OBJECT (bell_recur), squash_pointer,
				     &bell_recur);
		}
	      g_object_ref (bell_recur);
	      image = bell_recur;
	    }
	  else if (event_get_alarm (ev))
	    {
	      if (! bell)
		{
		  GdkPixbuf *pixbuf = gpe_find_icon ("bell");
		  bell = gtk_image_new_from_pixbuf (pixbuf);
		  g_object_weak_ref (G_OBJECT (bell), squash_pointer,
				     &bell);
		}
	      g_object_ref (bell);
	      image = bell;
	    }
	  else if (event_is_recurrence (ev))
	    {
	      if (! recur)
		{
		  GdkPixbuf *pixbuf = gpe_find_icon ("recur");
		  recur = gtk_image_new_from_pixbuf (pixbuf);
		  g_object_weak_ref (G_OBJECT (recur), squash_pointer,
				     &recur);
		}
	      g_object_ref (recur);
	      image = recur;
	    }

	  GtkWidget *item;
	  if (image)
	    {
	      item = gtk_image_menu_item_new_with_label (summary);
	      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
					     image);
	    }
	  else
	    item = gtk_menu_item_new_with_label (summary);

	  g_signal_connect (G_OBJECT (item), "activate",
			    G_CALLBACK (event_clicked), ev);
	  gtk_widget_show (item);
	  gtk_menu_attach (menu, item, 0, 1, row, row + 1);

	  row++;
	}
    }

  g_signal_connect (G_OBJECT (menu), "selection-done",
		    G_CALLBACK (menu_destroy), NULL);

  return menu;
}
