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
 * Dynamic interface configuration added by Florian Boor (florian.boor@kernelconcepts.de)
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
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE		/* Pour GlibC2 */
#endif
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
#include <gpe/render.h>
#include <gpe/question.h>
#include <gpe/smallbox.h>
#include <gpe/gtksimplemenu.h>

/* offset for page to interface */
#define PAGE_OFFSET 1


extern NWInterface_t *iflist;
extern gint iflen;
extern GtkWidget *mainw;
GtkWidget *table;
GtkWidget *create_nwstatic_widgets (NWInterface_t iface);
GtkWidget *create_nwdhcp_widgets (NWInterface_t iface);
GtkWidget *create_nwppp_widgets (NWInterface_t iface);

static guint not_added = 0;
static gchar *cfgfile;
static gboolean have_access = FALSE;
static GtkTooltips *tooltips;

static const char *help_devtype;

static void
show_current_config (GtkWidget * button)
{
	char *buffer = g_strdup ("");
	char *tmp = NULL;
	struct interface *ife;
	struct interface *int_list;
	GtkWidget *w, *label;

	int_list = if_getlist ();

	for (ife = int_list; ife; ife = ife->next)
	{
		if ((ife->flags & IFF_UP) && !(ife->flags & IFF_LOOPBACK))
		{
			tmp = if_to_infostr (ife);
			buffer = realloc (buffer,
					  strlen (tmp) + strlen (buffer) + 1);
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

	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), buffer);
	gtk_label_set_selectable(GTK_LABEL(label),TRUE);
	gtk_widget_show (label);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (w)->vbox), label);
	g_signal_connect_swapped (GTK_OBJECT (w),
				  "response",
				  G_CALLBACK (gtk_widget_destroy),
				  GTK_OBJECT (w));

	gtk_dialog_run (GTK_DIALOG (w));

	free (buffer);
}


static GList *
get_unconfigured_interfaces ()
{
	GList *result = NULL;
	struct if_nameindex *ifnames;
	int i, j;
	gboolean found;

	ifnames = if_nameindex ();
	for (j = 0; ifnames[j].if_name != NULL; j++)
	{
		found = FALSE;
		for (i = 0; i < iflen; i++)
		{
			if (!strcmp (ifnames[j].if_name, iflist[i].name))
			{
				found = TRUE;
				break;
			}
		}
		if (!found)
		{
			result = g_list_append (result,
						g_strdup (ifnames[j].
							  if_name));
		}
	}
	if_freenameindex (ifnames);
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

	ifbox[0].suggestions = get_unconfigured_interfaces ();

	if (ifbox[0].suggestions == NULL)
	{
		ifbox[0].suggestions =
			g_list_append (ifbox[0].suggestions,
				       g_strdup ("eth0"));
		ifbox[0].suggestions =
			g_list_append (ifbox[0].suggestions,
				       g_strdup ("wlan0"));
		ifbox[0].suggestions =
			g_list_append (ifbox[0].suggestions,
				       g_strdup ("bnep0"));
	}

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
			i = iflen-1;
		}
		else
			i = existing;
		
		ctable = create_nwstatic_widgets (iflist[i]);
		if (ctable)
		{
			label = gtk_label_new (iflist[i].name);
			gtk_notebook_append_page (GTK_NOTEBOOK (table),
						  GTK_WIDGET (ctable), label);
			gtk_widget_show_all (table);
		}
		else
			not_added++;
		gtk_notebook_set_current_page (GTK_NOTEBOOK (table), -1);
	}
}

static void
remove_interface (GtkWidget * widget, gpointer d)
{
	G_CONST_RETURN gchar *ifname;
	GtkWidget *page;

	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (table),
					  gtk_notebook_get_current_page
					  (GTK_NOTEBOOK (table)));
	ifname = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (table), page);
	if (gpe_question_ask
	    (_("Do you want to delete this interface?"), _("Question"),
	     "question", "!gtk-no", NULL, "!gtk-yes", NULL, NULL))
	{
		iflist[get_section_nr (ifname)].status = NWSTATE_REMOVED;
		gtk_notebook_remove_page (GTK_NOTEBOOK (table),
					  gtk_notebook_get_current_page
					  (GTK_NOTEBOOK (table)));
	}
}


void
changed_nwtype (GtkToggleButton * togglebutton, gpointer user_data)
{
	GtkWidget *ctable, *label;
	gchar wname[100];
	gint row =
		gtk_notebook_get_current_page (GTK_NOTEBOOK (table)) +
		not_added - PAGE_OFFSET;

	if (!gtk_toggle_button_get_active (togglebutton))
		return;		// just run once

	// look who called us...
	strcpy (wname, "static");
	strcat (wname, iflist[row].name);
	if (gtk_object_get_data (GTK_OBJECT (table), wname) == togglebutton)
	{
		iflist[row].isstatic = TRUE;
		iflist[row].isdhcp = FALSE;
		iflist[row].isppp = FALSE;
	}
	strcpy (wname, "dhcp");
	strcat (wname, iflist[row].name);
	if (gtk_object_get_data (GTK_OBJECT (table), wname) == togglebutton)
	{
		iflist[row].isstatic = FALSE;
		iflist[row].isdhcp = TRUE;
		iflist[row].isppp = FALSE;
	}
	strcpy (wname, "ppp");
	strcat (wname, iflist[row].name);
	if (gtk_object_get_data (GTK_OBJECT (table), wname) == togglebutton)
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
		gtk_notebook_remove_page (GTK_NOTEBOOK (table),
					  gtk_notebook_get_current_page
					  (GTK_NOTEBOOK (table)));
		gtk_notebook_insert_page (GTK_NOTEBOOK (table),
					  GTK_WIDGET (ctable), label,
					  row - not_added + PAGE_OFFSET);
		gtk_widget_show_all (table);
		gtk_notebook_set_page (GTK_NOTEBOOK (table),
				       row - not_added + PAGE_OFFSET);
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
	gtk_object_remove_data (GTK_OBJECT (table), wname);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_entry_set_text (GTK_ENTRY (label), wdata);
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (attach_to), label, 1, 2, clnr, clnr + 1,
			  (GtkAttachOptions) (GTK_FILL),
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
	gtk_object_remove_data (GTK_OBJECT (table), name);
	gtk_object_set_data_full (GTK_OBJECT (table), name, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	if (wdata)
		gtk_entry_set_text (GTK_ENTRY (label), wdata);
	gtk_entry_set_editable (GTK_ENTRY (label), TRUE);
	gtk_table_attach (GTK_TABLE (attach_to), label, 1, 2, clnr, clnr + 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);
}


GtkWidget *
create_nwstatic_widgets (NWInterface_t iface)
{
	GtkWidget *label, *container, *ctable;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border = gpe_get_border ();

	gchar wname[100];
	gchar *tmpval;

	// page headers

	ctable = gtk_table_new (3, 7, FALSE);

	container = gtk_hbox_new (TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	tmpval = g_strdup_printf ("<b>%s</b>", iface.name);
	gtk_label_set_markup (GTK_LABEL (label), tmpval);
	g_free (tmpval);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	label = gtk_radio_button_new_with_label_from_widget (NULL, "static");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "static");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isstatic);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "dhcp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "dhcp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isdhcp);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "ppp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "ppp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isppp);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);


	// page items  
	create_editable_entry (&iface, ctable, "address", _("Address"),
			       iface.address,
			       _
			       ("Enter your IP address here, e.g. 192.168.1.2"),
			       1);
	create_editable_entry (&iface, ctable, "netmask", _("Netmask"),
			       iface.netmask,
			       _
			       ("Enter your netmask here, e.g. 255.255.255.0 in most cases"),
			       2);
	create_editable_entry (&iface, ctable, "broadcast", _("Broadcast"),
			       iface.broadcast,
			       _
			       ("Enter your broadcast address here, usually the same like your IP with 255 as last number."),
			       3);
	create_editable_entry (&iface, ctable, "gateway", _("Gateway"),
			       iface.gateway,
			       _
			       ("Enter the IP Address of your default gateway here."),
			       5);
	return ctable;
}


GtkWidget *
create_nwdhcp_widgets (NWInterface_t iface)
{
	GtkWidget *label, *container, *ctable;
	guint gpe_boxspacing = gpe_get_boxspacing ();
	guint gpe_border = gpe_get_border ();

	gchar wname[100];
	gchar *tmpval;
	// page headers

	ctable = gtk_table_new (3, 7, FALSE);
	container = gtk_hbox_new (TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	tmpval = g_strdup_printf ("<b>%s</b>", iface.name);
	gtk_label_set_markup (GTK_LABEL (label), tmpval);
	g_free (tmpval);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	label = gtk_radio_button_new_with_label_from_widget (NULL, "static");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "static");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isstatic);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "dhcp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "dhcp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isdhcp);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "ppp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "ppp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isppp);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);


	// page items  
	create_editable_entry (&iface, ctable, "hostname", _("Hostname"),
			       iface.hostname,
			       _
			       ("Enter your desired hostname here. This parameter is optional."),
			       1);
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
	container = gtk_hbox_new (TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (ctable), gpe_border);

	gtk_table_attach (GTK_TABLE (ctable), container, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	tmpval = g_strdup_printf ("<b>%s</b>", iface.name);
	gtk_label_set_markup (GTK_LABEL (label), tmpval);
	g_free (tmpval);
	gtk_table_attach (GTK_TABLE (ctable), label, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL),
			  gpe_boxspacing, gpe_boxspacing);

	label = gtk_radio_button_new_with_label_from_widget (NULL, "static");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "static");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isstatic);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "dhcp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "dhcp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label),
				      iface.isdhcp);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);

	label = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON
							     (label), "ppp");
	gtk_tooltips_set_tip (tooltips, label, help_devtype, NULL);
	strcpy (wname, "ppp");
	strcat (wname, iface.name);
	gtk_widget_set_name (GTK_WIDGET (label), wname);
	gtk_widget_ref (label);
	gtk_object_set_data_full (GTK_OBJECT (table), wname, label,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (label), iface.isppp);
	gtk_signal_connect (GTK_OBJECT (label), "toggled",
			    GTK_SIGNAL_FUNC (changed_nwtype), NULL);
	gtk_container_add (GTK_CONTAINER (container), label);


	// page items  
	create_editable_entry (&iface, ctable, "provider", _("Provider"),
			       iface.provider,
			       _("Here you need to enter the name of the provider " \
				   "configuration to use for this interface."),
			       1);
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
		sprintf (cfgfile, "%s", "/etc/dillorc");
	}
	else
	{
		cfgfile =
			g_strdup_printf ("%s/.dillo/dillorc",
					 g_get_home_dir ());
		if (access (cfgfile, F_OK))
		{

			gchar *content;
			FILE *fnew;
			gint length;
			GError *err = NULL;

			mkdir (g_strdup_printf
			       ("%s/.dillo", g_get_home_dir ()), S_IRWXU);
			g_file_get_contents ("/etc/dillorc", &content,
					     &length, &err);
			fnew = fopen (cfgfile, "w");
			fprintf (fnew, "%s", content);
			fclose (fnew);
			g_free (content);
		}
	}

	//proxy for dillo
	tmpval = get_file_var (cfgfile, "http_proxy");
	create_editable_entry_simple (ctable, "proxy", _("Proxy"), tmpval,
				      _("If you want/need to use a proxy, enter it here. "\
	                  "(This only applies to dillo for now.)"),
				      1);
	g_free (tmpval);

	tmpval = get_file_var (cfgfile, "no_proxy");
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
				      _
				      ("Enter the IP of the DNS(name-) server to use here."),
				      5);
	g_free (tmpval);

	label = gtk_object_get_data (GTK_OBJECT (table), "nameserver");
	gtk_widget_set_sensitive (label, have_access);

	return ctable;
}


void
Network_Free_Objects ()
{
	set_file_open (FALSE);
	gtk_widget_destroy (table);
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
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].address, newval);
		}
		strcpy (wname, "netmask");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].netmask, newval);
		}
		strcpy (wname, "network");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].network, newval);
		}
		strcpy (wname, "gateway");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].gateway, newval);
		}
		strcpy (wname, "broadcast");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].broadcast, newval);
		}
		strcpy (wname, "hostname");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].hostname, newval);
		}
		strcpy (wname, "provider");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			newval = gtk_editable_get_chars (GTK_EDITABLE (entry),
							 0, -1);
			strcpy (iflist[sect].provider, newval);
		}
		strcpy (wname, "static");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			iflist[sect].isstatic =
				gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON (entry));
		}
		strcpy (wname, "dhcp");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
		if (entry)
		{
			iflist[sect].isdhcp =
				gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON (entry));
		}
		strcpy (wname, "ppp");
		strcat (wname, iflist[sect].name);
		entry = gtk_object_get_data (GTK_OBJECT (table), wname);
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

	entry = gtk_object_get_data (GTK_OBJECT (table), "proxy");
	if (entry)
	{
		newval = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
		change_cfg_value (cfgfile, "http_proxy", newval, '=');
	}
	entry = gtk_object_get_data (GTK_OBJECT (table), "no_proxy");
	if (entry)
	{
		newval = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
		change_cfg_value (cfgfile, "no_proxy", newval, '=');
	}
	g_free (cfgfile);

	// save nameserver
	strcpy (wname, "nameserver");
	entry = gtk_object_get_data (GTK_OBJECT (table), wname);
	if (entry)
	{
		newval = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
		suid_exec ("SDNS", newval);
	}
}

void
Network_Restore ()
{
	set_file_open (FALSE);
}


GtkWidget *
Network_Build_Objects ()
{
	GtkWidget *label, *ctable, *tablebox, *toolbar;
	gint row = 0;
	gint num_int = 0;
	GtkWidget *button;

	help_devtype =
		_("Here you may change the basic configuration type of "
		  "your device.\nUse \"static\" for manual interface "
		  "configuration, \"dhcp\" to use DHCP and \"ppp\" to make "
		  "the device a point-to-point device.");

	have_access = (access (NET_CONFIGFILE, W_OK) == 0);
	if (!have_access)
		have_access = !suid_exec ("CHEK", "");

	tooltips = gtk_tooltips_new ();

	tablebox = gtk_vbox_new (FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (tablebox), "tooltips", tooltips);

	// create toolbar
	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
				     GTK_ORIENTATION_HORIZONTAL);

	gtk_widget_set_name (toolbar, "toolbar");
	gtk_widget_ref (toolbar);
	gtk_widget_show (toolbar);
	gtk_box_pack_start (GTK_BOX (tablebox), toolbar, FALSE, FALSE, 0);

	label = gtk_image_new_from_pixbuf (gpe_find_icon ("new"));
	button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
					  _("Add Interface"),
					  _("Add Interface"),
					  _("Add a new Interface"), label,
					  (GtkSignalFunc) add_interface,
					  NULL);
	if (!have_access)
		gtk_widget_set_sensitive (button, FALSE);

	label = gtk_image_new_from_pixbuf (gpe_find_icon ("delete"));
	button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
					  _("Remove Interface"),
					  _("Remove Interface"),
					  _("Remove current Interface"),
					  label,
					  (GtkSignalFunc) remove_interface,
					  NULL);
	if (!have_access)
		gtk_widget_set_sensitive (button, FALSE);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

	label = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
					  gtk_toolbar_get_icon_size
					  (GTK_TOOLBAR (toolbar)));
	button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
					  _("Information"),
					  _("Show current configuration."),
					  _("Show current configuration."),
					  label,
					  (GtkSignalFunc) show_current_config,
					  NULL);

	// chreate tabbed notebook
	// this contains lookup list!
	table = gtk_notebook_new ();

	gtk_object_set_data (GTK_OBJECT (table), "table", table);
	gtk_widget_set_name (GTK_WIDGET (table), "table");

	gtk_container_set_border_width (GTK_CONTAINER (table), 2);	//gpe_border

	gtk_container_add (GTK_CONTAINER (tablebox), table);

	if (!set_file_open (TRUE))
		gpe_error_box (_("Couldn't read network configuration."));
	else
	{
		num_int = get_scheme_list ();
	}
	// create and add globals section
	ctable = create_global_widgets ();
	label = gtk_label_new (_("global"));
	gtk_notebook_append_page (GTK_NOTEBOOK (table), GTK_WIDGET (ctable),
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
			gtk_notebook_append_page (GTK_NOTEBOOK (table),
						  GTK_WIDGET (ctable), label);
		}
		else
			not_added++;	// we'll run into trouble if we have a loopback device between other in interfaces
	}
	return tablebox;
}
