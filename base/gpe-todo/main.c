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

#include <gtk/gtk.h>

#include "todo.h"

guint window_x = 240, window_y = 320;

static GtkWidget *the_notebook;
static GtkWidget *window;

static GtkWidget *top_level (void);

struct todo_list *lists = NULL;

static int ystep = 16;

GtkWidget *edit_todo(struct todo_list *list, struct todo_item *item);

void
add_new_event(struct todo_list *list, time_t t, const char *what, item_state state)
{
  struct todo_item *i = g_malloc(sizeof(struct todo_item));
  struct todo_item *li = list->items, *li2 = NULL;

  i->what = what;
  i->time = t;
  i->state = state;

  while (li && (li->time > t))
    {
      li2 = li;
      li = li->next;
    }

  i->next = li;
  i->prev = li2;

  if (li2)
    li2->next = i;
  else
    list->items = i;

  if (li)
    li->prev = i;

  gtk_widget_draw (list->widget, NULL);
}

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
  struct todo_item *i = t->items;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  gdk_draw_rectangle(drawable, white_gc, TRUE, 0, 0, max_width, max_height);

  y = 0;

  while (i)
    {
      gdk_draw_text(drawable, widget->style->font, black_gc,
		    4, y + widget->style->font->ascent, 
		    i->what, strlen(i->what));
      if (i->state == COMPLETED)
	gdk_draw_line (drawable, black_gc, 4, y + widget->style->font->ascent / 2, max_width - 4, y + widget->style->font->ascent / 2);
      y += ystep;
      i = i->next;
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
      struct todo_item *i = list->items;
      while (idx && i)
	{
	  i = i->next;
	  idx--;
	}
      if (i)
	gtk_widget_show_all (edit_todo (list, i));
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

  gtk_notebook_prepend_page (GTK_NOTEBOOK (notebook), vbox, label);
  list->widget = draw;
}

int
new_list_id (void)
{
  int id = 0;
  int found;
  do 
    {
      struct todo_list *t;
      id ++;
      found = 0;
      for (t = lists; t; t = t->next)
	{
	  if (t->id == id)
	    {
	      found = 1;
	      break;
	    }
	}
    } while (found);

  return id;
}

struct todo_list *
new_list (int id, const char *title)
{
  struct todo_list *t = g_malloc (sizeof (struct todo_list));
  t->items = NULL;
  t->title = title;
  t->id = id;
  t->next = lists;
  lists = t;
}

static void
ui_create_new_list(GtkWidget *widget,
		   GtkWidget *d)
{
  char *title = gtk_editable_get_chars (GTK_EDITABLE (d), 0, -1);
  int id = new_list_id ();
  struct todo_list *t = new_list (id, title);
  sql_add_list (id, title);
  gtk_container_remove (GTK_CONTAINER (window), the_notebook);
  the_notebook = top_level ();
  gtk_container_add (GTK_CONTAINER (window), the_notebook);
  gtk_widget_show_all (window);
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

static GtkWidget *
top_level (void)
{
  GtkWidget *notebook = gtk_notebook_new();
  GtkWidget *new_list = gtk_label_new ("New list");
  GtkWidget *new_window = build_new_list_window ();
  struct todo_list *l;

  for (l = lists; l; l = l->next)
    display_list (notebook, l);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), new_window, new_list);

  return notebook;
}

static void
open_window (void)
{
  GtkWidget *listbook = top_level();
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  the_notebook = listbook;
  gtk_container_add (GTK_CONTAINER (window), listbook);

  gtk_widget_set_usize (window, 240, 320);
  gtk_widget_show_all (window);
}

int
main(int argc, char *argv[])
{
  gtk_init (&argc, &argv);

  sql_start ();

  open_window ();
  
  gtk_main ();

  return 0;
}
