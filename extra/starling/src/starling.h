/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef STARLING_H
#define STARLING_H

#include <glib/gtypes.h>
#include <glib/gkeyfile.h>

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>

#include <libintl.h>
#define _(String)   String
#define gettext_noop(String)    String
#define N_(String)  gettext_noop(String)

#include "playlist.h"

typedef struct _Starling Starling;

struct _Starling {
    GtkWidget *notebook;
    GtkWidget *window;
    GtkWidget *artist;
    GtkWidget *title;
    GtkWidget *time;
    GtkWidget *playpause;
    GtkWidget *stop;
    GtkWidget *next;
    GtkWidget *prev;
    GtkWidget *scale;
    gboolean scale_pressed;
    GtkWidget *treeview;
    GtkWidget *random;
    GtkWidget *up;
    GtkWidget *down;
    GtkWidget *add;
    GtkWidget *clear;
    GtkWidget *save;
    GtkWidget *remove;
    GtkWidget *fs;
    gchar *fs_last_path;
    GtkWidget *textview;
    GtkWidget *webuser_entry;
    GtkWidget *webpasswd_entry;
    GtkWidget *web_submit;
    GtkWidget *web_count;
    gint bolded;
    PlayList *pl;
    gboolean has_lyrics;
    GKeyFile *keyfile;
    gint64 current_length;
    gboolean enqueued;
};

#endif
