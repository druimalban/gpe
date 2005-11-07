/*
 * Copyright (C) 2005, Luca De Cicco <ldecicco@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gpe/gpe-plugin.h>

/* All plugins */
GSList *loaded_plugin=NULL;

/* Main widget */
GtkWidget *_main_widget = NULL;

/* All plugins are contained here */
GtkWidget *_box=NULL ;


gpe_plugin_orientation _orientation = GPE_PLUGIN_VERTICAL;

/* Load all plugins in the plugin directory */
void load_all_plugins ()
{
	/* TODO */	
	
}

/* Adds a plugin to loaded_plugin */
void 
plugin_add (gpe_plugin_t plugin)
{
	loaded_plugin = g_slist_append (loaded_plugin, plugin);
}

/* Load a .so plugin  */
gpe_plugin_t 
plugin_load_from_file (gchar *filename)
{
	GModule *module;
	__plugin_t *plugin;	
	
	gpe_plugin_t p;
	
	
	module = g_module_open (filename, G_MODULE_BIND_LAZY);
	if (!module)
	{
		g_warning ("%s",g_module_error ());
		g_warning ("Can't open plugin %s\n",filename);
		return NULL;
	}
	
	if (!g_module_symbol (module, "__plugin__", (gpointer *)&plugin))
	{
		g_warning ("Can't find __plugin__ symbol");
		return NULL;
	}	
	p = plugin_new(plugin->name,plugin->widget_new(), 
				plugin->render_header, plugin->render_body, plugin->render_tail);

/*	printf("plugin name: %s\n",plugin->name); */
	return p;
}

/* Stolen from minilite */
/* FIXME */
gboolean external_event(GtkWindow *window, GdkEventConfigure *event, gpointer user_data)
{ 
	/*
	if (event->type == GDK_CONFIGURE)
	{
		printf ("------------------\nExternal event occurred\n\n\n\n");
		if ( event->width > event->height ) 
		{
			gtk_widget_destroy(_box);
			plugin_init(_main_widget,GPE_PLUGIN_HORIZONTAL);
		} else {
			gtk_widget_destroy(_box);
			plugin_init(_main_widget,GPE_PLUGIN_HORIZONTAL);
		}
		plugin_render_all ();
	}  
   */
	return FALSE;
}


void 
plugin_init (GtkWidget *w, gpe_plugin_orientation o)
{
	_main_widget = w;
	_orientation = o;
	_box = ( o == GPE_PLUGIN_HORIZONTAL ? gtk_hbox_new (FALSE, 0) : gtk_vbox_new (FALSE, 0) );
	
	gtk_container_add (GTK_CONTAINER (_main_widget), _box);	
	printf ("Adding configure-event signal to exernal_event handler\n");
	g_signal_connect (G_OBJECT (_main_widget), "configure-event", G_CALLBACK (external_event), NULL);
}


void 
plugin_frontend_delete (gpe_plugin_frontend_t fe)
{
	g_free (fe);
}

void
plugin_delete (gpe_plugin_t p)
{
	plugin_frontend_delete (p->frontend);
	g_free(p);
}



/* removes a plugin */
void plugin_remove (gpe_plugin_t plugin)
{
	if (plugin)
	{
		loaded_plugin = g_slist_remove(loaded_plugin, plugin);
		plugin_delete(plugin);	
	}
	else
	{
		g_warning ("Can't remove null plugin\n");
	}
}

gpe_plugin_frontend_t
plugin_frontend_new (GtkWidget *widget)
{
	if (widget == NULL )
	{
		g_warning ("Widget is null. Can't create plugin fronted\n");
		return NULL;
	}
	gpe_plugin_frontend_t fe = g_malloc(sizeof(struct gpe_plugin_frontend));
	
	fe->widget = widget;
	fe->scrolled = gtk_scrolled_window_new (NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (fe->scrolled),
       										widget);  
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (fe->scrolled), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	return fe;
}

gpe_plugin_t
plugin_new (gchar *name,GtkWidget *widget, void *render_header, 
			void *render_body, void *render_tail)
{
	gpe_plugin_t plugin = g_malloc (sizeof (struct gpe_plugin ));
	plugin->name = name;
	plugin->render_header = render_header;
	plugin->render_body = render_body;
	plugin->render_tail = render_tail;
	
	plugin->frontend = plugin_frontend_new (widget);
	return plugin;	
}

/* Renders all loaded plugins */
void plugin_render_all ()
{
	if (loaded_plugin)
	{
		GSList *iter;
		for ( iter = loaded_plugin ; iter ; iter=iter->next)
		{
			gpe_plugin_t plugin = iter->data;
			if (plugin)
			{
				/* Adding the plugin to the box */
				gtk_box_pack_start (GTK_BOX (_box), plugin->frontend->scrolled, TRUE, TRUE, 0);
				gtk_widget_show (plugin->frontend->scrolled);
				gtk_widget_ref (plugin->frontend->scrolled);
				gtk_widget_show (plugin->frontend->widget);
				gtk_widget_ref (plugin->frontend->widget);
				if(plugin->render_header)
					plugin->render_header();
				if(plugin->render_body)
					plugin->render_body();
				if(plugin->render_tail)
					plugin->render_tail();
			}
		}
		gtk_widget_show_all (_box);		
	}
}

/* Switches two plugins in the container widget */
void plugin_switch (gpe_plugin_t p1, gpe_plugin_t p2)
{
	GSList *p1_item, *p2_item;
	gpe_plugin_t temp;
	
	p1_item = g_slist_find (loaded_plugin, (gpointer)p1);
	p2_item = g_slist_find (loaded_plugin, (gpointer)p2);
	
	if ( p1_item == NULL || p2_item == NULL )
	{
		g_warning ("Plugin not in the list\n");
		return ;
	}
	
	temp = p2; 
	
	p2_item->data = p1;
	p1_item->data = temp;
	
}
