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

#define _(_x) gettext(_x)

static int ystep;

static void
new_todo_item(GtkWidget *w, gpointer list)
{
  GtkWidget *todo = edit_todo (list, NULL);
  gtk_widget_show_all (todo);
}

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;
  guint max_width;
  guint max_height;
  guint hour, y;
  struct todo_list *t = (struct todo_list *)user_data;
  GList *iter;
  GdkFont *font = widget->style->font;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  ystep = font->ascent + font->descent;

  gdk_draw_rectangle(drawable, white_gc, TRUE, 0, 0, max_width, max_height);

  y = 0;

  for (iter = t->items; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;

      gdk_draw_text (drawable, font, black_gc, 4, y + font->ascent, 
		    i->summary, strlen(i->summary));

      if (i->state == COMPLETED)
	gdk_draw_line (drawable, black_gc, 4, y + font->ascent / 2, 
		       4 + gdk_string_width (font, i->summary), 
		       y + font->ascent / 2);

      y += ystep;
    }

  return TRUE;
}

static void
draw_click_event (GtkWidget *widget,
		  GdkEventButton *event,
		  struct todo_list *list)
{
  if (event->type == GDK_2BUTTON_PRESS)
    {
      unsigned int idx = event->y / ystep;
      GList *i = list->items;
      while (idx && i)
	{
	  i = i->next;
	  idx--;
	}
      if (i)
	gtk_widget_show_all (edit_todo (list, i->data));
    }
}

static void
display_list (GtkWidget *notebook, struct todo_list *list)
{
  GtkWidget *draw = gtk_drawing_area_new();
  GtkWidget *label = gtk_label_new (list->title);
  GtkWidget *buttons = gtk_hbox_new (FALSE, 0);
  GtkWidget *new_event = gtk_button_new_with_label ("New item");
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  
  gtk_signal_connect (GTK_OBJECT (draw), "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      list);

  gtk_signal_connect (GTK_OBJECT (draw), "button_press_event",
		      GTK_SIGNAL_FUNC (draw_click_event), list);

  gtk_widget_add_events (GTK_WIDGET (draw), GDK_BUTTON_PRESS_MASK);

  gtk_box_pack_end (GTK_BOX (buttons), new_event, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), draw, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (new_event), "clicked",
		      GTK_SIGNAL_FUNC (new_todo_item), list);

  gtk_widget_show_all (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), -1);
  gtk_widget_draw (notebook, NULL);
  list->widget = draw;
}

static void
ui_create_new_list(GtkWidget *widget,
		   GtkWidget *d)
{
  char *title = gtk_editable_get_chars (GTK_EDITABLE (d), 0, -1);
  int id = new_list_id ();
  struct todo_list *t = new_list (id, title);
  display_list (the_notebook, t);
  sql_add_list (id, title);
}

static GtkWidget *
build_new_list_window (void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *ok = gtk_button_new_with_label ("OK");
  GtkWidget *buttons = gtk_hbox_new (FALSE, 0);
  GtkWidget *label = gtk_label_new ("Name:");
  GtkWidget *name = gtk_entry_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);

  if (lists == NULL)
    gtk_entry_set_text (GTK_ENTRY (name), "General");

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), name, TRUE, TRUE, 2);

  gtk_box_pack_end (GTK_BOX (buttons), ok, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);
  
  gtk_signal_connect (GTK_OBJECT (ok), "clicked",
		      GTK_SIGNAL_FUNC (ui_create_new_list), name);

  return vbox;
}

GtkWidget *
top_level (void)
{
  GtkWidget *notebook = gtk_notebook_new();
  GtkWidget *new_list = gtk_label_new ("New list");
  GtkWidget *new_window = build_new_list_window ();
  GSList *l;

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), new_window, new_list);

  for (l = lists; l; l = l->next)
    display_list (notebook, l->data);

  return notebook;
}

