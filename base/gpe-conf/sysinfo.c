/*
 * gpe-conf
 *
 * Copyright (C) 2003, 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE system information module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#define _(x) gettext(x)
#include <gtk/gtk.h>
#include <errno.h>
#include <fcntl.h>   
#include <sys/types.h>
#include <sys/utsname.h>
#include <net/if.h>
	
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "sysinfo.h"
#include "applets.h"
#include "storage.h"
#include "battery.h"
#include "storage.h"
#include "tools/interface.h"

/* local definitions */
#define MODEL_INFO 		"/proc/hal/model"
#define FAMILIAR_VINFO 	"/etc/familiar-version"
#define FAMILIAR_TIME 	"/etc/familiar-timestamp"
#define PIC_LINUX 		PREFIX "/share/pixmaps/system-info.png"
#define PIC_DEVICE 		PREFIX "/share/pixmaps/device-info.png"
#define PIC_FAMILIAR 	PREFIX "/share/pixmaps/familiar.png"
#define PIC_NET			PREFIX "/share/pixmaps/gpe-config-network.png"
#define P_CPUINFO 		"/proc/cpuinfo"
#define P_IPAQ			"/proc/hal/model"
#define P_PARTITIONS	"/proc/partitions"

/* local types and structs */
typedef enum
{
	M_IPAQ,
	M_SIMPAD,
	M_ZAURUS,
	M_OTHER
}
t_mach;

typedef struct 
{
	t_mach mach;
	char *model;
	char *cpu;
	int ram;
	int flash;
}
t_deviceinfo;


/* local functions */

int
get_flash_size()
{
	char **strv;
	int len = 0;
	GError *err;
	int i = 0;
	char *str = NULL;
	int result = 0;
	int v = 0;
	
	/* read partition info */
	if (g_file_get_contents(P_PARTITIONS,&str,&len,&err))
	{
		strv = g_strsplit(str,"\n",32);
		g_free(str);
		while (strv[i])
		{
			if (strstr(strv[i],"mtdblock"))
			{
				sscanf(strv[i],"%*i %*i %i %*s",&v);
				result += v;
			}
			i++;
		}
		g_strfreev(strv);
	}
	
	result /= 2048;
	
	return result;
}


t_deviceinfo 
get_device_info()
{
	t_deviceinfo result;
	char **strv;
	int len = 0;
	GError *err;
	int i = 0;
	char *str = NULL;
	
	result.mach = M_OTHER;
	result.model = NULL;
	result.cpu = NULL;
	result.ram = 0;
	result.flash = 0;

#ifdef __arm__
	/* check mach type and model */
	if (!access(P_IPAQ,F_OK))
	{
		result.mach = M_IPAQ;
		if (g_file_get_contents(P_IPAQ,&result.model,&len,&err))
			g_strstrip(result.model);
	}
	
	/* get cpu info, only ARM for now */
	if (g_file_get_contents(P_CPUINFO,&str,&len,&err))
	{
		strv = g_strsplit(str,"\n",128);
		g_free(str);
		result.cpu = g_strdup(strchr(strv[0],':')+1);
		g_strstrip(result.cpu);
		while (strv[i])
		{
			if (strstr(strv[i],"Hardware"))
			{
				result.model = g_strdup(strchr(strv[i],':')+1);
				if (strstr(strv[i],"Collie"))
				{
					result.mach = M_ZAURUS;
					g_free(result.model);
					result.model = g_strdup("Sharp Zaurus (Collie)");
				}
				if (strstr(strv[i],"Siemens")) //needs to be verfied
				{
					result.mach = M_SIMPAD;
				}
				break;
			}
			i++;
		}
		g_strfreev(strv);
	}
#endif
	
	/* memory and flash size */
	system_memory();
	result.ram = meminfo.total / 1024 + 1;
	result.flash = get_flash_size();
	return result;
}


char *
get_familiar_version()
{
	char *result = NULL;
	int len = 0;
	GError *err;
	
	if (g_file_get_contents(FAMILIAR_VINFO,&result,&len,&err))
	{
		if (strchr(result,'\n'))
			strchr(result,'\n')[0] = 0;
	}
	return g_strstrip(result);
}


char *
get_familiar_time()
{
	char *result = NULL;
	int len = 0;
	GError *err;
	
	if (g_file_get_contents(FAMILIAR_TIME,&result,&len,&err))
	{
		if (strchr(result,'\n'))
			strchr(result,'\n')[0] = 0;
	}
	return g_strstrip(result);
}

GtkWidget*
network_create_widgets (void)
{
	char *buffer = g_strdup ("");
	char *tmp = NULL;
	struct interface *ife;
	struct interface *int_list;
	GtkWidget *table, *label, *tw;
	char *ts;

	int_list = if_getlist ();

	for (ife = int_list; ife->next; ife = ife->next)
	{
		if ((ife->flags & IFF_UP) && !(ife->flags & IFF_LOOPBACK))
		{
			tmp = if_to_infostr (ife);
			buffer = realloc (buffer,
					  strlen (tmp) + strlen (buffer) + 1);
			buffer[strlen(buffer)] = 0;
			strcat (buffer, tmp);
			free (tmp);
		}
	}

	table = gtk_table_new(6,2,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("Network Status"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,0,1,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,2,2);
	
	tw = gtk_image_new_from_file(PIC_NET);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.0);
	gtk_table_attach(GTK_TABLE(table),tw,0,1,1,2,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,0,0);
/*	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<i>%s</i>",_("Device"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.8);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,2,1,2,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,0,0);
*/					 
	tw = gtk_label_new(NULL);
//!
ts = g_strdup_printf("%s",buffer);
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,2,1,3,GTK_FILL,
                     GTK_FILL,0,0);


/*	vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), buffer);
	gtk_label_set_selectable(GTK_LABEL(label),TRUE);
	gtk_box_pack_start(GTK_BOX (vbox), label, TRUE, TRUE, 0);
*/	
	g_free (buffer);
	return(table);
}


/* gpe-conf interface */

void
Sysinfo_Free_Objects ()
{
}


GtkWidget *
Sysinfo_Build_Objects (void)
{
	GtkWidget *notebook;
	GtkWidget *tw, *table;
	gchar *ts = NULL;
	gchar *fv,*ft;
	struct utsname uinfo;
	t_deviceinfo devinfo = get_device_info();
	
	uname(&uinfo);
	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook),TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (notebook),
					0 /*gpe_get_border ()*/);
	
	/* globals tab */
	
	table = gtk_table_new(6,2,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("System information"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,0,1,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,2,2);
	// os section 
	tw = gtk_image_new_from_file(PIC_LINUX);
	gtk_table_attach(GTK_TABLE(table),tw,0,1,1,4,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,0,0);
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<i>%s %s</i>",uinfo.sysname,_("Version"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.8);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,2,1,2,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,0,0);
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("%s",uinfo.release);
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,2,2,3,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,0,0);
	tw = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
	ts = g_strdup_printf("%s",uinfo.version);
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,2,3,4,GTK_FILL,
                     GTK_FILL,0,0);
	
	// familiar section
	fv = get_familiar_version();
	ft = get_familiar_time();
	if (fv && ft) /* is familiar? */
	{	
		tw = gtk_image_new_from_file(PIC_FAMILIAR);
		gtk_table_attach(GTK_TABLE(table),tw,0,1,4,7,GTK_FILL | GTK_EXPAND,
						 GTK_FILL,0,0);
		tw = gtk_label_new(NULL);
		ts = g_strdup_printf("<i>%s</i>",_("Familiar Version"));
		gtk_label_set_markup(GTK_LABEL(tw),ts);
		gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.8);
		g_free(ts);
		gtk_table_attach(GTK_TABLE(table),tw,1,2,4,5,GTK_FILL | GTK_EXPAND,
						 GTK_FILL,0,0);
		tw = gtk_label_new(NULL);
		ts = g_strdup_printf("%s",fv);
		gtk_label_set_markup(GTK_LABEL(tw),ts);
		gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
		g_free(ts);
		gtk_table_attach(GTK_TABLE(table),tw,1,2,5,6,GTK_FILL | GTK_EXPAND,
						 GTK_FILL,0,0);
		tw = gtk_label_new(NULL);
		gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
		ts = g_strdup_printf("%s",ft);
		gtk_label_set_markup(GTK_LABEL(tw),ts);
		gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
		g_free(ts);
		gtk_table_attach(GTK_TABLE(table),tw,1,2,6,7,GTK_FILL,
						 GTK_FILL,0,0);
		g_free(fv);
		g_free(ft);
	}
	
	tw = gtk_label_new(_("Global"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);

	/* device tab */
	table = gtk_table_new(6,2,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("Device information"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,0,1,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,2,2);
	
	// hardware section
	tw = gtk_image_new_from_file(PIC_DEVICE);
	gtk_table_attach(GTK_TABLE(table),tw,0,1,1,4,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,0,0);
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<i>%s</i>",_("Device"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.8);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,2,1,2,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,0,0);
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("%s",devinfo.model);
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,2,2,3,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,0,0);
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("%s:\t%s",_("CPU"),devinfo.cpu);
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,4,5,GTK_FILL,
                     GTK_FILL,2,0);
	tw = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
	/* TRANSLATORS: MB == Mega Bytes*/
	ts = g_strdup_printf("%s:\t%i %s",_("RAM"),devinfo.ram,_("MB"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,5,6,GTK_FILL,
                     GTK_FILL,2,0);
	tw = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
	/* TRANSLATORS: MB == Mega Bytes*/
	ts = g_strdup_printf("%s:\t%i %s",_("Flash"),devinfo.flash,_("MB"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,6,7,GTK_FILL,
                     GTK_FILL,2,0);

	tw = gtk_label_new(_("Hardware"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	
	/* battery tab */
	table = Battery_Build_Objects ();
	tw = gtk_label_new(_("Battery"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	
	/* storage tab */
	table = Storage_Build_Objects ();
	tw = gtk_label_new(_("Storage"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	
	/* network tab */
	table = network_create_widgets();
	tw = gtk_label_new(_("Network"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),0);
	gtk_widget_show_all(notebook);
	
	return notebook;
}
