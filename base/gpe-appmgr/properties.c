/* gpe-appmgr - a program launcher

   Copyright 2002 Robert Mibus;

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <gtk/gtk.h>
#include "properties.h"

int on_btn_close_clicked (GtkWidget *btn, gpointer data)
{
	GtkWidget *win;
	win = GTK_WIDGET(data);
	gtk_widget_hide (win);
	gtk_widget_destroy (win);
	return TRUE;
}

void show_properties (struct package *p)
{
	GtkWidget *win;
	GtkWidget *lst;
	GtkWidget *vbox;
	GtkWidget *btn;
	char *titles[]={"Item", "Data"};
	char *temp[2];

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	vbox = gtk_vbox_new (0,0);
	gtk_container_add (GTK_CONTAINER(win), vbox);

	lst = gtk_clist_new_with_titles (2, titles);
	gtk_box_pack_start_defaults (GTK_BOX(vbox), lst);

	btn = gtk_button_new_with_label ("Close");
	gtk_signal_connect (GTK_OBJECT(btn), "clicked",
			    (GtkSignalFunc) on_btn_close_clicked,
			    (gpointer)win);

	gtk_box_pack_start (GTK_BOX(vbox), btn, 0, 0, 0);
	
	while (p)
	{
		temp[0] = p->name;
		temp[1] = p->data;
		gtk_clist_append (GTK_CLIST(lst), temp);

		p = p->next;
	}
	gtk_clist_columns_autosize (GTK_CLIST (lst));

	gtk_widget_show_all (win);
}


