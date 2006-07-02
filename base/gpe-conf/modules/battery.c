/*
 * gpe-conf
 *
 * Copyright (C) 2003 - 2006  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE battery info and settings module.
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
#include <sys/ioctl.h> 
#include <sys/time.h>
#include <sys/types.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "battery.h"
#include "storage.h"
#include "applets.h"
#include "suid.h"
#include "parser.h"
#include "device.h"


#define PROC_BATTERY "/proc/asic/battery"
#define HAL_BATTERY "/proc/hal/battery"
#define PROC_APM "/proc/apm"

/* local types and structs */

typedef struct
{
	GtkWidget *bar;
	GtkWidget *label, *lchem, *lstate, *lvoltage, *llifetime;
}
t_infowidgets;

typedef struct {
        unsigned int  chemistry;
        unsigned int  status;
        unsigned int  voltage;    /* Voltage for battery #0; unknown for battery #1 */
        unsigned int  percentage; /* Percentage of full charge */
        unsigned int  life;       /* Life remaining in minutes */
}t_battery_data;

/* local definitions */

gchar *bat_chemistries[] = {
	"",
	"Alkaline",
	"NiCd",
	"NiMh",
	"Lithium Ion",
	"Lithium Polymer",
	"not installed"
};


static GtkWidget *vbox;
static t_infowidgets batt_int;
static t_infowidgets batt_ext;
static DeviceID_t device_id = DEV_UNKNOWN;

static FILE *file_apm = NULL;

/* local functions */

/* 
 * Read battery information from iPaq HAL, batteries needs to 
 * have a minimum length of two - we have two batteries in iPaq
 * sometimes.
 */
static int
read_ipaq_batteries(t_battery_data *batteries)
{
	int num_batteries = 0;
	gchar *batraw = NULL;
	gchar **batinfo;
	int pos = 0;
	GError *err = NULL;
	
	if (!g_file_get_contents(HAL_BATTERY, &batraw, NULL, &err))
		return 0;
	batinfo = g_strsplit(batraw, "\n", 50);
	g_free(batraw);
	
	while (batinfo[pos])
	{
		if (strstr(batinfo[pos], "Battery"))
		{
			memset(&batteries[num_batteries], 0, sizeof(t_battery_data));
			num_batteries++;
		}
		else
		{
			sscanf(batinfo[pos], " Chemistry  : 0x%02x", 
			       &batteries[num_batteries-1].chemistry);
			sscanf(batinfo[pos], " Status     : 0x%x", 
			       &batteries[num_batteries-1].status);
			sscanf(batinfo[pos], " Voltage    : 0x%x", 
			       &batteries[num_batteries-1].voltage);
			sscanf(batinfo[pos], " Percentage : 0x%x",
                   &batteries[num_batteries-1].percentage);
			sscanf(batinfo[pos], " Life (min) : %i", 
			       &batteries[num_batteries-1].life);
		}
		pos++;
	}
	g_strfreev(batinfo);
	return num_batteries;
}

void 
init_device(void)
{
	device_id = device_get_id();
	
	/* then check if we have apm support available */
	if (!access(PROC_APM, R_OK))
		file_apm = fopen(PROC_APM, "r");
}


static gint 
update_bat_values(gpointer data)
{
	gchar percstr[32];
	gchar tmp[128];
	t_battery_data battery_val[2];
	int num_batteries;
	float bperc;
	gboolean islearning = FALSE;

	/* check for ipaq device and read specific data if possible */
	if (device_id == DEV_IPAQ_SA || device_id == DEV_IPAQ_PXA)
	{
	
		parse_file(PROC_BATTERY," Is learning? %*s %s",tmp);
		if (strstr(tmp, "yes"))
			islearning = TRUE;
		num_batteries = read_ipaq_batteries(battery_val);
		if (num_batteries)
		{
			if (num_batteries < 2) {
				gtk_widget_hide(batt_ext.bar);
				gtk_widget_hide(batt_ext.label);
				gtk_widget_hide(batt_ext.lstate);
				gtk_widget_hide(batt_ext.lchem);
				gtk_widget_hide(batt_ext.lvoltage);
				gtk_widget_hide(batt_ext.llifetime);
			}
			else
			{
				gtk_widget_show(batt_ext.bar);
				gtk_widget_show(batt_ext.label);
				gtk_widget_show(batt_ext.lstate);
				gtk_widget_show(batt_ext.lchem);
				gtk_widget_show(batt_ext.lvoltage);
				gtk_widget_show(batt_ext.llifetime);
			}
			
			bperc=(float)battery_val[0].percentage / (float)100;
			if (bperc > 1.0)
				bperc = 1.0;
			
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (batt_int.bar),bperc);
			toolbar_set_style (batt_int.bar,  90 - (int) (bperc*100));
			if (battery_val[0].percentage <= 100)
				sprintf(percstr,"%d %%",(unsigned char)battery_val[0].percentage);
			else
				snprintf(percstr, 32, "%s",_("unknown"));
				
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(batt_int.bar),percstr);
		
			if ((battery_val[0].chemistry > 0) && (battery_val[0].chemistry <= 0x05))
				gtk_label_set_text(GTK_LABEL(batt_int.lchem),bat_chemistries[battery_val[0].chemistry]);
		
			switch (battery_val[0].status) {
				case H3600_BATT_STATUS_HIGH:
					sprintf(tmp,"%s",_("Status: high"));
					break;
				case H3600_BATT_STATUS_LOW:
					sprintf(tmp,"%s",_("Status: low"));
					break;
				case H3600_BATT_STATUS_CRITICAL:
					sprintf(tmp,"%s",_("Status: critical"));
					break;
				case H3600_BATT_STATUS_CHARGING:
					sprintf(tmp,"%s",_("Status: charging"));
					break;
				case H3600_BATT_STATUS_CHARGE_MAIN:
					sprintf(tmp,"%s",_("Status: charge main"));
					break;
				case H3600_BATT_STATUS_DEAD:
					sprintf(tmp,"%s",_("Status: dead"));
					break;
				case H3600_BATT_STATUS_FULL:
					sprintf(tmp,"%s",_("Status: full"));
					break;
				case H3600_BATT_STATUS_NOBATT:
					sprintf(tmp,"%s",_("Status: no battery"));
					break;
				default:
					sprintf(tmp,"%s",_("Status: unknown"));
					break;
			};
		
			if (islearning)
				sprintf(tmp+strlen(tmp)," (%s)",_("learning"));
			
			gtk_label_set_text(GTK_LABEL(batt_int.lstate),tmp);
		
			sprintf(tmp,"%s: %d mV",_("Voltage"),battery_val[0].voltage * 5000 / 1024);
			gtk_label_set_text(GTK_LABEL(batt_int.lvoltage),tmp);
		
			if (battery_val[0].life == 65535)
				sprintf(tmp,"%s",_("AC connected"));
			else
				sprintf(tmp,"%s: %d min.",_("Lifetime"),battery_val[0].life);
			
			gtk_label_set_text(GTK_LABEL(batt_int.llifetime),tmp);
		
			if (num_batteries > 1) {
				bperc=(float)battery_val[1].percentage / (float)100;
				if (bperc > 1.0)
					bperc = 1.0;
				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (batt_ext.bar), bperc);
				toolbar_set_style (batt_ext.bar,  90 - (int) (bperc*100));
				if (battery_val[1].percentage <= 100)
					sprintf(percstr,"%d %%",(unsigned char)battery_val[1].percentage);
				else
					snprintf(percstr, 32, "%s",_("unknown"));
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(batt_ext.bar),percstr);
			
				if ((battery_val[1].chemistry > 0) && (battery_val[1].chemistry <= 0x05))
					gtk_label_set_text(GTK_LABEL(batt_ext.lchem),bat_chemistries[battery_val[1].chemistry]);
			
				switch (battery_val[1].status) {
				case H3600_BATT_STATUS_HIGH:
						gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: high"));
						break;
					case H3600_BATT_STATUS_LOW:
						gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: low"));
						break;
					case H3600_BATT_STATUS_CRITICAL:
						gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: critical"));
						break;
					case H3600_BATT_STATUS_CHARGING:
						gtk_label_set_text(GTK_LABEL(batt_int.lstate),_("Status: charging"));
						break;
					case H3600_BATT_STATUS_CHARGE_MAIN:
						gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: charge main"));
						break;
					case H3600_BATT_NOT_INSTALLED:
						gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: not installed"));
						break;
					case H3600_BATT_STATUS_FULL:
						gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: full"));
						break;
					case H3600_BATT_STATUS_NOBATT:
						gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: no battery"));
						break;
					default:
						gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: unknown"));
						break;
				};
		
			sprintf(tmp,"%s: %d mV",_("Voltage"),battery_val[1].voltage * 5000 / 1024);
			gtk_label_set_text(GTK_LABEL(batt_ext.lvoltage),tmp);
		
			if (battery_val[1].life == 65535)
				sprintf(tmp,"%s",_("AC connected"));
			else
				sprintf(tmp,"%s: %d min.",_("Lifetime"),battery_val[1].life);
			gtk_label_set_text(GTK_LABEL(batt_ext.llifetime),tmp);
				
			} else {
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(batt_ext.bar),0);
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(batt_ext.bar),"");
			
				gtk_label_set_text(GTK_LABEL(batt_ext.lchem),_("not installed"));
				gtk_label_set_text(GTK_LABEL(batt_ext.lstate),_("Status: unknown"));
				gtk_label_set_text(GTK_LABEL(batt_ext.lvoltage),_("Voltage: unknown"));
				gtk_label_set_text(GTK_LABEL(batt_ext.llifetime),_("Lifetime: unknown"));
			}
			return TRUE;
		}
	} /* ipaq device */
	
	/* if not an ipaq use apm to get data */
	if (file_apm)
	{
		int percent;
		int remaining; 
		int status; 
		int flags; 
		int ac_connected;
		char unit[5];
		int barstate = 0;
		
		/* apm knows only about one battery */
		gtk_widget_hide(batt_ext.bar);
		gtk_widget_hide(batt_ext.label);
		gtk_widget_hide(batt_ext.lstate);
		gtk_widget_hide(batt_ext.lchem);
		gtk_widget_hide(batt_ext.lvoltage);
		gtk_widget_hide(batt_ext.llifetime);
		
		/* read data from proc entry */
		fclose(file_apm);
		file_apm = fopen(PROC_APM, "r");
		if (fscanf(file_apm,
			       "%*s %*s %*s 0x%x 0x%x 0x%x %i%% %i %4s",
		           &ac_connected,
		           &status,
		           &flags,
		           &percent, 
		           &remaining,
		           unit) >= 4)
		{
			if (ac_connected > 1)
				ac_connected = 0;
			if (strstr(unit,"sec"))
				remaining /= 60;
			else if (strstr(unit,"hou"))
				remaining *= 60;
						
			/* apm says: */
			if ((flags & 0x80) && (flags != 0xFF))
			{
				sprintf(tmp,"%s",_("No battery"));
			}
			else
			{
				switch (status)
				{
					case 0x00:
						if (ac_connected)
							sprintf(tmp,"%s",_("Status: full"));
						else
							sprintf(tmp,"%s",_("Status: high"));
						barstate = 0;
					break;
					case 0x01:
						sprintf(tmp,"%s",_("Status: low"));
						barstate = 85;
					break;
					case 0x02:
						sprintf(tmp,"%s",_("Status: critical"));
					    barstate = 99;
					break;
					case 0x03:
						sprintf(tmp,"%s",_("Status: charging"));
					break;
					case 0xFF:
						sprintf(tmp,"%s",_("Status: unknown"));
					break;
				}
			}
		
            if (percent > 100) percent=100;
            if (percent < 0 )  percent=0;

			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (batt_int.bar),
			                               (float)percent/100.0);
			toolbar_set_style (batt_int.bar, barstate);
			sprintf(percstr,"%d %%",percent);
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(batt_int.bar),percstr);
			
			gtk_label_set_text(GTK_LABEL(batt_int.lstate),tmp);
			
			if (ac_connected)
				sprintf(tmp,"%s",_("AC connected"));
			else if (remaining > 0)
			     	sprintf(tmp,"%s: %d min.",_("Lifetime"), remaining);
            	 else 
            		tmp[0] = 0;
			gtk_label_set_text(GTK_LABEL(batt_int.llifetime),tmp);
		}
	}
	else /* nothing we know available: hide widgets */
	{
		gtk_widget_hide(batt_ext.bar);
		gtk_widget_hide(batt_ext.label);
		gtk_widget_hide(batt_ext.lstate);
		gtk_widget_hide(batt_ext.lchem);
		gtk_widget_hide(batt_ext.lvoltage);
		gtk_widget_hide(batt_ext.llifetime);
	}

	return TRUE;
}


void
Battery_Free_Objects (void)
{
	if (file_apm) 
		fclose(file_apm);
}

void
Battery_Restore ()
{
	return;
}

GtkWidget *
Battery_Build_Objects (void)
{
	static GtkWidget *label1;
	gchar *fstr = NULL;
	static GtkWidget *bar_batt_int;
	t_infowidgets batt;
	int i;
	GtkWidget *viewport = gtk_viewport_new (NULL, NULL);
	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (sw),viewport);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);

	vbox = gtk_vbox_new (FALSE, 1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox),
					gpe_get_border ());
	gtk_container_add (GTK_CONTAINER (viewport), vbox);
	
	init_device();
	
	for (i=0; i<2; i++)
	{
		if (i==0) 
			fstr = g_strdup_printf ("%s%s %s",
					"<b><span foreground=\"black\">",
					_("Internal battery"), "</span></b>");
		else
			fstr = g_strdup_printf ("%s%s %s",
					"<b><span foreground=\"black\">",
					_("Jacket battery"), "</span></b>");
		label1 = gtk_label_new (NULL);
		batt.label = label1;
		gtk_label_set_markup (GTK_LABEL (label1), fstr);
		g_free (fstr);
		gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
		gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);

		label1 = gtk_label_new (NULL);
		gtk_misc_set_alignment (GTK_MISC (label1), 0.0, 0.5);
		batt.lchem = label1;
		gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);
		
		bar_batt_int = gtk_progress_bar_new ();
		batt.bar = bar_batt_int;
		gtk_widget_set_sensitive (bar_batt_int, TRUE);
		gtk_box_pack_start (GTK_BOX (vbox), bar_batt_int, FALSE, FALSE, 0);
		label1 = gtk_label_new (NULL);
		gtk_misc_set_alignment (GTK_MISC (label1), 0.0, 0.5);
		batt.lstate = label1;
		gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);
		
		label1 = gtk_label_new (NULL);
		batt.llifetime = label1;
		gtk_misc_set_alignment (GTK_MISC (label1), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);
		
		label1 = gtk_label_new (NULL);
		batt.lvoltage = label1;
		gtk_misc_set_alignment (GTK_MISC (label1), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);
			
		if (i==0)
		{			
			batt_int = batt;
			label1 = gtk_label_new (NULL);
			gtk_box_pack_start (GTK_BOX (vbox), label1, FALSE, FALSE, 0);
		}
		else
			batt_ext = batt;
	}
	gtk_widget_show_all(sw);
	
	/* they will be activated if necessary */
	gtk_widget_hide(batt_ext.bar);
	gtk_widget_hide(batt_ext.label);
	gtk_widget_hide(batt_ext.lstate);
	gtk_widget_hide(batt_ext.lchem);
	gtk_widget_hide(batt_ext.lvoltage);
	gtk_widget_hide(batt_ext.llifetime);
	
	update_bat_values(NULL);
	
	gtk_timeout_add (4000, (GtkFunction) update_bat_values, NULL);
	return sw;
}
