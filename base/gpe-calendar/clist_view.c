/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "event-db.h"
#include "event-ui.h"

#include "globals.h"

void 
selection_made( GtkWidget      *clist,
		gint            row,
		gint            column,
		GdkEventButton *event,
		GtkWidget      *widget)
{
  event_t ev;
    
  if (event->type == GDK_2BUTTON_PRESS)
    {
      guint hour = row;
      GtkWidget *appt;
      struct tm tm;
      
      ev = gtk_clist_get_row_data (GTK_CLIST (clist), row);

      if (ev) 
	appt = edit_event (ev);
      else 
	{
	  localtime_r (&viewtime, &tm);
	  tm.tm_hour = hour+1;
	  tm.tm_min = 0;
	  tm.tm_sec = 0;
	  appt = new_event (mktime (&tm), 1);
	}

      gtk_widget_show (appt);
    }
}
