#include <gtk/gtk.h>


gboolean
on_GPEBatMon_de_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
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
on_AboutOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_PrefsCancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_PrefsOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_GPEBatMon_key_press_event           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_GPEBatMon_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_MainMenu_deactivate                 (GtkMenuShell    *menushell,
                                        gpointer         user_data);

