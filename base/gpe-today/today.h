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

#include <stdlib.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include "xsettings-client.h"

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
#define DATAPATH(file) PREFIX "/share/" PACKAGE "/" #file

enum { PORTRAIT, LANDSCAPE };
enum { NO_SPEC, TILED_BG_IMG, STRECHED_BG_IMG, SOLID_BG_COL, MBDESKTOP_BG };

struct window_s {
    GtkWidget *toplevel;            /* top-level window */
      GtkWidget *vbox1;             /* top-level vertical box */
        GtkWidget *vpan1;

    gint mode;    /* either PORTRAIT or LANDSCAPE */
};

extern struct window_s window;

struct conf_s {
    int bg;
    XSettingsClient *xst_client;
};

extern struct conf_s conf;

struct myscroll {
	GtkAdjustment *adjust;
	GtkWidget *draw;
	GtkWidget *scrollbar;
	GSList *list;
	int yspacing;
	int width;
};

struct myscroll * myscroll_new(gboolean continuous);
void myscroll_update_upper_adjust(struct myscroll *scroll);

void markup_printf(PangoLayout *pl, const char *fmt, ...);

void set_background(const char *spec);

#endif /* !HAVE_TODAY_H */
