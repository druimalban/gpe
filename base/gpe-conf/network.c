/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE /* Pour GlibC2 */
#endif
#include <time.h>
#include "applets.h"
#include "network.h"
#include "parser.h"
#include "cfgfile.h"
#include "misc.h"

#include <gpe/errorbox.h>
#include <gpe/spacing.h>

extern NWInterface_t* iflist;
extern gint iflen;
extern GtkWidget *mainw;
GtkWidget *table;
GtkWidget* create_nwstatic_widgets(NWInterface_t iface);
GtkWidget* create_nwdhcp_widgets(NWInterface_t iface);
GtkWidget* create_nwppp_widgets(NWInterface_t iface);


void changed_nwtype(GtkToggleButton *togglebutton,gpointer user_data)
{
	GtkWidget *ctable, *label;
	gchar wname[100];
	gint row = gtk_notebook_get_current_page(GTK_NOTEBOOK(table))+1; // HACK: add 1 for loop

	if (!gtk_toggle_button_get_active(togglebutton)) return;

	// look who called us...
	strcpy(wname,"static");
	strcat(wname,iflist[row].name);  
	if (gtk_object_get_data(GTK_OBJECT(table),wname) == togglebutton) 
	{
		iflist[row].isstatic = TRUE;
		iflist[row].isdhcp = FALSE;
		iflist[row].isppp = FALSE;
	}	
	strcpy(wname,"dhcp");
	strcat(wname,iflist[row].name);  
	if (gtk_object_get_data(GTK_OBJECT(table),wname) == togglebutton)
	{
		iflist[row].isstatic = FALSE;
		iflist[row].isdhcp = TRUE;
		iflist[row].isppp = FALSE;
	}	
	strcpy(wname,"ppp");
	strcat(wname,iflist[row].name);  
	if (gtk_object_get_data(GTK_OBJECT(table),wname) == togglebutton)
	{
		iflist[row].isstatic = FALSE;
		iflist[row].isdhcp = FALSE;
		iflist[row].isppp = TRUE;
	}	
	
	ctable = NULL;
	if (iflist[row].isstatic) ctable = create_nwstatic_widgets(iflist[row]);
	if (iflist[row].isdhcp) ctable = create_nwdhcp_widgets(iflist[row]);
	if (iflist[row].isppp) ctable = create_nwppp_widgets(iflist[row]);
	if (ctable)	
	{
		label = gtk_label_new(iflist[row].name);
		gtk_notebook_remove_page(GTK_NOTEBOOK(table),gtk_notebook_get_current_page(GTK_NOTEBOOK(table)));
		gtk_notebook_insert_page(GTK_NOTEBOOK(table),GTK_WIDGET(ctable),label,row-1);
		gtk_widget_show_all(table);
		gtk_notebook_set_page(GTK_NOTEBOOK(table),row-1);
	}
}


void create_editable_entry(NWInterface_t* piface, GtkWidget* attach_to, gchar* wprefix, gchar* ltext, gchar* wdata, gint clnr)
{
	GtkWidget* label;
	gchar wname[100];
	guint gpe_boxspacing = gpe_get_boxspacing ();
	
	label = gtk_label_new(ltext);
	gtk_table_attach (GTK_TABLE (attach_to), label, 0, 1, clnr, clnr+1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			gpe_boxspacing, gpe_boxspacing);
	label = gtk_entry_new();
	strcpy(wname,wprefix);
	strcat(wname,piface->name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_remove_data(table,wname);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_entry_set_text(GTK_ENTRY(label),wdata);
	gtk_entry_set_editable(GTK_ENTRY(label),TRUE);
	gtk_table_attach (GTK_TABLE (attach_to), label, 1, 2, clnr, clnr+1,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				gpe_boxspacing, gpe_boxspacing);
}


GtkWidget* create_nwstatic_widgets(NWInterface_t iface)
{
	GtkWidget *label, *container, *ctable;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border     = gpe_get_border ();
 
	gchar wname[100];

	// page headers
	
	ctable=gtk_table_new(3,10,FALSE);
	container = gtk_hbox_new(TRUE,0);  
	
	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);
	gtk_container_set_border_width (GTK_CONTAINER (container), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL),
			gpe_boxspacing, gpe_boxspacing);
	  	  
	label = gtk_radio_button_new_with_label_from_widget(NULL,"static");
	strcpy(wname,"static");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isstatic);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);

	label = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(label),"dhcp");
	strcpy(wname,"dhcp");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isdhcp);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);
	
	label = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(label),"ppp");
	strcpy(wname,"ppp");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isppp);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);
	  
	  
	// page items  
	create_editable_entry(&iface, ctable, "address", _("Address"), iface.address, 1);
	create_editable_entry(&iface, ctable, "netmask",_("Netmask"), iface.netmask, 2);
	create_editable_entry(&iface, ctable, "broadcast",_("broadcast"), iface.broadcast, 3);
	create_editable_entry(&iface, ctable, "network",_("Network"), iface.network, 4);
	create_editable_entry(&iface, ctable, "gateway",_("Gateway"), iface.gateway, 5);
	return ctable;
}


GtkWidget* create_nwdhcp_widgets(NWInterface_t iface)
{
	GtkWidget *label, *container, *ctable;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border     = gpe_get_border ();
 
	gchar wname[100];

	// page headers
	
	ctable=gtk_table_new(3,10,FALSE);
	container = gtk_hbox_new(TRUE,0);  
	
	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);
	gtk_container_set_border_width (GTK_CONTAINER (container), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL),
			gpe_boxspacing, gpe_boxspacing);
	  	  
	label = gtk_radio_button_new_with_label_from_widget(NULL,"static");
	strcpy(wname,"static");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isstatic);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);

	label = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(label),"dhcp");
	strcpy(wname,"dhcp");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isdhcp);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);
	
	label = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(label),"ppp");
	strcpy(wname,"ppp");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isppp);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);
	  
	  
	// page items  
	create_editable_entry(&iface, ctable, "hostname", _("Hostname"), iface.hostname, 1);
	return ctable;
}


GtkWidget* create_nwppp_widgets(NWInterface_t iface)
{
	GtkWidget *label, *container, *ctable;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border     = gpe_get_border ();
 
	gchar wname[100];

	// page headers
	
	ctable=gtk_table_new(3,10,FALSE);
	container = gtk_hbox_new(TRUE,0);  
	
	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);
	gtk_container_set_border_width (GTK_CONTAINER (container), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL),
			gpe_boxspacing, gpe_boxspacing);
	  	  
	label = gtk_radio_button_new_with_label_from_widget(NULL,"static");
	strcpy(wname,"static");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isstatic);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);

	label = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(label),"dhcp");
	strcpy(wname,"dhcp");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isdhcp);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);
	
	label = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(label),"ppp");
	strcpy(wname,"ppp");
	strcat(wname,iface.name);  
	gtk_widget_set_name(GTK_WIDGET(label),wname);
	gtk_widget_ref(label);
	gtk_object_set_data_full(GTK_OBJECT (table), wname, label,
                            (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),iface.isppp);  
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
                      GTK_SIGNAL_FUNC (changed_nwtype),
                      NULL);
	gtk_container_add(GTK_CONTAINER(container),label);
	  
	  
	// page items  
	create_editable_entry(&iface, ctable, "provider", _("Provider"), iface.provider, 1);
	return ctable;
}


void Network_Free_Objects()
{
	set_file_open(FALSE);	
	gtk_widget_destroy(table);
}


void Network_Save()
{
	gint sect;
	GtkWidget* entry;
	gchar wname[100];
	gchar* newval;
	
	// traverse all...
	for (sect=0;sect<iflen;sect++)
	{
		strcpy(wname,"address");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry) 
		{
			newval=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
			strcpy(iflist[sect].address,newval);
		}	
		strcpy(wname,"netmask");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			newval=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
			strcpy(iflist[sect].netmask,newval);
		}
		strcpy(wname,"network");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			newval=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
			strcpy(iflist[sect].network,newval);
		}
		strcpy(wname,"gateway");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			newval=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
			strcpy(iflist[sect].gateway,newval);
		}
		strcpy(wname,"broadcast");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			newval=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
			strcpy(iflist[sect].broadcast,newval);
		}
		strcpy(wname,"hostname");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			newval=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
			strcpy(iflist[sect].hostname,newval);
		}
		strcpy(wname,"provider");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			newval=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
			strcpy(iflist[sect].provider,newval);
		}
		strcpy(wname,"static");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			iflist[sect].isstatic=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry));
		}
		strcpy(wname,"dhcp");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			iflist[sect].isdhcp=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry));
		}
		strcpy(wname,"ppp");
		strcat(wname,iflist[sect].name);  
		entry = gtk_object_get_data (GTK_OBJECT (table),wname);
		if (entry)
		{
			iflist[sect].isppp=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry));
		}
	}
	
	// save to file
	write_sections();
	set_file_open(FALSE);
	// activate changes
	if (system("/sbin/ifdown -a") < 0)
		gpe_error_box("Unable to activate changes.\n");
	else
		system("/sbin/ifup -a");
}


GtkWidget *Network_Build_Objects()
{  
	GtkWidget *label, *ctable;
	gint row = 0;
	gint num_int = 0;
	guint gpe_border     = gpe_get_border ();
	
	table = gtk_notebook_new();

    gtk_object_set_data (GTK_OBJECT (table), "table", table);
	gtk_widget_set_name(GTK_WIDGET(table),"table");

	gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);

	if (!set_file_open(TRUE))
		gpe_error_box( "Couldn't read network configuration.\n");
	else
	{
		num_int = get_scheme_list();
	}
	
	// add widgets
	for (row=0;row<num_int;row++)
	{
		ctable = NULL;
		if (iflist[row].isstatic) ctable = create_nwstatic_widgets(iflist[row]);
		if (iflist[row].isdhcp) ctable = create_nwdhcp_widgets(iflist[row]);
		if (iflist[row].isppp) ctable = create_nwppp_widgets(iflist[row]);
		if (ctable)	
		{
			label = gtk_label_new(iflist[row].name);
			gtk_notebook_append_page(GTK_NOTEBOOK(table),GTK_WIDGET(ctable),label);
		}
	} 

   return table;
}
