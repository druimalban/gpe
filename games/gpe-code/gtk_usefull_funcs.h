/* gtk_usefull_funcs.h, by Michael Berg <mberg@nmt.edu>
 * Forward declarations for the functions in gtk_usefull_funcs.c
 */

/* Forward declarations */
GtkWidget *button_new_with_properties (const gchar *label, int width,
				       int height, int is_sensitive);

GtkWidget *text_new_with_scrollbars (GtkWidget *vbox, 
				     int text_width, int text_height, 
				     int vscroll, int hscroll, 
				     int editable);

void dismiss_window (GtkWidget *widget, gpointer window);

void display_ok_dialog (const gchar *title, GtkWidget *widget, 
			int widget_spacing,
			GtkSignalFunc extra_ok_func, gpointer data,
			int resizable, int has_focus);

void display_yes_no_dialog (const gchar *title, GtkWidget *widget, 
			    int widget_spacing,
			    GtkSignalFunc yes_func, gpointer yes_data,
			    GtkSignalFunc no_func, gpointer no_data,
			    int resizable, int has_focus);

void display_text_file (const gchar *title, const gchar *file_to_open,
			GtkSignalFunc extra_ok_func, gpointer ok_data);


GtkWidget *create_submenu (GtkWidget *menu_bar, gchar *label, 
			   int right_justified);

GtkWidget *create_menu_item (GtkWidget *menu, gchar *label, 
			     GtkSignalFunc func, gpointer data);

GtkWidget *create_menu_check (GtkWidget *menu, gchar *label, 
			      GtkSignalFunc func, gpointer data);

GtkWidget *create_menu_radio (GtkWidget *menu, gchar *label, GSList **group,
			      GtkSignalFunc func, gpointer data);
