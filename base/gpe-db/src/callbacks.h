#include <gtk/gtk.h>


void
on_select_db1_activate                 (GtkMenuItem     *menuitem,
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
on_SelectDB_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_DBTableCList_select_row             (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_DBTableCList_unselect_row           (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_DBSelectionOK_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_DBSelectionCancel_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_OpenTable_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_GPE_DB_Main_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_DBSelection_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_GPE_DB_Main_key_press_event         (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_MainMenu_deactivate                 (GtkMenuShell    *menushell,
                                        gpointer         user_data);

gboolean
on_GPE_DB_Main_button_press_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_CloseDB_clicked                     (GtkButton       *button,
                                        gpointer         user_data);
