#include <gtk/gtk.h>


void
on_new_game1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quitter_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_w_human_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_w_deep_blue_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_w_crazy_yellow_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_b_human_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_b_deep_blue_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_b_crazy_yellow_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_novice1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_easy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_medium1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_good1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_crazy_yellows_props1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);


void
close_about                            (GtkButton       *button,
                                        gpointer         user_data);


gboolean
close_about2                           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);


