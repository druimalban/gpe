/*
 * gpe-conf
 *
 * Copyright (C) 2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE serial port configuration module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "serial.h"
#include "applets.h"
#include "suid.h"
#include "parser.h"


/* --- local types and constants --- */

#define GPE_SERIAL_CONF_DIR "/etc/gpe/gpe-conf-serial"
#define IPAQ_SERIAL "/dev/ttySA0"
#define GPSD_STARTUP_SCRIPT "/etc/init.d/gpsd"
#define INITTAB "/etc/inittab"

#define PARAM_BOOL 0
#define PARAM_INT 1
#define PARAM_CHAR 2
#define PARAM_FILE 3

typedef struct 
{
	gint type;
	gchar *var;
	gchar *valstr;
	gint valint;
	GtkWidget *label;
	GtkWidget *holder;	
}
t_param;

typedef struct 
{
	gchar *name;
	gchar *title;
	gint enabled;
	gchar *configfile;
	gint paramcount; 
	t_param *params; 	
}
t_serial_item;

/* --- module global variables --- */

static GtkWidget *notebook;
//static t_serial_item *serial_items;
//static gint num_items;

static struct 
{
	GtkWidget *rbConsole;	
	GtkWidget *rbGPSD;	
	GtkWidget *rbFree;	
}self;


/* --- local intelligence --- */

void assign_serial_port(t_serial_assignment type)
{
	int gpsd_installed = !access(GPSD_STARTUP_SCRIPT,F_OK);
	
	switch (type)
	{
		case SA_NONE:
			if (gpsd_installed)
			{	
				if (!access(GPSD_STARTUP_SCRIPT,X_OK)) 
				{	
					system(GPSD_STARTUP_SCRIPT " stop");
					system("killall gpsd.bin");
				}					
				system("chmod a-x " GPSD_STARTUP_SCRIPT);
			}
			/* we move getty to higher runlevels */
			change_cfg_value(INITTAB,"T0","34:respawn:/sbin/getty -L ttyC0 115200 vt100",':');
			system("telinit 3");
			system("telinit 2");
		break;
		case SA_GPSD:
			if (gpsd_installed) 
			{
				system ("chmod a+x " GPSD_STARTUP_SCRIPT);
				system(GPSD_STARTUP_SCRIPT " stop");
				system("killall gpsd.bin"); // due to a bug in gpsd initscript
				system(GPSD_STARTUP_SCRIPT " start");
			}					
			change_cfg_value(INITTAB,"T0","34:respawn:/sbin/getty -L ttyC0 115200 vt100",':');
			system("telinit 3");
			system("telinit 2");
		break;
		case SA_CONSOLE:
			if (gpsd_installed) 
			{
				if (!access(GPSD_STARTUP_SCRIPT,X_OK)) 
				{	
					system(GPSD_STARTUP_SCRIPT " stop");
					system("killall gpsd.bin");
				}					
				system ("chmod a-x " GPSD_STARTUP_SCRIPT);
			}
			change_cfg_value(INITTAB,"T0","23:respawn:/sbin/getty -L ttyC0 115200 vt100",':');
			system("telinit 3");
			system("telinit 2");
		break;
	}		
}

int get_serial_port_assignment()
{
	int i;
	
	if (parse_pipe("cat " INITTAB ,"T0:%d",&i))
	{
		gpe_error_box(_("Couldn't find default runlevel!"));
	}
	
	/* familiar runs getty on ttySA0 in runlevel 2 and 3 */ 
	if (i==23) return SA_CONSOLE;
	
	/* if not, gpsd strtup might be active */
	if (!access(GPSD_STARTUP_SCRIPT,X_OK)) return SA_GPSD;
	
	return SA_NONE;
}

gint serial_get_items(gchar *file)
{
/*  gchar *content, *tmpval;
  gchar **lines = NULL;
  const gchar seperator = '='; 
  gint length;
  gchar *delim;
  gint i = 0;
  gint j = 0;
  GError *err = NULL;

  tmpval = "";
  delim = g_strdup ("\n");
  if (!g_file_get_contents (file, &content, &length, &err))
  {
	  fprintf(stderr,"Could not access file: %s.\n",file);
	  if (access(file,F_OK)) // file exists, but access is denied
	  {
		  i = 0;
		  delim = NULL;
		  return 0;
	  }
  }
  lines = g_strsplit (content, delim, 1024);
  g_free (delim);
  delim = NULL;
  g_free (content);

  while (lines[i])
    {
      if ((g_strrstr (g_strchomp (lines[i]), var))
	  && (!g_str_has_prefix (g_strchomp (lines[i]), "#")))
	{
	  delim = lines[i];
	  j=get_first_char(delim);
	  if (j>0) {
		  tmpval = g_malloc(j);
		  strncpy(tmpval,delim,j);
	  	  lines[i] = g_strdup_printf ("%s%s%c%s", tmpval,var,seperator,val);
	  	  g_free(tmpval);
	  }
	  else
	  {
	  	lines[i] = g_strdup_printf ("%s%c%s", var,seperator, val);
	  }
	}
      i++;
    }
*/
return 0;
}


GtkWidget *serial_create_device_interface(const gchar* infile,gchar **iname)
{
/*	gchar *cfgfile = g_strdup_printf("%s/%s",GPE_SERIAL_CONF_DIR,infile);
	
	num_items = serial_get_items(cfgfile);
*/	
	return NULL;
}


/* --- gpe-conf interface --- */

void
Serial_Free_Objects ()
{
}

void
Serial_Save ()
{
	char tmp[2];
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbConsole)))
		snprintf(tmp,2,"%d",SA_CONSOLE);
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.rbGPSD)))
		snprintf(tmp,2,"%d",SA_GPSD);
	else snprintf(tmp,2,"%d",SA_NONE);
	
	suid_exec("SERU",tmp);
}

void
Serial_Restore ()
{
	
}

GtkWidget *
Serial_Build_Objects (void)
{
/*  DIR *serialdir;
  struct dirent *entry;
  int i = 0;
*/GtkWidget *label;
  GtkWidget *vbox;
  gchar iname[100];
  GtkTooltips *tooltips;
	
  int gpsd_installed = !access(GPSD_STARTUP_SCRIPT,F_OK);

  tooltips = gtk_tooltips_new();
  
  notebook = gtk_notebook_new();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_get_border ());
  gtk_object_set_data(GTK_OBJECT(notebook),"tooltips",tooltips);
  
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());
  gtk_tooltips_set_tip(tooltips,vbox,_("Simple interface to serial port configuration. Disabled components are not installed."),NULL);
  
  label = gtk_label_new(_("Simple"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,label);
  gtk_tooltips_set_tip(tooltips,label,_("Simple interface to serial port configuration. Disabled components are not installed."),NULL);
	
  label = gtk_label_new(NULL);
  snprintf(iname,100,"<b>%s</b>",_("Serial port default usage"));
  gtk_label_set_markup(GTK_LABEL(label),iname);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,TRUE,0);
  gtk_tooltips_set_tip(tooltips,label,_("Select desired software to use serial port here."),NULL);
  
  self.rbConsole = gtk_radio_button_new_with_label(NULL,_("Console"));
  gtk_box_pack_start(GTK_BOX(vbox),self.rbConsole,FALSE,TRUE,0);
  gtk_tooltips_set_tip(tooltips,self.rbConsole,_("This option runs console on serial port. (default)"),NULL);
  
  self.rbGPSD = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(self.rbConsole),_("GPS Daemon"));
  gtk_box_pack_start(GTK_BOX(vbox),self.rbGPSD,FALSE,TRUE,0);
  gtk_widget_set_sensitive(self.rbGPSD,gpsd_installed);
  gtk_tooltips_set_tip(tooltips,self.rbGPSD,_("This option enables the start of GPSD, which provides data from a GPS reciver to applications."),NULL);
  
  self.rbFree = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(self.rbConsole),_("Nothing (free)"));
  gtk_box_pack_start(GTK_BOX(vbox),self.rbFree,FALSE,TRUE,0);
  gtk_tooltips_set_tip(tooltips,self.rbFree,_("Don't start any software that uses the serial port."),NULL);
  
  switch (get_serial_port_assignment())
  {
	  case SA_CONSOLE:
		  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbConsole),TRUE);
	  break;
	  case SA_GPSD:
		  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbGPSD),TRUE);
	  break;
	  case SA_NONE:
		  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.rbFree),TRUE);
	  break;
  }
  
/*  if (access(GPE_SERIAL_CONF_DIR,R_OK))
	{
      serialdir = opendir (GPE_SERIAL_CONF_DIR);
	  if(serialdir)
		{
		  while ((entry = readdir (serialdir)))
		{
	
		  if (entry->d_name[0] == '.') 
			continue;
		  
          ainterface = serial_create_device_interface(entry->d_name,&iname);
		  label = gtk_label_new(iname);
		  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),ainterface,label);
		  i++;
		}
		  closedir (serialdir);
		}
    }
  else
    {
      gpe_error_box ("Couldn't get serial devices info!");
    }
*/	
  return notebook;
}
