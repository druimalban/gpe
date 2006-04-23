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
#include <libintl.h>
#include "globals.h"
#include "event-list.h"

#define _(x) gettext(x)

enum
{
  COL_EVENT,
  NUM_COLS
};

struct _GtkEventList
{
  GtkScrolledWindow widget;

  GtkTreeView *view;
  GSList *events;

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

      type = g_type_register_static (gtk_scrolled_window_get_type (),
				     "GtkEventList", &info, 0);
    }

  return type;
}

static void
gtk_event_list_base_class_init (gpointer klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_ref (gtk_vbox_get_type ());

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
	buffer = strftime_strdup_utf8_locale (_("%b %d"), &tm);
    }
  else
    {
      if (event_get_start (ev) < event_list->date)
	/* Starts in the past.  */
	{
	  time_t end = event_get_start (ev) + event_get_duration (ev);

	  localtime_r (&end, &tm);
	  if (tm.tm_year == event_list->tm.tm_year
	      && tm.tm_yday == event_list->tm.tm_yday)
	    buffer = strftime_strdup_utf8_locale (_("Until %R"), &tm);
	  else
	    buffer = strftime_strdup_utf8_locale (_("Until %b %d"), &tm);
	}
      else
	{
	  if (tm.tm_year == event_list->tm.tm_year
	      && tm.tm_yday == event_list->tm.tm_yday)
	    buffer = strftime_strdup_utf8_locale (_("%R"), &tm);
	  else
	    buffer = strftime_strdup_utf8_locale (_("%b %d %R"), &tm);
	}
    }

  g_object_set(renderer, "text", buffer, NULL);

  if (dealloc)
    g_free (buffer);
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
  g_object_set (renderer, "text", event_get_summary (ev), NULL);
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

  if (! gtk_tree_view_get_path_at_pos (view, event->x, event->y,
				       &path, NULL, NULL, NULL))
    /* No row at curser.  */
    return FALSE;

  model = gtk_tree_view_get_model (view);

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      Event *ev;

      gtk_tree_model_get (model, &iter, COL_EVENT, &ev, -1);

      set_time_and_day_view (event_get_start (ev));

      /* We allow the event to propagate so that the cell is
	 highlighted.  */
      return FALSE;
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
gtk_event_list_init (GTypeInstance *instance, gpointer klass)
{
  GtkEventList *event_list = GTK_EVENT_LIST (instance);
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (event_list), NULL);
  gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (event_list), NULL);

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (event_list),
				       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (event_list),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  event_list->view = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_container_add (GTK_CONTAINER (event_list),
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
  event_list->date = time (NULL);
  event_list->events = event_db_list_for_period (event_db,
						 event_list->date,
						 event_list->date
						 + 14 * 24 * 60 * 60);
  time_t next_reload = event_list->date + 2 * 24 * 60 * 60;
  for (e = event_list->events; e; e = e->next)
    {
      Event *ev = e->data;

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
