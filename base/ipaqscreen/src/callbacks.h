void
on_brightness_hscale_draw              (GtkWidget       *widget,
                                        GdkRectangle    *area,
                                        gpointer         user_data);

void
on_rotation_entry_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_calibrate_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_closebutton_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_mainwindow_destroy                  (GtkObject       *object,
                                        gpointer         user_data);
