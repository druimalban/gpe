/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sqlite.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gpe/event-db.h>
#include "today.h"

#define CALENDAR_DB "/.gpe/calendar"
#define UPDATE_INTERVAL 60000 /* 1 minute */

static GSList *calendar_entries;
static char *db_fname;
static int calendar_update_tag = 0;

struct calevent {
	event_t ev;
	PangoLayout *pl;
	gboolean in_progress;
	char *summary;
	char *description;
};

gboolean calendar_update(gpointer data);
static void calendar_add_event(event_t event);
static void free_calendar_entries(void);
static void remove_calendar_entry(struct calevent *ev);

static void refresh_calendar_widget(void)
{
	myscroll_update_upper_adjust(calendar.scroll);

	if (calendar.scroll->adjust->page_size >= calendar.scroll->adjust->upper)
		gtk_widget_hide(calendar.scroll->scrollbar);
	else if (!GTK_WIDGET_VISIBLE(calendar.scroll->scrollbar))
		gtk_widget_show(calendar.scroll->scrollbar);

	gtk_widget_queue_draw(calendar.scroll->draw);
	gtk_widget_queue_draw(calendar.toplevel);
}

int calendar_init(void)
{
	calendar.noevent = NULL;
	
	/* calendar db full path */
	db_fname = g_strdup_printf("%s/%s", g_get_home_dir(), CALENDAR_DB);

	calendar.toplevel = gtk_hbox_new(FALSE, 0);

	/* calendar icon */
	calendar.vboxlogo = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(calendar.toplevel), calendar.vboxlogo, FALSE, FALSE, 5);

	calendar.logo = gtk_image_new_from_file(IMAGEPATH(calendar.png));

	gtk_box_pack_start(GTK_BOX(calendar.vboxlogo), calendar.logo, FALSE, FALSE, 0);

	/* scrollable pango layout list */
	calendar.scroll = myscroll_new(TRUE);
	gtk_box_pack_start(GTK_BOX(calendar.toplevel), calendar.scroll->draw, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(calendar.toplevel), calendar.scroll->scrollbar,
			   FALSE, FALSE, 0);

	gtk_widget_show_all(calendar.toplevel);

	/* update and set regular update intervals */
	calendar_update(NULL);
	calendar_update_tag = g_timeout_add(UPDATE_INTERVAL, calendar_update, NULL);

	return 1;
}

void calendar_free(void)
{
	free_calendar_entries();
//	gtk_container_remove(GTK_CONTAINER(window.vbox1), calendar.toplevel);
	g_free(db_fname);
	g_source_remove(calendar_update_tag);
}

static void make_no_events_label(void)
{
	const char *str = _("No events today");
	
	if (calendar.noevent)
		return;

	calendar.noevent = gtk_widget_create_pango_layout(calendar.scroll->draw, NULL);
	pango_layout_set_wrap(calendar.noevent, PANGO_WRAP_WORD);
	calendar.scroll->list = g_slist_append(calendar.scroll->list, calendar.noevent);
	pango_layout_set_markup(calendar.noevent, str, strlen(str));
}

static void refresh_event_markup(struct calevent *cev)
{
	/* FIXME: how big should these be? */
	char strtimestart[25], strtimeend[25], timestr[50];
	char *color, *black = "black", *red = "red";
	
	if (!(cev->ev->flags & FLAG_UNTIMED)) {
		calendar_time_t end = cev->ev->start + cev->ev->duration;
		
		strftime(strtimestart, sizeof strtimestart, "%H:%M",
		         localtime(&cev->ev->start));
		strftime(strtimeend, sizeof strtimeend, "%H:%M",
		         localtime(&end));
		snprintf(timestr, sizeof timestr, "%s - %s", strtimestart,
		         strtimeend);
	} else {
		snprintf(timestr, sizeof timestr, "<i>%s</i>",
		         _("All day event"));
	}

	/* TODO: LOCATION / NOTES labels */
	color = (cev->in_progress ? red : black);
		
	markup_printf(cev->pl, "<span foreground=\"%s\"><b>%s</b>\n%s</span>",
	              color, cev->summary, timestr);
}

gboolean calendar_update(gpointer data)
{
	struct stat db;
	static time_t prev_modified = 0;
	static int prev_day = -1;
	struct tm *lt;
	time_t current_time;
	struct calevent *event;
	int i;

	if (stat(db_fname, &db) == -1) {
		if (g_slist_length(calendar_entries) != 0)
			free_calendar_entries();
		make_no_events_label();
		return TRUE;
	}

	if (db.st_mtime > prev_modified) { /* DB changed */
		prev_modified = db.st_mtime;
		calendar_events_db_update();
	}

	time(&current_time);
	lt = localtime(&current_time);
	current_time = mktime(lt);
	
	/* new day */
	if (prev_day != lt->tm_yday) {
		prev_day = lt->tm_yday;
		calendar_events_db_update();
		date_update();
	}
	
	/* check events' status */
	for (i = 0; (event = g_slist_nth_data(calendar_entries, i)); i++) {
		if ((event->ev->start + event->ev->duration) < current_time) {
			/* event finished */
			remove_calendar_entry(event);

			if (g_slist_length(calendar_entries) == 0)
				make_no_events_label();
			
			continue;
		}

		if (event->ev->start < current_time) { /* event in progress */
			if (event->in_progress == TRUE)
				continue;

			event->in_progress = TRUE;
			refresh_event_markup(event);			
		}
	}

	refresh_calendar_widget();

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
	
	if (!event_db_start())
	 	return; /* could not load the db */

	free_calendar_entries();

	/* this shows events until 23:59:59, might want to fix that */
	midtm = localtime(&current_time);
	midtm->tm_sec = 59;
	midtm->tm_min = 59;
	midtm->tm_hour = 23;
	midnight = mktime(midtm);

	/* TODO: show/not show All day events */
	events = event_db_untimed_list_for_period(midnight - 86400, midnight, TRUE);
	for (i = 0; (ev = (event_t) g_slist_nth_data(events, i)); i++)
		calendar_add_event(ev);
	event_db_list_destroy(events);
	
	events = event_db_list_for_period(current_time, midnight);
	for (i = 0; (ev = (event_t) g_slist_nth_data(events, i)); i++)
		calendar_add_event(ev);
	event_db_list_destroy(events);

	if (g_slist_length(calendar_entries) == 0)
		make_no_events_label();

	refresh_calendar_widget();

	event_db_stop();
}

static void calendar_add_event(event_t event)
{
	struct calevent *cal = g_malloc(sizeof(struct calevent));
	event_details_t details;

	cal->ev = g_malloc(sizeof(struct event_s));
	memcpy(cal->ev, event, sizeof(struct event_s));
	cal->in_progress = FALSE;
	calendar_entries = g_slist_append(calendar_entries, cal);

	details = event_db_get_details(event);
	cal->summary = g_strdup(details->summary);
	cal->description = g_strdup(details->description);
	event_db_forget_details(event);
	
	cal->pl = gtk_widget_create_pango_layout(calendar.scroll->draw, NULL);
	pango_layout_set_wrap(cal->pl, PANGO_WRAP_WORD);
	calendar.scroll->list = g_slist_append(calendar.scroll->list, cal->pl);

	refresh_event_markup(cal);
	
	/* change event->start for recurring events */
	if (event->recur) {
		int min, sec, hour;
		time_t now;
		struct tm *ev = localtime(&event->start);
		
		min = ev->tm_min;
		sec = ev->tm_sec;
		hour = ev->tm_hour;

		time(&now);
		ev = localtime(&now);
		ev->tm_min = min;
		ev->tm_sec = sec;
		ev->tm_hour = hour;

		event->start = mktime(ev);
	}
}

static void free_calevent(struct calevent *ev)
{
	g_free(ev->summary);
	g_free(ev->description);
	g_object_unref(ev->pl);
	g_free(ev);
}

static void remove_calendar_entry(struct calevent *ev)
{
	calendar_entries = g_slist_remove(calendar_entries, ev);
	calendar.scroll->list = g_slist_remove(calendar.scroll->list, ev->pl);
	free_calevent(ev);
}

static void free_calendar_entries(void)
{
	int i;
	struct calevent *event;

	for (i = 0; (event = g_slist_nth_data(calendar_entries, i)); i++)
		free_calevent(event);

	if (calendar.noevent) {
		g_object_unref(calendar.noevent);
		calendar.noevent = NULL;
	}
		
	g_slist_free(calendar.scroll->list);
	g_slist_free(calendar_entries);
	calendar_entries = calendar.scroll->list = NULL;
}
