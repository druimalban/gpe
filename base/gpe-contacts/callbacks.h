
void main_showlist(GtkObject *object);

extern void on_main_new_clicked (GtkWidget *widget, gpointer d);
extern void on_main_details_clicked (GtkWidget *widget, gpointer d);
extern void on_main_delete_clicked (GtkWidget *widget, gpointer d);

void
on_edit_bt_image_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_edit_cancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_edit_save_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_edit_clear_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_edit_cancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_edit_save_clicked                   (GtkButton       *button,
                                        gpointer         user_data);


void
on_categories_clicked (GtkButton *button, gpointer data);

void
on_bDetAdd_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_bDetRemove_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_clist8_select_row                   (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_clist8_unselect_row                 (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_setup_destroy                       (GtkObject       *object,
                                        gpointer         user_data);
