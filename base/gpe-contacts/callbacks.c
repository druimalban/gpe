#include <gtk/gtk.h>
#include <stdio.h>

#include "interface.h"
#include "support.h"
#include "db.h"
#include "proto.h"

#include <gpe/gtkdatecombo.h>

extern GtkWidget *clist;
extern gchar* active_chars;
extern void edit_person (struct person *p);
	

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
retrieve_special_fields (GtkWidget *edit, struct person *p)
{
  GSList *cl = gtk_object_get_data (GTK_OBJECT (edit), "category-widgets");
  db_delete_tag (p, "CATEGORY");
  while (cl)
    {
      GtkWidget *w = cl->data;
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
	{
	  guint c = (guint) gtk_object_get_data (GTK_OBJECT (w), "category");
	  char buf[32];
	  struct tag_value *t;
	  snprintf (buf, sizeof (buf) - 1, "%d", c);
	  buf[sizeof (buf) - 1] = 0;
	  db_set_multi_data (p, "CATEGORY", g_strdup (buf));
	}
      cl = cl->next;
    }

  db_delete_tag (p, "BIRTHDAY");
  {
    GtkDateCombo *c = GTK_DATE_COMBO (lookup_widget (edit, "datecombo"));
    if (c->set)
      {
	char buf[32];
	snprintf (buf, sizeof (buf) - 1, "%04d%02d%02d", c->year, c->month, c->day);
	buf[sizeof (buf) - 1] = 0;
	db_set_data (p, "BIRTHDAY", g_strdup (buf));
      }
  }
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

  retrieve_special_fields (edit, p);

  if (commit_person (p))
    {
      gtk_widget_destroy (edit);
      discard_person (p);
      update_display ();
    }
}



void
on_nbList_switch_page                  (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        gint             page_num,
                                        gpointer         user_data)
{
  switch (page_num)
    {
       case 0:
         clist = lookup_widget(GTK_WIDGET(notebook),"clist1");
         sprintf(active_chars,"ABC");
         update_display();
       break;	
       case 1:
         clist = lookup_widget(GTK_WIDGET(notebook),"clist2");
         sprintf(active_chars,"DEF");
         update_display();
       break;	
       case 2: 
         clist = lookup_widget(GTK_WIDGET(notebook),"clist3");
         sprintf(active_chars,"GHIJ");
         update_display();
       break;	
       case 3:
         clist = lookup_widget(GTK_WIDGET(notebook),"clist4");
         sprintf(active_chars,"KLMN");
         update_display();
       break;	
       case 4:
         clist = lookup_widget(GTK_WIDGET(notebook),"clist5");
         sprintf(active_chars,"OPQR");
         update_display();
       break;	
       case 5:
         clist = lookup_widget(GTK_WIDGET(notebook),"clist6");
         sprintf(active_chars,"STUV");
         update_display();
       break;	
       case 6:
         clist = lookup_widget(GTK_WIDGET(notebook),"clist7");
         sprintf(active_chars,"WXYZ");
         update_display(); 
       break;	
    }
}



void
selection_made                         (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  guint id;
	
  if (event->type == GDK_2BUTTON_PRESS)
    {
      struct person *p;
      id = (guint)gtk_clist_get_row_data (GTK_CLIST (clist), row);
      p = db_get_by_uid (id);
      edit_person (p);
    }
}

