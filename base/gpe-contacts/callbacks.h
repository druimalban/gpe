
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
on_b1_clicked                          (GtkButton       *button,
                                        gpointer         user_data);

void
on_button2_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button3_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button4_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button5_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button7_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button8_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_nbList_switch_page                  (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        gint             page_num,
                                        gpointer         user_data);
void 
selection_made 	                       (GtkWidget *clist, 
                                        gint row, 
										gint column, 
		                                GdkEventButton *event, 
										GtkWidget *widget);

void
on_button1_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

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
