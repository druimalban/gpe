#include <gtk/gtk.h>


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
