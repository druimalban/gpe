/*
 * gpe-conf
 *
 * Copyright (C) 2005  Florian Boor <florian.boor@kernelconcepts.de>
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
#include <ctype.h>

#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "../applets.h"


/* --- local types and constants --- */
/* this is from apps/kbdd/kbd.c */
#define KBD_TYPE_NONE		0
#define KBD_TYPE_FOLDABLE	1
#define KBD_TYPE_SNAPNTYPE	2
#define KBD_TYPE_STOWAWAY	3
#define KBD_TYPE_STOWAWAYXT	4
#define KBD_TYPE_HPSLIM		5
#define KBD_TYPE_SMARTBT	6
#define KBD_TYPE_LIRC		7
#define KBD_TYPE_BELKINIR	8
#define KBD_TYPE_FLEXIS		9
#define KBD_TYPE_BENQ_GAMEPAD	10
#define KBD_TYPE_POCKETVIK	11
#define KBD_TYPE_MICRO_FOLDAWAY	12
#define KBD_TYPE_MICRO_DATAPAD	13

#define DEFAULT_TTS "/dev/ttyS0"
#define KBDD_CONFIG "/etc/kbdd.conf"

/* --- module global variables --- */

int keyboard_type = KBD_TYPE_NONE;
gchar keyboard_port[PATH_MAX] = {0};

/* --- local intelligence --- */

int 
find_kbd_type(const char *ktype)
{
	if (strncmp("foldable", ktype, 8) == 0)
		return KBD_TYPE_FOLDABLE;
	else if (strncmp("snapntype", ktype, 9) == 0)
		return KBD_TYPE_SNAPNTYPE;
	else if (strncmp("stowaway", ktype, 8) == 0)
		return KBD_TYPE_STOWAWAY;
	else if (strncmp("stowawayxt", ktype, 8) == 0)
		return KBD_TYPE_STOWAWAYXT;
	else if (strncmp("hpslim", ktype, 6) == 0)
		return KBD_TYPE_HPSLIM;
	else if (strncmp("smartbt", optarg, 7) == 0)
		return KBD_TYPE_SMARTBT;
	else if (strncmp("flexis", ktype, 6) == 0)
		return KBD_TYPE_FLEXIS;
	else if (strncmp("lirc", optarg, 4) == 0)
		return KBD_TYPE_LIRC;
        else if (strncmp("belkinir", optarg, 8) == 0)
		return KBD_TYPE_BELKINIR;
	else if (strncmp("g250", optarg, 4) == 0)
		return KBD_TYPE_BENQ_GAMEPAD;
	else if (strncmp("pocketvik", optarg, 9) == 0)
		return KBD_TYPE_POCKETVIK;
	else if (strncmp("microfold", optarg, 9) == 0)
		return KBD_TYPE_MICRO_FOLDAWAY;
	else if (strncmp("micropad", optarg, 8) == 0)
		return KBD_TYPE_MICRO_DATAPAD;

	fprintf(stderr, "unrecognised keyboard type %s\n", ktype);

return KBD_TYPE_NONE;
}


void 
parse_config(const char *path, int *kbdtype, char port[])
{
char *needle;
FILE *fd;
char buf[PATH_MAX];
        
	fd = fopen(path, "r");
	if (!fd) {
		fprintf(stderr, "could not open config file %s\n", path);
		return;
	}
        
        while (!feof(fd)) {
		fgets(buf, PATH_MAX, fd);
		
		if (*buf == '#' || *buf == '\0') {
			/* It's a comment or a blank line */
			continue;
		}
		
		if ((needle = strstr(buf, "port:")) != 0) {
			needle += 5; 
			/* Trim whitespaces */
			while (isspace(*needle)) {
				needle++;
			}
			
			while (isspace(needle[strlen(needle)-1])) {
				needle[strlen(needle)-1] = '\0';
			}
			strncpy(port, needle, PATH_MAX);
		} else if ((needle = strstr(buf, "type:")) != 0) {
				needle += 5; 
				/* Trim whitespaces */
				while (isspace(*needle)) {
					needle++;
				}
			
				while (isspace(needle[strlen(needle)-1])) {
					needle[strlen(needle)-1] = '\0';
				}
					
				*kbdtype = find_kbd_type(needle);
			}
	}
}

static void
keyboard_selected(GtkToggleButton *tb, gpointer data)
{
	int keyboard = (int)data;
	if (gtk_toggle_button_get_active(tb))
		keyboard_type = keyboard;
}

/* --- gpe-conf interface --- */

void
Keyboard_Free_Objects ()
{
}

void
Keyboard_Save ()
{
}

void
Keyboard_Restore ()
{
}

GtkWidget *
Keyboard_Build_Objects (void)
{
	GtkWidget *label, *cbAny;
	GtkWidget *table;
	gchar *buf;
	GtkTooltips *tooltips;
	
	parse_config(KBDD_CONFIG, &keyboard_type, &keyboard_port);
	tooltips = gtk_tooltips_new ();
	buf = g_malloc (255);
	
	table = gtk_table_new (3, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), gpe_get_border ());
	gtk_table_set_row_spacings (GTK_TABLE (table), gpe_get_boxspacing ());
	gtk_table_set_col_spacings (GTK_TABLE (table), gpe_get_boxspacing ());

	gtk_object_set_data (GTK_OBJECT (table), "tooltips", tooltips);
	gtk_tooltips_set_tip (tooltips, 
	                      table, 
	                      _("External keyboard configuration."),
	                      NULL);

	label = gtk_label_new (NULL);
	snprintf (buf, 100, "<b>%s</b>", _("External Keyboard Type"));
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
	                  GTK_FILL, GTK_FILL, 0, 0);

	gtk_tooltips_set_tip (tooltips, label,
			              _("Here you may select your external keyboard model."),
	                      NULL);
						  
	cbAny = gtk_radio_button_new_with_label(NULL, _("None"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 1, 2,
	                  GTK_FILL, GTK_FILL, 0, 0);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_NONE);
	/* TRANSLATORS: Keyboard names below are brand names, 
	   check local names or keep original names */
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Foldable"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 2, 3,
	                  GTK_FILL, GTK_FILL, 0, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_FOLDABLE);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("SnapNType"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 3, 4,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_SNAPNTYPE)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_SNAPNTYPE);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Stowaway"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 4, 5,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_STOWAWAY)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_STOWAWAY);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("StowawayXT"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 5, 6,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_STOWAWAYXT)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_STOWAWAYXT);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("HP Slim"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 6, 7,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_HPSLIM)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_HPSLIM);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Smart BT"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 7, 8,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_SMARTBT)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_SMARTBT);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("LIRC"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 8, 9,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_LIRC)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_LIRC);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Belkin IR"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 9, 10,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_BELKINIR)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_BELKINIR);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Flexis"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 10, 11,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_FLEXIS)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_FLEXIS);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Benq Gamepad"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 11, 12,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_BENQ_GAMEPAD)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_BENQ_GAMEPAD);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Pocketvik"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 12, 13,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_POCKETVIK)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);	
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_POCKETVIK);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Micro Foldaway"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 13, 14,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_MICRO_FOLDAWAY)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_MICRO_FOLDAWAY);
	cbAny = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(cbAny), _("Micro Datapad"));
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 14, 15,
	                  GTK_FILL, GTK_FILL, 0, 0);
	if (keyboard_type == KBD_TYPE_MICRO_DATAPAD)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbAny), TRUE);
	g_signal_connect_after(G_OBJECT(cbAny), "toggled", 
	                       G_CALLBACK(keyboard_selected), (gpointer)KBD_TYPE_MICRO_DATAPAD);

	g_free(buf);
	return table;
}
