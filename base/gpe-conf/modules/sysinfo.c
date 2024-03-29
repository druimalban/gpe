/*
 * gpe-conf
 *
 * Copyright (C) 2003 - 2005, 2006, 2007, 2010  
 * Florian Boor <florian.boor@kernelconcepts.de>
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
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <sys/ioctl.h> 
	
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "sysinfo.h"
#include "applets.h"
#include "storage.h"
#include "battery.h"
#include "tools/interface.h"
#include "logread.h"
#include "device.h"

/* local definitions */
#define MODEL_INFO 		"/proc/hal/model"
#define FAMILIAR_VINFO 	"/etc/familiar-version"
#define OPENZAURUS_VINFO 	"/etc/openzaurus-version"
#define ANGSTROM_VINFO 	"/etc/angstrom-version"
#define MUCROSS_VINFO 	"/etc/mucross-version"
#define DEBIAN_VINFO 	"/etc/debian_version"
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


typedef struct 
{
	char *model;
	char *manufacturer;
	int ram;
	int flash;
}
t_deviceinfo;


/* local functions */

int
get_flash_size (void)
{
	gchar **strv;
	gsize len = 0;
	GError *err = NULL;
	gint i = 0;
	gchar *str = NULL;
	gint result = 0;
	gint v = 0;
	
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
				sscanf(strv[i],"%*s %x %*x %*s",&v);
				result += v;
			}
			i++;
		}
		g_strfreev(strv);
	}
	else
	{
		g_error_free(err);
		err = NULL;
	}
	
	result /= (1024 * 1024);

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
				if ((strstr(strv[i],"disc") && strstr(strv[i],"ide")))
				{
					sscanf(strv[i],"%*i %*i %i %*s",&v);
					result += v*2;
				}
				i++;
			}
			g_strfreev(strv);
		}
		else
		{
	 		g_error_free(err);
			err = NULL;
		}
		result /= 2048;
	}
	
	return result;
}


t_deviceinfo 
get_device_info(void)
{
	t_deviceinfo result;
	struct utsname uinfo;
	result.model = NULL;
	result.manufacturer = NULL;
	result.ram = 0;
	result.flash = 0;

	uname(&uinfo);
	
	result.model = g_strdup (device_get_name ());
	result.manufacturer = g_strdup (device_get_manufacturer ());
	
	if (!result.model)
		result.model = g_strdup_printf("%s", uinfo.machine);
	
	/* memory and flash size */
	
	system_memory();
	result.ram = meminfo.total;
	result.flash = get_flash_size();
	return result;
}


char *
get_distribution_version()
{
	gchar *result = NULL;
	gchar *tmp = NULL;
	gsize len = 0;
	GError *err = NULL;
	
	/* check for Familiar */
	if (g_file_get_contents(FAMILIAR_VINFO,&tmp,&len,NULL))
	{
		if (strchr(tmp,'\n'))
			strchr(tmp,'\n')[0] = 0;
    	/*TRANSLATORS: "Familiar" is the name of a linux distribution.*/
		result = g_strdup_printf("%s %s", _("Familiar"), g_strstrip(strstr(tmp, " ")));
		g_free(tmp);
	 	return result;
	}
	
	/* check for OpenZaurus */
	if (g_file_get_contents(OPENZAURUS_VINFO,&tmp,&len,NULL))
	{
		if (strchr(tmp,'\n'))
			strchr(tmp,'\n')[0] = 0;
    	/*TRANSLATORS: "OpenZaurus" is the name of a linux distribution.*/
		result = g_strdup_printf("%s %s", _("OpenZaurus"), g_strstrip(strstr(tmp, " ")));
		g_free(tmp);
	 	return result;
	}
	
	/* check for Debian */
	if (g_file_get_contents(DEBIAN_VINFO, &tmp, &len, NULL))
	{
    	/*TRANSLATORS: "Debian" and "Ubuntu" are names of linux distributions.*/
		result = g_strdup_printf("%s %s", _("Debian / Ubuntu"), g_strstrip(tmp));
		g_free(tmp);
	 	return result;
	}
	
	/* check for Angstrom */
	if (g_file_get_contents(ANGSTROM_VINFO, &tmp, &len, NULL))
	{
		if (strchr(tmp,'\n'))
			strchr(tmp,'\n')[0] = 0;
    	/*TRANSLATORS: "Ångström" is the name of a linux distribution.*/
		result = g_strdup_printf("%s %s", _("Ångström"), g_strstrip(strstr(tmp, " ")));
		g_free(tmp);
	 	return result;
	}
	
	/* check for µCross */
	if (g_file_get_contents(MUCROSS_VINFO, &tmp, &len, NULL))
	{
		if (strchr(tmp,'\n'))
			strchr(tmp,'\n')[0] = 0;
    	/*TRANSLATORS: "µCross" is the name of a linux distribution.*/
		result = g_strdup_printf("%s %s", _("µCross"), g_strstrip(strstr(tmp, " ")));
		g_free(tmp);
	 	return result;
	}

	/* check for OpenEmbedded */
	if (g_file_get_contents(OE_VERSION,&tmp,&len,&err))
	{
	   	/*TRANSLATORS: "OpenEmbedded" is the name of a linux distribution.*/
		result = g_strdup_printf(_("OpenEmbedded"));
		g_free(tmp);
	 	return (result);
	}
	else
	{
		g_error_free(err);
		err = NULL;
	}
	
	return result;
}


char *
get_distribution_time()
{
	gchar *result = NULL;
	gchar *tmp = NULL;
	gsize len = 0;
	GError *err = NULL;

	/* Familiar */
	if (g_file_get_contents(FAMILIAR_TIME,&result,&len,&err))
	{
		if (strchr(result,'\n'))
			strchr(result,'\n')[0] = 0;
		return g_strstrip(result);
	}
	else
	{
		g_error_free(err);
		err = NULL;
	}
	
	/* OpenEmbedded */
	if (g_file_get_contents(OE_VERSION,&tmp,&len,&err))
	{
		if (strchr(tmp, '\n'))
			strchr(tmp, '\n')[0] = 0;
		if ((strlen(tmp) >= 10) && g_ascii_isdigit(tmp[0])) /* Openembedded has the date here only */
			result = g_strdup_printf("%s %.4s-%.2s-%.2s\n%.2s:%.2s", 
				_("Build"), &tmp[0], &tmp[4], &tmp[6], &tmp[8], &tmp[10]);
		else
			result = g_strdup_printf("%s: %s", _("Build"), tmp);

		g_free(tmp);
	 	return (result);
	}
	else
	{
		g_error_free(err);
		err = NULL;
	}

	if (!result) 
		result = g_strdup(" ");

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
	GtkWidget *table, *tw, *sw, *vp;
	char *ts, *nwshort, *nwlong;
	int pos = 0;

	int_list = if_getlist ();

	sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), 
	                                    GTK_SHADOW_NONE);
	
	table = gtk_table_new(6,2,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_container_set_border_width(GTK_CONTAINER(table),gpe_get_border());
	vp = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(vp), table);
	gtk_container_add(GTK_CONTAINER(sw),vp);
	
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
			{
				gchar *p = strstr(nwshort,"Ethernet");
				if (p)
				{
					sprintf(p,_("Wireless"));
					p[strlen(_("Wireless"))] = '\n';
				}
				tw = gtk_image_new_from_file(PIC_WLAN); 
			}
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
			gtk_table_attach(GTK_TABLE(table),tw,0,3,3+pos,4+pos,GTK_FILL,
            	GTK_FILL,0,0);
			
			g_free (tmp);
			pos+=3;
		}
	}

	ts = g_strdup_printf("%s: %s",_("Default Gateway"),network_get_gateway());
	tw = gtk_label_new(NULL);
	gtk_label_set_text(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	gtk_table_attach(GTK_TABLE(table),tw,0,3,4+pos,5+pos,GTK_FILL,
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
	gtk_table_attach(GTK_TABLE(table),tw,0,1,1,4,GTK_FILL, GTK_FILL,0,0);
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
	gtk_table_attach(GTK_TABLE(table),tw,1,2,3,4,GTK_FILL, GTK_FILL,0,0);
	
	/* distribution section */
	fv = get_distribution_version();
	ft = get_distribution_time();

	if (fv && ft) /* is a known one? */
	{	
		tw = gtk_label_new(" ");
		gtk_table_attach(GTK_TABLE(table),tw,0,3,4,5,GTK_FILL, GTK_FILL,2,0);
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
		gtk_table_attach(GTK_TABLE(table),tw,1,2,7,8,GTK_FILL | GTK_EXPAND
					     ,GTK_FILL,0,0);
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
	gtk_table_attach(GTK_TABLE(table),tw,0,1,1,4,GTK_FILL,
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
	
	/* CPU */	
	tw = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(tw), _("Manufacturer:"));
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	gtk_table_attach(GTK_TABLE(table),tw,0,1,5,6,GTK_FILL, GTK_FILL,2,0);
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("%s", devinfo.manufacturer ? devinfo.manufacturer : "");
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,3,5,6,GTK_FILL, GTK_FILL,2,0);
	
	/* RAM */
	tw = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
	gtk_label_set_markup(GTK_LABEL(tw),_("RAM:"));
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	gtk_table_attach(GTK_TABLE(table),tw,0,1,6,7,GTK_FILL, GTK_FILL,2,0);

	tw = gtk_label_new(NULL);
	/* TRANSLATORS: MB == Mega Bytes*/
	ts = g_strdup_printf("%i %s",devinfo.ram,_("MB"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,3,6,7,GTK_FILL, GTK_FILL,2,0);
	tw = gtk_label_new(NULL);
	gtk_label_set_line_wrap(GTK_LABEL(tw),TRUE);
	
	/* Storage */
	tw = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(tw),_("Storage:"));
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	gtk_table_attach(GTK_TABLE(table),tw,0,1,7,8,GTK_FILL, GTK_FILL,2,0);
	
	/* TRANSLATORS: MB == Mega Bytes*/
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("%i %s",devinfo.flash,_("MB"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.2);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table),tw,1,3,7,8,GTK_FILL, GTK_FILL,2,0);

	tw = gtk_label_new(_("Hardware"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	gtk_widget_show_all(table);
	gtk_widget_show(tw);
	
	/* important to make pacge change below work */
	gtk_widget_show_all(notebook);
	
	/* battery tab */
	table = Battery_Build_Objects ();
	tw = gtk_label_new(_("Battery"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	gtk_widget_show(table);
	gtk_widget_show(tw);
	
	/* storage tab */
	table = Storage_Build_Objects ();
	tw = gtk_label_new(_("Storage"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	gtk_widget_show_all(table);
	gtk_widget_show(tw);
	
	/* network tab */
	table = network_create_widgets();
	tw = gtk_label_new(_("Networks"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	gtk_widget_show_all(table);
	gtk_widget_show(tw);
	
	/* logfiles tab */
	table = Logread_Build_Objects();
	tw = gtk_label_new(_("Log Display"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,tw);
	gtk_widget_show_all(table);
	gtk_widget_show(tw);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),whichtab);
	
	return notebook;
}
