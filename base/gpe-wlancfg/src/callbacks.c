#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <ctype.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include <stdlib.h>
#include <string.h>

extern GtkWidget *GPE_WLAN;
static gboolean HasChanged = FALSE;
static gint SchemeListSelRow = -1;
static gint SchemeListLastSelection;

Scheme_t *CurrentScheme;
gint sc; /*	Scheme counter */

void
on_GeneralHelp_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;
GtkWidget *MainWin;

	MainWin=lookup_widget(GTK_WIDGET(button),"GPE_WLANCFG");
	HelpWin=create_GeneralHelpWin ();
	gtk_window_set_transient_for(GTK_WINDOW(HelpWin),GTK_WINDOW(MainWin));
	gtk_widget_show(HelpWin);
}


void
on_GeneralHelpOK_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;

	HelpWin=lookup_widget(GTK_WIDGET(button),"GeneralHelpWin");
	gtk_widget_hide(HelpWin);
	gtk_widget_destroy(HelpWin);
}


gboolean
on_HelpWin_de_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
 
	
	gtk_widget_hide(widget);
	gtk_widget_destroy(widget);

	return TRUE;
}


gboolean
on_GPE_WLANCFG_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *WinWidget;
GtkWidget *SchemeList;

	SchemeList=lookup_widget(GPE_WLAN,"SchemeList");
	WinWidget=lookup_widget(GPE_WLAN,"PseudoMain");
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(WinWidget)) == 1) {
		if (HasChanged) {
			WinWidget=create_SaveChanges();
			gtk_window_set_transient_for(GTK_WINDOW(WinWidget),GTK_WINDOW(GPE_WLAN));
			gtk_widget_show(WinWidget);
		} else {
			WinWidget=lookup_widget(GPE_WLAN,"PseudoMain");
			gtk_notebook_set_page(GTK_NOTEBOOK(WinWidget),0);
		}
	} else {
			int i;
			for (i=0;i<sc;i++)
				memcpy(&schemelist[i],gtk_clist_get_row_data(GTK_CLIST(SchemeList),i),sizeof(Scheme_t));

			//kc-or: write_sections(schemelist,sc);
			write_back_configfile(WLAN_CONFIGFILE, schemelist, sc);
			gtk_main_quit();
	}

return TRUE;
}


void
on_SaveChangesYes_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget 	*SaveDialog;
GtkWidget 	*widget;
GtkWidget 	*menu;
GtkWidget 	*SchemeList;
gchar 		*ListEntry[4];
int 		i;
gchar 		tempmode[20];
gchar		tempchannel[20];
gchar		tempfreq[20];
gchar		temprate[20];
	
	HasChanged = FALSE;
	SaveDialog=lookup_widget(GTK_WIDGET(button),"SaveChanges");

	SchemeList=lookup_widget(GPE_WLAN,"SchemeList");
	for (i = 0; i < 4; i++) {
		ListEntry[i]=(gchar *)malloc(32 * sizeof(char));
		memset(ListEntry[i],0,32);
	}

	/* suck values */

	//kc-or: Added line-checking... fixed bug in widget lookup
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"Scheme");
	if (strcmp(CurrentScheme->Scheme, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_Scheme]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Scheme]=LINE_NEW;
		strcpy(CurrentScheme->Scheme, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"Instance");
	if (strcmp(CurrentScheme->Instance, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_Instance]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Instance]=LINE_NEW;
		strcpy(CurrentScheme->Instance, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"Socket");
	if (strcmp(CurrentScheme->Socket, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_Socket]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Socket]=LINE_NEW;
		strcpy(CurrentScheme->Socket, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"HWAddress");
	if (strcmp(CurrentScheme->HWAddress, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_HWAddress]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_HWAddress]=LINE_NEW;
		strcpy(CurrentScheme->HWAddress, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"Info");
	if (strcmp(CurrentScheme->Info, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_Info]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Info]=LINE_NEW;
		strcpy(CurrentScheme->Info, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ESSID");
	if (strcmp(CurrentScheme->ESSID, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_ESSID]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_ESSID]=LINE_NEW;
		strcpy(CurrentScheme->ESSID, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"NWID");
	if (strcmp(CurrentScheme->NWID, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_NWID]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_NWID]=LINE_NEW;
		strcpy(CurrentScheme->NWID, gtk_entry_get_text(GTK_ENTRY(widget)));
	}

	menu=GTK_WIDGET(gtk_option_menu_get_menu(GTK_OPTION_MENU(lookup_widget(GTK_WIDGET(GPE_WLAN),"Mode"))));
	widget=gtk_menu_get_active(GTK_MENU(menu));
	i = g_list_index (GTK_MENU_SHELL (menu)->children, widget);
	switch(i)
	{
		case 0:
			strcpy(tempmode, "AD-HOC");
		break;
		case 1:
			strcpy(tempmode, "MANAGED");
		break;
		case 2:
			strcpy(tempmode, "MASTER");
 		break;
		case 3:
			strcpy(tempmode, "REPEATER");
		break;
 		case 4:
			strcpy(tempmode, "SECONDARY");
 		break;
		case 5:
		default:	
			strcpy(tempmode, "AUTO");
 		break;
	}
	
	if (strcmp(CurrentScheme->Mode, tempmode)!=0)
	{
		if (CurrentScheme->lines[L_Mode]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Mode]=LINE_NEW;
		strcpy(CurrentScheme->Mode, tempmode);
	}

	strcpy(tempchannel, "");
	strcpy(tempfreq, "");

	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ChannelFreq");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) 
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"Frequency");
		strcpy(tempfreq,gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	else
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ChannelChan");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) 
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ChannelNr");
			strcpy(tempchannel,gtk_entry_get_text(GTK_ENTRY(widget)));			
		}
	}

	if (strcmp(CurrentScheme->Channel, tempchannel)!=0)
	{
		if (CurrentScheme->lines[L_Channel]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Channel]=LINE_NEW;
		strcpy(CurrentScheme->Channel, tempchannel);
	}

	if (strcmp(CurrentScheme->Frequency, tempfreq)!=0)
	{
		if (CurrentScheme->lines[L_Frequency]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Frequency]=LINE_NEW;
		strcpy(CurrentScheme->Frequency, tempfreq);
	}
	
	menu=GTK_WIDGET(gtk_option_menu_get_menu(GTK_OPTION_MENU(lookup_widget(GTK_WIDGET(GPE_WLAN),"Rate"))));
	widget=gtk_menu_get_active(GTK_MENU(menu));
	i = g_list_index (GTK_MENU_SHELL (menu)->children, widget);
	switch(i)
	{
		case 0:
			strcpy(temprate, "auto");
		break;
		case 1:
			strcpy(temprate, "1M");
		break;
		case 2:
			strcpy(temprate, "2M");
 		break;
		case 5:
		default:	
			strcpy(temprate, "11M");
 		break;
	}

	if (strcmp(CurrentScheme->Rate, temprate)!=0)
	{
		if (CurrentScheme->lines[L_Rate]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Rate]=LINE_NEW;
		strcpy(CurrentScheme->Rate, temprate);
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"EncryptionOn");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_key1");
		if (strcmp(CurrentScheme->key1, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			if (CurrentScheme->lines[L_key1]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key1]=LINE_NEW;
			strcpy(CurrentScheme->key1, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}
		
		widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_key2");
		if (strcmp(CurrentScheme->key2, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			if (CurrentScheme->lines[L_key2]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key2]=LINE_NEW;
			strcpy(CurrentScheme->key2, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_key3");
		if (strcmp(CurrentScheme->key3, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			if (CurrentScheme->lines[L_key3]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key3]=LINE_NEW;
			strcpy(CurrentScheme->key3, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_key4");
		if (strcmp(CurrentScheme->key4, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			if (CurrentScheme->lines[L_key4]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key4]=LINE_NEW;
			strcpy(CurrentScheme->key4, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ActiveKeyNr");
		if (strcmp(CurrentScheme->ActiveKey, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			if (CurrentScheme->lines[L_ActiveKey]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_ActiveKey]=LINE_NEW;
			strcpy(CurrentScheme->ActiveKey, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}

	}
	else
	{
		if (strcmp(CurrentScheme->ActiveKey, "0")!=0)
		{
			if (CurrentScheme->lines[L_ActiveKey]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_ActiveKey]=LINE_NEW;
			strcpy(CurrentScheme->ActiveKey, "0");	
		}
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"KeysStrings");

	if (CurrentScheme->KeyFormat!=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		if (CurrentScheme->lines[L_KeyFormat]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_KeyFormat]=LINE_NEW;
		CurrentScheme->KeyFormat=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));		
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_iwconfig");
	if (strcmp(CurrentScheme->iwconfig, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_iwconfig]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwconfig]=LINE_NEW;
		strcpy(CurrentScheme->iwconfig, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_iwspy");
	if (strcmp(CurrentScheme->iwspy, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_iwspy]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwspy]=LINE_NEW;
		strcpy(CurrentScheme->iwspy, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_iwpriv");
	if (strcmp(CurrentScheme->iwpriv, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_iwpriv]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwpriv]=LINE_NEW;
		strcpy(CurrentScheme->iwpriv, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_rts");
	if (strcmp(CurrentScheme->rts, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_rts]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_rts]=LINE_NEW;
		strcpy(CurrentScheme->rts, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_frag");
	if (strcmp(CurrentScheme->frag, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_frag]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_frag]=LINE_NEW;
		strcpy(CurrentScheme->frag, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLAN),"ed_sens");
	if (strcmp(CurrentScheme->sens, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		if (CurrentScheme->lines[L_sens]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_sens]=LINE_NEW;
		strcpy(CurrentScheme->sens, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	
	// kc-or: clist besser doch erst hier updaten :)
	if (SchemeListSelRow == -1) {
		/* New entry */
		strcpy(ListEntry[0],CurrentScheme->Scheme);
		strcpy(ListEntry[1],CurrentScheme->Socket);
		strcpy(ListEntry[2],CurrentScheme->Instance);
		strcpy(ListEntry[3],CurrentScheme->HWAddress);
		gtk_clist_append(GTK_CLIST(SchemeList),ListEntry);
		gtk_clist_set_row_data(GTK_CLIST(SchemeList), GTK_CLIST(SchemeList)->rows-1, (gpointer)CurrentScheme);
	} else {
		/* Update existing entry */
		gtk_clist_set_text(GTK_CLIST(SchemeList),SchemeListSelRow,0,CurrentScheme->Scheme);
		gtk_clist_set_text(GTK_CLIST(SchemeList),SchemeListSelRow,1,CurrentScheme->Socket);
		gtk_clist_set_text(GTK_CLIST(SchemeList),SchemeListSelRow,2,CurrentScheme->Instance);
		gtk_clist_set_text(GTK_CLIST(SchemeList),SchemeListSelRow,3,CurrentScheme->HWAddress);
	}
	
	gtk_widget_hide(SaveDialog);
	gtk_widget_destroy(SaveDialog);
	widget=lookup_widget(GPE_WLAN,"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(widget),0);
}


void
on_SaveChangesNo_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *SaveDialog;
GtkWidget *widget;

	HasChanged = FALSE;
	SaveDialog=lookup_widget(GTK_WIDGET(button),"SaveChanges");
	gtk_widget_hide(SaveDialog);
	gtk_widget_destroy(SaveDialog);
//	free(CurrentScheme);
	widget=lookup_widget(GPE_WLAN,"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(widget),0);
}


void
on_RFParamsHelp_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;
GtkWidget *MainWin;

	MainWin=lookup_widget(GTK_WIDGET(button),"GPE_WLANCFG");
	HelpWin=create_RFParamsHelp ();
	gtk_window_set_transient_for(GTK_WINDOW(HelpWin),GTK_WINDOW(MainWin));
	gtk_widget_show(HelpWin);
}


void
on_RFParamsOK_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;

	HelpWin=lookup_widget(GTK_WIDGET(button),"RFParamsHelp");
	gtk_widget_hide(HelpWin);
	gtk_widget_destroy(HelpWin);
}


gboolean
on_SaveChanges_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	return TRUE;
}


void
on_WEPHelp_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;
GtkWidget *MainWin;

	MainWin=lookup_widget(GTK_WIDGET(button),"GPE_WLANCFG");
	HelpWin=create_WEPHelpWin ();
	gtk_window_set_transient_for(GTK_WINDOW(HelpWin),GTK_WINDOW(MainWin));
	gtk_widget_show(HelpWin);
}


void
on_KeyHelp_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;
GtkWidget *MainWin;

	MainWin=lookup_widget(GTK_WIDGET(button),"GPE_WLANCFG");
	HelpWin=create_EncKeysHelpWin ();
	gtk_window_set_transient_for(GTK_WINDOW(HelpWin),GTK_WINDOW(MainWin));
	gtk_widget_show(HelpWin);
}


void
on_ExpertHelp_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;
GtkWidget *MainWin;

	MainWin=lookup_widget(GTK_WIDGET(button),"GPE_WLANCFG");
	HelpWin=create_ExpertHelpWin ();
	gtk_window_set_transient_for(GTK_WINDOW(HelpWin),GTK_WINDOW(MainWin));
	gtk_widget_show(HelpWin);
}


void
on_WEPHelpOK_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;

	HelpWin=lookup_widget(GTK_WIDGET(button),"WEPHelpWin");
	gtk_widget_hide(HelpWin);
	gtk_widget_destroy(HelpWin);
}


void
on_EncKeysHelpOK_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;

	HelpWin=lookup_widget(GTK_WIDGET(button),"EncKeysHelpWin");
	gtk_widget_hide(HelpWin);
	gtk_widget_destroy(HelpWin);
}


void
on_ExpertHelpOK_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;

	HelpWin=lookup_widget(GTK_WIDGET(button),"ExpertHelpWin");
	gtk_widget_hide(HelpWin);
	gtk_widget_destroy(HelpWin);
}


void
on_ChannelDefault_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
GtkWidget *widget;

	HasChanged = TRUE;

	widget=lookup_widget(GTK_WIDGET(togglebutton),"Frequency");
	gtk_widget_set_sensitive(widget,FALSE);
	widget=lookup_widget(GTK_WIDGET(togglebutton),"ChannelNr");
	gtk_widget_set_sensitive(widget,FALSE);
}


void
on_ChannelFreq_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
GtkWidget *widget;

	HasChanged = TRUE;
	
	widget=lookup_widget(GTK_WIDGET(togglebutton),"Frequency");
	gtk_widget_set_sensitive(widget,TRUE);
	widget=lookup_widget(GTK_WIDGET(togglebutton),"ChannelNr");
	gtk_widget_set_sensitive(widget,FALSE);
}


void
on_ChannelChan_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
GtkWidget *widget;

	HasChanged = TRUE;

	widget=lookup_widget(GTK_WIDGET(togglebutton),"Frequency");
	gtk_widget_set_sensitive(widget,FALSE);
	widget=lookup_widget(GTK_WIDGET(togglebutton),"ChannelNr");
	gtk_widget_set_sensitive(widget,TRUE);
}


void
on_EncryptionOn_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	strcpy(CurrentScheme->Encryption,"on");
	HasChanged = TRUE;
}


void
on_EncryptionOff_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	strcpy(CurrentScheme->Encryption,"off");
	HasChanged = TRUE;
}


void
on_ModeOpen_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	strcpy(CurrentScheme->EncMode,"open");
	HasChanged = TRUE;
}


void
on_ModeRestricted_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	strcpy(CurrentScheme->EncMode,"shared");
	HasChanged = TRUE;
}


void
on_Rate_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}


void
on_Frequency_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;	
}


void
on_Channe_lNr_changed                  (GtkEditable     *editable,
                                        gpointer         user_data)
{
}


void
on_ActiveKeyNr_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}


void
on_KeysHex_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}


void
on_KeysStrings_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}


void
on_DeleteYes_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *DeleteVerify;
GtkWidget *SchemeList;
GtkWidget *widget;
Scheme_t  *Scheme;

	DeleteVerify=lookup_widget(GTK_WIDGET(button),"DeleteVerify");
	gtk_widget_hide(DeleteVerify);
	gtk_widget_destroy(DeleteVerify);
	SchemeList=lookup_widget(GPE_WLAN, "SchemeList");
	// free scheme here?
	// kc-or:  Free Scheme and marc scheme-dimensions to be deleted...
	Scheme = gtk_clist_get_row_data(GTK_CLIST(SchemeList), SchemeListSelRow);
	delete_list[delete_list_count][0] = Scheme->scheme_start;
	delete_list[delete_list_count][1] = Scheme->scheme_end;
	delete_list_count++;
	
	free(Scheme);
	// kc-or:  end...
	gtk_clist_remove(GTK_CLIST(SchemeList), SchemeListSelRow);
	widget=lookup_widget(GPE_WLAN,"SchemeDelete");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GPE_WLAN,"SchemeEdit");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GPE_WLAN,"ListItemUp");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GPE_WLAN,"ListItemDown");
	gtk_widget_set_sensitive(widget, FALSE);
	SchemeListSelRow = -1;
	sc--;
}


void
on_DeleteNo_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *DeleteVerify;
GtkWidget *SchemeList;

	DeleteVerify=lookup_widget(GTK_WIDGET(button),"DeleteVerify");
	gtk_widget_hide(DeleteVerify);
	gtk_widget_destroy(DeleteVerify);
	SchemeList=lookup_widget(GPE_WLAN, "SchemeList");
}


void
on_SchemeHelp_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;
GtkWidget *MainWin;

	MainWin=lookup_widget(GTK_WIDGET(button),"GPE_WLANCFG");
	HelpWin=create_SchemeHelpWin ();
	gtk_window_set_transient_for(GTK_WINDOW(HelpWin),GTK_WINDOW(MainWin));
	gtk_widget_show(HelpWin);
}


void
on_SchemeHelpOK_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *HelpWin;

	HelpWin=lookup_widget(GTK_WIDGET(button),"SchemeHelpWin");
	gtk_widget_hide(HelpWin);
	gtk_widget_destroy(HelpWin);
}


void
on_ListItemUp_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	if (SchemeListSelRow > 0) {
		GtkWidget *SchemeList;
		SchemeList=lookup_widget(GTK_WIDGET(button),"SchemeList");
		gtk_clist_row_move (GTK_CLIST(SchemeList), SchemeListSelRow, SchemeListSelRow - 1);
		SchemeListSelRow -= 1;
		if (SchemeListSelRow < (GTK_CLIST(SchemeList)->rows - 1)) {
			GtkWidget *ListItemDown;
			ListItemDown=lookup_widget(GTK_WIDGET(button),"ListItemDown");
			gtk_widget_set_sensitive(ListItemDown, TRUE);
		}
		if (SchemeListSelRow == 0) {
			GtkWidget *ListItemUp;
			ListItemUp=lookup_widget(GTK_WIDGET(button),"ListItemUp");
			gtk_widget_set_sensitive(ListItemUp, FALSE);
		}
	}
}


void
on_ListItemDown_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *SchemeList;

	SchemeList=lookup_widget(GTK_WIDGET(button),"SchemeList");
	if (SchemeListSelRow < (GTK_CLIST(SchemeList)->rows - 1)) {
		gtk_clist_row_move (GTK_CLIST(SchemeList), SchemeListSelRow, SchemeListSelRow + 1);
		SchemeListSelRow += 1;
		if (SchemeListSelRow == (GTK_CLIST(SchemeList)->rows - 1)) {
			GtkWidget *ListItemDown;
			ListItemDown=lookup_widget(GTK_WIDGET(button),"ListItemDown");
			gtk_widget_set_sensitive(ListItemDown, FALSE);
		}
		if (SchemeListSelRow > 0) {
			GtkWidget *ListItemUp;
			ListItemUp=lookup_widget(GTK_WIDGET(button),"ListItemUp");
			gtk_widget_set_sensitive(ListItemUp, TRUE);
		}
	}
}


void
on_SchemeDelete_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *DeleteVerify;

	DeleteVerify=create_DeleteVerify();
	gtk_window_set_transient_for(GTK_WINDOW(DeleteVerify),GTK_WINDOW(GPE_WLAN));
	gtk_widget_show(DeleteVerify);
}


void
on_SchemeNew_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *widget;

	widget=lookup_widget(GTK_WIDGET(button),"Frequency");
	gtk_widget_set_sensitive(widget,FALSE);
	widget=lookup_widget(GTK_WIDGET(button),"Channe_lNr");
 	gtk_widget_set_sensitive(widget,FALSE);

	CurrentScheme=(Scheme_t *)malloc(sizeof(Scheme_t));
	memset(CurrentScheme,0,sizeof(Scheme_t));
	CurrentScheme->IsRow = -1;
	strcpy(CurrentScheme->Scheme,"*");
	strcpy(CurrentScheme->Socket,"*");
	strcpy(CurrentScheme->Instance,"*");
	strcpy(CurrentScheme->HWAddress,"*");
	strcpy(CurrentScheme->NWID,"any");
	strcpy(CurrentScheme->Mode,"auto");
	strcpy(CurrentScheme->Rate,"11M");
	strcpy(CurrentScheme->Encryption,"off");
	strcpy(CurrentScheme->EncMode,"");
	strcpy(CurrentScheme->ActiveKey,"[1]");
	//kc-or: Zeilen als zwingend neu initialisieren...
	CurrentScheme->lines[L_Scheme]=LINE_NEW;
	CurrentScheme->lines[L_Socket]=LINE_NEW;
	CurrentScheme->lines[L_Instance]=LINE_NEW;
	CurrentScheme->lines[L_HWAddress]=LINE_NEW;
	CurrentScheme->lines[L_NWID]=LINE_NEW;
	CurrentScheme->lines[L_Mode]=LINE_NEW;
	CurrentScheme->lines[L_Rate]=LINE_NEW;
	CurrentScheme->lines[L_Encryption]=LINE_NEW;
	CurrentScheme->lines[L_EncMode]=LINE_NEW;
	CurrentScheme->lines[L_key1]=LINE_NEW;
	//kc-or: End
	
	
	HasChanged = TRUE;

	widget=lookup_widget(GTK_WIDGET(button),"SchemeList");
	gtk_clist_unselect_all(GTK_CLIST(widget));

	SchemeListSelRow = -1;
	sc++;
	
	widget=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(widget), 1);
}


void EditScheme(GtkWidget	*button)
{
	GtkWidget *SchemeList;
	GtkWidget *widget;
	gint i;

	SchemeList=lookup_widget(button, "SchemeList");
	
	CurrentScheme = (Scheme_t *) gtk_clist_get_row_data (GTK_CLIST(SchemeList), SchemeListLastSelection);

	widget=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(widget), 1);

	/* assign values */
	widget=lookup_widget(GTK_WIDGET(button),"Scheme");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Scheme);	
	widget=lookup_widget(GTK_WIDGET(button),"Instance");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Instance);	
	widget=lookup_widget(GTK_WIDGET(button),"Socket");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Socket);	
	widget=lookup_widget(GTK_WIDGET(button),"HWAddress");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->HWAddress);	
	widget=lookup_widget(GTK_WIDGET(button),"Info");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Info);	
	widget=lookup_widget(GTK_WIDGET(button),"ESSID");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->ESSID);	
	widget=lookup_widget(GTK_WIDGET(button),"NWID");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->NWID);

	widget=lookup_widget(GTK_WIDGET(button),"Mode");
	gtk_option_menu_set_history(GTK_OPTION_MENU(widget),5);
	for (i=0;i<strlen(CurrentScheme->Mode);i++)
		CurrentScheme->Mode[i]=toupper(CurrentScheme->Mode[i]);

	if (!strcmp("AD-HOC",CurrentScheme->Mode)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),0);
	if (!strcmp("MANAGED",CurrentScheme->Mode)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),1);
	if (!strcmp("MASTER",CurrentScheme->Mode)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),2);
	if (!strcmp("REPEATER",CurrentScheme->Mode)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),3);
	if (!strcmp("SECONDARY",CurrentScheme->Mode)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),4);
	if (!strcmp("AUTO",CurrentScheme->Mode)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),5);

	widget=lookup_widget(GTK_WIDGET(button),"Frequency");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Frequency);	

	widget=lookup_widget(GTK_WIDGET(button),"ChannelNr");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Channel);	

	if (strlen(CurrentScheme->Frequency) > 4) {
		widget=lookup_widget(GTK_WIDGET(button),"ChannelFreq");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
	}
	else
	{
		if (strlen(CurrentScheme->Channel)){
			widget=lookup_widget(GTK_WIDGET(button),"ChannelChan");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
		else
		{
			widget=lookup_widget(GTK_WIDGET(button),"ChannelDefault");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
	}
	widget=lookup_widget(GTK_WIDGET(button),"Rate");
	gtk_option_menu_set_history(GTK_OPTION_MENU(widget),3);

	for (i=0;i<strlen(CurrentScheme->Rate);i++)
		CurrentScheme->Rate[i]=toupper(CurrentScheme->Rate[i]);

	if (!strcmp("AUTO",CurrentScheme->Rate)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),0);
	if (!strcmp("1M",CurrentScheme->Rate)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),1);
	if (!strcmp("2M",CurrentScheme->Rate)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),2);
	if (!strcmp("11M",CurrentScheme->Rate)) 		
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),3);

	if (CurrentScheme->key1[0]){
		widget=lookup_widget(GTK_WIDGET(button),"EncryptionOn");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
	}
	else
	{
		widget=lookup_widget(GTK_WIDGET(button),"EncryptionOff");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
	}

	widget=lookup_widget(GTK_WIDGET(button),"ed_key1");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key1);	
	widget=lookup_widget(GTK_WIDGET(button),"ed_key2");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key2);	
	widget=lookup_widget(GTK_WIDGET(button),"ed_key3");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key3);	
	widget=lookup_widget(GTK_WIDGET(button),"ed_key4");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key4);
	widget=lookup_widget(GTK_WIDGET(button),"ActiveKeyNr");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->ActiveKey);

	widget=lookup_widget(GTK_WIDGET(button),"KeysHex");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),!CurrentScheme->KeyFormat);		
	widget=lookup_widget(GTK_WIDGET(button),"KeysStrings");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),CurrentScheme->KeyFormat);		

	widget=lookup_widget(GTK_WIDGET(button),"ed_iwconfig");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->iwconfig);	
	widget=lookup_widget(GTK_WIDGET(button),"ed_iwspy");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->iwspy);	
	widget=lookup_widget(GTK_WIDGET(button),"ed_iwpriv");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->iwpriv);	
	widget=lookup_widget(GTK_WIDGET(button),"ed_rts");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->rts);	
	widget=lookup_widget(GTK_WIDGET(button),"ed_frag");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->frag);	
	widget=lookup_widget(GTK_WIDGET(button),"ed_sens");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->sens);	

}

void
on_SchemeEdit_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	EditScheme(GTK_WIDGET(button));
}


void
on_Scheme_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}


void
on_Socket_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}


void
on_Instance_changed                    (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}


void
on_HWAddress_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}

void
on_ChannelNr_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
}

void
on_SchemeList_select_row               (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *widget;

	widget=lookup_widget(GTK_WIDGET(clist),"SchemeDelete");
	gtk_widget_set_sensitive(widget, TRUE);
	widget=lookup_widget(GTK_WIDGET(clist),"SchemeEdit");
	gtk_widget_set_sensitive(widget, TRUE);
	if (row > 0) {
		widget=lookup_widget(GTK_WIDGET(clist),"ListItemUp");
		gtk_widget_set_sensitive(widget, TRUE);
	}
	if (row < (clist->rows-1)) {
		widget=lookup_widget(GTK_WIDGET(clist),"ListItemDown");
		gtk_widget_set_sensitive(widget, TRUE);
	}
	SchemeListSelRow = row;
	SchemeListLastSelection = row;
	
}


void
on_SchemeList_unselect_row             (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *widget;

	widget=lookup_widget(GTK_WIDGET(clist),"SchemeDelete");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GTK_WIDGET(clist),"SchemeEdit");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GTK_WIDGET(clist),"ListItemUp");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GTK_WIDGET(clist),"ListItemDown");
	gtk_widget_set_sensitive(widget, FALSE);
	SchemeListSelRow = -1;
}



void
on_SchemeUse_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_GPE_WLANCFG_show                    (GtkWidget       *widget,
                                        gpointer         user_data)
{
	GtkWidget *SchemeList;
	gint i;
	gchar *ListEntry[4];
	gint arow;
 	sc = parse_configfile(WLAN_CONFIGFILE);
		
	for (arow=0; arow<sc; arow++)
	{
		SchemeList=lookup_widget(GPE_WLAN,"SchemeList");
		for (i = 0; i < 4; i++) {
			ListEntry[i]=(gchar *)malloc(32 * sizeof(char));
			memset(ListEntry[i],0,32);
		}
	
		/* New entry */
		CurrentScheme=(Scheme_t *)malloc(sizeof(Scheme_t));
		memcpy(CurrentScheme,&schemelist[arow],sizeof(Scheme_t));
		strcpy(ListEntry[0],CurrentScheme->Scheme);
		strcpy(ListEntry[1],CurrentScheme->Socket);
		strcpy(ListEntry[2],CurrentScheme->Instance);
		strcpy(ListEntry[3],CurrentScheme->HWAddress);
		arow = gtk_clist_append(GTK_CLIST(SchemeList),ListEntry);
		gtk_clist_set_row_data(GTK_CLIST(SchemeList), GTK_CLIST(SchemeList)->rows-1, (gpointer)CurrentScheme);
	}	
}

gboolean
on_SchemeList_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

	if (event->type == GDK_2BUTTON_PRESS)
		EditScheme(widget);
	
	return FALSE;
}



