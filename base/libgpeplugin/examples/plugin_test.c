/*
 * Copyright (C) 2005, Luca De Cicco <ldecicco@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */


#include <gpe/gpe-plugin.h>
#include <glib.h>
#include <gtk/gtk.h>

gpe_plugin_t a,b,c,d;

void render_body()
{
	
}

int
main( int argc , char *argv[])
{
	GtkWidget *main_window, *button, *hbox;
	GTimer *t;	
	gdouble secs;
	
	t = g_timer_new ();	
	g_timer_start (t);
	gtk_init (&argc, &argv);
	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  	gtk_window_set_title (GTK_WINDOW (main_window), "Plugin test");
  	g_signal_connect (G_OBJECT (main_window), "delete-event", gtk_main_quit, NULL);
	
	hbox = gtk_hbox_new (FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	
	plugin_init(main_window, 0);
	plugin_parse_xml_interface("iface.xml");
	
	a = plugin_new ("Plugin A",  button, NULL, render_body , NULL)	;
	b = plugin_load_from_file("./plugin_foo.so");		 /*Calendar plugin*/
	c = plugin_load_from_file("./todo_plugin.so");		/*todo plugin */
	d = plugin_load_from_file("./calendar_plugin.so");
	
	gtk_window_set_default_size (GTK_WINDOW(main_window), 200,200);	
	plugin_add_to_box(a, "Vbox1");
	//plugin_add_to_box(b, "Hbox2" );
	plugin_add_to_box(c, "Hbox3");
	plugin_add_to_box(d, "Hbox2");
	
	//plugin_add ( c );
	//plugin_add ( d );

	//plugin_switch ( c , d);
	plugin_render_all ();
	secs = g_timer_elapsed (t,NULL);
	printf ("Time elapsed: %f\n", secs);
	
	gtk_widget_show_all (main_window );
	gtk_main();
}
