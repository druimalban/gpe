/*
* This file is part of GPE-Memo
*
* Copyright (C) 2007 Alberto García Hierro
*	<skyhusker@rm-fr.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
*/

#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>


typedef struct {
    GtkWidget *main_window;
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *treeview;
    GtkListStore *store;
} Interface;

void interface_init (Interface *in);
void interface_record (Interface *in, const gchar *filename);

#endif
