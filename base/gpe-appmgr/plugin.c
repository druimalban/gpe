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
#include <gdk/gdkx.h>
#include "plugin.h"
#include "main.h"
#include "package.h"
#include "gnome-exec.h"

void plugin_exec (GtkWidget *sock, struct package *p)
{
	char *exec=NULL;
	char *cmd[] = {"/bin/sh", "-c", NULL};
	exec = g_strdup_printf ("%s --xid %ld", package_get_data (p, "command"),
				GDK_WINDOW_XWINDOW (sock->window));
	cmd[2] = exec;
	gnome_execute_async (NULL, 3, cmd);
}

void plugin_load (char *name)
{
	GtkWidget *sock;
	struct package *p=NULL;
	char *fn=NULL;

	fn = g_strdup_printf ("/usr/share/gpe/appmgr/plugins/%s",name);
	//fn = g_strdup_printf ("./%s.p",name);

	p = package_read (fn);
	if (!p)
	{
		g_print ("AAARGH! NO PACKAGE FILE!\n");
		return;
	}

	sock = gtk_socket_new ();
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook),
				  sock,
				  create_tab_label (package_get_data (p,"title"),
						    package_get_data (p, "icon")));
	gtk_widget_realize (sock);

	plugin_exec (sock, p);
}

