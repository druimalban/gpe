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
	if (strcmp(CurrentScheme->Scheme, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Scheme]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Scheme]=LINE_NEW;
		strcpy(CurrentScheme->Scheme, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edInstance");
	if (strcmp(CurrentScheme->Instance, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Instance]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Instance]=LINE_NEW;
		strcpy(CurrentScheme->Instance, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edSocket");
	if (strcmp(CurrentScheme->Socket, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Socket]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Socket]=LINE_NEW;
		strcpy(CurrentScheme->Socket, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edMAC");
	if (strcmp(CurrentScheme->HWAddress, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_HWAddress]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_HWAddress]=LINE_NEW;
		strcpy(CurrentScheme->HWAddress, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edInfo");
	if (strcmp(CurrentScheme->Info, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Info]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Info]=LINE_NEW;
		strcpy(CurrentScheme->Info, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edESSID");
	if (strcmp(CurrentScheme->ESSID, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_ESSID]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_ESSID]=LINE_NEW;
		strcpy(CurrentScheme->ESSID, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edNWID");
	if (strcmp(CurrentScheme->NWID, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_NWID]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_NWID]=LINE_NEW;
		strcpy(CurrentScheme->NWID, gtk_entry_get_text(GTK_ENTRY(widget)));
	}

	menu=GTK_WIDGET(gtk_option_menu_get_menu(GTK_OPTION_MENU(lookup_widget(GTK_WIDGET(GPE_WLANCFG), "cbMode"))));
	widget=gtk_menu_get_active(GTK_MENU(menu));

	switch(g_list_index (GTK_MENU_SHELL (menu)->children, widget))
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
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Mode]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Mode]=LINE_NEW;
		strcpy(CurrentScheme->Mode, tempmode);
	}

	strcpy(tempchannel, "");
	strcpy(tempfreq, "");

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbFrequency");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) 
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbFrequency");
		strcpy(tempfreq,gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	else
	{
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbChannel");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) 
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbChannel");
			strcpy(tempchannel,gtk_entry_get_text(GTK_ENTRY(widget)));			
		}
	}

	if (strcmp(CurrentScheme->Channel, tempchannel)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Channel]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Channel]=LINE_NEW;
		strcpy(CurrentScheme->Channel, tempchannel);
	}

	if (strcmp(CurrentScheme->Frequency, tempfreq)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Frequency]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Frequency]=LINE_NEW;
		strcpy(CurrentScheme->Frequency, tempfreq);
	}
	
	menu=GTK_WIDGET(gtk_option_menu_get_menu(GTK_OPTION_MENU(lookup_widget(GTK_WIDGET(GPE_WLANCFG), "cbRate"))));
	widget=gtk_menu_get_active(GTK_MENU(menu));
	
	switch(g_list_index (GTK_MENU_SHELL (menu)->children, widget))
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
		case 3:
		default:	
			strcpy(temprate, "11M");
 		break;
	}

	if (strcmp(CurrentScheme->Rate, temprate)!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_Rate]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_Rate]=LINE_NEW;
		strcpy(CurrentScheme->Rate, temprate);
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbOpen");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		if (strcmp(CurrentScheme->EncMode, "open")!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_EncMode]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_EncMode]=LINE_NEW;
			strcpy(CurrentScheme->EncMode, "open");	
		}
	} else
	{
		if (strcmp(CurrentScheme->EncMode, "shared")!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_EncMode]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_EncMode]=LINE_NEW;
			strcpy(CurrentScheme->EncMode, "shared");	
		}
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbOn");	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		if (strcmp(CurrentScheme->Encryption, "on")!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_Encryption]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_Encryption]=LINE_NEW;
			strcpy(CurrentScheme->Encryption, "on");	
		}
		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey1");
		if (strcmp(CurrentScheme->key1, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key1]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key1]=LINE_NEW;
			strcpy(CurrentScheme->key1, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}
		
		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey2");
		if (strcmp(CurrentScheme->key2, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key2]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key2]=LINE_NEW;
			strcpy(CurrentScheme->key2, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey3");
		if (strcmp(CurrentScheme->key3, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key3]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key3]=LINE_NEW;
			strcpy(CurrentScheme->key3, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edKey4");
		if (strcmp(CurrentScheme->key4, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_key4]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_key4]=LINE_NEW;
			strcpy(CurrentScheme->key4, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}

		widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "sbActiveKey");
		if (strcmp(CurrentScheme->ActiveKey, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_ActiveKey]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_ActiveKey]=LINE_NEW;
			strcpy(CurrentScheme->ActiveKey, gtk_entry_get_text(GTK_ENTRY(widget)));	
		}

	}
	else
	{
		if (strcmp(CurrentScheme->Encryption, "off")!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_Encryption]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_Encryption]=LINE_NEW;
			strcpy(CurrentScheme->Encryption, "off");	
		}

		
		if (strcmp(CurrentScheme->ActiveKey, "0")!=0)
		{
			HasChanged = TRUE;
			if (CurrentScheme->lines[L_ActiveKey]==LINE_NOT_PRESENT)
				CurrentScheme->lines[L_ActiveKey]=LINE_NEW;
			strcpy(CurrentScheme->ActiveKey, "0");	
		}
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "rbStringValues");

	if (CurrentScheme->KeyFormat!=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_KeyFormat]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_KeyFormat]=LINE_NEW;
		CurrentScheme->KeyFormat=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));		
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edIwConfig");
	if (strcmp(CurrentScheme->iwconfig, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_iwconfig]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwconfig]=LINE_NEW;
		strcpy(CurrentScheme->iwconfig, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edIwSpy");
	if (strcmp(CurrentScheme->iwspy, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_iwspy]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwspy]=LINE_NEW;
		strcpy(CurrentScheme->iwspy, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edIwPriv");
	if (strcmp(CurrentScheme->iwpriv, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_iwpriv]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_iwpriv]=LINE_NEW;
		strcpy(CurrentScheme->iwpriv, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}

	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edRTS");
	if (strcmp(CurrentScheme->rts, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_rts]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_rts]=LINE_NEW;
		strcpy(CurrentScheme->rts, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edFrag");
	if (strcmp(CurrentScheme->frag, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_frag]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_frag]=LINE_NEW;
		strcpy(CurrentScheme->frag, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG), "edSens");
	if (strcmp(CurrentScheme->sens, gtk_entry_get_text(GTK_ENTRY(widget)))!=0)
	{
		HasChanged = TRUE;
		if (CurrentScheme->lines[L_sens]==LINE_NOT_PRESENT)
			CurrentScheme->lines[L_sens]=LINE_NEW;
		strcpy(CurrentScheme->sens, gtk_entry_get_text(GTK_ENTRY(widget)));	
	}
	
	widget=lookup_widget(GPE_WLANCFG, "nbPseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(widget), 0);	
}

gboolean on_GPE_WLANCFG_de_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	
	GtkWidget 	*MainWindow;
	GtkWidget 	*SaveWindow;

	GtkWidget 	*treeview;
 	GtkTreeIter	iter;
	GtkListStore	*liststore;

	gchar      	*ListEntry[4];

	int 		count;
	Scheme_t*	Scheme;
	Scheme_t*	OrigScheme;

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
			SaveWindow = create_dlgSaveChanges();
			gtk_window_set_transient_for(GTK_WINDOW(SaveWindow), GTK_WINDOW(GPE_WLANCFG));
			if (gtk_dialog_run(GTK_DIALOG(SaveWindow)) != GTK_RESPONSE_YES) 
			{
				memcpy(CurrentScheme, OrigScheme, sizeof(Scheme_t));
			} else
			{
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
					
					strcpy(ListEntry[0], CurrentScheme->Scheme);
					strcpy(ListEntry[1], CurrentScheme->Socket);
					strcpy(ListEntry[2], CurrentScheme->Instance);
					strcpy(ListEntry[3], CurrentScheme->HWAddress);
				
					gtk_list_store_append(liststore, &iter);
					gtk_list_store_set(liststore, &iter, 0, ListEntry[0], 1, ListEntry[1], 2 , ListEntry[2], 3, ListEntry[3], 4, CurrentScheme, -1);
				}
			}
			gtk_widget_destroy(SaveWindow);


		} else 
		{
			gtk_notebook_set_page(GTK_NOTEBOOK(MainWindow), 0);
		}
		free(OrigScheme);
	} else 
	{
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);
			
			for (count = 0; count<SchemeCount; count++)
			{
				
				gtk_tree_model_get (GTK_TREE_MODEL(liststore), &iter, 4, &Scheme, -1);
				gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter);
				memcpy(&schemelist[count], Scheme, sizeof(Scheme_t));
				free(Scheme);
			}
			
			write_back_configfile(WLAN_CONFIGFILE, schemelist, SchemeCount);
			gtk_main_quit();
	}

	return TRUE;
}


void on_GPE_WLANCFG_show (GtkWidget *widget, gpointer user_data)
{
	gint       	count;
	gint		aRow;
	gchar      	*ListEntry[4];

	GtkWidget	*treeview;
 	GtkTreeIter	iter;
	GtkListStore	*liststore;
	
	treeview=lookup_widget(GPE_WLANCFG, "tvSchemeList");
	liststore = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
	
	SchemeCount = parse_configfile(WLAN_CONFIGFILE);
		
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
		
		strcpy(ListEntry[0], CurrentScheme->Scheme);
		strcpy(ListEntry[1], CurrentScheme->Socket);
		strcpy(ListEntry[2], CurrentScheme->Instance);
		strcpy(ListEntry[3], CurrentScheme->HWAddress);
		
		gtk_list_store_append(liststore, &iter);
		gtk_list_store_set(liststore, &iter, 0, ListEntry[0], 1, ListEntry[1], 2 , ListEntry[2], 3, ListEntry[3], 4, CurrentScheme, -1);
	}	

}


void on_btnUp_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget	*treeview;
 	GtkTreeSelection *selection;
	GtkTreeIter	iter;
	GtkTreeIter	prev;
	GtkTreeModel	*liststore;
	GtkTreePath     *treepath;
	void		*Values[5];
	
	treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if(gtk_tree_selection_get_selected (selection, &liststore, &iter))
	{
		treepath = gtk_tree_model_get_path(liststore, &iter);
		gtk_tree_path_prev(treepath);
		if (gtk_tree_model_get_iter(liststore, &prev, treepath))
		{
			gtk_list_store_insert_before(GTK_LIST_STORE(liststore), &prev, &prev);
			gtk_tree_model_get(liststore, &iter, 0, &Values[0], 1, &Values[1], 2, &Values[2], 3, &Values[3], 4, &Values[4], -1);
			gtk_list_store_set(GTK_LIST_STORE(liststore), &prev, 0, Values[0], 1, Values[1], 2 , Values[2], 3, Values[3], 4, Values[4], -1);
			gtk_list_store_remove(GTK_LIST_STORE(liststore), &iter);
			gtk_tree_selection_select_iter(selection, &prev);
			HasChanged = TRUE;
		}
		gtk_tree_path_free(treepath);
	}
}


void on_btnDown_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget	*treeview;
 	GtkTreeSelection *selection;
	GtkTreeIter	iter;
	GtkTreeIter	next;
	GtkTreeModel	*liststore;
	void		*Values[5];
	
	treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if(gtk_tree_selection_get_selected (selection, &liststore, &iter))
	{
		next = iter;
		if (gtk_tree_model_iter_next(liststore, &next))
		{
			gtk_list_store_insert_after(GTK_LIST_STORE(liststore), &next, &next);
			gtk_tree_model_get (liststore, &iter, 0, &Values[0], 1, &Values[1], 2, &Values[2], 3, &Values[3], 4, &Values[4], -1);
			gtk_list_store_set(GTK_LIST_STORE(liststore), &next, 0, Values[0], 1, Values[1], 2 , Values[2], 3, Values[3], 4, Values[4], -1);
			gtk_list_store_remove(GTK_LIST_STORE(liststore), &iter);
			gtk_tree_selection_select_iter(selection, &next);
			HasChanged = TRUE;
		}
	}
}


void on_btnDelete_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget	*DeleteVerify;
	
	Scheme_t	*Scheme;
	

	GtkWidget	*treeview;
 	GtkTreeSelection *selection;
	GtkTreeIter	iter;
	GtkTreeModel	*liststore;
	
	treeview  = lookup_widget(GPE_WLANCFG, "tvSchemeList");
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if(gtk_tree_selection_get_selected (selection, &liststore, &iter))
	{
		DeleteVerify=create_dlgDeleteEntry();
		gtk_window_set_transient_for(GTK_WINDOW(DeleteVerify), GTK_WINDOW(GPE_WLANCFG));
		if (gtk_dialog_run(GTK_DIALOG(DeleteVerify)) == GTK_RESPONSE_YES)
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
		gtk_widget_destroy(DeleteVerify);
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
	
		if (!strcmp(CurrentScheme->EncMode, "open"))
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbOpen");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);		
		}
		else
		{
			widget=lookup_widget(GTK_WIDGET(GPE_WLANCFG),"rbRestricted");
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
		gtk_entry_set_text(GTK_ENTRY(widget),CurrentScheme->ActiveKey);
	
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

	MainWin=lookup_widget(GTK_WIDGET(button), "GPE_WLANCFG");
	HelpWin=create_GeneralHelpWin();
	
	gtk_window_set_transient_for(GTK_WINDOW(HelpWin), GTK_WINDOW(MainWin));

	
	textview=lookup_widget(HelpWin, "twHelp");
	buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

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
	
	gtk_dialog_run(GTK_DIALOG(HelpWin));
	gtk_widget_destroy(HelpWin);
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

