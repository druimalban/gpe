/*****************************************************************************
 * gsoko/gsoko.c : main function
 *****************************************************************************
 * Copyright (C) 2000 Jean-Michel Grimaldi
 *
 * Author: Jean-Michel Grimaldi <jm@via.ecp.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *****************************************************************************/

#include <stdio.h>	/* EOF */
#include <string.h>	/* strrchr() */
#include <unistd.h>	/* chdir() */
#include <stdlib.h>	/* exit() */

#include "gsoko.h"

int main(int argc, char *argv[])
{
	GtkWidget *menubar;
	char* sdir;

 	gtk_init(&argc, &argv);	/* process gtk+ arguments */

	/* change the working directory to the one containing the binary */
	sdir = argv[0];
	*strrchr(sdir, '/') = 0;	/* XXX this should be OS dependent : windows = '\\' */
	chdir(sdir);

	/* create a new window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), W_TITLE);

	/* fixed size */
	gtk_window_set_policy(
		GTK_WINDOW(window),
		FALSE,	/* allow_shrink */
		FALSE,	/* allow_grow */
		FALSE);	/* auto_shrink */

	/* window: delete */
	gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(delete_event), NULL);

	/* window: key_press and key_release */
	gtk_signal_connect(GTK_OBJECT(window), "key_press_event", GTK_SIGNAL_FUNC(key_press_event), NULL);

	/* create a box to pack widgets into, and put it into window */
	box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), box);

	/* create the menu and put it into box */
	get_main_menu(window, &menubar);
	gtk_box_pack_start(GTK_BOX(box), menubar, FALSE, TRUE, 0);
          
	/* create a drawing area and put it into box */
	darea = gtk_drawing_area_new();
	gtk_widget_set_usize(darea, W_DAREA, H_DAREA);
	gtk_box_pack_start(GTK_BOX(box), darea, TRUE, TRUE, 0);

	/* darea: expose */
	gtk_signal_connect(GTK_OBJECT(darea), "expose_event",  GTK_SIGNAL_FUNC(expose_event), NULL);

	/* show our objects */
	gtk_widget_show(menubar);
	gtk_widget_show(darea);
	gtk_widget_show(box);
	gtk_widget_show(window);

	load_pixmaps();	/* load the pixmaps (needs darea) */
	init_var();	/* initialize globals */

	gtk_main();

	save_level();

	exit(0);
}

