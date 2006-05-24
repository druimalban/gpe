/* event-list.c - Event list widget implementation.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gpe/event-db.h>
#include <stdlib.h>
#include "globals.h"
#include "event-list.h"
#include "event-menu.h"

#ifdef IS_HILDON
#define ARROW_SIZE 30
#else
#define ARROW_SIZE 20
#endif

enum
{
  COL_EVENT,
  NUM_COLS
};

struct _GtkEventList
{
  GtkVBox widget;

  GtkTreeView *view;
  GSList *events;

  GtkEntry *entry;
  GtkComboBox *combo;

  guint timeout;

  /* Time of last update.  */
  time_t date;
  struct tm tm;
};

typedef struct
{
  GtkTreeViewClass tree_box_class;
  GObjectClass parent_class;
} EventListClass;

static void gtk_event_list_base_class_init (gpointer klass);
static void gtk_event_list_dispose (GObject *obj);
static void gtk_event_list_finalize (GObject *object);
static void gtk_event_list_init (GTypeInstance *instance, gpointer klass);

static GtkWidgetClass *parent_class;

GType
gtk_event_list_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (EventListClass),
	gtk_event_list_base_class_init,
	NULL,
	NULL,
	NULL,
	NULL,
	sizeof (struct _GtkEventList),
	0,
	gtk_event_list_init
      };

      type = g_type_register_static (gtk_vbox_get_type (),
				     "GtkEventList", &info, 0);
    }

  return type;
}

static void
gtk_event_list_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_event_list_finalize;
  object_class->dispose = gtk_event_list_dispose;

  widget_class = (GtkWidgetClass *) klass;
}

static void
gtk_event_list_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gtk_event_list_finalize (GObject *object)
{
  GtkEventList *event_list = GTK_EVENT_LIST (object);

  if (event_list->timeout > 0)
    /* Cancel any outstanding timeout.  */
    g_source_remove (event_list->timeout);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static char *
time_to_string (GtkEventList *event_list, Event *ev, struct tm *tm)
{
  if (is_reminder (ev))
    {
      if (tm->tm_year == event_list->tm.tm_year)
	return strftime_strdup_utf8_locale (_("%b %d"), tm);
      else
	return strftime_strdup_utf8_locale (_("%b %d, %y"), tm);
    }
  else
    {
      if (tm->tm_year == event_list->tm.tm_year
	  && tm->tm_yday == event_list->tm.tm_yday)
	return strftime_strdup_utf8_locale (_("%R"), tm);
      else if (tm->tm_year == event_list->tm.tm_year)
	return strftime_strdup_utf8_locale (_("%b %d %R"), tm);
      else
	return strftime_strdup_utf8_locale (_("%b %d, %y %R"), tm);
    }
}

static void
date_cell_data_func (GtkTreeViewColumn *col,
		     GtkCellRenderer *renderer,
		     GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer el)
{
  GtkEventList *event_list = GTK_EVENT_LIST (el);
  Event *ev;
  time_t t;
  struct tm tm;
  gchar *buffer;
  gboolean dealloc = TRUE;;
  
  gtk_tree_model_get (model, iter, COL_EVENT, &ev, -1);

  t = event_get_start (ev);
  localtime_r (&t, &tm);
  if (is_reminder (ev))
    {
      if (tm.tm_year == event_list->tm.tm_year
	  && tm.tm_yday == event_list->tm.tm_yday)
	{
	  buffer = _("Today");
	  dealloc = FALSE;
	}
      else
	buffer = time_to_string (event_list, ev, &tm);
    }
  else
    {
      if (event_get_start (ev) < event_list->date)
	/* Starts in the past.  */
	{
	  time_t end = event_get_start (ev) + event_get_duration (ev);

	  localtime_r (&end, &tm);
	  char *t = time_to_string (event_list, ev, &tm);
	  buffer = g_strdup_printf (_("Until %s"), t);
	  g_free (t);
	}
      else
	buffer = time_to_string (event_list, ev, &tm);
    }

  g_object_set(renderer, "text", buffer, NULL);

  if (dealloc)
    g_free (buffer);

  GdkColor color;
  if (event_get_color (ev, &color))
    g_object_set (renderer, "cell-background-gdk", &color, NULL);
  else
    g_object_set (renderer, "cell-background-gdk", NULL, NULL);
}

static void
summary_cell_data_func (GtkTreeViewColumn *col,
			GtkCellRenderer *renderer,
			GtkTreeModel *model,
			GtkTreeIter *iter,
			gpointer user_data)
{
  Event *ev;

  gtk_tree_model_get (model, iter, COL_EVENT, &ev, -1);
  char *s = event_get_summary (ev);
  g_object_set (renderer, "text", s, NULL);
  g_free (s);

  GdkColor color;
  if (event_get_color (ev, &color))
    g_object_set (renderer, "cell-background-gdk", &color, NULL);
  else
    g_object_set (renderer, "cell-background-gdk", NULL, NULL);
}

static void
end_cell_data_func (GtkTreeViewColumn *col,
		    GtkCellRenderer *renderer,
		    GtkTreeModel *model,
		    GtkTreeIter *iter,
		    gpointer el)
{
  GtkEventList *event_list = GTK_EVENT_LIST (el);
  Event *ev;
  time_t t;
  struct tm tm;
  gchar *buffer;
  gboolean dealloc = TRUE;;
  
  gtk_tree_model_get (model, iter, COL_EVENT, &ev, -1);

  t = event_get_start (ev) + event_get_duration (ev);
  localtime_r (&t, &tm);
  if (is_reminder (ev))
    {
      if (tm.tm_year == event_list->tm.tm_year
	  && tm.tm_yday == event_list->tm.tm_yday)
	{
	  buffer = _("Today");
	  dealloc = FALSE;
	}
      else
	buffer = time_to_string (event_list, ev, &tm);
    }
  else
    buffer = time_to_string (event_list, ev, &tm);

  g_object_set (renderer, "text", buffer, NULL);

  if (dealloc)
    g_free (buffer);

  GdkColor color;
  if (event_get_color (ev, &color))
    g_object_set (renderer, "cell-background-gdk", &color, NULL);
  else
    g_object_set (renderer, "cell-background-gdk", NULL, NULL);
}

static gboolean
button_press (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  GtkTreeView *view = GTK_TREE_VIEW (widget);
  GtkTreePath *path;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (event->type != GDK_BUTTON_PRESS)
    /* We only catch single clicks.  */
    return FALSE;

  if (event->button != 1 && event->button != 3)
    return FALSE;

  if (! gtk_tree_view_get_path_at_pos (view, event->x, event->y,
				       &path, NULL, NULL, NULL))
    /* No row at curser.  */
    return FALSE;

  model = gtk_tree_view_get_model (view);

  int ret = gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);
  if (! ret)
    return FALSE;

  Event *ev;
  gtk_tree_model_get (model, &iter, COL_EVENT, &ev, -1);

  if (event->button == 1)
    {
      GtkTreeSelection *tree_sel = gtk_tree_view_get_selection (view);
      GtkTreeIter sel;
      if (gtk_tree_selection_get_selected (tree_sel, NULL, &sel))
	{
	  Event *s;
	  gtk_tree_model_get (model, &sel, COL_EVENT, &s, -1);

	  if (ev == s)
	    set_time_and_day_view (event_get_start (ev));
	}

      /* We allow the event to propagate so that the cell is
	 highlighted.  */
      return FALSE;
    }
  else if (event->button == 3)
    {
      GtkMenu *event_menu = event_menu_new (ev, TRUE);
      gtk_menu_popup (event_menu, NULL, NULL, NULL, NULL,
		      event->button, event->time);
    }

  return FALSE;
}

static gboolean
key_press (GtkWidget *widget, GdkEventKey *k, gpointer d)
{
  GtkTreeView *view = GTK_TREE_VIEW (widget);

  switch (k->keyval)
    {
    case GDK_space:
    case GDK_Return:
      {
	GtkTreeSelection *sel = gtk_tree_view_get_selection (view);
	GtkTreeModel *model;
	GtkTreeIter iter;
	Event *ev;

	if (! gtk_tree_selection_get_selected (sel, &model, &iter))
	  return FALSE;

	gtk_tree_model_get (model, &iter, COL_EVENT, &ev, -1);
	set_time_and_day_view (event_get_start (ev));
	return TRUE;
      }

    default:
      break;
    }

  return FALSE;
}

static void
arrow_click (GtkWidget *w, gpointer data)
{
  GtkEventList *event_list = GTK_EVENT_LIST (data);
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;

  int i = MAX (1, atoi (gtk_entry_get_text (event_list->entry)) + d);
  char buf[10];
  snprintf (buf, sizeof (buf) - 1, "%d", i);
  buf[sizeof (buf) - 1] = 0;

  gtk_entry_set_text (event_list->entry, buf);
}

static int
reload (GtkWidget *w, gpointer data)
{
  gtk_event_list_reload_events (GTK_EVENT_LIST (data));

  return FALSE;
}

static void
gtk_event_list_init (GTypeInstance *instance, gpointer klass)
{
  GtkEventList *event_list = GTK_EVENT_LIST (instance);
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  GtkBox *box = GTK_BOX (gtk_hbox_new (FALSE, 0));
  gtk_box_pack_start (GTK_BOX (instance), GTK_WIDGET (box),
		      FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (box));

  /* To center the main widgets we put a greedy empty label at the
     start and a second at the end.  */
  GtkWidget *label = gtk_label_new ("");
  gtk_widget_show (label);
  gtk_box_pack_start (box, label, TRUE, TRUE, 0);

  event_list->entry = GTK_ENTRY (gtk_entry_new ());
  gtk_entry_set_width_chars (event_list->entry, 3);
  gtk_entry_set_text (event_list->entry, "2");
  g_signal_connect (G_OBJECT (event_list->entry), "changed",
		    (GCallback) reload, event_list);

  GtkWidget *arrow_l, *arrow_r, *arrow_button_l, *arrow_button_r;
#ifdef IS_HILDON
  arrow_l = gtk_image_new_from_stock(GTK_STOCK_GO_BACK,
				     GTK_ICON_SIZE_SMALL_TOOLBAR);
  arrow_r = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD,
				     GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
  arrow_l = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_OUT);
  arrow_r = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
#endif
  arrow_button_l = gtk_button_new ();
  arrow_button_r = gtk_button_new ();

  gtk_widget_set_size_request (arrow_button_l, ARROW_SIZE, ARROW_SIZE);
  gtk_widget_set_size_request (arrow_button_r, ARROW_SIZE, ARROW_SIZE);
  GTK_WIDGET_UNSET_FLAGS (arrow_button_l, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (arrow_button_r, GTK_CAN_FOCUS);

  gtk_container_add (GTK_CONTAINER (arrow_button_l), arrow_l);
  gtk_widget_show (arrow_l);
  gtk_container_add (GTK_CONTAINER (arrow_button_r), arrow_r);
  gtk_widget_show (arrow_r);

  gtk_button_set_relief (GTK_BUTTON (arrow_button_l), GTK_RELIEF_NONE);
  gtk_button_set_relief (GTK_BUTTON (arrow_button_r), GTK_RELIEF_NONE);

  g_object_set_data (G_OBJECT (arrow_button_l), "direction", (gpointer)0);
  g_object_set_data (G_OBJECT (arrow_button_r), "direction", (gpointer)1);

  g_signal_connect (G_OBJECT (arrow_button_l),
		    "clicked", G_CALLBACK (arrow_click), event_list);
  g_signal_connect (G_OBJECT (arrow_button_r),
		    "clicked", G_CALLBACK (arrow_click), event_list);

  gtk_box_pack_start (box, arrow_button_l, FALSE, FALSE, 0);
  gtk_widget_show (arrow_button_l);

  gtk_box_pack_start (box, GTK_WIDGET (event_list->entry), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (event_list->entry));
  gtk_box_pack_start (box, arrow_button_r, FALSE, FALSE, 0);
  gtk_widget_show (arrow_button_r);

  event_list->combo = GTK_COMBO_BOX (gtk_combo_box_new_text ());
  gtk_combo_box_append_text (event_list->combo, _("days"));
  gtk_combo_box_append_text (event_list->combo, _("weeks"));
  gtk_combo_box_append_text (event_list->combo, _("months"));
  gtk_combo_box_append_text (event_list->combo, _("years"));
  gtk_combo_box_set_active (event_list->combo, 1);
  g_signal_connect (G_OBJECT (event_list->combo), "changed",
		    (GCallback) reload, event_list);
  gtk_box_pack_start (box, GTK_WIDGET (event_list->combo), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (event_list->combo));

  /* The second greedy empty label.  */
  label = gtk_label_new ("");
  gtk_widget_show (label);
  gtk_box_pack_start (box, label, TRUE, TRUE, 0);


  GtkScrolledWindow *win
    = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
				       GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (instance), GTK_WIDGET (win), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (win));

  event_list->view = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_tree_view_set_rules_hint (event_list->view, TRUE);
  gtk_container_add (GTK_CONTAINER (win),
		     GTK_WIDGET (event_list->view));
  gtk_widget_show (GTK_WIDGET (event_list->view));

  g_signal_connect (event_list->view, "button-press-event",
		    (GCallback) button_press, NULL);
  g_signal_connect (event_list->view, "key_press_event",
		    (GCallback) key_press, NULL);

  /* We don't need to waste space showing the headers.  */
  gtk_tree_view_set_headers_visible (event_list->view, FALSE);

  /* Create the columns.  */
  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (event_list->view, col);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "scale", 0.8, NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
					   date_cell_data_func, event_list,
					   NULL);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (event_list->view, col);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "scale", 0.8, NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
					   summary_cell_data_func, NULL, NULL);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (event_list->view, col);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "scale", 0.8, NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, renderer,
					   end_cell_data_func, event_list,
					   NULL);

  gtk_event_list_reload_events (event_list);
}

void
gtk_event_list_reload_events (GtkEventList *event_list)
{
  GtkTreeModel *model;
  GtkListStore *list;
  GSList *e;
  GtkTreeIter iter;

  if (event_list->timeout > 0)
    /* Cancel any outstanding timeout.  */
    {
      g_source_remove (event_list->timeout);
      event_list->timeout = 0;
    }

 restart:
  /* First, deallocate the old model to free some memory.  */
  model = gtk_tree_view_get_model (event_list->view);
  if (model)
    {
      list = GTK_LIST_STORE (model);

      /* And release the events.  */
      event_list_unref (event_list->events);

      /* Drop our reference.  */
      g_object_unref (list);
      gtk_tree_view_set_model (event_list->view, NULL);
    }

  /* Create a new list.  */
  list = gtk_list_store_new (1, G_TYPE_POINTER);

  /* Get the events for the next 14 days.  */
  int shift[] = { 1, 7, 31, 365 };
  int days = MAX (1, atoi (gtk_entry_get_text (event_list->entry)))
    * shift[gtk_combo_box_get_active (event_list->combo)];

  event_list->date = time (NULL);
  event_list->events = event_db_list_for_period (event_db,
						 event_list->date,
						 event_list->date
						 + days * 24 * 60 * 60);
  event_list->events = g_slist_sort (event_list->events, event_compare_func);
  time_t next_reload = event_list->date + 2 * 24 * 60 * 60;
  for (e = event_list->events; e; e = e->next)
    {
      Event *ev = e->data;

      if (! event_get_visible (ev))
	continue;

      /* Add a new row to the model.  */
      gtk_list_store_append (list, &iter);
      gtk_list_store_set (list, &iter, COL_EVENT, ev, -1);

      next_reload = MIN (next_reload,
			 event_get_start (ev) + event_get_duration (ev));
      if (event_get_start (ev) >= event_list->date)
	next_reload = MIN (next_reload, event_get_start (ev));
    }

  /* Add the new model.  */
  gtk_tree_view_set_model (event_list->view, GTK_TREE_MODEL (list));

  if (event_list->events)
    /* Schedule a callback for a refresh.  */
    {
      if (event_list->timeout > 0)
	g_source_remove (event_list->timeout);

      if (next_reload < event_list->date)
	{
	  g_warning ("next_reload (%ld) < event_list->date (%ld)",
		     next_reload, event_list->date);
	  next_reload = event_list->date;
	}

      next_reload -= event_list->date;
      if (next_reload == 0)
	next_reload = 1;
      event_list->timeout = g_timeout_add (next_reload * 1000,
					   (GSourceFunc)
					   (gtk_event_list_reload_events),
					   event_list);


      /* If it took more than 10 seconds to do all of this then the
	 machine/process went to sleep between getting the time and
	 setting the timeout.  Try again.  */
      if (time (NULL) - event_list->date > 10)
	goto restart;

      localtime_r (&event_list->date, &event_list->tm);
    }
}

GtkWidget *
gtk_event_list_new (void)
{
  GtkWidget *widget = g_object_new (gtk_event_list_get_type (), NULL);

  return widget;
}
