#include <gtk/gtk.h>
#include <sqlite.h>

#include "interface.h"
#include "support.h"
#include "db.h"
#include "gtkdatecombo.h"

void main_showlist(GtkWidget *object) 
{
  /* redisplays the list in the main window */
  GtkWidget *widget;
  gchar *text;
  char query[1024];
  
  /* get category ID */
  widget = lookup_widget (object, "main_cat");
  text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  
  snprintf (query, 1023, "select ID from contact_cats where Name='%s'", text);
}

void on_main_new_clicked (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = edit_window ();
  GtkWidget *name = lookup_widget (w, "name_entry");
  gtk_widget_show (w);
  gtk_widget_grab_focus (name);
}

void on_main_details_clicked (GtkWidget *widget, gpointer d)
{
}

void on_main_delete_clicked (GtkWidget *widget, gpointer d)
{
}

GtkWidget*
create_date_combo (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
  return gtk_date_combo_new ();
}

void 
store_filename(GtkWidget *w, GtkFileSelection *selector) 
{
  gchar *selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (selector));
}

void
on_edit_bt_image_clicked (GtkButton       *button,
			  gpointer         user_data)
{
  GtkWidget *filesel = gtk_file_selection_new ("Select image");

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC (store_filename), filesel);

  gtk_signal_connect_object (GTK_OBJECT (
				 GTK_FILE_SELECTION (filesel)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) filesel);

  gtk_signal_connect_object (GTK_OBJECT (
				 GTK_FILE_SELECTION (filesel)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) filesel);

  gtk_widget_show_all (filesel);
}

#if 0
void
structure_add_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
structure_edit_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
structure_delete_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{

}
#endif

