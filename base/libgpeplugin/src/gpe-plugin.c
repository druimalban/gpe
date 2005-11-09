/*
 * Copyright (C) 2005, Luca De Cicco <ldecicco@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gpe/gpe-plugin.h>
#include <string.h>

/* All plugins */
GSList *loaded_plugin=NULL;

/* Main widget */
GtkWidget *_main_widget = NULL;

/* All plugins are contained here */
GtkWidget *_box=NULL ;

GSList *parent_list=NULL;
GHashTable *widgets;
gchar *parent=NULL;
gint state;

gboolean using_xml_interface = FALSE;

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
	g_signal_connect (G_OBJECT (_main_widget), "configure-event", G_CALLBACK (external_event), NULL);
	
	widgets = g_hash_table_new (g_str_hash, g_str_equal);
	parent = g_malloc(64);

	
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
				if (!using_xml_interface)
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

/* Add widget named "name" to hash list */
static gboolean add_widget (GHashTable **hash, box_type type, const gchar *name)
{
	widget_box_t wb = (widget_box_t)g_hash_table_lookup (*hash, name);
	
	if (! wb)
	{
		GtkWidget *w;
		GtkWidget *parent_w;
		widget_box_t w_box ;
		widget_box_t new_w_box = (widget_box_t)g_malloc(sizeof(struct widget_box));
		
		if( parent != NULL){
			w_box = (widget_box_t)g_hash_table_lookup(*hash, parent);
		}else{
			w_box = NULL;
		}
		/* If this is the first widget it has no parent so we will use the 
			root widget 
		*/
		if(w_box){	
			parent_w = w_box->widget;
		}else{
			parent_w = _box;
		}
		
		/* TODO spacing written in xml*/
		if (type == HBOX)
		{
			w = gtk_hbox_new (TRUE, 0);
		}else{
			w = gtk_vbox_new (TRUE, 0);
		}
		/* 
			printf ("Adding widget %s(%p) child of %s(%p)\n", name, w,parent,parent_w); 
		*/
		gtk_box_pack_end (GTK_BOX (parent_w), w, TRUE, TRUE, 2);
		
		new_w_box->widget = w;
		new_w_box->type = type;
		g_hash_table_insert (*hash,g_locale_from_utf8 (name, -1, NULL, NULL, NULL), new_w_box);
		return TRUE;
	}else{
		g_warning ("Already defined widget name: %s\n",name);

		return FALSE;
	}
}

static void set_parent (const gchar *name)
{
	strcpy(parent,name);
	parent_list = g_slist_append (parent_list, (gpointer)(g_strdup(name)));
	
}

static void unset_parent ()
{
	GSList *last = g_slist_last (parent_list );
	gchar *oldparent = (gchar *)last->data;
	
	
	if (parent_list == NULL){
		g_warning ("unset_parent called before set_parent was called");
		return ;
	}
	
	strcpy (parent, oldparent);
	parent_list = g_slist_delete_link (parent_list , last);
}



 /* Called for close tags </foo> */
void 
xml_widget_read_end_element (GMarkupParseContext *context,
							const gchar         *element_name,
							gpointer             user_data,
							GError             **error)
{
	unset_parent();
}


void 
xml_widget_read_start_element ( GMarkupParseContext *context,
		const gchar *e_n,
		const gchar **attr_names,
		const gchar **attr_values,
		gpointer user_data,
		GError **error)
{
 
	
	if (!strcmp (e_n, "vbox"))
	{
		state = VBOX;
	}else if (!strcmp (e_n, "hbox")){
		state = HBOX;
	
	}else {
		state = ERROR;
	}
	
	if (state != ERROR)
	{
		if (!strcmp (attr_names[0], "name"))
		{
			gchar *name = g_strdup(attr_values[0]);
			add_widget (&widgets, state, name);
			set_parent (name);
			g_free( name);
		}
		else
		{
			state = ERROR;
		}
	}
	
}

void plugin_add_to_box(gpe_plugin_t plugin, gchar *name)
{
	GtkWidget *w = plugin->frontend->scrolled;
	GtkWidget *parent ;
	
	widget_box_t wb = (widget_box_t)g_hash_table_lookup(widgets, name);
	/* printf ("Adding %p to parent %p\n", w, wb->widget); */
	if (!wb){
		parent = _box;
	}else{
		parent = wb->widget;
	}
	gtk_box_pack_start (GTK_BOX (parent), w, TRUE, TRUE, 0);
	plugin_add (plugin);
	
}

gboolean plugin_parse_xml_interface(gchar *file)
{
	FILE *fp;
	GMarkupParser parser = {xml_widget_read_start_element,
				xml_widget_read_end_element,NULL, NULL, NULL};	

	GMarkupParseContext *pc = g_markup_parse_context_new(&parser, 0, NULL, NULL);
	
	
	fp = fopen (file, "r");
	if (fp == NULL)
	{
		g_warning ("Can't open xml interface file");
		g_markup_parse_context_free (pc);
		return FALSE;
	}
	while (! feof (fp))
	{
		char buf[1024];
		int n;
		GError *err = NULL;
		
		n = fread (buf, 1, 1024, fp);
		if (g_markup_parse_context_parse (pc, buf, n, &err) == FALSE)
		{
          g_warning (err->message);
          g_error_free (err);
          g_markup_parse_context_free (pc);
          fclose (fp);
          return FALSE;
        }
    }
	g_markup_parse_context_free (pc);
    fclose (fp);
	using_xml_interface = TRUE;
	//g_hash_table_foreach(widgets, print_entry, NULL);	
	return TRUE;
}
