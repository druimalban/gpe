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
 * WLAN detection code from prismstumbler source
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
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

#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/ioctl.h> 
	
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "sysinfo.h"
#include "applets.h"
#include "storage.h"
#include "battery.h"
#include "tools/interface.h"
#include "logread.h"

/* local definitions */
#define MODEL_INFO 		"/proc/hal/model"
#define FAMILIAR_VINFO 	"/etc/familiar-version"
#define FAMILIAR_TIME 	"/etc/familiar-timestamp"
#define OE_VERSION 		"/etc/version"
#define PIC_LINUX 		PREFIX "/share/pixmaps/system-info.png"
#define PIC_DEVICE 		PREFIX "/share/pixmaps/device-info.png"
#define PIC_DISTRI		PREFIX "/share/pixmaps/distribution.png"
#define PIC_NET			PREFIX "/share/pixmaps/gpe-config-network.png"
#define PIC_WLAN 		PREFIX "/share/pixmaps/pccard-network.png"
#define P_CPUINFO 		"/proc/cpuinfo"
#define P_IPAQ			"/proc/hal/model"
#define P_PARTITIONS	"/proc/partitions"
#define P_ROUTE			"/proc/net/route"
#define P_MTD			"/proc/mtd"

#define strpos(a,b) (strstr(a,b)-a)

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
	GError *err = NULL;
	int i = 0;
	char *str = NULL;
	int result = 0;
	int v = 0;
	
	/* read mtd info */
	i=0;
	if (g_file_get_contents(P_MTD,&str,&len,&err))
	{
		strv = g_strsplit(str,"\n",32);
		g_free(str);
		while (strv[i])
		{
			/* check mtd partitions, masq mtd ram stuff */
			if (strstr(strv[i],"mtd") && !strstr(strv[i],"ram")) 
			{
				sscanf(strv[i],"%*s %0x %*0x %*s",&v);
				result += v;
			}
			i++;
		}
		g_strfreev(strv);
	}
	result /= (1024 * 1024);
	result++;

	/* nothing in mtd, try partitions */
	if (result == 0)
	{
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
	}
	
	return result;
}


t_deviceinfo 
get_device_info()
{
	t_deviceinfo result;
	char **strv;
	int len = 0;
	GError *err = NULL;
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
				g_strstrip(result.model);
				if (strstr(strv[i],"Collie"))
				{
					result.mach = M_ZAURUS;
					g_free(result.model);
					result.model = g_strdup("Sharp Zaurus (Collie)");
				}
				if (strstr(strv[i],"Husky"))
				{
					result.mach = M_ZAURUS;
					g_free(result.model);
					result.model = g_strdup("Sharp Zaurus (Husky)");
				}
				if (strstr(strv[i],"Poodle"))
				{
					result.mach = M_ZAURUS;
					g_free(result.model);
					result.model = g_strdup("Sharp Zaurus (Poodle)");
				}
				if (strstr(strv[i],"Shepherd"))
				{
					result.mach = M_ZAURUS;
					g_free(result.model);
					result.model = g_strdup("Sharp Zaurus (Shepherd)");
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
#else
#ifdef __arm__
	result.cpu = g_strdup(_("ARM"));
#endif
#ifdef __i386__
	result.cpu = g_strdup(_("Intel x86 or compatible"));
#endif
#ifdef __mips__
	result.cpu = g_strdup(_("Mips"));
	#ifdef __sgi__
		result.model = g_strdup(_("Silicon Graphics Machine"));
	#endif
#endif
#ifdef _POWER
	result.cpu = g_strdup(_("IBM Power or PowerPC"));
#endif
	if (!result.model)
		result.model = g_strdup(MACHINE);
#endif
	
	/* memory and flash size */
	system_memory();
	result.ram = meminfo.total / 1024 + 1;
	result.flash = get_flash_size();
	return result;
}


char *
get_distribution_version()
{
	char *result = NULL;
	char *tmp = NULL;
	int len = 0;
	GError *err = NULL;
	
	/* check for Familiar */
	if (g_file_get_contents(FAMILIAR_VINFO,&tmp,&len,&err))
	{
		if (strchr(tmp,'\n'))
			strchr(tmp,'\n')[0] = 0;
    	/*TRANSLATORS: "Familiar" is the name of a linux distribution.*/
		result = g_strdup_printf("%s %s", _("Familiar"), g_strstrip(tmp));
		g_free(tmp);
	 	return result;
	}
	
	/* check for OpenEmbedded */
	if (g_file_get_contents(OE_VERSION,&tmp,&len,&err))
	{
	   	/*TRANSLATORS: "Familiar" is the name of a linux distribution.*/
		result = g_strdup_printf(_("OpenEmbedded"));
		g_free(tmp);
	 	return (result);
	}
	
	return result;
}


char *
get_distribution_time()
{
	char *result = NULL;
	char *tmp = NULL;
	int len = 0;
	GError *err = NULL;
	
	/* Familiar */
	if (g_file_get_contents(FAMILIAR_TIME,&result,&len,&err))
	{
		if (strchr(result,'\n'))
			strchr(result,'\n')[0] = 0;
		return g_strstrip(result);
	}
	
	/* OpenEmbedded */
	if (g_file_get_contents(OE_VERSION,&tmp,&len,&err))
	{
		if (strlen(tmp) >= 10)
			result = g_strdup_printf("%s %4s-%2s-%2s %2s:%2s", 
				_("Build"), &tmp[0], &tmp[4], &tmp[6], &tmp[8], &tmp[10]);
		else
			result = g_strdup_printf("%s %s",_("Build"),tmp);
		g_free(tmp);
	 	return (result);
	}
	
	return result;
}


gboolean 
device_is_wlan(char* ifname)
{
	int fd;
	struct iwreq wrq;
	gboolean result;

	fd = socket (AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	return 0;

	memset (&wrq, 0, sizeof (wrq));
	strncpy (wrq.ifr_name, ifname, IFNAMSIZ);
	if (ioctl (fd, SIOCGIWNAME, &wrq) < 0)
		result = FALSE;
	else
		result = TRUE;
	close (fd);

	return result;
}


char* 
network_get_gateway()
{
	FILE *froute;
	int i = 0;
	unsigned long long gw = 0;
	unsigned long long dest = 1;	
	char *result;
	unsigned char a,b,c,d;

	froute = fopen(P_ROUTE,"r");
		if (!froute) return NULL;
	
	while (i != EOF)
	{
		i = fscanf(froute,"%*s %llX %llX %*i %*i %*i %*i %*s %*i %*i %*i\n", &dest, &gw);
		if (i > 1)
		{
			if (dest == 0)
				break;
		}
	}
	if (gw)
	{
		d = (gw & 0xFF000000)/0xFFFFFF;
		c = (gw & 0x00FF0000)/0xFFFF;
		b = (gw & 0x0000FF00)/0xFF;
		a = gw & 0x000000FF;
		result = g_strdup_printf("%i.%i.%i.%i",a,b,c,d);
	}
	else
		result = g_strdup("<none>");
	fclose(froute);
	return result;
}


GtkWidget*
network_create_widgets (void)
{
	char *tmp = NULL;
	struct interface *ife;
	struct interface *int_list;
	GtkWidget *table, *tw, *sw;
	char *ts, *nwshort, *nwlong;
	int pos = 0;

	int_list = if_getlist ();

	sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	
	table = gtk_table_new(6,2,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_container_set_border_width(GTK_CONTAINER(table),gpe_get_border());
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(sw),table);
	
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("Network Status"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,3,0,1,GTK_FILL | GTK_EXPAND,
                     GTK_FILL,2,2);
	
	for (ife = int_list; ife; ife = ife->next)
	{
		if ((ife->flags & IFF_UP) && !(ife->flags & IFF_LOOPBACK))
		{
			tmp = if_to_infostr (ife);

			/* split string into long and short lines */
			nwlong = strstr(tmp,_("HWaddr (MAC)"));
			if (nwlong != NULL) 
			{
				tmp[strpos(tmp,_("HWaddr (MAC)"))-1] = 0;
				nwlong[strlen(nwlong)-2] = 0;
			}
			nwshort = tmp;

			/* create widgets */
			if (device_is_wlan(ife->name))
				tw = gtk_image_new_from_file(PIC_WLAN); 
			else
				tw = gtk_image_new_from_file(PIC_NET);
			gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.0);
			gtk_table_attach(GTK_TABLE(table),tw,0,1,1+pos,2+pos,
				GTK_FILL , GTK_FILL,0,0);

			tw = gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(tw),nwshort);
			gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
			gtk_table_attach(GTK_TABLE(table),tw,1,2,1+pos,3+pos,
				GTK_FILL , GTK_FILL,0,0);
			
			tw = gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(tw),nwlong);
			gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
			gtk_table_attach(GTK_TABLE(table),tw,0,2,3+pos,4+pos,GTK_FILL,
            	GTK_FILL,0,0);
			
			g_free (tmp);
			pos+=3;
		}
	}

	ts = g_strdup_printf("%s: %s",_("Default Gateway"),network_get_gateway());
	tw = gtk_label_new(NULL);
	gtk_label_set_text(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,4+pos,5+pos,GTK_FILL,
          	GTK_FILL,0,0);
	g_free(ts);
	
	return(sw);
}


/* gpe-conf interface */

void
Sysinfo_Free_Objects ()
{
	gtk_main_quit();
}


GtkWidget *
Sysinfo_Build_Objects (int whichtab)
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
	gtk_container_set_border_width (GTK_CONTAINER (notebook), 0 );
	
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
	
	/* distribution section */
	fv = get_distribution_version();
	ft = get_distribution_time();
	if (fv && ft) /* is a known one? */
	{	
		tw = gtk_label_new(" ");
		gtk_table_attach(GTK_TABLE(table),tw,0,3,4,5,GTK_FILL,
                     GTK_FILL,2,0);
		tw = gtk_image_new_from_file(PIC_DISTRI);
		gtk_table_attach(GTK_TABLE(table),tw,0,1,5,8,GTK_FILL | GTK_EXPAND,
						 GTK_FILL,0,0);
		tw = gtk_label_new(NULL);
		ts = g_strdup_printf("<i>%s</i>",_("Distribution"));
		gtk_label_set_markup(GTK_LABEL(tw),ts);
		gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.8);
		g_free(ts);
		gtk_table_attach(GTK_TABLE(table),tw,1,2,5,6,GTK_FILL | GTK_EXPAND,
						 GTK_FILL,0,0);
		tw = gtk_label_new(NULL);
		ts = g_strdup_printf("%s",fv);
		gtk_label_set_markup(GTK_LABEL(tw),ts);
		gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
		g_free(ts);
		gtk_table_attach(GTK_TABLE(table),tw,1,2,6,7,GTK_FILL | GTK_EXPAND,
						 GTK_FILL,0,0);
		tw = gtk_label_new(NULL);
		gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
		ts = g_strdup_printf("%s",ft);
		gtk_label_set_markup(GTK_LABEL(tw),ts);
		gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
		g_free(ts);
		gtk_table_attach(GTK_TABLE(table),tw,1,2,7,8,GTK_FILL,
						 GTK_FILL,0,0);
		g_free(fv);
		g_free(ft);
	}
	
	tw = gtk_label_new(_("System"));
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
	ts = g_strdup_printf("<i>%s</i>",_("Device Name"));
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
	tw = gtk_label_new(" ");
	gtk_table_attach(GTK_TABLE(table),tw,0,2,4,5,GTK_FILL,
                     GTK_FILL,2,0);
					 
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("%s:\t%s",_("CPU"),devinfo.cpu);
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,5,6,GTK_FILL,
                     GTK_FILL,2,0);
	tw = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
	/* TRANSLATORS: MB == Mega Bytes*/
	ts = g_strdup_printf("%s:\t%i %s",_("RAM"),devinfo.ram,_("MB"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,6,7,GTK_FILL,
                     GTK_FILL,2,0);
	tw = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
	/* TRANSLATORS: MB == Mega Bytes*/
	ts = g_strdup_printf("%s:\t%i %s",_("Flash"),devinfo.flash,_("MB"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,0,2,7,8,GTK_FILL,
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
	tw = gtk_label_new(_("Networks"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	
	/* logfiles tab */
	table = Logread_Build_Objects();
	tw = gtk_label_new(_("Log Display"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	
	gtk_widget_show_all(notebook);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),whichtab);
	
	return notebook;
}
