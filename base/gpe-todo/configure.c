/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>

#include <gtk/gtk.h>

#include "todo.h"
#include "pixmaps.h"
#include "errorbox.h"
#include "render.h"
#include "picturebutton.h"

#include "tick.xpm"
#include "box.xpm"

#define _(_x) gettext(_x)

static void
ui_del_category (GtkWidget *widget,
		 gpointer user_data)
{
  GtkCList *clist = GTK_CLIST (user_data);
  if (clist->selection)
    {
      guint row = (guint)clist->selection->data;
      struct todo_category *t = gtk_clist_get_row_data (clist, row);
      del_category (t);
      gtk_clist_remove (clist, row);
      gtk_widget_draw (GTK_WIDGET (clist), NULL);
      categories_menu ();
    }
}

static void
ui_create_new_category (GtkWidget *widget,
			GtkWidget *d)
{
  GtkWidget *entry = gtk_object_get_data (GTK_OBJECT (d), "entry");
  char *title = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  GSList *l;
  GtkWidget *clist = gtk_object_get_data (GTK_OBJECT (d), "clist");
  gchar *line_info[1];
  guint row;
  struct todo_category *t;

  if (title[0] == 0)
    {
      gpe_error_box (_("Category name must not be blank"));
      gtk_widget_destroy (d);
      return;
    }
  
  for (l = categories; l; l = l->next)
    {
      struct todo_category *t = l->data;
      if (!strcmp (title, t->title))
	{
	  gpe_error_box (_("A category by that name already exists"));
	  gtk_widget_destroy (d);
	  return;
	}
    }

  t = new_category (title);
  line_info[0] = title;
  row = gtk_clist_append (GTK_CLIST (clist), line_info);
  gtk_clist_set_row_data (GTK_CLIST (clist), row, t);
  categories_menu ();
  gtk_widget_destroy (d);
}

static void
close_window(GtkWidget *widget,
	     GtkWidget *w)
{
  gtk_widget_destroy (w);
}

static void
new_category_box (GtkWidget *w, gpointer data)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_DIALOG);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *ok;
  GtkWidget *cancel;
  GtkWidget *buttons = gtk_hbox_new (FALSE, 0);
  GtkWidget *label = gtk_label_new ("Name:");
  GtkWidget *name = gtk_entry_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  guint x, y;
  guint screen_width, screen_height;
  GtkRequisition requisition;

  ok = gpe_picture_button (window->style, _("OK"), "ok");
  cancel = gpe_picture_button (window->style, _("Cancel"), "cancel");

  gtk_widget_show (ok);
  gtk_widget_show (cancel);
  gtk_widget_show (buttons);
  gtk_widget_show (label);
  gtk_widget_show (hbox);
  gtk_widget_show (name);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), name, TRUE, TRUE, 2);

  gtk_box_pack_end (GTK_BOX (buttons), ok, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (buttons), cancel, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);

  gtk_object_set_data (GTK_OBJECT (window), "entry", name);
  gtk_object_set_data (GTK_OBJECT (window), "clist", data);
  
  gtk_signal_connect (GTK_OBJECT (ok), "clicked",
		      GTK_SIGNAL_FUNC (ui_create_new_category), window);
  
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked",
		      close_window, window);

  gtk_widget_show (vbox);

  gtk_window_set_title (GTK_WINDOW (window), _("New category"));

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_realize (window);
  gtk_widget_size_request (window, &requisition);  
  gdk_window_get_pointer (NULL, &x, &y, NULL);
  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();

  x = CLAMP (x - (requisition.width / 2), 0, MAX (0, screen_width - requisition.width));
  y = CLAMP (y - 24, 0, MAX (0, screen_height - requisition.height));
  gtk_widget_set_uposition (window, MAX (x, 0), MAX (y, 0));

  gtk_widget_show (window);
  gtk_widget_grab_focus (name);
}


static void
close_configure (GtkWidget *w, gpointer data)
{
  gtk_widget_destroy (data);
}

void
configure (GtkWidget *w, gpointer list)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist = gtk_clist_new (1);
  GtkWidget *pw;
  GtkWidget *frame = gtk_frame_new (_("Categories"));
  GtkWidget *vboxtop = gtk_vbox_new (FALSE, 0);
  GtkWidget *okbutton;
  GSList *l;

  for (l = categories; l; l = l->next)
    {
      struct todo_category *t = l->data;
      gchar *line_info[1];
      guint row;
      
      line_info[0] = (gchar *)t->title;
      row = gtk_clist_append (GTK_CLIST (clist), line_info);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, t);
    }
  
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);

  gtk_widget_realize (window);

  okbutton = gpe_picture_button (window->style, _("OK"), "ok");

  gtk_signal_connect (GTK_OBJECT (okbutton), "clicked", close_configure, window);

  pw = gpe_render_icon (window->style, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("New"), 
			   _("Create a new category"), 
			   _("Create a new category"),
			   pw, new_category_box, clist);

  pw = gpe_render_icon (window->style, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Delete"),
			   _("Delete the selected category"), 
			   _("Delete the selected category"),
			   pw, ui_del_category, clist);

  gtk_widget_show (toolbar);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

  gtk_widget_show (clist);
  gtk_box_pack_start (GTK_BOX (vbox), clist, TRUE, TRUE, 0);

  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vboxtop), frame, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxtop), okbutton, FALSE, FALSE, 0);
  gtk_widget_show (vboxtop);
  gtk_container_add (GTK_CONTAINER (window), vboxtop);
  
  gtk_widget_set_usize (window, 240, 320);

  gtk_widget_show (window);
}
