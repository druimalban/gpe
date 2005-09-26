/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <gtk/gtk.h>
#include "today.h"

static void date_box_press(GtkWidget *wid, gpointer data);

void date_init(void)
{
	GtkWidget *date_icon, *hbox;

	date.label_text = NULL;

	/* toplevel eventbox */
	date.toplevel = gtk_event_box_new();
	gtk_widget_set_name(date.toplevel, "date_box");

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(date.toplevel), hbox);

	/* date icon */
	date_icon = gtk_image_new_from_file(IMAGEPATH(date.png));

	gtk_box_pack_start(GTK_BOX(hbox), date_icon, FALSE, FALSE, 5);

	/* date label */
	date.label = gtk_label_new(NULL);
	gtk_widget_set_name(date.label, "date_label");
	gtk_box_pack_start(GTK_BOX(hbox), date.label, FALSE, FALSE, 0);

	gtk_widget_show_all(date.toplevel);

	date_update();

	/* date refreshing stuff */

	/* user can click the date area to get it refreshed */
	gtk_widget_set_events(date.toplevel, GDK_BUTTON_PRESS_MASK);
	gtk_signal_connect(GTK_OBJECT(date.toplevel), "button_press_event",
	                   GTK_SIGNAL_FUNC(date_box_press), NULL);

	/* TODO: listen for HUP signal? */

	/* gtkrc */
	gtk_rc_parse_string("widget '*date_box' style 'date_box'");
	gtk_rc_parse_string("widget '*date_label' style 'date_label'");
}

void date_free(void)
{
	g_free(date.label_text);
	gtk_widget_unref(date.toplevel);
}

static void date_box_press(GtkWidget *wid, gpointer data)
{
	date_update();
}

void date_update(void)
{
	char str[69];
	time_t t;

	time(&t);
	strftime(str, sizeof str, _("%A, %d %B %Y"), localtime(&t));
	
	g_free(date.label_text);
	date.label_text = g_locale_to_utf8(str, -1, NULL, NULL, NULL);

	gtk_label_set_text(GTK_LABEL(date.label), date.label_text);
}
