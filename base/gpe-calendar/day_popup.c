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
  struct tm tm;
  GDate *date = data;

  g_date_to_struct_tm (date, &tm);
  set_time_and_day_view (mktime (&tm));
}

static void
new_clicked (GtkWidget *widget, gpointer data)
{
  struct tm tm;
  GDate *date = data;

  g_date_to_struct_tm (date, &tm);
  GtkWidget *w = new_event (mktime (&tm) + 12 * 60 * 60, 1);
  gtk_widget_show (w);
}

static void
event_clicked (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = edit_event (EVENT (d));
  gtk_widget_show (w);
}

struct data
{
  GDate date;
  GSList *events;
};

static void
destroy (gpointer user_data, GObject *object)
{
  struct data *data = user_data;
  event_list_unref (data->events);
  g_free (data);
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
day_popup (const GDate *date, const GSList *events)
{
  GtkMenu *menu = GTK_MENU (gtk_menu_new ());
  int row = 0;

  struct data *data;
  data = g_malloc0 (sizeof (*data));
  g_object_weak_ref (G_OBJECT (menu), destroy, data);

  data->date = *date;

  char s[20];
  g_date_strftime (s, sizeof (s), _("%a %d %B"), date);
  GtkWidget *item = gtk_image_menu_item_new_with_label (s);
  GdkPixbuf *pixbuf = gpe_find_icon ("day_view");
  GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (day_clicked), &data->date);
  gtk_widget_show (item);
  gtk_menu_attach (menu, item, 0, 1, row, row + 1);
  row ++;

  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (new_clicked), &data->date);
  gtk_widget_show (item);
  gtk_menu_attach (menu, item, 0, 1, row, row + 1);
  row++;

  const GSList *l;
  for (l = events; l; l = l->next)
    {
      Event *ev = EVENT (l->data);

      g_object_ref (ev);
      data->events = g_slist_prepend (data->events, ev);

      char *timestr = NULL;
      if (! is_reminder (ev))
	{
	  time_t s = event_get_start (ev);
	  GDate start;
	  g_date_set_time (&start, s);
	  if (g_date_compare (&start, date) == 0)
	    {
	      struct tm tm;
	      localtime_r (&s, &tm);
	      timestr = strftime_strdup_utf8_locale (TIMEFMT, &tm);
	    }
	  else
	    {
	      time_t e = event_get_start (ev) + event_get_duration (ev);
	      GDate end;
	      g_date_set_time (&end, e);
	      if (g_date_compare (&end, date) == 0)
		{
		  struct tm tm;
		  localtime_r (&e, &tm);
		  char *b = strftime_strdup_utf8_locale (TIMEFMT, &tm);
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

  g_signal_connect (G_OBJECT (menu), "selection-done",
		    G_CALLBACK (menu_destroy), NULL);

  return menu;
}
