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
#include "today.h"
#ifdef 0
void todo_init(void)
{
	GtkWidget *vboxlogo, *logo, *label;
	GdkPixmap *pix;
	GdkBitmap *mask;
	GdkPixbuf *img;

	todo_module.toplevel = gtk_hbox_new(FALSE, 0);

	/* todo icon */
	vboxlogo = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(todo_module.toplevel), vboxlogo,
	                   FALSE, FALSE, 5);

	img = gdk_pixbuf_new_from_file(IMAGEPATH(gpe-todo.png));
	if (!img) { gpe_error_box(_("Could not load pixmap")); exit(1); }
	gdk_pixbuf_render_pixmap_and_mask(img, &pix, &mask, 255);
	logo = gtk_pixmap_new(pix, mask);
	gdk_pixbuf_unref(img);
	gtk_box_pack_start(GTK_BOX(vboxlogo), logo, FALSE, FALSE, 0);

	label = gtk_label_new("TODO MODULE");
	gtk_box_pack_start(GTK_BOX(todo_module.toplevel), label,
	                   FALSE, FALSE, 5);

	gtk_widget_show_all(todo_module.toplevel);
}
#endif
void todo_free(void)
{

}

void todo_update(void)
{

}
