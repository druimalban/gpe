#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include <stdlib.h>

extern GtkWidget *GPE_WLAN;
static gboolean HasChanged = FALSE;
static gint SchemeListSelRow = -1;

struct Scheme_t {
	int IsRow;
	char Scheme[32];
	char Socket[32];
	char Instance[32];
	char HWAddress[32];
	char Info[64];
	char ESSID[32];
	char NWID[32];
	char Mode[16];
	char Channel[32];
	char Rate[8];
	char Encryption[4];
	char EncMode[16];
	gboolean KeyFormat;
	char key1[64];
	char key2[64];
	char key3[64];
	char key4[64];
	char ActiveKey[4];
	char iwconfig[256];
	char iwspy[256];
	char iwpriv[256];
} *CurrentScheme;

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
		gtk_main_quit();
	}

return TRUE;
}


void
on_SaveChangesYes_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *SaveDialog;
GtkWidget *widget;
GtkWidget *SchemeList;
gchar *ListEntry[4];
int i;

	HasChanged = FALSE;
	SaveDialog=lookup_widget(GTK_WIDGET(button),"SaveChanges");

	SchemeList=lookup_widget(GPE_WLAN,"SchemeList");
	for (i = 0; i < 4; i++) {
		ListEntry[i]=(gchar *)malloc(32 * sizeof(char));
		memset(ListEntry[i],0,32);
	}
	if (SchemeListSelRow == -1) {
		/* New entry */
		strcpy(ListEntry[0],CurrentScheme->Scheme);
		strcpy(ListEntry[1],CurrentScheme->Socket);
		strcpy(ListEntry[2],CurrentScheme->Instance);
		strcpy(ListEntry[3],CurrentScheme->HWAddress);
		gtk_clist_append(GTK_CLIST(SchemeList),ListEntry);
		gtk_clist_set_row_data(GTK_CLIST(SchemeList), GTK_CLIST(SchemeList)->rows-1, (gpointer)CurrentScheme);
	} else {
		/* Update existing entry !? */
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
	free(CurrentScheme);
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
	widget=lookup_widget(GTK_WIDGET(togglebutton),"Channe_lNr");
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
	widget=lookup_widget(GTK_WIDGET(togglebutton),"Channe_lNr");
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
	widget=lookup_widget(GTK_WIDGET(togglebutton),"Channe_lNr");
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
	strcpy(CurrentScheme->EncMode,"restricted");
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
	HasChanged = TRUE;
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

	DeleteVerify=lookup_widget(GTK_WIDGET(button),"DeleteVerify");
	gtk_widget_hide(DeleteVerify);
	gtk_widget_destroy(DeleteVerify);
	SchemeList=lookup_widget(GPE_WLAN, "SchemeList");
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

	CurrentScheme=(struct Scheme_t *)malloc(sizeof(struct Scheme_t));
	memset(CurrentScheme,0,sizeof(struct Scheme_t));
	CurrentScheme->IsRow = -1;
	strcpy(CurrentScheme->Scheme,"*");
	strcpy(CurrentScheme->Socket,"*");
	strcpy(CurrentScheme->Instance,"*");
	strcpy(CurrentScheme->HWAddress,"*");
	strcpy(CurrentScheme->NWID,"any");
	strcpy(CurrentScheme->Mode,"auto");
	strcpy(CurrentScheme->Rate,"11M");
	strcpy(CurrentScheme->Encryption,"enc off");
	strcpy(CurrentScheme->EncMode,"");
	strcpy(CurrentScheme->ActiveKey,"[1]");
	HasChanged = TRUE;

	widget=lookup_widget(GTK_WIDGET(button),"SchemeList");
	gtk_clist_unselect_all(GTK_CLIST(widget));

	SchemeListSelRow = -1;

	widget=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(widget), 1);
}


void
on_SchemeEdit_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *SchemeList;
GtkWidget *widget;

	SchemeList=lookup_widget(GTK_WIDGET(button),"SchemeList");

	CurrentScheme = (struct Scheme_t *) gtk_clist_get_row_data (GTK_CLIST(SchemeList), SchemeListSelRow);

	widget=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(widget), 1);
}


void
on_Scheme_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
	strcpy(CurrentScheme->Scheme,gtk_entry_get_text(GTK_ENTRY(editable)));
}


void
on_Socket_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
	strcpy(CurrentScheme->Socket,gtk_entry_get_text(GTK_ENTRY(editable)));
}


void
on_Instance_changed                    (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
	strcpy(CurrentScheme->Instance,gtk_entry_get_text(GTK_ENTRY(editable)));
}


void
on_HWAddress_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
	HasChanged = TRUE;
	strcpy(CurrentScheme->HWAddress,gtk_entry_get_text(GTK_ENTRY(editable)));
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


