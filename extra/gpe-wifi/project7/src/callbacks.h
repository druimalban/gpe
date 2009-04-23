#include <gtk/gtk.h>


void
on_window1_show                        (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_button1_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_button2_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);


gboolean
on_button1_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_button3_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_button3_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_button2_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);
