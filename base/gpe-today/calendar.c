/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gpe/errorbox.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sqlite.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "event-db.h"
#include "today.h"

#define CALENDAR_DB "/.gpe/calendar"
#define UPDATE_INTERVAL 60000 /* 1 minute */

static GSList *calendar_entries = NULL;
char *db_fname;
static int calendar_update_tag = 0;

gboolean calendar_update(gpointer data);
static void calendar_add_event(event_t event);
static void free_calendar_entries(void);

struct calevent {
	event_t ev;
	GtkWidget *wid;
	  GtkWidget *summary;
	  GtkWidget *time;
	char *time_str;
	gboolean in_progress;
};

int calendar_init(void)
{
 	GdkPixmap *pix;
	GdkBitmap *mask;
	GtkWidget *scroll;
	char *home = g_get_home_dir();

	/* calendar db full path */
	db_fname = g_malloc(strlen(home) + strlen(CALENDAR_DB) + 1);
	strcpy(db_fname, home);
	strcat(db_fname, CALENDAR_DB);

	/* fire db up */
	event_db_start();

	calendar.toplevel = gtk_hbox_new(FALSE, 0);

	/* calendar icon */
	calendar.vboxlogo = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(calendar.toplevel), calendar.vboxlogo, FALSE,
	                   FALSE, 5);

	load_pixmap(IMAGEPATH(calendar.png), &pix, &mask, 130);
	calendar.logo = gtk_pixmap_new(pix, mask);

	gtk_box_pack_start(GTK_BOX(calendar.vboxlogo), calendar.logo, FALSE,
	                   FALSE, 0);

	/* scrolled window with a vbox */
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_parent_window(scroll, window.toplevel->window);
	gtk_box_pack_start(GTK_BOX(calendar.toplevel), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
	                               GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	calendar.viewport = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(calendar.viewport), GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(scroll), calendar.viewport);

	calendar.eventsvbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(calendar.viewport), calendar.eventsvbox);

	gtk_widget_show_all(calendar.toplevel);

	/* update and set regular update intervals */
	calendar_update(NULL);
	calendar_update_tag = g_timeout_add(UPDATE_INTERVAL, calendar_update, NULL);

	/* gtkrc stuff */
	gtk_rc_parse_string("widget '*calendar_time' style 'calendar_time'");
	gtk_rc_parse_string("widget '*calendar_summary' style 'calendar_summary'");
	gtk_rc_parse_string("widget '*event_in_progress-summary' style 'event_in_progress-summary'");
	gtk_rc_parse_string("widget '*event_in_progress-time' style 'event_in_progress-time'");

	return 1;
}

void calendar_free(void)
{
	free_calendar_entries();
	gtk_container_remove(GTK_CONTAINER(window.vbox1), calendar.toplevel);
	g_free(db_fname);
	g_source_remove(calendar_update_tag);
	event_db_stop();
}

gboolean calendar_update(gpointer data)
{
	struct stat db;
	static time_t prev_modified = 0;
	time_t current_time;
	int i;
	struct calevent *event;

	if (stat(db_fname, &db) == -1) {
		gpe_perror_box("Error stating calendar DB");
		return TRUE;
	}

	if (db.st_mtime > prev_modified) { /* DB changed */
		prev_modified = db.st_mtime;
		calendar_events_db_update();
	}

	/* check events' status */
	time(&current_time);
	for (i = 0; (event = (struct calevent *) g_slist_nth_data(calendar_entries, i)); i++) {
		if (event->ev->start < current_time) { /* event in progress */
			if (event->in_progress == TRUE)
				continue;

			if (event->ev->recur) { /* recurring event */
				/* TODO: must check better if it is in progress */
				/* same problem with finished recurring events */
			}

			/* event is in progress */
			event->in_progress = TRUE;
			gtk_widget_set_name(event->summary, "event_in_progress-summary");
			gtk_widget_set_name(event->time, "event_in_progress-time");
			
			continue;
		}
			
		if ((event->ev->start + event->ev->duration) > current_time) {
			/* event finished */
			/* TODO: simply remove the event's widget */
			calendar_events_db_update();
			/*
			  calendar_entries = g_slist_remove(calendar_entries, event);
			  unpack
			*/

			continue;
		}
	}

	return TRUE;
}

void calendar_events_db_update(void)
{
 	time_t current_time = time(NULL);
	time_t midnight;
	struct tm *midtm;
	int i;
	event_t ev;
	static GSList *events = NULL;
	
	if (!event_db_refresh())
	 	return; /* could not load the db */

	free_calendar_entries();

	/* this shows events until 23:59:59, might want to fix that */

	midtm = localtime(&current_time);
	midtm->tm_sec = 59;
	midtm->tm_min = 59;
	midtm->tm_hour = 23;
	midnight = mktime(midtm);

	/* TODO: show/not show All day events */
	/* FIXME: all day events are not getting displayed */
/*	events = event_db_untimed_list_for_period(current_time, midnight, TRUE);
	for (i = 0; (ev = (event_t) g_slist_nth_data(events, i)); i++)
		calendar_add_event(ev);
	event_db_list_destroy(events);
*/
	events = event_db_list_for_period(current_time, midnight);
	for (i = 0; (ev = (event_t) g_slist_nth_data(events, i)); i++)
		calendar_add_event(ev);

}

static void calendar_add_event(event_t event)
{
	GtkWidget *vbox, *label;
	struct calevent *cal = g_malloc(sizeof(struct calevent));
	event_details_t details;

	vbox = gtk_vbox_new(FALSE, 0);
	cal->ev = event;
	cal->wid = vbox;
	cal->in_progress = FALSE;
	calendar_entries = g_slist_append(calendar_entries, (gpointer) cal);

	details = event_db_get_details(event);

	cal->summary = label = gtk_label_new(details->summary);
	gtk_widget_set_name(label, "calendar_summary");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	if (!(event->flags & FLAG_UNTIMED)) {
		/* FIXME: how big should the strings be? */
		char strtimestart[25];
		char strtimeend[25];
		char str[50];
		calendar_time_t end = event->start + event->duration;

		strftime(strtimestart, sizeof strtimestart, "%H:%M",
		         localtime(&event->start));
		strftime(strtimeend, sizeof strtimeend, "%H:%M",
		         localtime(&end));
		snprintf(str, sizeof str, "%s - %s", strtimestart, strtimeend);

		cal->time_str = g_strdup(str);
		cal->time = label = gtk_label_new(str);
		gtk_widget_set_name(label, "calendar_time");
	} else {
		cal->time = label = gtk_label_new(_("All day event"));
		gtk_widget_set_name(label, "calendar_alldayevent");
	}
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* TODO: LOCATION / NOTES labels */

	gtk_box_pack_start(GTK_BOX(calendar.eventsvbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show_all(vbox);
}

static void free_calendar_entries(void)
{
	int i;
	struct calevent *event;

	for (i = 0; (event = (struct calevent *) g_slist_nth_data(calendar_entries, i)); i++) {
		gtk_container_remove(GTK_CONTAINER(calendar.eventsvbox), event->wid);
		g_free(event->time_str);
		g_free(event);
	}

	g_slist_free(calendar_entries);
	calendar_entries = NULL;
}
