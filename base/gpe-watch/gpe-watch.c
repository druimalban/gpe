/*
 * Copyright (C) 2003 Nils Faerber <nils@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>    
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <libintl.h>
#include <sys/time.h>
#include <time.h>

#include <gtk/gtk.h>

/* GPE specific */
#ifdef USE_GPE
#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/render.h>
#include <gpe/picturebutton.h>
#include <gpe/errorbox.h>
#include <gpe/gpetimesel.h>
#include <gpe/gpeclockface.h>
#endif


static struct gpe_icon my_icons[] = {
	{ "icon", PREFIX "/share/pixmaps/gpe-watch.png" },
	{ NULL, NULL }
};


/* GTK UI */
GtkWidget *window;				/* Main window */
GtkStyle *style;
GtkObject *hours, *minutes, *seconds;
GtkWidget *date;
guint TimeoutID;
gboolean AsDialog = FALSE;

/* helper for i8n */
#define _(x) gettext(x)



gboolean clock_idle (gpointer data)
{
time_t t;
struct tm tm;

	time (&t);
	localtime_r (&t, &tm);

	gtk_adjustment_set_value(GTK_ADJUSTMENT(hours), tm.tm_hour);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(minutes), tm.tm_min);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(seconds), tm.tm_sec);

	if (!AsDialog) {
		char datestr[128];

		strftime(datestr, 128, "<b>%a %d %b %G%nWeek %W</b>", &tm);
		gtk_label_set_markup(GTK_LABEL(date), datestr);
	}

return TRUE;
}

int
main (int argc, char *argv[])
{
/* GTK Widgets */
GdkColor col;
gchar *color = "gray80";
GtkWidget *clockface;
GtkWidget *vbox;
GtkWidget *ScrolledWindow;

#ifdef USE_GPE
	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);
#else
	gtk_set_locale ();
	gtk_init (&argc, &argv);
#endif

	while (argc-- > 0) {
		if (strcmp("-d",argv[argc]) == 0)
			AsDialog = TRUE;
	}

	gdk_color_parse (color, &col);
  
	/* GTK Window stuff */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Watch");
	if (AsDialog) {
		gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
		gtk_window_set_default_size(GTK_WINDOW(window), 140, 140);
	}
	gtk_widget_realize (window);

	/* Destroy handler */
	g_signal_connect (G_OBJECT (window), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);

#ifdef USE_GPE
	gpe_set_window_icon (window, "icon");
#endif

	ScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add (GTK_CONTAINER (window), ScrolledWindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(ScrolledWindow);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (ScrolledWindow), vbox);
	gtk_widget_show(vbox);

	hours = gtk_adjustment_new(1.0,0.0,23.0,1,15,1);
	minutes = gtk_adjustment_new(2.0,0.0,59.0,1,15,1);
	seconds = gtk_adjustment_new(2.0,0.0,59.0,1,15,1);

	clockface = gpe_clock_face_new(GTK_ADJUSTMENT(hours), GTK_ADJUSTMENT(minutes), GTK_ADJUSTMENT(seconds));
	gtk_box_pack_start (GTK_BOX (vbox), clockface, TRUE, TRUE, 0);
	gtk_widget_show (clockface);

	if (!AsDialog) {
		date = gtk_label_new("");
		gtk_box_pack_start (GTK_BOX (vbox), date, TRUE, TRUE, 10);
		gtk_widget_show(date);
	}

	clock_idle(NULL);

	TimeoutID = gtk_timeout_add(1000 /*ms*/, clock_idle, NULL);

	gtk_widget_show (window);
	gtk_main ();
	gtk_timeout_remove(TimeoutID);

return (0);
}
