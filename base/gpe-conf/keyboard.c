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
 * GPE external keyboard configuration module.
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

#define KBD_FILE "/etc/gpe/keyboards.def"
#define KBDCFG_FILE "/etc/keyboards.cfg"


typedef struct			/* this holds a keyboard definition in the gui */
{
	char *identifier;
	char *module_name;
	char *package_name;
	char *device_name;
	char *support_package;
	GtkWidget *rbSelect;
	GtkWidget *bInstall;
}
t_kbd;


/* --- module global variables --- */

static t_kbd *kbds = NULL;
static int numkbds = 0;


/* --- local intelligence --- */

// writes the config file
static int
write_keyboard_cfg (int whichkbd)
{
	FILE *fnew;
	
	fnew = fopen (KBDCFG_FILE, "w");
	if (!fnew)
	{
		fprintf (stderr, "Could not write to file: %s.\n", KBDCFG_FILE);
		return TRUE;
	}

	fprintf (fnew, "KBD_IDENT=%s\n", kbds[whichkbd].identifier);
	fprintf (fnew, "KBD_DEVICE=%s\n", kbds[whichkbd].device_name);
	fclose (fnew);
	return FALSE;
}

// reads keyboard definition file
static int
get_keyboard_defs ()
{
	gchar *content;
	gchar **lines = NULL;
	gchar **el = NULL;
	int i, j, length;
	GError *err = NULL;
	gchar buf[255];
	gchar *val;

	i = 1;			// !
	if (!g_file_get_contents (KBD_FILE, &content, &length, &err))
	{
		gpe_error_box ("Could not read keyboard definition file.");
	}
	lines = g_strsplit (content, "[", 42);
	g_free (content);
	while (lines[i])
	{
		kbds = realloc (kbds, (i) * sizeof (t_kbd));
		kbds[i - 1].module_name = NULL;
		kbds[i - 1].package_name = NULL;
		kbds[i - 1].device_name = NULL;
		kbds[i - 1].support_package = NULL;

		j = 1;
		el = g_strsplit (lines[i], "\n", 5);
		snprintf (buf, strcspn (el[0], "]\n") + 1, "%s", el[0]);
		kbds[i - 1].identifier = g_strdup (buf);

		while (el[j])
		{
			if (val = strstr (el[j], "module-package"))
				kbds[i - 1].package_name =
					g_strdup (g_strchomp
						  (strstr (val, "=") + 1));
			else if (val = strstr (el[j], "module-name"))
				kbds[i - 1].module_name =
					g_strdup (g_strchomp
						  (strstr (val, "=") + 1));
			else if (val = strstr (el[j], "device"))
				kbds[i - 1].device_name =
					g_strdup (g_strchomp
						  (strstr (val, "=") + 1));
			else if (val = strstr (el[j], "support-package"))
				kbds[i - 1].support_package =
					g_strdup (g_strchomp
						  (strstr (val, "=") + 1));
			j++;
		}
		if (j < 4)
		{
			snprintf (buf, 255, "%s: %s",
				  _("Error reading keyboard definition)"),
				  kbds[i].identifier);
			gpe_error_box (buf);
		}
		g_strfreev (el);
		i++;
	}
	kbds[i - 1].package_name = g_strdup("");
	kbds[i - 1].module_name = NULL;
	kbds[i - 1].device_name = g_strdup("none");
	kbds[i - 1].support_package = NULL;
	numkbds = i;
	g_strfreev (lines);
	return numkbds;
}

void do_install (GtkWidget * button, GdkEventButton * event,
		 gpointer user_data)
{

}

/* --- gpe-conf interface --- */

void
Keyboard_Free_Objects ()
{
}

void
Keyboard_Save ()
{
	int i;
// write name of device to config file
	for (i=0;i<numkbds;i++)
	{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kbds[i].rbSelect)))
		{
			write_keyboard_cfg(i);
			break;
		}
	}
// add according module to /etc/modules
	
// do instant activation
}

void
Keyboard_Restore ()
{

}

GtkWidget *
Keyboard_Build_Objects (void)
{
	GtkWidget *label;
	GtkWidget *table;
	gchar *buf;
	GtkTooltips *tooltips;
	int i;

	tooltips = gtk_tooltips_new ();

	buf = malloc (255);

	table = gtk_table_new (3, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table),
					gpe_get_border ());
	gtk_table_set_row_spacings (GTK_TABLE (table), gpe_get_boxspacing ());
	gtk_table_set_col_spacings (GTK_TABLE (table), gpe_get_boxspacing ());

	gtk_object_set_data (GTK_OBJECT (table), "tooltips", tooltips);
	gtk_tooltips_set_tip (tooltips, table,
			      _("External keyboard configuration. Disabled buttons \
					are currently not installed."),
			      NULL);

	label = gtk_label_new (NULL);
	snprintf (buf, 100, "<b>%s</b>", _("External Keyboard Type"));
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
			  GTK_FILL, GTK_FILL, 0, 0);

	gtk_tooltips_set_tip (tooltips, label,
			      _("Here you may select your external keyboard model. \
				  To install software support connect your device to \
				  internet and click the \"Install\" button."),
			      NULL);

	get_keyboard_defs ();

	label = NULL;

	for (i = 0; i < numkbds; i++)
	{
		kbds[i].rbSelect =
			gtk_radio_button_new_with_label_from_widget (label,
								     kbds[i].
								     identifier);
		label = kbds[i].rbSelect;
		gtk_table_attach (GTK_TABLE (table), kbds[i].rbSelect, 0, 1,
				  i + 1, i + 2, GTK_FILL | GTK_EXPAND,
				  GTK_FILL, 0, 0);
		gtk_tooltips_set_tip (tooltips, kbds[i].rbSelect,
				      _
				      ("Tap here to select this keyboard model."),
				      NULL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (kbds[i].rbSelect), TRUE);
		
		if (kbds[i].support_package)
		{
			kbds[i].bInstall = gtk_button_new_with_label (_("Install"));
			gtk_table_attach (GTK_TABLE (table), kbds[i].bInstall, 1, 2,
				  i + 1, i + 2, GTK_FILL | GTK_EXPAND,
				  GTK_FILL, 0, 0);
			gtk_tooltips_set_tip (tooltips, kbds[i].bInstall,
				      _
				      ("Tap here to install support for this keyboard."),
				      NULL);
			g_signal_connect (kbds[i].bInstall, "clicked",
				  G_CALLBACK (do_install), &kbds[i]);
		}
	}

	g_free (buf);

	return table;
}
