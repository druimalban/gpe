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
#include "../serial.h"

#define N_(_x) (_x)

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
#define KBD_TYPE_MAX	14

#define DEFAULT_TTS "/dev/ttyS0"
#define KBDD_CONFIG "/etc/kbdd.conf"
#define KBDD_CMD PREFIX "/bin/kbdd"

typedef struct
{
	char *idstr;
	int type;
	char *name;
}t_keyboard;

/* --- module global variables --- */

int keyboard_type = KBD_TYPE_NONE;
gchar keyboard_port[PATH_MAX] = {0};
static GtkWidget *cbPort;

/* TRANSLATORS: Keyboard names below are brand names, 
   please check local names or keep original names */
t_keyboard ktable[] = {
	{"none", KBD_TYPE_NONE, N_("none")},
	{"foldable", KBD_TYPE_FOLDABLE, N_("Foldable")},
	{"snapntype", KBD_TYPE_SNAPNTYPE, N_("SnapNType")},
	{"stowaway", KBD_TYPE_STOWAWAY, N_("Stowaway")},
	{"stowawayxt", KBD_TYPE_STOWAWAYXT, N_("StowawayXT")},
	{"hpslim", KBD_TYPE_HPSLIM, N_("HP Slim")},
	{"smartbt", KBD_TYPE_SMARTBT, N_("Smart BT")},
	{"lirc", KBD_TYPE_LIRC, N_("LIRC")},
	{"belkinir", KBD_TYPE_BELKINIR, N_("Belkin IR")},
	{"flexis", KBD_TYPE_FLEXIS, N_("Flexis")},
	{"g250", KBD_TYPE_BENQ_GAMEPAD, N_("Benq Gamepad")},
	{"pocketvik", KBD_TYPE_POCKETVIK, N_("Pocketvik")},
	{"microfold", KBD_TYPE_MICRO_FOLDAWAY, N_("Micro Foldaway")},
	{"micropad", KBD_TYPE_MICRO_DATAPAD, N_("Micro Datapad")},
	{"--dummy--", KBD_TYPE_MAX}
};	

/* --- local intelligence --- */

void 
keyboard_get_config(const char **model, const char **port)
{
	const char *text;
	int i;
	const char* kport = NULL;
	
	text = gtk_entry_get_text (GTK_ENTRY(GTK_COMBO(cbPort)->entry));
	for (i=0; i<num_ports; i++)
	{
		if (!strcmp(portlist[i][0], text))
			kport = portlist[i][1];
	}
	if (kport == NULL) 
		kport = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(cbPort)->entry));
	snprintf(keyboard_port, PATH_MAX, "%s", kport);
	
	*model = ktable[keyboard_type].idstr;
	*port = keyboard_port;
}

int 
find_kbd_type(const char *ktype)
{
	int i;
	
	for (i=1; i<KBD_TYPE_MAX; i++)
		if (strncmp(ktable[i].idstr, ktype, strlen(ktable[i].idstr)) == 0)
			return ktable[i].type;

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
keyboard_selected(GtkWidget *tb,  GdkEventButton *event, gpointer data)
{
	int i;
	const gchar *sel = gtk_entry_get_text(GTK_ENTRY(tb));
	
	for (i = 0; i < KBD_TYPE_MAX; i++)
	{
    	if (! strcmp(sel, ktable[i].name))
		{
			keyboard_type = ktable[i].type;
			break;
		}
	}
}

/* this is run suid root apply the settings */
void
keyboard_save(char *type, char *port)
{
	change_cfg_value (KBDD_CONFIG, "type:", type, ' ');
	change_cfg_value (KBDD_CONFIG, "port:", port, ' ');
	system("/etc/init.d/kbdd stop");
	system("/etc/init.d/kbdd start");
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
	GtkWidget *table, *vp, *sw;
	gchar *buf;
	GtkTooltips *tooltips;
	GSList *ports = NULL;
	GSList *keyboards = NULL;
	int i;
	
	parse_config(KBDD_CONFIG, &keyboard_type, &keyboard_port);
	tooltips = gtk_tooltips_new ();
	buf = g_malloc (255);
	
	table = gtk_table_new (2, 16, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), gpe_get_border ());
	gtk_table_set_row_spacings (GTK_TABLE (table), gpe_get_boxspacing ());
	gtk_table_set_col_spacings (GTK_TABLE (table), gpe_get_boxspacing ());

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), 
	                                    GTK_SHADOW_NONE);
	vp = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(vp), table);
	gtk_container_add(GTK_CONTAINER(sw), vp);
	
	gtk_object_set_data (GTK_OBJECT (table), "tooltips", tooltips);
	gtk_tooltips_set_tip (tooltips, table, 
	                      _("External keyboard configuration."), NULL);

	label = gtk_label_new (NULL);
	snprintf (buf, 100, "<b>%s</b>", _("External Keyboard Port"));
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
	                  GTK_FILL, GTK_FILL, 0, 0);

	gtk_tooltips_set_tip (tooltips, label,
			              _("Here you may select your external keyboard model."),
	                      NULL);
						  
	/* port selector */
	for (i=0; i<num_ports; i++)
		ports = g_slist_append(ports, portlist[i][0]);
	cbPort = gtk_combo_new ();
	gtk_combo_set_popdown_strings(GTK_COMBO(cbPort), (GList *)ports);
	gtk_table_attach(GTK_TABLE(table), cbPort, 0, 2, 1, 2,
	                 GTK_FILL, GTK_FILL, 0, 0);
	gtk_combo_set_value_in_list(GTK_COMBO(cbPort), FALSE, FALSE);
	gtk_entry_set_text (GTK_ENTRY(GTK_COMBO(cbPort)->entry), keyboard_port);
	for (i=0; i<num_ports; i++)
	{
		if (!strcmp(portlist[i][1], keyboard_port))
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cbPort)->entry),
			                   portlist[i][0]);
	}
	
	/* keyboard selection */
	label = gtk_label_new (NULL);
	snprintf (buf, 100, "<b>%s</b>", _("External Keyboard Type"));
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 2, 3,
	                  GTK_FILL, GTK_FILL, 0, 0);
	
	cbAny = gtk_combo_new();
	for (i = 0; i < KBD_TYPE_MAX; i++)
	{
    	keyboards = g_slist_append(keyboards, ktable[i].name);
		if (keyboard_type == ktable[i].type)
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cbAny)->entry), ktable[i].name);
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(cbAny), (GList *)keyboards);
	gtk_combo_set_value_in_list(GTK_COMBO(cbAny), TRUE, FALSE);
	g_signal_connect_after(G_OBJECT(GTK_COMBO(cbAny)->entry), "changed", 
	                       G_CALLBACK(keyboard_selected), NULL);
	gtk_table_attach (GTK_TABLE (table), cbAny, 0, 2, 4, 5,
	                  GTK_FILL, GTK_FILL, 0, 0);
	g_free(buf);
	return sw;
}
