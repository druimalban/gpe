/*

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Library General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "helptext.h"

#include "config-parser.h"

extern GtkWidget 	*GPE_WLANCFG;
static gboolean		HasChanged = FALSE;

Scheme_t 		*CurrentScheme;
gint 			SchemeCount; 
int			save_config;
void   			EditSimple(void);

void get_changes(void)
{
	GtkWidget 	*widget;
	GtkWidget 	*menu;
	gchar 		*ListEntry[4];
	gint 		count;
	gchar 		tempmode[20];
	gchar		tempchannel[20];
	gchar		tempfreq[20];
	gchar		temprate[20];
	
	for (count = 0; count < 4; count++) 
	{
		ListEntry[count]=(gchar *)malloc(32 * sizeof(char));
		memset(ListEntry[count], 0, 32);
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edScheme");
	if (strncmp(CurrentScheme->Scheme, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Scheme]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Scheme]=LINE_NEW;
		strncpy(CurrentScheme->Scheme, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edInstance");
	if (strncmp(CurrentScheme->Instance, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Instance]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Instance]=LINE_NEW;
		strncpy(CurrentScheme->Instance, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edSocket");
	if (strncmp(CurrentScheme->Socket, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Socket]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Socket]=LINE_NEW;
		strncpy(CurrentScheme->Socket, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edMAC");
	if (strncmp(CurrentScheme->HWAddress, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_HWAddress]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_HWAddress]=LINE_NEW;
		strncpy(CurrentScheme->HWAddress, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edInfo");
	if (strncmp(CurrentScheme->Info, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Info]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Info]=LINE_NEW;
		strncpy(CurrentScheme->Info, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edESSID");
	if (strncmp(CurrentScheme->ESSID, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_ESSID]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_ESSID]=LINE_NEW;
		strncpy(CurrentScheme->ESSID, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edNWID");
	if (strncmp(CurrentScheme->NWID, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_NWID]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_NWID]=LINE_NEW;
		strncpy(CurrentScheme->NWID, gtk_entry_get_text(GTK_ENTRY(widget)), 32);
	}

	menu=GTK_WIDGET(gtk_option_menu_get_menu(GTK_OPTION_MENU(lookup_widget(GTK_WIDGET(GPE_WLANCFG), "cbMode"))));
	widget=gtk_menu_get_active(GTK_MENU(menu));

	switch(g_list_index (GTK_MENU_SHELL (menu)->children, widget))
	{
		case 0:
			strncpy(tempmode, "AD-HOC", 16);
		break;
		case 1:
			strncpy(tempmode, "MANAGED", 16);
		break;
		case 2:
			strncpy(tempmode, "MASTER", 16);
 		break;
		case 3:
			strncpy(tempmode, "REPEATER", 16);
		break;
 		case 4:
			strncpy(tempmode, "SECONDARY", 16);
 		break;
		case 5:
		default:	
			strncpy(tempmode, "AUTO", 16);
 		break;
	}
	
	if (strncmp(CurrentScheme->Mode, tempmode, 16)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Mode]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Mode]=LINE_NEW;
		strncpy(CurrentScheme->Mode, tempmode, 16);
	}

	strcpy(tempchannel, "");
	strcpy(tempfreq, "");

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbFrequency");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) 
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbFrequency");
		strncpy(tempfreq,gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	else
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbChannel");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) 
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbChannel");
			strncpy(tempchannel,gtk_entry_get_text(GTK_ENTRY(widget)), 32);			
		}
	}

	if (strncmp(CurrentScheme->Channel, tempchannel, 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Channel]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Channel]=LINE_NEW;
		strncpy(CurrentScheme->Channel, tempchannel, 32);
	}

	if (strncmp(CurrentScheme->Frequency, tempfreq, 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Frequency]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Frequency]=LINE_NEW;
		strncpy(CurrentScheme->Frequency, tempfreq, 32);
	}
	
	menu=GTK_WIDGET(gtk_option_menu_get_menu(GTK_OPTION_MENU(lookup_widget(GTK_WIDGET(GPE_WLANCFG), "cbRate"))));
	widget=gtk_menu_get_active(GTK_MENU(menu));
	
	switch(g_list_index (GTK_MENU_SHELL (menu)->children, widget))
	{
		case 0:
			strncpy(temprate, "auto", 8);
		break;
		case 1:
			strncpy(temprate, "1M", 8);
		break;
		case 2:
			strncpy(temprate, "2M", 8);
 		break;
		case 3:
		default:	
			strncpy(temprate, "11M", 8);
 		break;
	}

	if (strncmp(CurrentScheme->Rate, temprate, 8)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Rate]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Rate]=LINE_NEW;
		strncpy(CurrentScheme->Rate, temprate, 8);
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbOpen");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		if (strncmp(CurrentScheme->EncMode, "open", 16)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_EncMode]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_EncMode]=LINE_NEW;
			strncpy(CurrentScheme->EncMode, "open", 16);	
		}
	} else
	{
		if (strncmp(CurrentScheme->EncMode, "shared", 16)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_EncMode]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_EncMode]=LINE_NEW;
			strncpy(CurrentScheme->EncMode, "shared", 16);	
		}
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbOn");	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		if (strncmp(CurrentScheme->Encryption, "on", 4)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_Encryption]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_Encryption]=LINE_NEW;
			strncpy(CurrentScheme->Encryption, "on", 4);	
		}
		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey1");
		if (strncmp(CurrentScheme->key1, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key1]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key1]=LINE_NEW;
			strncpy(CurrentScheme->key1, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}
		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey2");
		if (strncmp(CurrentScheme->key2, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key2]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key2]=LINE_NEW;
			strncpy(CurrentScheme->key2, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey3");
		if (strncmp(CurrentScheme->key3, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key3]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key3]=LINE_NEW;
			strncpy(CurrentScheme->key3, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey4");
		if (strncmp(CurrentScheme->key4, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key4]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key4]=LINE_NEW;
			strncpy(CurrentScheme->key4, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbActiveKey");
		if (strncmp(CurrentScheme->ActiveKey, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_ActiveKey]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_ActiveKey]=LINE_NEW;
			strncpy(CurrentScheme->ActiveKey, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}

	}
	else 
	{
		if (strncmp(CurrentScheme->Encryption, "off", 4)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_Encryption]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_Encryption]=LINE_NEW;
			strncpy(CurrentScheme->Encryption, "off", 4);	
		}

		
		if (strncmp(CurrentScheme->ActiveKey, "0", 4)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_ActiveKey]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_ActiveKey]=LINE_NEW;
			strncpy(CurrentScheme->ActiveKey, "0", 4);	
		}
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbStringValues");

	if (CurrentScheme->KeyFormat!=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		HasChanged = TRUE;
/* kc-or: Key format will never produce a new line
		if (CurrentScheme->lines[L_KeyFormat]==LINE_NOT_PRESENT)
		{	
			CurrentScheme->lines[L_KeyFormat]=LINE_NEW;
		}
*/		CurrentScheme->KeyFormat=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));		
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edIwConfig");
	if (strncmp(CurrentScheme->iwconfig, gtk_entry_get_text(GTK_ENTRY(widget)), 256)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_iwconfig]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwconfig]=LINE_NEW;
		strncpy(CurrentScheme->iwconfig, gtk_entry_get_text(GTK_ENTRY(widget)), 256);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edIwSpy");
	if (strncmp(CurrentScheme->iwspy, gtk_entry_get_text(GTK_ENTRY(widget)), 256)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_iwspy]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwspy]=LINE_NEW;
		strncpy(CurrentScheme->iwspy, gtk_entry_get_text(GTK_ENTRY(widget)), 256);	
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edIwPriv");
	if (strncmp(CurrentScheme->iwpriv, gtk_entry_get_text(GTK_ENTRY(widget)), 256)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_iwpriv]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwpriv]=LINE_NEW;
		strncpy(CurrentScheme->iwpriv, gtk_entry_get_text(GTK_ENTRY(widget)), 256);	
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edRTS");
	if (strncmp(CurrentScheme->rts, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_rts]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_rts]=LINE_NEW;
		strncpy(CurrentScheme->rts, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edFrag");
	if (strncmp(CurrentScheme->frag, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_frag]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_frag]=LINE_NEW;
		strncpy(CurrentScheme->frag, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edSens");
	if (strncmp(CurrentScheme->sens, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_sens]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_sens]=LINE_NEW;
		strncpy(CurrentScheme->sens, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GPE_WLANCFG, "nbPseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(widget), 0);	
}

void get_changes_simple(void)
{
	GtkWidget 	*widget;
	GtkWidget 	*menu;
	gchar 		tempmode[20];
	gchar		tempchannel[20];
	
	
	menu=GTK_WIDGET(gtk_option_menu_get_menu(GTK_OPTION_MENU(lookup_widget(GTK_WIDGET(GPE_WLANCFG), "cbMode"))));
	widget=gtk_menu_get_active(GTK_MENU(menu));

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbInfrastructure");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) 
	{
		strncpy(tempmode, "MANAGED", 16);	
	}
	else
	{
		strncpy(tempmode, "AD-HOC", 16);	
	}

	if (strncmp(CurrentScheme->Mode, tempmode, 16)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Mode]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Mode]=LINE_NEW;
		strncpy(CurrentScheme->Mode, tempmode, 16);
	}

	strcpy(tempchannel, "");
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbChannel_simple");
	strncpy(tempchannel,gtk_entry_get_text(GTK_ENTRY(widget)), 32);			
	
	if (strncmp(CurrentScheme->Channel, tempchannel, 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Channel]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Channel]=LINE_NEW;
		strncpy(CurrentScheme->Channel, tempchannel, 32);
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edESSID_simple");
	if (strncmp(CurrentScheme->ESSID, gtk_entry_get_text(GTK_ENTRY(widget)), 32)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_ESSID]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_ESSID]=LINE_NEW;
		strncpy(CurrentScheme->ESSID, gtk_entry_get_text(GTK_ENTRY(widget)), 32);	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbAuthOpen");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		if (strncmp(CurrentScheme->EncMode, "open", 16)!=0)
		{
			if (strlen(CurrentScheme->EncMode)) 
				HasChanged = TRUE;
			if (CurrentScheme->lines[L_EncMode]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_EncMode]=LINE_NEW;
			strncpy(CurrentScheme->EncMode, "open", 16);	
		}
	} else
	{
		if (strncmp(CurrentScheme->EncMode, "shared", 16)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_EncMode]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_EncMode]=LINE_NEW;
			strncpy(CurrentScheme->EncMode, "shared", 16);	
		}
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbWEPenable");	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		if (strncmp(CurrentScheme->Encryption, "on", 4)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_Encryption]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_Encryption]=LINE_NEW;
			strncpy(CurrentScheme->Encryption, "on", 4);	
		}
		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey1_simple");
		if (strncmp(CurrentScheme->key1, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key1]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key1]=LINE_NEW;
			strncpy(CurrentScheme->key1, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}
		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey2_simple");
		if (strncmp(CurrentScheme->key2, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key2]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key2]=LINE_NEW;
			strncpy(CurrentScheme->key2, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey3_simple");
		if (strncmp(CurrentScheme->key3, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key3]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key3]=LINE_NEW;
			strncpy(CurrentScheme->key3, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey4_simple");
		if (strncmp(CurrentScheme->key4, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key4]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key4]=LINE_NEW;
			strncpy(CurrentScheme->key4, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbActiveKey_simple");
		if (strncmp(CurrentScheme->ActiveKey, gtk_entry_get_text(GTK_ENTRY(widget)), 64)!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_ActiveKey]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_ActiveKey]=LINE_NEW;
			strncpy(CurrentScheme->ActiveKey, gtk_entry_get_text(GTK_ENTRY(widget)), 64);	
		}

	}
	else 
	{
		if (strncmp(CurrentScheme->Encryption, "off", 4)!=0)
		{
			if (CurrentScheme->lines[L_Encryption]!=LINE_NOT_PRESENT)			
				HasChanged = TRUE;
			if (CurrentScheme->lines[L_Encryption]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_Encryption]=LINE_NEW;
			strncpy(CurrentScheme->Encryption, "off", 4);	
		}

		
		if (strncmp(CurrentScheme->ActiveKey, "0", 4)!=0)
		{
			if (CurrentScheme->lines[L_ActiveKey]!=LINE_NOT_PRESENT)
				HasChanged = TRUE;
			if (CurrentScheme->lines[L_ActiveKey]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_ActiveKey]=LINE_NEW;
			strncpy(CurrentScheme->ActiveKey, "0", 4);	
		}
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbKeyString");

	if (CurrentScheme->KeyFormat!=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		HasChanged = TRUE;
		CurrentScheme->KeyFormat=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));		
	}
}


gboolean on_GPE_WLANCFG_de_event(GtkWidget *widget, gpointer user_data)
{
	
	GtkWidget 	*MainWindow;
	GtkWidget 	*treeview;
	GtkWidget       *dialog;
 	GtkTreeIter	iter;
	GtkListStore	*liststore;
	GtkTreeSelection *selection;
	gboolean quit_app = (gboolean) user_data;
	
	gchar      	*ListEntry[4];
	gint            answer;
	int 		count;
	Scheme_t*	Scheme;
	Scheme_t*	OrigScheme;

	save_config = !quit_app;
	
	treeview = lookup_widget(GPE_WLANCFG, "tvSchemeList");
	liststore = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));

	MainWindow = lookup_widget(GPE_WLANCFG, "nbPseudoMain");
	
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(MainWindow)) == 1) 
	{
		OrigScheme=malloc(sizeof(Scheme_t));
		memcpy(OrigScheme, CurrentScheme, sizeof(Scheme_t));
		get_changes();
		if (HasChanged) 
		{
			if (quit_app)
			{
	       		dialog = gtk_message_dialog_new (GTK_WINDOW (GPE_WLANCFG),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 _("Settings have been changed.\nDo you want to save the settings?"));
				answer = gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
			}
			else
			{
				answer = GTK_RESPONSE_YES;
			}
			if (answer != GTK_RESPONSE_YES) 
			{
				memcpy(CurrentScheme, OrigScheme, sizeof(Scheme_t));
			} 
			else
			{
				save_config = TRUE;
				if (CurrentScheme->NewScheme)
				{
					CurrentScheme->NewScheme = FALSE;
					SchemeCount++;
	
					treeview = lookup_widget(GPE_WLANCFG, "tvSchemeList");
					liststore = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
					
					for (count = 0; count < 4; count++) 
					{
						ListEntry[count]=(gchar *)malloc(32 * sizeof(char));
						memset(ListEntry[count],0,32);
					}
					
					strncpy(ListEntry[0], CurrentScheme->Scheme, 32);
					strncpy(ListEntry[1], CurrentScheme->Socket, 32);
					strncpy(ListEntry[2], CurrentScheme->Instance, 32);
					strncpy(ListEntry[3], CurrentScheme->HWAddress, 32);
				
					gtk_list_store_append(liststore, &iter);
					gtk_list_store_set(liststore, &iter, 0, ListEntry[0], 1, ListEntry[1], 2 , ListEntry[2], 3, ListEntry[3], 4, CurrentScheme, -1);

				} else
				{

					treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
					selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

					for (count = 0; count < 4; count++) 
					{
						ListEntry[count]=(gchar *)malloc(32 * sizeof(char));
						memset(ListEntry[count],0,32);
					}

					strncpy(ListEntry[0], CurrentScheme->Scheme, 32);
					strncpy(ListEntry[1], CurrentScheme->Socket, 32);
					strncpy(ListEntry[2], CurrentScheme->Instance, 32);
					strncpy(ListEntry[3], CurrentScheme->HWAddress, 32);
					
					if(gtk_tree_selection_get_selected (selection, (GtkTreeModel**)&liststore, &iter))
						gtk_list_store_set(liststore, &iter, 0, ListEntry[0], 1, ListEntry[1], 2 , ListEntry[2], 3, ListEntry[3], 4, CurrentScheme, -1);
					
				}
			}
			//kc-or: Only ask if really changed
			HasChanged = FALSE;
		} else 
		{
			gtk_notebook_set_page(GTK_NOTEBOOK(MainWindow), 0);
		}
		free(OrigScheme);
	} else 
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(MainWindow)) == 2) 
	{
		OrigScheme=malloc(sizeof(Scheme_t));
		memcpy(OrigScheme, CurrentScheme, sizeof(Scheme_t));
		get_changes_simple();
		if (HasChanged) 
		{
			if (quit_app)
			{
	       		dialog = gtk_message_dialog_new (GTK_WINDOW (GPE_WLANCFG),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 _("Settings have been changed.\nDo you want to save the settings?"));
				answer = gtk_dialog_run(GTK_DIALOG(dialog));
				gtk_widget_destroy(dialog);
			}
			else
			{
				answer = GTK_RESPONSE_YES;
			}
			if (answer != GTK_RESPONSE_YES) 
			{
				memcpy(CurrentScheme, OrigScheme, sizeof(Scheme_t));
			} else
			{
				save_config = TRUE;
				treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

				for (count = 0; count < 4; count++) 
				{
					ListEntry[count]=(gchar *)malloc(32 * sizeof(char));
					memset(ListEntry[count],0,32);
				}

				strncpy(ListEntry[0], CurrentScheme->Scheme, 32);
				strncpy(ListEntry[1], CurrentScheme->Socket, 32);
				strncpy(ListEntry[2], CurrentScheme->Instance, 32);
				strncpy(ListEntry[3], CurrentScheme->HWAddress, 32);
					
				if(gtk_tree_selection_get_selected (selection, (GtkTreeModel**)&liststore, &iter))
					gtk_list_store_set(liststore, &iter, 0, ListEntry[0], 1, ListEntry[1], 2 , ListEntry[2], 3, ListEntry[3], 4, CurrentScheme, -1);
					
			}
			
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);
		
			for (count = 0; count<SchemeCount; count++)
			{
			
				gtk_tree_model_get (GTK_TREE_MODEL(liststore), &iter, 4, &Scheme, -1);
				gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter);
				memcpy(&schemelist[count], Scheme, sizeof(Scheme_t));
			}
			
			if (save_config) write_back_configfile(WLAN_CONFIGFILE, schemelist, SchemeCount);
		}
		free(OrigScheme);
		if (quit_app) 
			gtk_main_quit();
	} else 
	{
		if (HasChanged && quit_app)
		{
	       	dialog = gtk_message_dialog_new (GTK_WINDOW (GPE_WLANCFG),
                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                                GTK_MESSAGE_QUESTION,
                                                GTK_BUTTONS_YES_NO,
                                                 _("Settings have been changed.\nDo you want to save the settings?"));
			answer = gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			if (answer == GTK_RESPONSE_YES) save_config = TRUE; 
		}
		else
		{
			save_config = HasChanged;
		}

		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);
		
		for (count = 0; count<SchemeCount; count++)
		{
			
			gtk_tree_model_get (GTK_TREE_MODEL(liststore), &iter, 4, &Scheme, -1);
			gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter);
			memcpy(&schemelist[count], Scheme, sizeof(Scheme_t));
		}
			
		if (save_config) write_back_configfile(WLAN_CONFIGFILE, schemelist, SchemeCount);
		if (quit_app)
			gtk_main_quit();
	}

	HasChanged = FALSE;
	return TRUE;
}


void on_GPE_WLANCFG_show (GtkWidget *widget, gpointer user_data)
{
	gint       	count;
	gint		aRow;
	gchar      	*ListEntry[4];
	gint            found_wildcards = 0;

	GtkWidget	*treeview;
 	GtkTreeIter	iter;
	GtkTreeIter     tempiter;
	GtkListStore	*liststore;
	GtkTreeSelection *selection;
	
	treeview=lookup_widget(GPE_WLANCFG, "tvSchemeList");
	liststore = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
	
	SchemeCount = parse_configfile(WLAN_CONFIGFILE);
	
	if (SchemeCount == 0) exit(-1); // exit if no schemes are there
	
	for (aRow=0; aRow<SchemeCount; aRow++)
	{
		for (count = 0; count < 4; count++) 
		{
			ListEntry[count]=(gchar *)malloc(32 * sizeof(char));
			memset(ListEntry[count],0,32);
		}
	
		/* New entry */
		CurrentScheme=(Scheme_t *)malloc(sizeof(Scheme_t));
		
		memcpy(CurrentScheme, &schemelist[aRow], sizeof(Scheme_t));
		
		CurrentScheme->NewScheme = FALSE;
		
		strncpy(ListEntry[0], CurrentScheme->Scheme, 32);
		strncpy(ListEntry[1], CurrentScheme->Socket, 32);
		strncpy(ListEntry[2], CurrentScheme->Instance, 32);
		strncpy(ListEntry[3], CurrentScheme->HWAddress, 32);
		
		gtk_list_store_append(liststore, &iter);
		gtk_list_store_set(liststore, &iter, 0, ListEntry[0], 1, ListEntry[1], 2 , ListEntry[2], 3, ListEntry[3], 4, CurrentScheme, -1);
		
		if (!strcmp(CurrentScheme->Scheme, "*") &&
		    !strcmp(CurrentScheme->Socket, "*")	&&
		    !strcmp(CurrentScheme->Instance, "*") &&
		    !strcmp(CurrentScheme->HWAddress, "*"))
		{
			tempiter = iter;
			found_wildcards = 1;
		}
	}	
	
	if (found_wildcards)
	{
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_select_iter (selection, &tempiter);
		
		EditSimple();
	}
    HasChanged = FALSE;
}

void on_btnDelete_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget       *dialog;
	
	Scheme_t	*Scheme;
	

	GtkWidget	*treeview;
 	GtkTreeSelection *selection;
	GtkTreeIter	iter;
	GtkTreeModel	*liststore;
	
	gint            answer;
	
	treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if(gtk_tree_selection_get_selected (selection, &liststore, &iter))
	{

		dialog = gtk_message_dialog_new (GTK_WINDOW (GPE_WLANCFG),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("Do you realy want to delete this entry??"));
		answer = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		if (answer == GTK_RESPONSE_YES)
		{
			gtk_tree_model_get (liststore, &iter, 4, &Scheme, -1);
			delete_list[delete_list_count][0] = Scheme->scheme_start;
			delete_list[delete_list_count][1] = Scheme->scheme_end;
			delete_list_count++;
			
			free(Scheme);
			
			gtk_list_store_remove(GTK_LIST_STORE(liststore), &iter);

			SchemeCount--;	
			HasChanged = TRUE;
		};
	}
}

void on_btnNew_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget	*notebook;
	GtkWidget	*widget;
	
	widget=lookup_widget(GTK_WIDGET(button),"sbFrequency");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GTK_WIDGET(button),"sbChannel");
 	gtk_widget_set_sensitive(widget, FALSE);

	CurrentScheme=(Scheme_t *)malloc(sizeof(Scheme_t));
	memset(CurrentScheme,0,sizeof(Scheme_t));
	
	CurrentScheme->NewScheme = TRUE;
	
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

	notebook = lookup_widget(GTK_WIDGET(button), "nbPseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 1);

	notebook = lookup_widget(GTK_WIDGET(button), "nbConfigSelection");
	gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 0);

	HasChanged = TRUE;
}

void EditScheme(void)
{
	GtkWidget	*treeview;
	GtkWidget	*widget;
 	GtkTreeSelection *selection;
	GtkTreeIter	iter;
	GtkTreeModel	*liststore;
	gint		count;
	
	treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if(gtk_tree_selection_get_selected (selection, &liststore, &iter))
	{
		gtk_tree_model_get (liststore, &iter, 4, &CurrentScheme, -1);
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "nbPseudoMain");
		gtk_notebook_set_page(GTK_NOTEBOOK(widget), 1);
		
		widget = lookup_widget(GTK_WIDGET(GPE_WLANCFG), "nbConfigSelection");
		gtk_notebook_set_page(GTK_NOTEBOOK(widget), 0);
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edScheme");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Scheme);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edInstance");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Instance);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edSocket");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Socket);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edMAC");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->HWAddress);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edInfo");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Info);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edESSID");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->ESSID);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edNWID");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->NWID);
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "cbMode");
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 5);
		for (count=0;count<strlen(CurrentScheme->Mode); count++)
			CurrentScheme->Mode[count]=toupper(CurrentScheme->Mode[count]);
	
		if (!strcmp("AD-HOC",CurrentScheme->Mode)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 0);
		if (!strcmp("MANAGED",CurrentScheme->Mode)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 1);
		if (!strcmp("MASTER",CurrentScheme->Mode)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 2);
		if (!strcmp("REPEATER",CurrentScheme->Mode)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 3);
		if (!strcmp("SECONDARY",CurrentScheme->Mode)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 4);
		if (!strcmp("AUTO",CurrentScheme->Mode)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 5);
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbFrequency");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Frequency);	
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbChannel");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Channel);	
	
		if (strlen(CurrentScheme->Frequency) > 4) {
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbFrequency");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
			gtk_widget_set_sensitive(widget, TRUE);
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
			gtk_widget_set_sensitive(widget, FALSE);			
		}
		else
		{
			if (strlen(CurrentScheme->Channel)){
				widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbChannel");
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		

				widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
				gtk_widget_set_sensitive(widget, FALSE);
				widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
				gtk_widget_set_sensitive(widget, TRUE);			
			}
			else
			{
				widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbDefaultChannel");
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
				
				widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
				gtk_widget_set_sensitive(widget, FALSE);
				widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
				gtk_widget_set_sensitive(widget, FALSE);			
			}
		}
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"cbRate");
		gtk_option_menu_set_history(GTK_OPTION_MENU(widget),3);
	
		for (count=0; count<strlen(CurrentScheme->Rate);count++)
			CurrentScheme->Rate[count]=toupper(CurrentScheme->Rate[count]);
	
		if (!strcmp("AUTO",CurrentScheme->Rate)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget),0);
		if (!strcmp("1M",CurrentScheme->Rate)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget),1);
		if (!strcmp("2M",CurrentScheme->Rate)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget),2);
		if (!strcmp("11M",CurrentScheme->Rate)) 		
			gtk_option_menu_set_history(GTK_OPTION_MENU(widget),3);
	
		if (!strcmp(CurrentScheme->Encryption, "on"))
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbOn");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
		else
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbOff");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
	
		if (!strcmp(CurrentScheme->EncMode, "shared"))
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbRestricted");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
		else
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbOpen");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edKey1");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key1);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edKey2");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key2);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edKey3");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key3);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edKey4");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key4);
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbActiveKey");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), atoi(CurrentScheme->ActiveKey));
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbHex");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),!CurrentScheme->KeyFormat);		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbStringValues");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),CurrentScheme->KeyFormat);		
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edIwConfig");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->iwconfig);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edIwSpy");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->iwspy);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edIwPriv");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->iwpriv);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edRTS");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->rts);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edFrag");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->frag);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edSens");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->sens);	
	}
}

void EditSimple(void)
{
	GtkWidget	*treeview;
	GtkWidget	*widget;
 	GtkTreeSelection *selection;
	GtkTreeIter	iter;
	GtkTreeModel	*liststore;
	
	treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if(gtk_tree_selection_get_selected (selection, &liststore, &iter))
	{
		gtk_tree_model_get (liststore, &iter, 4, &CurrentScheme, -1);
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "nbPseudoMain");
		gtk_notebook_set_page(GTK_NOTEBOOK(widget), 2);
		
		widget = lookup_widget(GTK_WIDGET(GPE_WLANCFG), "nbSimple");
		gtk_notebook_set_page(GTK_NOTEBOOK(widget), 0);
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbInfrastructure");
		if (!strcmp("AD-HOC",CurrentScheme->Mode))
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbAdHoc");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		} else
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbInfrastructure");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		}
		

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbChannel_simple");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->Channel);	

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "cbDefault_simple");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),!(strlen(CurrentScheme->Channel)));

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edESSID_simple");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->ESSID);	

		if (!strcmp(CurrentScheme->Encryption, "on"))
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbWEPenable");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
		else
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbWEPdisable");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
	
		if (!strcmp(CurrentScheme->EncMode, "shared"))
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbAuthShared");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
		else
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbAuthOpen");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbKeyHex");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),!CurrentScheme->KeyFormat);		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbKeyString");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),CurrentScheme->KeyFormat);		

		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edKey1_simple");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key1);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edKey2_simple");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key2);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edKey3_simple");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key3);	
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"edKey4_simple");
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->key4);
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbActiveKey_simple");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), atoi(CurrentScheme->ActiveKey));
	}
}

void on_btnEdit_clicked (GtkButton *button, gpointer user_data)
{
	EditScheme();
}


void on_btnHelp_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget	*HelpWin;
	GtkWidget	*MainWin;
	GtkWidget	*textview;
	GtkTextBuffer	*buffer;
	GtkWidget	*notebook;
	GtkTextIter	iter;
	GtkTextMark *mark = NULL;

	MainWin=lookup_widget(GTK_WIDGET(button), "GPE_WLANCFG");
	HelpWin=create_GeneralHelpWin();
	
	gtk_window_set_transient_for(GTK_WINDOW(HelpWin), GTK_WINDOW(MainWin));

	
	textview=lookup_widget(HelpWin, "twHelp");
	buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

	notebook = lookup_widget(GTK_WIDGET(GPE_WLANCFG), "nbPseudoMain");

	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) == 1) // advanced view
	{
		notebook = lookup_widget(GTK_WIDGET(GPE_WLANCFG), "nbConfigSelection");
		switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)))
		{
			case 0:	gtk_text_buffer_set_text(buffer, scheme_help, -1); break;
			case 1:	gtk_text_buffer_set_text(buffer, general_help, -1); break;
			case 2: gtk_text_buffer_set_text(buffer, rfparams_help, -1); break;
			case 3: gtk_text_buffer_set_text(buffer, wep_help, -1); break;
			case 4: gtk_text_buffer_set_text(buffer, enckey_help, -1); break;
			case 5: gtk_text_buffer_set_text(buffer, expert_help, -1); break;
		}
	}
	else
	{
		notebook = lookup_widget(GTK_WIDGET(GPE_WLANCFG), "nbSimple");
		switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)))
		{
			case 0:	gtk_text_buffer_set_text(buffer, general_help_simple, -1); break;
			case 1:	gtk_text_buffer_set_text(buffer, wep_help, -1);
					gtk_text_buffer_insert_at_cursor(buffer,enckey_help,-1);					
			break;
		}
	}
	
	/* scroll to first line of text */
	gtk_widget_show_all(HelpWin);	
	gtk_text_buffer_get_start_iter(buffer,&iter);
	mark = gtk_text_buffer_create_mark(buffer,"start0",&iter,FALSE);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview),mark,0,FALSE,0,0);

	gtk_dialog_run(GTK_DIALOG(HelpWin));
	gtk_widget_destroy(HelpWin);
}

void on_btnExpert_clicked (GtkButton *button, gpointer user_data)
{
	
	GtkWidget 	*MainWindow;
	GtkWidget       *dialog;
	GtkWidget 	*treeview;
 	GtkTreeIter	iter;
	GtkListStore	*liststore;
	GtkTreeSelection *selection;
	
	gchar      	*ListEntry[4];
	gint            answer;
	gint 		switch_to_simple = (gint)user_data;
	
	int 		count;
	Scheme_t*	OrigScheme;

	treeview = lookup_widget(GPE_WLANCFG, "tvSchemeList");
	liststore = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));

	MainWindow = lookup_widget(GPE_WLANCFG, "nbPseudoMain");
	
	OrigScheme=malloc(sizeof(Scheme_t));
	memcpy(OrigScheme, CurrentScheme, sizeof(Scheme_t));
	if (switch_to_simple)
		get_changes_simple();
	else
		get_changes();
	
	if (HasChanged) 
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (GPE_WLANCFG),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("Settings have been changed.\nDo you want to save the settings?"));
		answer = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if (answer != GTK_RESPONSE_YES) 
		{
			memcpy(CurrentScheme, OrigScheme, sizeof(Scheme_t));
		} else
		{
			save_config = TRUE;
			treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

			for (count = 0; count < 4; count++) 
			{
				ListEntry[count]=(gchar *)malloc(32 * sizeof(char));
				memset(ListEntry[count],0,32);
			}

			strncpy(ListEntry[0], CurrentScheme->Scheme, 32);
			strncpy(ListEntry[1], CurrentScheme->Socket, 32);
			strncpy(ListEntry[2], CurrentScheme->Instance, 32);
			strncpy(ListEntry[3], CurrentScheme->HWAddress, 32);
				
			if(gtk_tree_selection_get_selected (selection, (GtkTreeModel**)&liststore, &iter))
				gtk_list_store_set(liststore, &iter, 0, ListEntry[0], 1, ListEntry[1], 2 , ListEntry[2], 3, ListEntry[3], 4, CurrentScheme, -1);
				
			for (count = 0; count < 4; count++) 
				free(ListEntry[count]);
		}

		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);

		HasChanged = FALSE;
	}
	free(OrigScheme);

	gtk_notebook_set_page(GTK_NOTEBOOK(MainWindow), switch_to_simple ? 2 : 0);	
}


void on_tvSchemeList_row_activated (GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, gpointer user_data)
{
	EditScheme();
}

void on_rbFrequency_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkWidget	*widget;
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbFrequency");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
		gtk_widget_set_sensitive(widget, TRUE);
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
		gtk_widget_set_sensitive(widget, FALSE);			
	} else
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbChannel");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
			gtk_widget_set_sensitive(widget, FALSE);
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
			gtk_widget_set_sensitive(widget, TRUE);			
		} else
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
			gtk_widget_set_sensitive(widget, FALSE);
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
			gtk_widget_set_sensitive(widget, FALSE);			
		}
	}

}


void on_rbChannel_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkWidget	*widget;

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbFrequency");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
		gtk_widget_set_sensitive(widget, TRUE);
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
		gtk_widget_set_sensitive(widget, FALSE);			
	} else
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbChannel");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
			gtk_widget_set_sensitive(widget, FALSE);
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
			gtk_widget_set_sensitive(widget, TRUE);			
		} else
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbFrequency");
			gtk_widget_set_sensitive(widget, FALSE);
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"sbChannel");
			gtk_widget_set_sensitive(widget, FALSE);			
		}
	}
}

void
on_cbDefault_simple_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	GtkWidget	*widget;

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbChannel_simple");
	gtk_widget_set_sensitive(widget,!gtk_toggle_button_get_active(togglebutton));
	
	if (gtk_toggle_button_get_active(togglebutton))
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbChannel_simple");
		gtk_entry_set_text(GTK_ENTRY(widget),"");
	}		
}
