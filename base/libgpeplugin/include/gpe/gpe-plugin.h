/*
 * Copyright (C) 2005, Luca De Cicco <ldecicco@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef __GPE_PLUGIN_H__
#define __GPE_PLUGIN_H__

#include <glib.h>
#include <gtk/gtk.h>

#define DECL_PLUGIN __plugin_t __plugin__ = { 
#define DEFINE_WIDGET(widget_)  .widget_new = widget_,
#define DEFINE_RENDER_BODY(rfunc)  .render_body = rfunc,
#define DEFINE_RENDER_HEADER(rfunc) .render_header = rfunc,
#define DEFINE_RENDER_TAIL(rfunc)  .render_tail = rfunc,
#define DEFINE_NAME(name_)  .name = name_,
#define DECL_END };	

typedef enum { 	GPE_PLUGIN_VERTICAL = 0,
		GPE_PLUGIN_HORIZONTAL
} gpe_plugin_orientation;
	

typedef GtkWidget *(* GtkWidgetNew) ();
typedef void (*RenderFunc)();

typedef struct 
{
	GtkWidget * (*widget_new)();
	gchar *name;
	void (*render_header)();
	void (*render_body)();	
	void (*render_tail)();	
} __plugin_t;


typedef struct gpe_plugin_frontend
{
	GtkWidget *widget;
	GtkWidget *scrolled;
} *gpe_plugin_frontend_t;



typedef struct gpe_plugin
{
	/* Plugin name */
	gchar *name ;	
	/* Function pointer called to render the plugin */
	void (*render_header)();
	void (*render_body)();	
	void (*render_tail)();	
	gpe_plugin_frontend_t frontend;
	
} *gpe_plugin_t ;

typedef enum  { 
		VBOX=0,
		HBOX=1,
		ERROR
} box_type;

typedef struct widget_box {
	GtkWidget *widget;
	box_type type;
} *widget_box_t ;



void 
plugin_add (gpe_plugin_t plugin);

void 
plugin_init (GtkWidget *w, gpe_plugin_orientation o);


gpe_plugin_t
plugin_new (gchar *name,GtkWidget *widget, void *render_header, 
			void *render_body, void *render_tail);

void
plugin_remove (gpe_plugin_t plugin);

void 
plugin_render_all ();

void 
plugin_add_to_box(gpe_plugin_t plugin, gchar *name);

gpe_plugin_t 
plugin_load_from_file (gchar *filename);

gboolean
plugin_parse_xml_interface(gchar *file);



#endif
