#include <gtk/gtk.h>

#include "config-parser.h"

/*
#define	LINE_NOT_PRESENT 	0xffff
#define	LINE_NEW 			0xfffe
#define L_Scheme 	0
#define L_Socket 	1
#define L_Instance 	2
#define L_HWAddress	3
#define L_Info 		4
#define L_ESSID 	5
#define L_NWID		6
#define L_Mode 		7
#define L_Channel 	8
#define L_Rate 		9
#define L_Encryption 10
#define L_EncMode 	11
#define L_KeyFormat 12
#define L_key1 		13
#define L_key2 		14
#define L_key3 		15
#define L_key4 		16
#define L_ActiveKey	17
#define L_iwconfig	18
#define L_iwspy		19
#define L_iwpriv	20


typedef struct {
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
	unsigned int lines[21];
} Scheme_t;
*/
extern Scheme_t *CurrentScheme;

void
on_GeneralHelp_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_GeneralHelpOK_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_GeneralHelpWin_de_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_GPE_WLANCFG_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_SaveChangesYes_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_SaveChangesNo_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_RFParamsHelp_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_HelpWin_de_event                    (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_RFParamsOK_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_GeneralHelp_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_RFParamsHelp_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_SaveChanges_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_WEPHelp_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_KeyHelp_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_ExpertHelp_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_WEPHelpOK_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_EncKeysHelpOK_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_ExpertHelpOK_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_ChannelDefault_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ChannelFreq_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ChannelChan_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_EncryptionOn_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_EncryptionOff_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ModeOpen_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ModeRestricted_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_Mode_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_Sensitivity_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_Rate_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_Frequency_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_Channe_lNr_changed                  (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_ActiveKeyNr_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_KeysHex_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_KeysStrings_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_Mode_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_GPE_WLAN_de_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_DeleteYes_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_DeleteNo_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_SchemeHelp_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_SchemeHelpOK_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_ListItemUp_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_ListItemDown_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_SchemeDelete_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_SchemeNew_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_SchemeEdit_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_Scheme_changed                      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_Socket_changed                      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_Instance_changed                    (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_HWAddress_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_SchemeList_select_row               (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_SchemeList_unselect_row             (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_GPE_WLANCFG_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_SchemeUse_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_GPE_WLANCFG_show                    (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_SchemeList_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_ChannelNr_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);
