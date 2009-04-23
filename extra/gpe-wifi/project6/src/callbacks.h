#include <gtk/gtk.h>

void
set_return_cmd			       (gchar *return_str);

void
on_window1_show                        (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_radiobutton7_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_radiobutton6_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_radiobutton8_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_radiobutton9_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_button1_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_radiobutton5_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_radiobutton12_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
