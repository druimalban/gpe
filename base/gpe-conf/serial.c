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
#define GPSD_STARTUP_SCRIPT "/etc/init.d/gpsd"
#define GPSD_STARTUP_LINK "/etc/rc2.d/S96gpsd"

#define INITTAB "/etc/inittab"
#define GPSD_CONFIG "/etc/gpsd.conf"

#ifdef MACH_IPAQ
  #define FIRST_SERIAL "/dev/ttyC0"
#else
  #define FIRST_SERIAL "/dev/ttyS0"
#endif

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
static int gpsd_installed;

static struct
{
	GtkWidget *rbConsole;
	GtkWidget *rbGPSD;
	GtkWidget *rbFree;
	GtkWidget *rbEarthmate;
	GtkWidget *rbNMEA;
	GtkWidget *cbBaudrate;
	GtkWidget *cbPort;
} self;

char *portlist[][2] = {	{"Internal Serial (RS232)", FIRST_SERIAL},
	{"Bluetooth RFCOMM 0","/dev/bluetooth/rfcomm/0"},
	{"Bluetooth RFCOMM 1","/dev/bluetooth/rfcomm/1"},
	{"PCMCIA / CF Port 1","/dev/tts/0"},
	{"PCMCIA / CF Port 2","/dev/tts/1"}
	};

int num_ports = sizeof(portlist) / sizeof(char*) / 2;

gboolean
getty_uses_port(const gchar* port)
{
	gchar *content;
	gchar **lines = NULL;
	gint length;
	int i = 0;
	GError *err = NULL;
	gboolean result = FALSE;

	if (!g_file_get_contents (INITTAB, &content, &length, &err))
	{
		fprintf (stderr, "Could not access inittab.\n");
		return FALSE;
	}
	
	lines = g_strsplit (content, "\n", 2048);
	g_free (content);

	/* search for line that mentiones getty and our port */
	while (lines[i])
	{
		if ((g_strrstr (g_strchomp (lines[i]), strrchr(port,'/')+1)
			&& g_strrstr (g_strchomp (lines[i]), "getty")))
		{
		    if (!g_str_has_prefix (g_strstrip (lines[i]), "#"))
			{
				result = TRUE;
				break;
			}
		}
		i++;
	}
	g_strfreev (lines);
	return result;
}

void
inittab_change_getty(gboolean turniton)
{
	gchar *content;
	gchar **lines = NULL;
	gint length;
	FILE *fnew;
	gint i = 0;
	gint j = 0;
	GError *err = NULL;
	gboolean foundline = FALSE;

	if (!g_file_get_contents (INITTAB, &content, &length, &err))
	{
		fprintf (stderr, "Could not access inittab.\n");
		return;
	}
	
	lines = g_strsplit (content, "\n", 2048);
	g_free (content);

	/* search for line that mentiones getty and our port */
	while (lines[i])
	{
		if ((g_strrstr (g_strchomp (lines[i]), strrchr(FIRST_SERIAL,'/')+1)
			&& g_strrstr (g_strchomp (lines[i]), "getty")))
		{
			foundline = TRUE;
		    if (g_str_has_prefix (g_strstrip (lines[i]), "#") == turniton)
			{
				gchar *tmp = lines[i];
				if (turniton)
					lines[i] = g_strdup(strchr(lines[i], '#')+1);
				else
					lines[i] = g_strdup_printf("#%s",lines[i]);
				g_free(tmp);
			}
		}
		i++;
	}
	
	if (i)
		i--;

	fnew = fopen (INITTAB, "w");
	if (!fnew)
	{
		fprintf (stderr, "Could not write to inittab.\n");
		return;
	}

	for (j = 0; j < i; j++)
	{
		fprintf (fnew, "%s\n", lines[j]);
	}
	
	/* didn't find getty line? add it! */
	if (!foundline && turniton)
		fprintf(fnew, "S:2345:respawn:/sbin/getty %s 115200 vt100\n",
		        strrchr(FIRST_SERIAL,'/')+1);
	
	fclose (fnew);
	g_strfreev (lines);
}


/* --- local intelligence --- */

void
assign_serial_port (t_serial_assignment type)
{
	int gpsd_installed = !access (GPSD_STARTUP_SCRIPT, F_OK);

	switch (type)
	{
	case SA_NONE:
		if (gpsd_installed)
		{
			if (!access (GPSD_STARTUP_SCRIPT, X_OK))
			{
				system (GPSD_STARTUP_SCRIPT " stop");
				system ("killall gpsd.bin");
				system ("killall gpsd");
			}
			system ("chmod a-x " GPSD_STARTUP_SCRIPT);
		}
		/* we disable getty */
		inittab_change_getty(FALSE);
		
		system ("telinit q");
		break;
	case SA_GPSD:
		if (gpsd_installed)
		{
			system ("chmod a+x " GPSD_STARTUP_SCRIPT);
			system (GPSD_STARTUP_SCRIPT " stop");
			system ("killall gpsd.bin");	// due to a bug in gpsd initscript
			system (GPSD_STARTUP_SCRIPT " start");
			if (access (GPSD_STARTUP_LINK, F_OK))	// create link if not there
				system ("ln -s " GPSD_STARTUP_SCRIPT " "
					GPSD_STARTUP_LINK);
		}
		inittab_change_getty(FALSE);
		system ("telinit q");
		break;
	case SA_CONSOLE:
		if (gpsd_installed)
		{
			if (!access (GPSD_STARTUP_SCRIPT, X_OK))
			{
				system (GPSD_STARTUP_SCRIPT " stop");
				system ("killall gpsd.bin");
			}
			system ("chmod a-x " GPSD_STARTUP_SCRIPT);
		}
		inittab_change_getty(TRUE);
		system ("telinit q");
		break;
	}
}

/* identifies what is using the serial port */
int
get_serial_port_assignment ()
{
	//int runlevel;
	int result = SA_NONE;

/*	if (parse_pipe ("/sbin/runlevel ", "%*s %d", &runlevel))
	{
		gpe_error_box (_("Couldn't identify current runlevel!"));
		runlevel = 0;
	}
*/
	/* S:2345:respawn:/sbin/getty ttyS0 115200 vt100 */
	if (getty_uses_port(FIRST_SERIAL))
			result = SA_CONSOLE;

	/* check for gpsd startup script */
	if (!access (GPSD_STARTUP_SCRIPT, X_OK))
		result = SA_GPSD;

	return result;
}


void
update_gpsd_settings (char *baud, int emate, char* port)
{
	if (emate)
	{
		change_cfg_value (GPSD_CONFIG, "-T", "e", ' ');
	}
	else
	{
		change_cfg_value (GPSD_CONFIG, "-T", "e", 0);
	}

	change_cfg_value (GPSD_CONFIG, "-s", baud, ' ');
	change_cfg_value (GPSD_CONFIG, "-p", port, ' ');
}


gint
serial_get_items (gchar * file)
{
	return 0;
}


GtkWidget *
serial_create_device_interface (const gchar * infile, gchar ** iname)
{
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
	char *tmp;
	const char *text;
	const char *port = NULL;
	int i;

	/* this first, gpsd is reloaded in next step */
	if (gpsd_installed)
	{
		text = gtk_entry_get_text (GTK_ENTRY
					     (GTK_COMBO (self.cbPort)->entry));
		for (i=0;i<num_ports;i++)
		{
			if (!strcmp(portlist[i][0],text))
				port = portlist[i][1];
		}
		if (port == NULL) port = gtk_entry_get_text (GTK_ENTRY
					     (GTK_COMBO (self.cbPort)->entry));
			
		tmp = g_strdup_printf(" %s %s",
			 gtk_entry_get_text (GTK_ENTRY
					     (GTK_COMBO (self.cbBaudrate)->entry)),port);
		if (gtk_toggle_button_get_active
		    (GTK_TOGGLE_BUTTON (self.rbEarthmate)))
			tmp[0] = '1';
		else
			tmp[0] = '0';
		
		suid_exec ("SGPS", tmp);
		g_free(tmp);
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self.rbConsole)))
		tmp = g_strdup_printf("%d", SA_CONSOLE);
	else if (gtk_toggle_button_get_active
		 (GTK_TOGGLE_BUTTON (self.rbGPSD)))
		tmp = g_strdup_printf("%d", SA_GPSD);
	else
		tmp = g_strdup_printf("%d", SA_NONE);

	suid_exec ("SERU", tmp);
	g_free(tmp);
}

void
Serial_Restore ()
{

}

GtkWidget *
Serial_Build_Objects (void)
{
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *table;
	gchar iname[100];
	GtkTooltips *tooltips;
	GSList *bauds = NULL;
	GSList *ports = NULL;
	gchar cur_baud[10] = { "4800" };
	gchar cur_port[33] = { FIRST_SERIAL };
	gint ibaud = 4800;
	gchar etmp[10];
	gboolean emate = FALSE;
	gboolean getty_installed;
	int i;

	gpsd_installed = !access (GPSD_STARTUP_SCRIPT, F_OK);
	getty_installed = !access ("/sbin/getty", F_OK);

	if (!access (GPSD_CONFIG, R_OK))
	{
		if (!parse_file (GPSD_CONFIG, "-T %c", etmp) ||
		    !parse_file (GPSD_CONFIG, "-T%c", etmp))
			emate = TRUE;
		parse_file (GPSD_CONFIG, "-s %d", &ibaud);
		parse_file (GPSD_CONFIG, "-p %32s", cur_port);
	}
	snprintf (cur_baud, 10, "%d", ibaud);
	bauds = g_slist_append (bauds, "1200");
	bauds = g_slist_append (bauds, "2400");
	bauds = g_slist_append (bauds, "4800");
	bauds = g_slist_append (bauds, "9600");
	bauds = g_slist_append (bauds, "19200");
	bauds = g_slist_append (bauds, "38400");
	bauds = g_slist_append (bauds, "115000");

	for (i=0;i<num_ports;i++)
		ports = g_slist_append (ports,portlist[i][0]);
	tooltips = gtk_tooltips_new ();

	notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (notebook),
					gpe_get_border ());
	g_object_set_data (G_OBJECT (notebook), "tooltips", tooltips);

	/* port settings */

	vbox = gtk_vbox_new (FALSE, gpe_get_boxspacing ());
	gtk_tooltips_set_tip (tooltips, vbox,
			      _("Simple interface to serial port configuration. Disabled components are not installed."),
			      NULL);

	label = gtk_label_new (_("Port A"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
	gtk_tooltips_set_tip (tooltips, label,
			      _("Simple interface to serial port configuration. Disabled components are not installed."),
			      NULL);

	label = gtk_label_new (NULL);
	snprintf (iname, 100, "<b>%s</b>", _("Serial port default usage"));
	gtk_label_set_markup (GTK_LABEL (label), iname);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
	gtk_tooltips_set_tip (tooltips, label,
			      _("Select desired software to use serial port here."),
			      NULL);

	self.rbConsole = gtk_radio_button_new_with_label (NULL, _("Console"));
	gtk_box_pack_start (GTK_BOX (vbox), self.rbConsole, FALSE, TRUE, 0);
	gtk_tooltips_set_tip (tooltips, self.rbConsole,
			      _("This option runs console on serial port. (default)"),
			      NULL);
	gtk_widget_set_sensitive (self.rbConsole, getty_installed);
    
	if (gpsd_installed)
		self.rbGPSD =
			gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
			                                              (self.rbConsole),
							                            _("GPS Daemon"));
	else
		self.rbGPSD =
			gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
			                                              (self.rbConsole),
							                            _("GPS Daemon (needs 'gpsd')"));
		
	gtk_box_pack_start (GTK_BOX (vbox), self.rbGPSD, FALSE, TRUE, 0);
	gtk_widget_set_sensitive (self.rbGPSD, gpsd_installed);
	gtk_tooltips_set_tip (tooltips, self.rbGPSD,
			      _("This option enables the start of GPSD, which provides data from a GPS reciver to applications."),
			      NULL);

	self.rbFree =
		gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (self.rbConsole),
							     _
							     ("Nothing (free)"));
	gtk_box_pack_start (GTK_BOX (vbox), self.rbFree, FALSE, TRUE, 0);
	gtk_tooltips_set_tip (tooltips, self.rbFree,
			      _("Don't start any software that uses the serial port."),
			      NULL);

	switch (get_serial_port_assignment ())
	{
	case SA_CONSOLE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (self.rbConsole), TRUE);
		break;
	case SA_GPSD:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self.rbGPSD),
					      TRUE);
		break;
	case SA_NONE:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self.rbFree),
					      TRUE);
		break;
	}

	/* gpsd settings */

/* THAT ARE: 
    	-D integer   [ set debug level ]
  -L longitude [ set longitude ]
  		-S integer   [ set port for daemon ]
  -T e         [ earthmate flag ]
  		-h           [ help message ]
  -l latitude  [ set latitude ]
  		-p string    [ set gps device name ]
  -s baud_rate [ set baud rate on gps device ]
  -c           [ use dgps service for corrections ]
  -d host      [ set dgps server ]
  -r port      [ set dgps rtcm-sc104 port ]
*/
	table = gtk_table_new (3, 3, FALSE);

	gtk_tooltips_set_tip (tooltips, table,
			      _("This page configures the GPS receiver software."),
			      NULL);
	gtk_widget_set_sensitive (table, gpsd_installed);

	label = gtk_label_new (_("GPS Receiver"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);
	gtk_tooltips_set_tip (tooltips, label,
			      _("This page configures the GPS receiver software."),
			      NULL);

	label = gtk_label_new (NULL);
	snprintf (iname, 100, "<b>%s</b>", _("GPS settings"));
	gtk_label_set_markup (GTK_LABEL (label), iname);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1, GTK_FILL,
			  GTK_FILL, 0, gpe_get_boxspacing ());

	label = gtk_label_new (_("Receiver Type"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 1, 2, GTK_FILL,
			  GTK_FILL, 0, 0);

	self.rbNMEA = gtk_radio_button_new_with_label (NULL, _("NMEA"));
	gtk_table_attach (GTK_TABLE (table), self.rbNMEA, 0, 1, 2, 3,
			  GTK_FILL, GTK_FILL, gpe_get_boxspacing (),
			  gpe_get_boxspacing ());
	gtk_tooltips_set_tip (tooltips, self.rbNMEA,
			      _("The GPS receiver sends NMEA data (default, no need to change for most receivers)."),
			      NULL);

	self.rbEarthmate =
		gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (self.rbNMEA),
							     _("Earthmate"));
	gtk_table_attach (GTK_TABLE (table), self.rbEarthmate, 1, 2, 2, 3,
			  GTK_FILL, GTK_FILL, gpe_get_boxspacing (),
			  gpe_get_boxspacing ());
	gtk_widget_set_sensitive (self.rbEarthmate, gpsd_installed);
	gtk_tooltips_set_tip (tooltips, self.rbEarthmate,
			      _("GPS receiver is an Earthmate GPS."), NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self.rbEarthmate),
				      emate);

	label = gtk_label_new (_("Baud Rate"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 3, 4, GTK_FILL,
			  GTK_FILL, 0, 0);

	self.cbBaudrate = gtk_combo_new ();
	gtk_combo_set_popdown_strings (GTK_COMBO (self.cbBaudrate),
				       (GList *) bauds);
	gtk_table_attach (GTK_TABLE (table), self.cbBaudrate, 0, 2, 4, 5,
			  GTK_FILL, GTK_FILL, gpe_get_boxspacing (),
			  gpe_get_boxspacing ());
	gtk_combo_set_value_in_list (GTK_COMBO (self.cbBaudrate), FALSE,
				     FALSE);
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (self.cbBaudrate)->entry),
			    cur_baud);

	label = gtk_label_new (_("Serial Port"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 5, 6, GTK_FILL,
			  GTK_FILL, 0, 0);

	self.cbPort = gtk_combo_new ();
	gtk_combo_set_popdown_strings (GTK_COMBO (self.cbPort),
				       (GList *) ports);
	gtk_table_attach (GTK_TABLE (table), self.cbPort, 0, 2, 6, 7,
			  GTK_FILL, GTK_FILL, gpe_get_boxspacing (),
			  gpe_get_boxspacing ());
	gtk_combo_set_value_in_list (GTK_COMBO (self.cbPort), FALSE,
				     FALSE);
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (self.cbPort)->entry),cur_port);
	for (i=0;i<num_ports;i++)
	{
		if (!strcmp(portlist[i][1],cur_port))
			gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (self.cbPort)->entry),
			    portlist[i][0]);
	}
	return notebook;
}
