#include <gtk/gtk.h>
#include <sqlite.h>

#include "interface.h"
#include "support.h"
#include "db.h"

#include <gpe/gtkdatecombo.h>

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


void
on_structure_save_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{

}

/* ... */

void
on_edit_clear_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_edit_cancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_destroy (GTK_WIDGET (user_data));
}


void
on_edit_save_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *edit = (GtkWidget *)user_data;
  GtkWidget *w;
  GSList *data = NULL;
  GSList *tags;
  gchar *s;
  struct person *p = gtk_object_get_data (GTK_OBJECT (edit), "person");
  if (p == NULL)
    p = new_person ();
  
  for (tags = gtk_object_get_data (GTK_OBJECT (edit), "tag-widgets");
       tags;
       tags = tags->next)
    {
      GtkWidget *w = tags->data;
      gchar *text = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
      gchar *tag = gtk_object_get_data (GTK_OBJECT (w), "db-tag");
      db_set_data (p, tag, text);
    }

  if (commit_person (p))
    {
      gtk_widget_destroy (edit);
      discard_person (p);
      update_display ();
    }
}
