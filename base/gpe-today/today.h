/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef HAVE_TODAY_H
#define HAVE_TODAY_H

#include <libintl.h>
#include <gtk/gtk.h>

#include "calendar.h"
#include "todo.h"
#include "date.h"
#include "mail.h"

#define _(_x) gettext(_x)

#ifndef PREFIX
# define PREFIX "/usr/local"
#endif

#ifndef PACKAGE
# define PACKAGE "gpe-today"
#endif

#define IMAGEPATH(file) PREFIX "/share/" PACKAGE "/pixmaps/" #file
//#define IMAGEPATH(file) "./pixmaps/" #file
#define DATAPATH(file) PREFIX "/share/" PACKAGE "/" #file

enum { PORTRAIT, LANDSCAPE };

struct {
	GtkWidget *toplevel;            /* top-level window */
	  GtkWidget *vbox1;             /* top-level vertical box */

	gint height;  /* window height */
	gint width;   /* window width */
	gint mode;    /* either PORTRAIT or LANDSCAPE */
} window;

struct myscroll {
	GtkAdjustment *adjust;
	GtkWidget *draw;
	GtkWidget *scrollbar;
	GSList *list;
	int yspacing;
	int width;
};

struct myscroll * myscroll_new(gboolean continuous);

void markup_printf(PangoLayout *pl, const char *fmt, ...);

int load_pixmap_non_critical(const char *path, GdkPixmap **pixmap,
                             GdkBitmap **mask, int alpha);

void load_pixmap(const char *path, GdkPixmap **pixmap, GdkBitmap **mask,
                 int alpha);

#endif /* !HAVE_TODAY_H */
