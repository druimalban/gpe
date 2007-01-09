#include <gtk/gtk.h>


gboolean
on_GPE_Alarm_de_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_NewAlarm_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmOptions_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmDelete_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmCancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

int insert_rtcd_alarms(void);

void
on_AlarmOptTry_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmOptOK_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlTone2On_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_AlTone2Off_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_AlarmCList_select_row               (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_AlarmDelete_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmEdit_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data);


void
on_AlarmCList_unselect_row             (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_AlarmHour_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_AlarmMinute_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_AlarmComment_changed                (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_AlarmType_released                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmType_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmType_selected                  (GtkMenuShell *menu_shell,
                   			gpointer data);

void
on_AlarmTry_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_StopAlarm_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmTry_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmOptOK_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmOptTry_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_file1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_MainMenu_deactivate                 (GtkMenuShell    *menushell,
                                        gpointer         user_data);

gboolean
on_GPE_Alarm_key_press_event           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_PrefsOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_PrefsCancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_AlarmsPrefs_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_MainMenu_deactivate                 (GtkMenuShell    *menushell,
                                        gpointer         user_data);

void
on_AlarmAboutOK_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_AlarmAbout_de_event                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
RTCD_sighandler                        (int signo);

void
on_AlarmMute_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmDelay_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_AlarmACK_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_GPE_Alarm_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_AlarmVolume_changed                 (GtkAdjustment	*adj,
                                        gpointer         user_data);

void set_audio_volume(gint value);


gboolean
on_AlarmWin_de_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);
