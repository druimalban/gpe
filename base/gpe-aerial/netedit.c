/*
 * gpe-aerial (c) 2003 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * Networks editor module.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gpe/spacing.h>
#include <gpe/errorbox.h>
#include <libintl.h>
#include "netedit.h"

#define _(x) gettext(x)

extern GtkWidget *devices_window;

static GtkWidget *lGateway;
static GtkWidget *eGateway;
static GtkWidget *lSubnet;
static GtkWidget *eSubnet;
static GtkWidget *lIP;
static GtkWidget *eIP;
static GtkWidget *tbDhcp;
static GtkWidget *tbWEP;
static GtkWidget *lWEPKey;
static GtkWidget *eWEPKey;
static GtkWidget *eSsid;
static GtkWidget *eChannel;
static GtkWidget *rbTypeAdHoc;


static void
ok_clicked (GtkWidget * w, GtkWidget * window)
{
	netinfo_t *ni;
	char *tmp;
	int t;
	unsigned char i1, i2, i3, i4;
	
	ni = g_object_get_data(G_OBJECT(window),"netinfo");
	if (!ni) return;
		
	tmp = gtk_editable_get_chars(GTK_EDITABLE(eSsid),0,-1);
	if (strcmp(ni->netinfo.ssid,tmp))
	{
		snprintf(ni->netinfo.ssid,32,tmp);
		ni->netinfo.userset |= USET_SSID;
	}
	free(tmp);
	
	t = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rbTypeAdHoc));
	if (t != ni->netinfo.mode)
	{
		ni->netinfo.mode = t;
		ni->netinfo.userset |= USET_MODE;
	}
	t = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tbWEP));
	if (t != ni->netinfo.wep)
	{
		ni->netinfo.wep = t;
		ni->netinfo.userset |= USET_WEP;
	}
	t = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tbDhcp));
	if (t != ni->netinfo.dhcp)
	{
		ni->netinfo.dhcp = t;
		ni->netinfo.userset |= USET_DHCP;
	}
	t = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(eChannel));
	if (t != ni->netinfo.channel)
	{
		ni->netinfo.channel = t;
		ni->netinfo.userset |= USET_CHANNEL;
	}
	tmp = gtk_editable_get_chars(GTK_EDITABLE(eWEPKey),0,-1);
	if (strcmp(ni->netinfo.wep_key,tmp))
	{
		snprintf(ni->netinfo.wep_key,32,tmp);
		ni->netinfo.userset |= USET_WEPKEY;
	}
	free(tmp);
	
	tmp = gtk_editable_get_chars(GTK_EDITABLE(eIP),0,-1);
	if (strlen(tmp) && (sscanf(tmp,"%hhu.%hhu.%hhu.%hhu",&i1,&i2,&i3,&i4) < 4))
	{
		gpe_error_box(_("The IP address you entered is not valid."));
		return;
	}
	else
	{
		if (strlen(tmp))
		{
			ni->netinfo.userset |= USET_IP;
			ni->netinfo.ip[0] = i1;
			ni->netinfo.ip[1] = i2;
			ni->netinfo.ip[2] = i3;
			ni->netinfo.ip[3] = i4;
		}
	}
	free(tmp);
	
	tmp = gtk_editable_get_chars(GTK_EDITABLE(eSubnet),0,-1);
	if (strlen(tmp) && (sscanf(tmp,"%hhu.%hhu.%hhu.%hhu",&i1,&i2,&i3,&i4) < 4))
	{
		gpe_error_box(_("The subnet mask address you entered is not valid."));
		return;
	}
	else
	{
		if (strlen(tmp))
		{
			ni->netinfo.userset |= USET_NETMASK;
			ni->netinfo.netmask[0] = i1;
			ni->netinfo.netmask[1] = i2;
			ni->netinfo.netmask[2] = i3;
			ni->netinfo.netmask[3] = i4;
		}
	}
	free(tmp);
	
	tmp = gtk_editable_get_chars(GTK_EDITABLE(eGateway),0,-1);
	if (strlen(tmp) && (sscanf(tmp,"%hhu.%hhu.%hhu.%hhu",&i1,&i2,&i3,&i4) < 4))
	{
		gpe_error_box(_("The gateway address you entered is not valid."));
		return;
	}
	else
	{
		if (strlen(tmp))
		{
			ni->netinfo.userset |= USET_GATEWAY;
			ni->netinfo.gateway[0] = i1;
			ni->netinfo.gateway[1] = i2;
			ni->netinfo.gateway[2] = i3;
			ni->netinfo.gateway[3] = i4;
		}
	}
	free(tmp);
	
	gtk_widget_destroy(window);
}

static void 
dhcp_activated()
{
	gboolean state;
	
	state = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tbDhcp));
	gtk_widget_set_sensitive(lGateway,state);	
	gtk_widget_set_sensitive(eGateway,state);	
	gtk_widget_set_sensitive(lSubnet,state);	
	gtk_widget_set_sensitive(eSubnet,state);	
	gtk_widget_set_sensitive(lIP,state);	
	gtk_widget_set_sensitive(eIP,state);	
}


static void 
wep_activated()
{
	gboolean state;
	
	state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tbWEP));
	gtk_widget_set_sensitive(lWEPKey,state);	
	gtk_widget_set_sensitive(eWEPKey,state);	
}


void
network_edit (netinfo_t * ni)
{
	char *tmp;
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *hsep = gtk_hseparator_new();
	GtkWidget *sw = gtk_scrolled_window_new(NULL,NULL);
	GtkWidget *vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());
	GtkWidget *hbox = gtk_hbutton_box_new();
	GtkWidget *lname = gtk_label_new (NULL);
	GtkWidget *lSsid = gtk_label_new(_("SSID"));
	GtkWidget *laddr = gtk_label_new (NULL);
	GtkWidget *bOK = gtk_button_new_from_stock (GTK_STOCK_OK);
	GtkWidget *bCancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	GtkWidget *lType = gtk_label_new (_("Mode"));
	GtkWidget *rbTypeManaged = gtk_radio_button_new_with_label(NULL,_("Managed"));
	GtkWidget *lChannel = gtk_label_new (_("Channel"));
	GtkWidget *lWEP = gtk_label_new (_("WEP enabled"));
	GtkWidget *lDhcp = gtk_label_new (_("Use DHCP"));
	GtkWidget *table = gtk_table_new (6, 3, FALSE);
	
	rbTypeAdHoc = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rbTypeManaged),_("Ad-Hoc"));
	eChannel = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(1.0,1.0,14.0,1.0,1.0,1.0)),1.0,0);
	eSsid = gtk_entry_new();
	tbDhcp = gtk_check_button_new();
	lGateway = gtk_label_new (_("Gateway"));
	eGateway = gtk_entry_new ();
	lSubnet = gtk_label_new (_("Subnet mask"));
	eSubnet = gtk_entry_new ();
	lIP = gtk_label_new (_("IP Address"));
	eIP = gtk_entry_new ();
	tbWEP = gtk_check_button_new();
	lWEPKey = gtk_label_new (_("WEP-Key"));
	eWEPKey = gtk_entry_new ();

	g_object_set_data(G_OBJECT(window),"netinfo",ni);
	gtk_window_set_title (GTK_WINDOW (window), _("Network editor"));
	gpe_set_window_icon (GTK_WIDGET (window), "gpe-aerial");
	
	gtk_container_set_border_width (GTK_CONTAINER (vbox), gpe_get_border());
	gtk_table_set_row_spacings (GTK_TABLE (table), gpe_get_boxspacing());
	gtk_table_set_col_spacings (GTK_TABLE (table), gpe_get_boxspacing());
  
	tmp = g_strdup_printf ("<b>%s</b>", ni->netinfo.ssid);
	gtk_label_set_markup (GTK_LABEL (lname), tmp);
	g_free (tmp);

	tmp = g_strdup_printf ("BSSID: %s", ni->netinfo.bssid);
	gtk_label_set_markup (GTK_LABEL (laddr), tmp);
	g_free (tmp);
	
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(eChannel),1.0,14.0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	
	gtk_misc_set_alignment (GTK_MISC (lname), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lSsid), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (laddr), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lType), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lChannel), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lWEP), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lSubnet), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lDhcp), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lIP), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lGateway), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (lWEPKey), 0.0, 0.5);

	
	gtk_table_attach(GTK_TABLE (table), lname, 0, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), laddr, 0, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lSsid, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), eSsid, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lType, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), rbTypeManaged, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), rbTypeAdHoc, 2, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lChannel, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), eChannel, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lWEP, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), tbWEP, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lDhcp, 0, 1, 6, 7, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), tbDhcp, 1, 2, 6, 7, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lIP, 0, 1, 7, 8, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), eIP, 1, 3, 7, 8, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lSubnet, 0, 1, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), eSubnet, 1, 3, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lGateway, 0, 1, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), eGateway, 1, 3, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), lWEPKey, 0, 1, 11, 12, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE (table), eWEPKey, 1, 3, 11, 12, GTK_FILL, GTK_FILL, 0, 0);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw),vbox);
	gtk_container_add(GTK_CONTAINER(window),sw);
	
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hsep, FALSE, TRUE, 0);
				
	gtk_box_pack_start(GTK_BOX (hbox), bCancel, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (hbox), bOK, FALSE, TRUE, 0);

	gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	gtk_widget_realize (window);

	g_signal_connect (G_OBJECT (tbDhcp), "toggled",
				  G_CALLBACK (dhcp_activated), NULL);
	g_signal_connect (G_OBJECT (tbWEP), "toggled",
				  G_CALLBACK (wep_activated), NULL);
				  
	/* fill in values */
	
	gtk_entry_set_text(GTK_ENTRY(eSsid),ni->netinfo.ssid);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbTypeAdHoc),ni->netinfo.mode);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(eChannel),(gfloat)ni->netinfo.channel);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tbWEP),ni->netinfo.wep);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tbDhcp),ni->netinfo.dhcp);
	tmp=g_strdup_printf("%hhu.%hhu.%hhu.%hhu",ni->netinfo.ip[0],ni->netinfo.ip[1],ni->netinfo.ip[2],ni->netinfo.ip[3]);
	gtk_entry_set_text(GTK_ENTRY(eIP),tmp);
	free(tmp);
	tmp=g_strdup_printf("%hhu.%hhu.%hhu.%hhu",ni->netinfo.netmask[0],ni->netinfo.netmask[1],ni->netinfo.netmask[2],ni->netinfo.netmask[3]);
	gtk_entry_set_text(GTK_ENTRY(eSubnet),tmp);
	free(tmp);
	tmp=g_strdup_printf("%hhu.%hhu.%hhu.%hhu",ni->netinfo.gateway[0],ni->netinfo.gateway[1],ni->netinfo.gateway[2],ni->netinfo.gateway[3]);
	gtk_entry_set_text(GTK_ENTRY(eGateway),tmp);
	free(tmp);
	gtk_entry_set_text(GTK_ENTRY(eWEPKey),ni->netinfo.wep_key);
	
	gtk_widget_show_all (window);
	
	/* update enabled widgets */
	dhcp_activated();
	wep_activated();
	
	if (strlen(ni->netinfo.ssid))
	{
		gtk_widget_hide(lSsid);
		gtk_widget_hide(eSsid);
	}
		
	g_signal_connect (G_OBJECT (bOK), "clicked",
				  G_CALLBACK (ok_clicked), window);
	g_signal_connect_swapped (G_OBJECT (bCancel), "clicked",
				  G_CALLBACK (gtk_widget_destroy), window);
}
