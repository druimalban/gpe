#include <gtk/gtk.h>
#include <sqlite.h>

#include "interface.h"
#include "support.h"
#include "db.h"
#include "gtkdatecombo.h"

void main_showlist(GtkObject *object) 
{
  /* redisplays the list in the main window */
  GtkWidget *widget;
  gchar *text;
  char query[1024];
  
  /* get category ID */
  widget = lookup_widget(object, "main_cat");
  text = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  
  snprintf (query, 1023, "select ID from contact_cats where Name='%s'", text);
}

void on_main_new_clicked (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = create_edit ();
  gtk_widget_show (w);
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
on_edit_bt_image_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *filesel = gtk_file_selection_new ("Select image");

  gtk_widget_show_all (filesel);
}

