#include <gtk/gtk.h>


void
on_Start_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_Stop_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_Pause_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_Forw_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_DeleteList_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Rew_clicked                         (GtkButton       *button,
                                        gpointer         user_data);

void
on_AddList_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_Slider_changed                     (GtkAdjustment       *adj,
                                        gpointer         user_data);


void
on_VolScale_changed                     (GtkAdjustment       *adj,
                                        gpointer         user_data);


void
on_BassScale_changed                     (GtkAdjustment       *adj,
                                        gpointer         user_data);


void
on_TrebleScale_changed                     (GtkAdjustment       *adj,
                                        gpointer         user_data);



void
on_filesel_ok_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_filesel_cancel_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_filesel_cancel_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_FileList_select_row                 (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_normal1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_repeat_1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_repeat_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_shuffle1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_FileList_click_column               (GtkCList        *clist,
                                        gint             column,
                                        gpointer         user_data);
