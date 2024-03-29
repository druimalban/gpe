/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003 - 2005  Florian Boor <florian.boor@kernelconcepts.de>
 *               2004  Ole Reinhardt <ole.reinhardt@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Dynamic interface configuration added by Florian Boor (florian.boor@kernelconcepts.de)
 *
 * Wireless LAN support added by Ole Reinhardt (ole.reinhardt@kernelconcepts.de)
 * Some wireless code is taken from iwconfig.c (wireless tools) 
 *     Copyright (c) 1997-2002 Jean Tourrilhes <jt@hpl.hp.com>
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <time.h>
#include <libintl.h>
#include <net/if.h>
#define _(x) gettext(x)

#include "applets.h"
#include "network.h"
#include "parser.h"
#include "cfgfile.h"
#include "misc.h"
#include "suid.h"
#include "tools/interface.h"

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/smallbox.h>
#include <gpe/gtksimplemenu.h>

/* offset for page to interface */
#define PAGE_OFFSET 1


extern NWInterface_t *iflist;
extern gint iflen;
extern GtkWidget *mainw;
GtkWidget *notebook;
GtkWidget *create_nwstatic_widgets (NWInterface_t iface);
GtkWidget *create_nwdhcp_widgets (NWInterface_t iface);
GtkWidget *create_nwppp_widgets (NWInterface_t iface);

static gchar *cfgfile;
static gboolean have_access = FALSE;
static GtkTooltips *tooltips;

static const char *help_devtype;
static const char *help_wifi;
static const char *help_wificonfig;
static const char *help_essid;
static const char *help_wifimode;
static const char *help_encmode;
static const char *help_wepkey;
static const char *help_selectkey;
static const char *help_channel;

static void
show_current_config (GtkWidget * button)
{
	char *buffer = g_strdup ("");
	char *tmp = NULL;
	struct interface *ife;
	struct interface *int_list;
	GtkWidget *w, *label, *sw;

	int_list = if_getlist ();

	for (ife = int_list; ife; ife = ife->next)
	{
		if ((ife->flags & IFF_UP) && !(ife->flags & IFF_LOOPBACK))
		{
			tmp = if_to_infostr (ife);
			buffer = realloc (buffer, strlen (tmp) + strlen (buffer) + 1);
			strcat (buffer, tmp);
			free (tmp);
		}
	}

	w = gtk_dialog_new_with_buttons (_("Current Config"),
					 GTK_WINDOW (mainw),
					 GTK_DIALOG_MODAL |
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT,
					 NULL);

	gtk_window_set_default_size(GTK_WINDOW(w), 400, 400);	
	
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

	gtk_label_set_markup (GTK_LABEL (label), buffer);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), label);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(label->parent), GTK_SHADOW_NONE);
	
	gtk_widget_show_all (sw);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (w)->vbox), sw);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (w)->vbox), 
	                                gpe_get_border());
	
	g_signal_connect_swapped (G_OBJECT (w),
				  "response",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (w));

	gtk_dialog_run (GTK_DIALOG (w));

	free (buffer);
}

void
copy_new_interfaces(void)
{
	struct interface *ife;
	struct interface *int_list;
		
	int_list = if_getlist ();
	
	system_printf ("/bin/cp %s %s", NET_NEWFILE, NET_CONFIGFILE);
	system_printf ("chmod 0644 %s", NET_CONFIGFILE);
	system_printf ("/bin/rm -f %s", NET_NEWFILE);
	
	for (ife = int_list; ife; ife = ife->next)
	{
		if ((ife->flags & IFF_UP) && !(ife->flags & IFF_LOOPBACK))
		{
			gchar *cmd = g_strdup_printf("/sbin/ifdown %s", ife->name);
			system(cmd);
			g_free(cmd);
			cmd = g_strdup_printf("/sbin/ifup %s", ife->name);
			system(cmd);
			g_free(cmd);
		}
	}
}

static GList *
get_unconfigured_interfaces (void)
{
#define num_suggestions 3	
	
	GList *result = NULL;
	gint i,j;
	gboolean found;
	gchar buffer[256];
	gchar *sep;
	gchar *name;
	FILE *fd;
	gchar suggestions[num_suggestions][6] = {"eth0", "wlan0", "bnep0"};
	
	fd = fopen(_PATH_PROCNET_DEV, "r");
	if (fd)
	{
		fgets(buffer, 256, fd);		// chuck first two lines;
		fgets(buffer, 256, fd);
		while (!feof(fd)) {
			if (fgets(buffer, 256, fd) == NULL)
				break;
			name = buffer;
			sep = strrchr(buffer, ':');
			if (sep) *sep = 0;
			while(*name == ' ') name++;
	
			found = FALSE;
			for (i = 0; i < iflen; i++)
			{
				if (!strcmp (name, iflist[i].name))
				{
					found = TRUE;
					break;
				}
			}
			if (!found)
			{
				result = g_list_append (result, g_strdup (name));
			}
		}
		fclose(fd);
	}
	for (j = 0; j < num_suggestions; j++)
	{
		found = FALSE;
		for (i = 0; i < iflen; i++)
		{
			if (!strcmp (suggestions[j], iflist[i].name))
			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			result = g_list_append (result, g_strdup (suggestions[j]));
		}

	}
	
	return (result);
}


static void
add_interface (GtkWidget * widget, gpointer d)
{
	gchar *ifname;
	GtkWidget *ctable = NULL;
	GtkWidget *label;
	gint i;
	gint existing = -1;
	struct box_desc2 ifbox[2];
	gchar buffer[256];
	gchar *sep;
	gchar *name;
	FILE *fd;	

	ifbox[0].suggestions = get_unconfigured_interfaces ();

	ifbox[0].label = g_strdup (_("Name:"));
	ifbox[0].value = ifbox[0].suggestions->data;
	ifbox[1].suggestions = NULL;
	ifbox[1].label = NULL;
	ifbox[1].value = NULL;

	if (smallbox_x2 (_("New Interface"), ifbox))
	{
		ifname = g_strdup (ifbox[0].value);
		for (i = 0; i < iflen; i++)
			if (!strcmp (ifname, iflist[i].name))
			{
				if (iflist[i].status != NWSTATE_REMOVED)
				{
					gpe_error_box(
						_("This interface definition already exists!"));
					return;
				}
				else
					existing = i; /* remember this interface definition */
			}
			
		if (existing < 0)
		{
			iflen++;
			iflist = (NWInterface_t *) realloc (iflist,
								iflen *
								sizeof (NWInterface_t));
			memset (&iflist[iflen - 1], '\0', sizeof (NWInterface_t));
			iflist[iflen - 1].status = NWSTATE_NEW;
			strcpy (iflist[iflen - 1].name, ifname);
			iflist[iflen - 1].isstatic = TRUE;
			iflist[iflen - 1].isinet = TRUE;
			iflist[iflen - 1].isloop = FALSE;
			iflist[iflen - 1].isdhcp = FALSE;
			iflist[iflen - 1].isppp = FALSE;
			
			iflist[iflen-1].iswireless = FALSE;
			strcpy(iflist[iflen-1].essid, "any");
			iflist[iflen-1].mode = MODE_MANAGED;
			iflist[iflen-1].encmode = ENC_OFF;
			iflist[iflen-1].keynr = 1;
			
			i = iflen-1;
			
			fd = fopen(_PATH_PROCNET_WIRELESS, "r");
			if (fd != NULL)
			{
				fgets(buffer, 256, fd);		// chuck first two lines;
				fgets(buffer, 256, fd);
				while (!feof(fd)) 
				{
					if (fgets(buffer, 256, fd) == NULL)
						break;
					name = buffer;
					sep = strrchr(buffer, ':');
					if (sep) *sep = 0;
					while(*name == ' ') name++;
					if (!strcmp(name, ifname))
						iflist[iflen - 1].iswireless = TRUE;
				}
			fclose(fd);
			}
		}
		else
			i = existing;
		
		ctable = create_nwstatic_widgets (iflist[i]);
		if (ctable)
		{
			label = gtk_label_new (iflist[i].name);
			iflist[i].uipos = gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			                                            GTK_WIDGET (ctable),
			                                            label);
			gtk_widget_show_all (notebook);
			g_object_set_data(G_OBJECT(ctable), "ifnr", (gpointer)i);
		}
		else
			iflist[i].uipos = -1;
		
		gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), -1);
	}
}

static void
remove_interface (GtkWidget * widget, gpointer d)
{
	G_CONST_RETURN gchar *ifname;
	GtkWidget *page;

	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook),
					  gtk_notebook_get_current_page
					  (GTK_NOTEBOOK (notebook)));
	if (!page) 
		return;
	ifname = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (notebook), page);
	if (gpe_question_ask
	    (_("Do you want to delete this interface?"), _("Question"),
	     "question", "!gtk-no", NULL, "!gtk-yes", NULL, NULL))
	{
		iflist[get_section_nr (ifname)].status = NWSTATE_REMOVED;
		gtk_notebook_remove_page (GTK_NOTEBOOK (notebook),
					  gtk_notebook_get_current_page
					  (GTK_NOTEBOOK (notebook)));
	}
}


void
changed_nwtype (GtkToggleButton * togglebutton, gpointer user_data)
{
	GtkWidget *ctable, *label, *page;
	gchar wname[100];
	gint row;
	
	page = gtk_notebook_get_nth_page(GTK_NOTEBOOK (notebook), 
	                                 gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook)));
	row = (gint)g_object_get_data(G_OBJECT(page), "ifnr");

	if (!gtk_toggle_button_get_active (togglebutton))
		return;		// just run once

	// look who called us...
	strcpy (wname, "static");
	strcat (wname, iflist[row].name);
	if (g_object_get_data (G_OBJECT (notebook), wname) == togglebutton)
	{
		iflist[row].isstatic = TRUE;
		iflist[row].isdhcp = FALSE;
		iflist[row].isppp = FALSE;
	}
	strcpy (wname, "dhcp");
	strcat (wname, iflist[row].name);
	if (g_object_get_data (G_OBJECT (notebook), wname) == togglebutton)
	{
		iflist[row].isstatic = FALSE;
		iflist[row].isdhcp = TRUE;
		iflist[row].isppp = FALSE;
	}
	strcpy (wname, "ppp");
	strcat (wname, iflist[row].name);
	if (g_object_get_data (G_OBJECT (notebook), wname) == togglebutton)
	{
		iflist[row].isstatic = FALSE;
		iflist[row].isdhcp = FALSE;
		iflist[row].isppp = TRUE;
	}

	ctable = NULL;
	if (iflist[row].isstatic)
		ctable = create_nwstatic_widgets (iflist[row]);
	if (iflist[row].isdhcp)
		ctable = create_nwdhcp_widgets (iflist[row]);
	if (iflist[row].isppp)
		ctable = create_nwppp_widgets (iflist[row]);
	if (ctable)
	{
		label = gtk_label_new (iflist[row].name);
		gtk_notebook_remove_page (GTK_NOTEBOOK (notebook),
					  gtk_notebook_get_current_page
					  (GTK_NOTEBOOK (notebook)));
		gtk_notebook_insert_page (GTK_NOTEBOOK (notebook),
					  GTK_WIDGET (ctable), label,
					  iflist[row].uipos);
		gtk_widget_show_all (notebook);
		gtk_notebook_set_page (GTK_NOTEBOOK (notebook),
				      iflist[row].uipos);
		g_object_set_data(G_OBJECT(ctable), "ifnr", (gpointer)row);
	}
}


void
create_editable_entry (NWInterface_t * piface, GtkWidget * attach_to,
		       gchar * wprefix, gchar * ltext, gchar * wdata,
		       gchar * tooltip, gint clnr)
{
	GtkWidget *label;
	gchar wname[100];
	
	guint gpe_boxspacing = gpe_get_boxspacing ();

	label = gtk_label_new (ltext);
	gtk_tooltips_set_tip (tooltips, label, tooltip, NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (attach_to), label, 0, 1, clnr, clnr + 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	label = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, label, tooltip, NULL);
	strcpy (wname, wprefix);
	strcat (wname, piface->name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_steal_data (G_OBJECT (notebook), wname);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_entry_set_text (GTK_ENTRY (label), wdata);
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (attach_to), label, 1, 2, clnr, clnr + 1,
			  (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
}

void
create_editable_entry_simple (GtkWidget * attach_to, gchar * name,
			      gchar * ltext, gchar * wdata, gchar * tooltip,
			      gint clnr)
{
	GtkWidget *label;
	guint gpe_boxspacing = gpe_get_boxspacing ();

	label = gtk_label_new (ltext);
	gtk_tooltips_set_tip (tooltips, label, tooltip, NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (attach_to), label, 0, 1, clnr, clnr + 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	label = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, label, tooltip, NULL);
	gtk_widget_set_name (GTK_WIDGET (label), name);
	gtk_widget_ref (label);
	g_object_steal_data (G_OBJECT (notebook), name);
	g_object_set_data_full (G_OBJECT (notebook), name, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	if (wdata)
		gtk_entry_set_text (GTK_ENTRY (label), wdata);
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (attach_to), label, 1, 2, clnr, clnr + 1,
			  (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
}

char *key2text(char* key)
{
	if ((key[0] == 's') && (key[1] == ':'))
		return &key[2];
	return key;
}

#define IW_ENCODING_TOKEN_MAX	32	/* 256 bits (for now) */


char *text2key(char* text, char* buffer)
{
	char *	buff;
	char *	hex;
	char *	out;
	char *	p;
	int     ishex; // if true, this is a valid hex-code
	char    temp[3];
	int     keylen = 0;
	int     count; 
	ishex = TRUE;
	
	if (strlen(text) == 0) return text;
	
	/* Third case : as hexadecimal digits */
	buff = malloc(IW_ENCODING_TOKEN_MAX + strlen(text) + 1);
	if(buff == NULL)
	{
		fprintf(stderr, "Malloc failed (string too long ?)\n");
	
		return text;
	}
	/* Preserve original buffers (both in & out) */
	hex = buff + IW_ENCODING_TOKEN_MAX;
	strcpy(hex, text);
	out = buff;

	/* Parse */
	p = strtok(hex, "-:;.,");
	while((p != (char *) NULL) && (keylen < IW_ENCODING_TOKEN_MAX))
	{
		int	temph;
		int	templ;
		int	count;
		int	len;
		/* Get each char separatly (and not by two) so that we don't
		* get confused by 'enc' (=> '0E'+'0C') and similar */
		count = sscanf(p, "%1X%1X", &temph, &templ);
		if(count < 1)
		{
			ishex = FALSE;
			break;
		}
		/* Fixup odd strings such as '123' is '01'+'23' and not '12'+'03'*/
		len = strlen(p);
		if(len % 2)
			count = 1;
		/* Put back two chars as one byte */
		if(count == 2)
			templ |= temph << 4;
		else
			templ = temph;
		out[keylen++] = (unsigned char) (templ & 0xFF);
		/* Check where to get next char from */
		if(len > count)	/* Token not finished yet */
			p += count;
		else
			p = strtok((char *) NULL, "-:;.,");
	}
	if (!ishex)
	{
		sprintf(buffer, "s:%s", text);
		return buffer;
	} else
	{
		if ((keylen == 5) || (keylen == 13) || (keylen == 29))
		{
			buffer[0] = '\0';
			
			for (count = 0; count < keylen; count++)
			{
				sprintf(temp, "%02x", out[count]);
				strcat(buffer, temp);
				if ((count & 0x01) == 1) strcat(buffer, "-");
			}
			free (buff);
			return buffer;
		}
		sprintf(buffer, "s:%s", text);
		free(buff);
		return buffer;
      }
}



void
show_wificonfig(GtkWidget *window, NWInterface_t *iface)
{
	GtkWidget *dialog, *btnok;
	GtkWidget *scrolledwindow;
	GtkWidget *label, *rb, *container, *ctable;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border = gpe_get_border ();

	gchar *tmpval1, *tmpval2;
	gint response;
	char  tmp_key[256];
	
	dialog = gtk_dialog_new_with_buttons (_("WiFi config"),
					NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL,
					GTK_RESPONSE_REJECT,
					NULL);
	
	/* set window parameters depending on screen size */
	if (gdk_screen_height() > 400)
	{
		gtk_window_set_default_size(GTK_WINDOW(dialog), 240, 320);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW (window));
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	}
	else
		gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_NORMAL);
	
	btnok = gtk_dialog_add_button(GTK_DIALOG(dialog), 
	                              GTK_STOCK_OK, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS(btnok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(btnok);
	
	// page headers

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), 
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
				
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scrolledwindow, TRUE, TRUE, 0);
	
	ctable = gtk_table_new (8, 2, FALSE);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwindow), ctable);

	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	tmpval1 = g_strdup_printf (_("WiFi config for %s"), iface->name);
	tmpval2 = g_strdup_printf ("<b>%s</b>", tmpval1);
	gtk_label_set_markup (GTK_LABEL (label), tmpval2);
	g_free (tmpval1);
  g_free (tmpval2);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	// page items  
	
	label = gtk_label_new (_("ESSID"));
	gtk_tooltips_set_tip (tooltips, label, help_essid, NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	label = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, label, help_essid, NULL);
	gtk_widget_set_name (GTK_WIDGET (label), "essid");
	gtk_widget_ref (label);
	g_object_steal_data (G_OBJECT (notebook), "essid");
	g_object_set_data_full (G_OBJECT (notebook), "essid" , label,
				  (GtkDestroyNotify) gtk_widget_unref);
	
	gtk_entry_set_text (GTK_ENTRY (label), iface->essid);
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (ctable), label, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
			       
	label = gtk_label_new(_("Mode"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	gtk_table_attach (GTK_TABLE (ctable), label, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	
	container = gtk_hbox_new (TRUE, 0);
	
	label = gtk_radio_button_new_with_label_from_widget (NULL, _("managed"));
	gtk_tooltips_set_tip (tooltips, label, help_wifimode, NULL);

	gtk_widget_set_name (GTK_WIDGET (label), "mode_managed");
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), "mode_managed", label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface->mode == MODE_MANAGED ? TRUE : FALSE);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), _("ad-hoc"));
	gtk_tooltips_set_tip (tooltips, label, help_wifimode, NULL);

	gtk_widget_set_name (GTK_WIDGET (label), "mode_adhoc");
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), "mode_adhoc", label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface->mode != MODE_MANAGED ? TRUE : FALSE);
	gtk_container_add (GTK_CONTAINER (container), label);

	gtk_table_attach (GTK_TABLE (ctable), container, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);


	label = gtk_label_new (_("Channel"));
	gtk_tooltips_set_tip (tooltips, label, help_channel, NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 1, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	label = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, label, help_essid, NULL);
	gtk_widget_set_name (GTK_WIDGET (label), "channel");
	gtk_widget_ref (label);
	g_object_steal_data (G_OBJECT (notebook), "channel");
	g_object_set_data_full (G_OBJECT (notebook), "channel" , label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_entry_set_text (GTK_ENTRY (label), iface->channel);
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (ctable), label, 1, 2, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	label = gtk_label_new(_("WEP"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	gtk_table_attach (GTK_TABLE (ctable), label, 0, 1, 4, 5,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	
	container = gtk_hbox_new (FALSE, 0);
	
	label = gtk_radio_button_new_with_label_from_widget (NULL, _("off"));
	gtk_tooltips_set_tip (tooltips, label, help_encmode, NULL);

	gtk_widget_set_name (GTK_WIDGET (label), "enc_off");
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), "enc_off", label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface->encmode == ENC_OFF ? TRUE : FALSE);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), _("open"));
	gtk_tooltips_set_tip (tooltips, label, help_encmode, NULL);

	gtk_widget_set_name (GTK_WIDGET (label), "enc_open");
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), "enc_open", label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface->encmode == ENC_OPEN ? TRUE : FALSE);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), _("restricted"));
	gtk_tooltips_set_tip (tooltips, label, help_encmode, NULL);

	gtk_widget_set_name (GTK_WIDGET (label), "enc_restricted");
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), "enc_restricted", label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface->encmode == ENC_RESTRICTED ? TRUE : FALSE);
	gtk_container_add (GTK_CONTAINER (container), label);

	gtk_table_attach (GTK_TABLE (ctable), container, 1, 2, 4, 5,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	rb = gtk_radio_button_new_with_label_from_widget (NULL, _("Key 1"));
	gtk_tooltips_set_tip (tooltips, rb, help_selectkey, NULL);
	gtk_widget_set_name (GTK_WIDGET (rb), "key1select");
	gtk_widget_ref (rb);
	g_object_set_data_full (G_OBJECT (notebook), "key1select", rb,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rb),
				      iface->keynr == 1 ? TRUE : FALSE);

	gtk_table_attach (GTK_TABLE (ctable), rb, 0, 1, 5, 6,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	label = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, label, help_wepkey, NULL);
	gtk_widget_set_name (GTK_WIDGET (label), "key1");
	gtk_widget_ref (label);
	g_object_steal_data (G_OBJECT (notebook), "key1");
	g_object_set_data_full (G_OBJECT (notebook), "key1" , label,
				  (GtkDestroyNotify) gtk_widget_unref);
	
	gtk_entry_set_text (GTK_ENTRY (label), key2text(iface->key[0]));
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (ctable), label, 1, 2, 5, 6,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	rb = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rb), _("Key 2"));
	gtk_tooltips_set_tip (tooltips, rb, help_selectkey, NULL);
	gtk_widget_set_name (GTK_WIDGET (rb), "key2select");
	gtk_widget_ref (rb);
	g_object_set_data_full (G_OBJECT (notebook), "key2select", rb,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rb),
				      iface->keynr == 2 ? TRUE : FALSE);

	gtk_table_attach (GTK_TABLE (ctable), rb, 0, 1, 6, 7,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	label = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, label, help_wepkey, NULL);
	gtk_widget_set_name (GTK_WIDGET (label), "key2");
	gtk_widget_ref (label);
	g_object_steal_data (G_OBJECT (notebook), "key2");
	g_object_set_data_full (G_OBJECT (notebook), "key2" , label,
				  (GtkDestroyNotify) gtk_widget_unref);
	
	gtk_entry_set_text (GTK_ENTRY (label), key2text(iface->key[1]));
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (ctable), label, 1, 2, 6, 7,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	rb = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rb), _("Key 3"));
	gtk_tooltips_set_tip (tooltips, rb, help_selectkey, NULL);
	gtk_widget_set_name (GTK_WIDGET (rb), "key3select");
	gtk_widget_ref (rb);
	g_object_set_data_full (G_OBJECT (notebook), "key3select", rb,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rb),
				      iface->keynr == 3 ? TRUE : FALSE);

	gtk_table_attach (GTK_TABLE (ctable), rb, 0, 1, 7, 8,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
	label = gtk_entry_new ();
	gtk_tooltips_set_tip (tooltips, label, help_wepkey, NULL);
	gtk_widget_set_name (GTK_WIDGET (label), "key3");
	gtk_widget_ref (label);
	g_object_steal_data (G_OBJECT (notebook), "key3");
	g_object_set_data_full (G_OBJECT (notebook), "key3" , label,
				  (GtkDestroyNotify) gtk_widget_unref);
	
	gtk_entry_set_text (GTK_ENTRY (label), key2text(iface->key[2]));
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (ctable), label, 1, 2, 7, 8,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	if (gdk_screen_height() > 400)
	{		
		rb = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON(rb), _("Key 4"));
		gtk_tooltips_set_tip (tooltips, rb, help_selectkey, NULL);
		gtk_widget_set_name (GTK_WIDGET (rb), "key4select");
		gtk_widget_ref (rb);
		g_object_set_data_full (G_OBJECT (notebook), "key4select", rb,
					  (GtkDestroyNotify) gtk_widget_unref);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rb),
						  iface->keynr == 4 ? TRUE : FALSE);
	
	
		gtk_table_attach (GTK_TABLE (ctable), rb, 0, 1, 8, 9,
				  (GtkAttachOptions) (GTK_FILL),
				  (GtkAttachOptions) (GTK_FILL),
				  gpe_boxspacing, gpe_boxspacing);
		label = gtk_entry_new ();
		gtk_tooltips_set_tip (tooltips, label, help_wepkey, NULL);
		gtk_widget_set_name (GTK_WIDGET (label), "key4");
		gtk_widget_ref (label);
		g_object_steal_data (G_OBJECT (notebook), "key4");
		g_object_set_data_full (G_OBJECT (notebook), "key4" , label,
					  (GtkDestroyNotify) gtk_widget_unref);
		
		gtk_entry_set_text (GTK_ENTRY (label), key2text(iface->key[3]));
		gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
		gtk_table_attach (GTK_TABLE (ctable), label, 1, 2, 8, 9,
				  (GtkAttachOptions) (GTK_FILL),
				  (GtkAttachOptions) (GTK_FILL),
				  gpe_boxspacing, gpe_boxspacing);
	}
			       
	gtk_widget_show_all(dialog);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	
	if (response == GTK_RESPONSE_OK)
	{
		label = g_object_get_data (G_OBJECT (notebook), "essid");
		strncpy(iface->essid, gtk_editable_get_chars(GTK_EDITABLE (label), 0, -1), 31);
		
		label = g_object_get_data (G_OBJECT (notebook), "channel");
		strncpy(iface->channel, gtk_editable_get_chars(GTK_EDITABLE (label), 0, -1), 31);		
		label = g_object_get_data (G_OBJECT (notebook), "key1");
		strncpy(iface->key[0], text2key(gtk_editable_get_chars(GTK_EDITABLE (label), 0, -1), tmp_key), 127);
		
		label = g_object_get_data (G_OBJECT (notebook), "key2");
		strncpy(iface->key[1], text2key(gtk_editable_get_chars(GTK_EDITABLE (label), 0, -1), tmp_key), 127);
		
		label = g_object_get_data (G_OBJECT (notebook), "key3");
		strncpy(iface->key[2], text2key(gtk_editable_get_chars(GTK_EDITABLE (label), 0, -1), tmp_key), 127);
		
		if (gdk_screen_height() > 400)
		{		
			label = g_object_get_data (G_OBJECT (notebook), "key4");
			strncpy(iface->key[3], text2key(gtk_editable_get_chars(GTK_EDITABLE (label), 0, -1), tmp_key), 127);
		}
		label = g_object_get_data (G_OBJECT (notebook), "mode_managed");
		iface->mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (label)) ? MODE_MANAGED : MODE_ADHOC;
		
		label = g_object_get_data (G_OBJECT (notebook), "enc_off");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (label)))
		{
			iface->encmode = ENC_OFF; 
		} else 
		{
			label = g_object_get_data (G_OBJECT (notebook), "enc_open");
			iface->encmode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (label)) ? ENC_OPEN : ENC_RESTRICTED;
		}
		
		label = g_object_get_data (G_OBJECT (notebook), "key1select");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (label)))
		{
			iface->keynr = 1;
		} else
		{
			label = g_object_get_data (G_OBJECT (notebook), "key2select");
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (label)))
			{
				iface->keynr = 2;
			} else
			{
				label = g_object_get_data (G_OBJECT (notebook), "key3select");
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (label)))
				{
					iface->keynr = 3;
				} else 
					iface->keynr = 4;
			}
		}
		if (strlen(iface->essid) == 0) strcpy(iface->essid, "any");
	
	}

	gtk_widget_destroy (dialog);
}


void
changed_wifi (GtkToggleButton * togglebutton, gpointer user_data)
{
	GtkWidget *widget, *page;
	gchar wname[100];
	gint ifnr;
	
	page = gtk_notebook_get_nth_page(GTK_NOTEBOOK (notebook), 
	                                 gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook)));
	ifnr = (gint)g_object_get_data(G_OBJECT(page), "ifnr");

	strcpy (wname, "wificonfig");
	strcat (wname, iflist[ifnr].name);
	
	iflist[ifnr].iswireless = gtk_toggle_button_get_active(togglebutton);
	
	widget = g_object_get_data (G_OBJECT (togglebutton), wname);
	
	gtk_widget_set_sensitive(widget, iflist[ifnr].iswireless);
}

void
clicked_wificonfig (GtkButton *button, gpointer user_data)
{
	GtkWidget *page;
	gint ifnr;
	
	page = gtk_notebook_get_nth_page(GTK_NOTEBOOK (notebook), 
	                                 gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook)));
	ifnr = (gint)g_object_get_data(G_OBJECT(page), "ifnr");

	show_wificonfig(gtk_widget_get_toplevel(GTK_WIDGET(button)), &iflist[ifnr]);
}

GtkWidget *
create_nwstatic_widgets (NWInterface_t iface)
{
	GtkWidget *label, *container, *ctable, *togglebutton;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border = gpe_get_border ();

	gchar wname[100];
	gchar *tmpval;

	// page headers

	ctable = gtk_table_new (8, 4, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(ctable), gpe_boxspacing);
	gtk_table_set_row_spacings(GTK_TABLE(ctable), gpe_boxspacing);
	
	container = gtk_hbox_new (TRUE, gpe_boxspacing);

	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 0, 2, 1, 2,
			  GTK_FILL, GTK_FILL, 0, 0);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	tmpval = g_strdup_printf ("<b>%s %s</b>",_("Interface"), iface.name);
	gtk_label_set_markup (GTK_LABEL (label), tmpval);
	g_free (tmpval);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 2, 0, 1, GTK_FILL,
			  GTK_FILL, 0, 0);

	label = gtk_radio_button_new_with_label_from_widget (NULL, "static");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "static");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isstatic);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "dhcp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "dhcp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isdhcp);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "ppp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "ppp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isppp);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);


	// page items  
	create_editable_entry (&iface, ctable, "address", _("Address"),
			       iface.address, 
				   _("Enter your IP address here, e.g. 192.168.1.2"), 2);
	create_editable_entry (&iface, ctable, "netmask", _("Netmask"),
			       iface.netmask, 
				   _("Enter your netmask here, e.g. 255.255.255.0 in most cases"),
			       3);
	create_editable_entry (&iface, ctable, "broadcast", _("Broadcast"),
			       iface.broadcast,
			       _("Enter your broadcast address here, usually the same like your IP with 255 as last number."),
			       4);
	create_editable_entry (&iface, ctable, "gateway", _("Gateway"),
			       iface.gateway,
			       _("Enter the IP Address of your default gateway here."),
			       5);
			       
	container = gtk_hbox_new (TRUE, 0);
	gtk_table_attach (GTK_TABLE (ctable), container, 0, 2, 7, 8, GTK_FILL,
			  GTK_FILL, 0, 0);


#ifndef NO_WIFI
	togglebutton = gtk_check_button_new_with_label(_("WiFi device"));
	gtk_container_add(GTK_CONTAINER(container), togglebutton);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton), iface.iswireless);
	strcpy (wname, "wifi");
	strcat (wname, iface.name);
	gtk_widget_set_name(togglebutton, wname);
	gtk_widget_ref (togglebutton);

	label = gtk_button_new_with_label(_("Configure"));
	gtk_button_set_image(GTK_BUTTON(label), 
	                     gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, 
						                          GTK_ICON_SIZE_BUTTON));
	gtk_container_add(GTK_CONTAINER(container), label);
	gtk_widget_set_sensitive(label, iface.iswireless);
	strcpy (wname, "wificonfig");
	strcat (wname, iface.name);
	gtk_widget_set_name(label, wname);
	
	g_object_set_data_full (G_OBJECT (togglebutton), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	g_signal_connect (G_OBJECT (togglebutton), "toggled",
			    G_CALLBACK (changed_wifi), NULL);
	g_signal_connect (G_OBJECT (label), "clicked", 
			    G_CALLBACK (clicked_wificonfig), NULL);
	gtk_widget_ref (label);
	gtk_tooltips_set_tip (tooltips, togglebutton, help_wifi, NULL);
	gtk_tooltips_set_tip (tooltips, label, help_wificonfig, NULL);
#endif
	return ctable;
}


GtkWidget *
create_nwdhcp_widgets (NWInterface_t iface)
{
	GtkWidget *label, *container, *ctable, *togglebutton;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border = gpe_get_border ();

	gchar wname[100];
	gchar *tmpval;
	// page headers

	ctable = gtk_table_new (7, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(ctable), gpe_boxspacing);
	gtk_table_set_row_spacings(GTK_TABLE(ctable), gpe_boxspacing);
	container = gtk_hbox_new (TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 0, 2, 1, 2,
			  GTK_FILL, GTK_FILL, 0, 0);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	tmpval = g_strdup_printf ("<b>%s %s</b>", _("Interface"), iface.name);
	gtk_label_set_markup (GTK_LABEL (label), tmpval);
	g_free (tmpval);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 2, 0, 1, 
	                  GTK_FILL, GTK_FILL, 0, 0);

	label = gtk_radio_button_new_with_label_from_widget (NULL, "static");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "static");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isstatic);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "dhcp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "dhcp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isdhcp);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "ppp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "ppp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isppp);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);


	// page items  
	create_editable_entry (&iface, ctable, "hostname", _("Hostname"),
			       iface.hostname,
			       _("Enter your desired hostname here. This parameter is optional."),
			       2);
	container = gtk_hbox_new (TRUE, 0);
	gtk_table_attach (GTK_TABLE (ctable), container, 0, 2, 6, 7,
			          GTK_FILL, GTK_FILL, 0, 0);

#ifndef NO_WIFI
	togglebutton = gtk_check_button_new_with_label(_("WiFi device"));
	gtk_container_add(GTK_CONTAINER(container), togglebutton);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton), iface.iswireless);
	strcpy (wname, "wifi");
	strcat (wname, iface.name);
	gtk_widget_set_name(togglebutton, wname);
	gtk_widget_ref (togglebutton);

	label = gtk_button_new_with_label(_("Configure"));
	gtk_button_set_image(GTK_BUTTON(label), 
	                     gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, 
						                          GTK_ICON_SIZE_BUTTON));
	gtk_container_add(GTK_CONTAINER(container), label);
	gtk_widget_set_sensitive(label, iface.iswireless);
	strcpy (wname, "wificonfig");
	strcat (wname, iface.name);
	gtk_widget_set_name(label, wname);
	
	g_object_set_data_full (G_OBJECT (togglebutton), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	g_signal_connect (G_OBJECT (togglebutton), "toggled",
			    G_CALLBACK (changed_wifi), NULL);

	g_signal_connect (G_OBJECT (label), "clicked", 
			    G_CALLBACK (clicked_wificonfig), NULL);

	gtk_widget_ref (label);
	gtk_tooltips_set_tip (tooltips, togglebutton, help_wifi, NULL);
	gtk_tooltips_set_tip (tooltips, label, help_wificonfig, NULL);
#endif	       
	return ctable;
}


GtkWidget *
create_nwppp_widgets (NWInterface_t iface)
{
	GtkWidget *label, *container, *ctable;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border = gpe_get_border ();

	gchar wname[100];
	gchar *tmpval;

	// page headers

	ctable = gtk_table_new (3, 7, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(ctable), gpe_boxspacing);
	gtk_table_set_row_spacings(GTK_TABLE(ctable), gpe_boxspacing);
	container = gtk_hbox_new (TRUE, gpe_boxspacing);

	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 0, 2, 1, 2,
			          GTK_FILL, GTK_FILL, 0, 0);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	tmpval = g_strdup_printf ("<b>%s %s</b>",_("Interface"), iface.name);
	gtk_label_set_markup (GTK_LABEL (label), tmpval);
	g_free (tmpval);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 2, 0, 1,
			          GTK_FILL, GTK_FILL, 0, 0);

	label = gtk_radio_button_new_with_label_from_widget (NULL, "static");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "static");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isstatic);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "dhcp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "dhcp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isdhcp);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "ppp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "ppp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	g_object_set_data_full (G_OBJECT (notebook), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isppp);
	g_signal_connect (G_OBJECT (label), "toggled",
			    G_CALLBACK (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);


	// page items  
	create_editable_entry (&iface, ctable, "provider", _("Provider"),
			       iface.provider,
			       _("Here you need to enter the name of the provider " \
				   "configuration to use for this interface."),
			       2);
	return ctable;
}


GtkWidget *
create_global_widgets ()
{
	gchar *tmpval;

	GtkWidget *label, *ctable;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border = gpe_get_border ();

	// page headers
	ctable = gtk_table_new (2, 7, FALSE);

	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	tmpval = g_strdup_printf ("<b>%s</b>", _("Global Settings"));
	gtk_label_set_markup (GTK_LABEL (label), tmpval);
	g_free (tmpval);

	gtk_table_attach (GTK_TABLE (ctable), label, 0, 3, 0, 1,
			  (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	// which config file to use
	if (!getuid ())		// if we are root we change global config file
	{
		cfgfile = malloc (20 * sizeof (gchar));
		sprintf (cfgfile, "%s", "/etc/profile");
	}
	else
	{
		cfgfile =
			g_strdup_printf ("%s/.profile",
					 g_get_home_dir ());
		if (access (cfgfile, F_OK))
		{

			gchar *content;
			FILE *fnew;
			gsize length;
			GError *err = NULL;

			if (g_file_get_contents ("/etc/profile", &content,
				                     &length, &err))
			{
				fnew = fopen (cfgfile, "w");
				fprintf (fnew, "%s", content);
				fclose (fnew);
				g_free (content);
			}
		}
	}

	// proxy, global setting
	tmpval = get_file_var (cfgfile, "export http_proxy");
	create_editable_entry_simple (ctable, "proxy", _("Proxy"), tmpval,
				      _("If you want/need to use a proxy, enter it here. "\
	                  "(This only applies to dillo for now.)"),
				      1);
	g_free (tmpval);

	tmpval = get_file_var (cfgfile, "export no_proxy");
	create_editable_entry_simple (ctable, "no_proxy", _("No proxy for"),
				      tmpval,
				      _("Here you should enter your local domain for that you" \
	                   " don't want to use a proxy."),
				      2);
	g_free (tmpval);

	// nameserver
	tmpval = get_file_var ("/etc/resolv.conf", "nameserver");
	create_editable_entry_simple (ctable, "nameserver", _("DNS server"),
				      tmpval,
				      _("Enter the IP of the DNS(name-) server to use here."),
				      5);
	g_free (tmpval);

	// system-hostname
	gchar *content;
	gchar **lines;
	gsize length;
	gchar *delim;
	gint i = 0;
	gchar *hostfile = "/etc/hostname";

	GError *err = NULL;

	delim = g_strdup ("\n");
	if (!g_file_get_contents (hostfile, &content, &length, &err))
	{
        	fprintf(stderr,"Could not access file: %s\n", hostfile);
        	return NULL;
	}
	lines = g_strsplit (content, delim, 2048);
	g_free (delim);
	delim = NULL;
	g_free (content);
	tmpval = lines[i];

	create_editable_entry_simple (ctable, "system-hostname", _("Hostname"),
				      tmpval,
				      _("Enter the Hostname to use for this machine"),
				      3);
	g_free (tmpval);

	label = g_object_get_data (G_OBJECT (notebook), "nameserver");
	gtk_widget_set_sensitive (label, have_access);

	return ctable;
}

void 
notebook_change_tab(GtkNotebook *notebook, GtkNotebookPage *page,
                    guint page_num, gpointer user_data)
{
	GtkWidget *delbutton = g_object_get_data(G_OBJECT(notebook), "deletebutton");

	gtk_widget_set_sensitive(delbutton, (page_num == 0) ? FALSE : TRUE);
}


void
Network_Free_Objects ()
{
	set_file_open (FALSE);
	gtk_widget_destroy (notebook);
}


void
Network_Save ()
{
	gint sect;
	GtkWidget *entry;
	gchar wname[100];
	gchar *newval;

	// traverse all...
	for (sect = 0; sect < iflen; sect++)
	{
		strcpy (wname, "address");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].address, newval);
		}
		strcpy (wname, "netmask");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].netmask, newval);
		}
		strcpy (wname, "network");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].network, newval);
		}
		strcpy (wname, "gateway");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].gateway, newval);
		}
		strcpy (wname, "broadcast");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].broadcast, newval);
		}
		strcpy (wname, "hostname");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].hostname, newval);
		}
		strcpy (wname, "provider");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].provider, newval);
		}
		strcpy (wname, "static");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			iflist[sect].isstatic =
				gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON (entry));
		}
		strcpy (wname, "dhcp");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			iflist[sect].isdhcp =
				gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON (entry));
		}
		strcpy (wname, "ppp");
		strcat (wname, iflist[sect].name);
		entry = g_object_get_data (G_OBJECT (notebook), wname);
		if (entry)
		{
			iflist[sect].isppp =
				gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON (entry));
		}
	}

	// save interface data to file
	write_sections ();
	set_file_open (FALSE);

	// copy and activate interface config
	suid_exec ("CPIF", "CPIF");

	entry = g_object_get_data (G_OBJECT (notebook), "proxy");
	if (entry)
	{
		newval = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
		change_cfg_value (cfgfile, "export http_proxy", newval, '=');
	}
	entry = g_object_get_data (G_OBJECT (notebook), "no_proxy");
	if (entry)
	{
		newval = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
		change_cfg_value (cfgfile, "export no_proxy", newval, '=');
	}
	g_free (cfgfile);

	// save nameserver
	strcpy (wname, "nameserver");
	entry = g_object_get_data (G_OBJECT (notebook), wname);
	if (entry)
	{
		newval = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
		if (strlen(newval))
			suid_exec ("SDNS", newval);
		g_free(newval);
	}

	//save system-hostname
	entry = g_object_get_data (G_OBJECT (notebook), "system-hostname");
	if (entry)
	{
		newval = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

		if (strlen(newval))
			suid_exec ("HOST", newval);
		g_free(newval);
	}
}

void
Network_Restore ()
{
	set_file_open (FALSE);
}


GtkWidget *
Network_Build_Objects (gboolean ignore, GtkWidget *toolbar)
{
	GtkWidget *label, *ctable, *tablebox;
	gint row = 0;
	gint num_int = 0;
	GtkToolItem *item, *delbutton;

	help_devtype =
		_("Here you may change the basic configuration type of "
		  "your device.\nUse \"static\" for manual interface "
		  "configuration, \"dhcp\" to use DHCP and \"ppp\" to make "
		  "the device a point-to-point device.");
	
	help_wifi = 
		_("Enable this option if your device is a WiFi device");
	help_wificonfig = 
		_("Click to set the wireless options for this device");
	help_wifimode = 
		_("Set the mode your WiFi card should work. (managed or ad-hoc).");

	help_encmode = 
		_("Set the enrcyption mode: off for no WEP encryption, "
	          "open for open system and restricted for resticted WEP "
		  "connections only.");
		  
	help_essid = 
		_("Enter the essid of your WiFi network here.");
	
	help_wepkey = 
		_("Enter your WEP key here in (iwconfig format), e.g.: \"s:12345\"");
		
	help_selectkey =
		_("Select the WEP key to use.");
		
	help_channel =
		_("Enter the channel number or frequency if needed. If not needed leave this field blank.");
	
	have_access = (access (NET_CONFIGFILE, W_OK) == 0);
	if (!have_access)
		have_access = !suid_exec ("CHEK", " ");

	tooltips = gtk_tooltips_new ();

	tablebox = gtk_vbox_new (FALSE, 0);
	g_object_set_data (G_OBJECT (tablebox), "tooltips", tooltips);

	/* insert toolbar items */
	delbutton = item = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(remove_interface), 
	                 NULL);
	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
	                     _("Remove current Interface"), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
	if (!have_access)
		gtk_widget_set_sensitive (GTK_WIDGET(item), FALSE);
    gtk_widget_show(GTK_WIDGET(item));
	
	item = gtk_tool_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(add_interface), 
	                 NULL);
	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
	                     _("Add a new Interface"), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
	if (!have_access)
		gtk_widget_set_sensitive (GTK_WIDGET(item), FALSE);
    gtk_widget_show(GTK_WIDGET(item));
	
	item = gtk_tool_button_new_from_stock(GTK_STOCK_DIALOG_INFO);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("Info"));
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(show_current_config), 
	                 NULL);
	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
	                     _("Show current configuration."), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
    gtk_widget_show(GTK_WIDGET(item));
	
   	// create tabbed notebook
	// this contains lookup list!
	notebook = gtk_notebook_new ();
	gtk_notebook_set_scrollable (GTK_NOTEBOOK(notebook), TRUE);

	g_object_set_data (G_OBJECT (notebook), "deletebutton", delbutton);
	g_object_set_data (G_OBJECT (notebook), "notebook", notebook);
	g_signal_connect (G_OBJECT(notebook), "switch-page", G_CALLBACK(notebook_change_tab), NULL);

	gtk_container_set_border_width (GTK_CONTAINER (notebook), 2);	//gpe_border

	gtk_container_add (GTK_CONTAINER (tablebox), notebook);

	if (!set_file_open (TRUE))
		gpe_error_box (_("Couldn't read network configuration."));
	else
	{
		num_int = get_scheme_list ();
	}
	// create and add globals section
	ctable = create_global_widgets ();
	label = gtk_label_new (_("global"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET (ctable),
				  label);

	// add interface widgets
	for (row = 0; row < num_int; row++)
	{
		ctable = NULL;
		if (iflist[row].isstatic)
			ctable = create_nwstatic_widgets (iflist[row]);
		if (iflist[row].isdhcp)
			ctable = create_nwdhcp_widgets (iflist[row]);
		if (iflist[row].isppp)
			ctable = create_nwppp_widgets (iflist[row]);
		if (ctable)
		{
			if (!have_access)
				gtk_widget_set_sensitive (ctable, FALSE);
			label = gtk_label_new (iflist[row].name);
			iflist[row].uipos = gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			                                              GTK_WIDGET (ctable), 
			                                              label);
			g_object_set_data(G_OBJECT(ctable), "ifnr", (gpointer)row);
		}
		else
			iflist[row].uipos = -1;
	}
	return tablebox;
}
