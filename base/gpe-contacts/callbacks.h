
void main_showlist(GtkObject *object);

extern void on_main_new_clicked (GtkWidget *widget, gpointer d);
extern void on_main_details_clicked (GtkWidget *widget, gpointer d);
extern void on_main_delete_clicked (GtkWidget *widget, gpointer d);

GtkWidget*
gtk_date_combo_new (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

GtkWidget*
create_date_combo (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

void
on_edit_bt_image_clicked               (GtkButton       *button,
                                        gpointer         user_data);
