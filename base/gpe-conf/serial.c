/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003  Florian Boor <florian.boor@kernelconcepts.de>
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

#define GPE_SERIAL_CONF_DIR "/etc/gpe/gpe-conf-serial"

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



static GtkWidget *notebook;

/* --- local intelligence --- */

GtkWidget *create_device_interface(const gchar* infile,gchar **iname)
{
	gchar *cfgfile = g_strdup_printf("%s/%s",GPE_SERIAL_CONF_DIR,infile);
	
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

}

void
Serial_Restore ()
{
	
}

GtkWidget *
Serial_Build_Objects (void)
{
  DIR *serialdir;
  struct dirent *entry;
  int i = 0;
  GtkWidget* ainterface, *label;
  gchar iname[100];

  notebook = gtk_notebook_new();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_get_border ());
  if (access(GPE_SERIAL_CONF_DIR,R_OK))
	{
      serialdir = opendir (GPE_SERIAL_CONF_DIR);
	  if(serialdir)
		{
		  while ((entry = readdir (serialdir)))
		{
	
		  if (entry->d_name[0] == '.') 
			continue;
		  
          ainterface = create_device_interface(entry->d_name,&iname);
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
  return notebook;
}
