/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>

#include "today.h"
#include "today-prefs.h"

/* Private functions, public ones are in today.h */
static void resize_callback (GtkWidget *, gpointer);
static void init_main_window (void);
static void load_modules (void);

int main(int argc, char **argv)
{
	if (!gpe_application_init(&argc, &argv))
		exit(1);

	gtk_rc_parse(DATAPATH(gtkrc));
	
	init_main_window();
	load_modules();
	
	gtk_widget_show_all(window.toplevel);

	gtk_main();

	exit(0);
}

static void resize_callback(GtkWidget *widget, gpointer data)
{
	gint new_height, new_width;

	/* check if window got resized (eg: screen rotation) */

	gdk_window_get_size(window.toplevel->window, &new_width, &new_height);

	if ((new_height >= new_width) == window.mode) { /* screen got rotated */
		window.mode = !window.mode;
	}

	window.height = new_height;
	window.width = new_width;
}

static void init_main_window(void)
{
	window.toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window.toplevel), _("Summary"));
	gtk_widget_set_name(window.toplevel, "main_window");
	gtk_widget_realize(window.toplevel);
	window.height = window.width = 0;

	window.vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window.toplevel), window.vbox1);

	/* final stuff */
	gtk_signal_connect(GTK_OBJECT(window.toplevel), "destroy",
			   GTK_SIGNAL_FUNC(gtk_exit), NULL);

	gtk_signal_connect(GTK_OBJECT(window.toplevel), "size_allocate",
			   GTK_SIGNAL_FUNC(resize_callback), NULL);
}

static void load_modules(void)
{
	date_init();
	gtk_box_pack_start(GTK_BOX(window.vbox1), date.toplevel, FALSE, FALSE, 0);

	calendar_init();
	gtk_box_pack_start(GTK_BOX(window.vbox1), calendar.toplevel,
	                   TRUE, TRUE, 0);
}

void load_pixmap(const char *path, GdkPixmap **pixmap, GdkBitmap **mask,
                 int alpha)
{
	if (!load_pixmap_non_critical(path, pixmap, mask, alpha)) {
		gpe_error_box_fmt("Could not load pixmap\n%s", path);
		exit(1);
	}
}

/*
 * return 0: failure
 * return 1: success
 */
int load_pixmap_non_critical(const char *path, GdkPixmap **pixmap,
                             GdkBitmap **mask, int alpha)
{
	GdkPixbuf *img;

#if GTK_MAJOR_VERSION >= 2
	img = gdk_pixbuf_new_from_file(path, NULL);
#else
	img = gdk_pixbuf_new_from_file(path);
#endif
	if (!img)
		return 0;

	gdk_pixbuf_render_pixmap_and_mask(img, pixmap, mask, alpha);
	gdk_pixbuf_unref(img);

	return 1;
}
