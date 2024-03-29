/* event-list.c - Event list widget implementation.
   Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>

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

struct _EventList
{
  GtkVBox widget;

  GtkWidget *scrolled_window;
  gboolean narrow;
  int char_width;
  GtkTreeView *view;
  GSList *events;
  /* Whether to ignore the visibility of calendars and to just show
     all the events.  */
  gboolean show_all;

  GtkBox *period_box;
  GtkEntry *entry;
  GtkComboBox *combo;

  guint timeout;

  /* Time of last update.  */
  time_t date;
  struct tm tm;

  EventDB *edb;

  gboolean pending_reload;
};

typedef struct
{
  GtkTreeViewClass tree_box_class;

  guint event_clicked_signal;
  EventListEventClicked event_clicked;
  guint event_key_pressed_signal;
  EventListEventKeyPressed event_key_pressed;
} EventListClass;

static void event_list_base_class_init (gpointer klass,
					    gpointer klass_data);
static void event_list_dispose (GObject *obj);
static void event_list_finalize (GObject *object);
static void event_list_init (GTypeInstance *instance, gpointer klass);
static gboolean event_list_expose_event (GtkWidget *widget,
					 GdkEventExpose *event);
static void event_list_size_request (GtkWidget *widget,
				     GtkRequisition *requisition);
static void event_list_size_allocate (GtkWidget *widget,
				      GtkAllocation *allocation);

static GtkWidgetClass *parent_class;

GType
event_list_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (EventListClass),
	NULL,
	NULL,
	event_list_base_class_init,
	NULL,
	NULL,
	sizeof (struct _EventList),
	0,
	event_list_init
      };

      type = g_type_register_static (gtk_vbox_get_type (),
				     "EventList", &info, 0);
    }

  return type;
}

static void
event_list_base_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  EventListClass *event_list_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = event_list_finalize;
  object_class->dispose = event_list_dispose;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->expose_event = event_list_expose_event;
  widget_class->size_request = event_list_size_request;
  widget_class->size_allocate = event_list_size_allocate;

  event_list_class = EVENT_LIST_CLASS (klass);
  event_list_class->event_clicked_signal
    = g_signal_new ("event-clicked",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventListClass, event_clicked),
		    NULL,
		    NULL,
		    gtk_marshal_NONE__POINTER_POINTER,
		    G_TYPE_NONE,
		    2,
		    G_TYPE_POINTER,
		    G_TYPE_POINTER);
  event_list_class->event_key_pressed_signal
    = g_signal_new ("event-key-pressed",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (EventListClass, event_clicked),
		    NULL,
		    NULL,
		    gtk_marshal_NONE__POINTER_POINTER,
		    G_TYPE_NONE,
		    2,
		    G_TYPE_POINTER,
		    G_TYPE_POINTER);
}

static void
event_list_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
event_list_finalize (GObject *object)
{
  EventList *event_list = EVENT_LIST (object);

  if (event_list->timeout > 0)
    /* Cancel any outstanding timeout.  */
    g_source_remove (event_list->timeout);

  if (event_list->edb)
    g_object_unref (event_list->edb);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static char *
time_to_string (EventList *event_list, Event *ev, struct tm *tm)
{
  if (is_reminder (ev))
    {
      if (tm->tm_year == event_list->tm.tm_year)
	return strftime_strdup_utf8_locale (event_list->narrow
					    ? _("%-m/%-d")
					    : _("%b %-d"), tm);
      else
	return strftime_strdup_utf8_locale (event_list->narrow
					    ? _("%m/%d/%t")
					    : _("%b %d, %y"), tm);
    }
  else
    {
      if (tm->tm_year == event_list->tm.tm_year
	  && tm->tm_yday == event_list->tm.tm_yday)
	return strftime_strdup_utf8_locale (_("%-H:%M"), tm);
      else if (tm->tm_year == event_list->tm.tm_year)
	return strftime_strdup_utf8_locale (event_list->narrow
					    ? _("%-m/%-d %-H:%M")
					    : _("%b %-d %-H:%M"), tm);
      else
	return strftime_strdup_utf8_locale (event_list->narrow
					    ? _("%-m/%-d/%y %-H:%M")
					    : _("%b %-d, %y %-H:%M"), tm);
    }
}

static void
date_cell_data_func (GtkTreeViewColumn *col,
		     GtkCellRenderer *renderer,
		     GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer el)
{
  EventList *event_list = EVENT_LIST (el);
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
	  if (event_get_untimed (ev))
	    end -= 24 * 60 * 60;
	  else
	    end --;

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
  if (event_get_color (ev, &color, NULL))
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
  char *s = event_get_summary (ev, NULL);
  g_object_set (renderer, "text", s ?: "", NULL);
  g_free (s);

  GdkColor color;
  if (event_get_color (ev, &color, NULL))
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
  EventList *event_list = EVENT_LIST (el);
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
  if (event_get_color (ev, &color, NULL))
    g_object_set (renderer, "cell-background-gdk", &color, NULL);
  else
    g_object_set (renderer, "cell-background-gdk", NULL, NULL);
}

static gboolean
button_release (GtkWidget *widget, GdkEventButton *event,
		EventList *event_list)
{
  GtkTreeView *view = GTK_TREE_VIEW (widget);
  GtkTreePath *path;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (event->type != GDK_BUTTON_RELEASE)
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

	  if (ev != s)
	    /* We only send the "event-clicked" event if the event is
	       selected.  */
	    return FALSE;
	}
    }

  EventListClass *event_list_class = EVENT_LIST_GET_CLASS (event_list);
  g_signal_emit (event_list,
		 event_list_class->event_clicked_signal,
		 0, ev, event);

  return FALSE;
}

static gboolean
key_press (GtkWidget *widget, GdkEventKey *k, EventList *event_list)
{
  GtkTreeView *view = GTK_TREE_VIEW (widget);

  GtkTreeSelection *sel = gtk_tree_view_get_selection (view);
  GtkTreeModel *model;
  GtkTreeIter iter;
  Event *ev;

  if (! gtk_tree_selection_get_selected (sel, &model, &iter))
    return FALSE;

  gtk_tree_model_get (model, &iter, COL_EVENT, &ev, -1);

  EventListClass *event_list_class = EVENT_LIST_GET_CLASS (event_list);
  g_signal_emit (event_list, event_list_class->event_key_pressed_signal,
		 0, ev, k);

  return FALSE;
}

static void
arrow_click (GtkWidget *w, gpointer data)
{
  EventList *event_list = EVENT_LIST (data);
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
  event_list_reload_events (EVENT_LIST (data));

  return FALSE;
}

static void
event_list_init (GTypeInstance *instance, gpointer klass)
{
  EventList *event_list = EVENT_LIST (instance);
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  GtkScrolledWindow *win
    = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
  event_list->scrolled_window = GTK_WIDGET (win);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
				       GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_end (GTK_BOX (instance), GTK_WIDGET (win), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (win));

  event_list->view = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_tree_view_set_rules_hint (event_list->view, TRUE);
  gtk_container_add (GTK_CONTAINER (win),
		     GTK_WIDGET (event_list->view));
  gtk_widget_show (GTK_WIDGET (event_list->view));

  g_signal_connect (event_list->view, "button-release-event",
		    (GCallback) button_release, event_list);
  g_signal_connect (event_list->view, "key_press_event",
		    (GCallback) key_press, event_list);

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
}

static void
event_list_reload_events_hard (EventList *event_list)
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

  /* Get the events for the indicated time period.  */
  int shift[] = { 1, 7, 31, 365 };
  int days = 14;
  if (event_list->entry)
    days = MAX (1, atoi (gtk_entry_get_text (event_list->entry)))
      * shift[gtk_combo_box_get_active (event_list->combo)];

  event_list->date = time (NULL);
  event_list->events = event_db_list_for_period (event_list->edb,
						 event_list->date,
						 event_list->date
						 + days * 24 * 60 * 60,
						 NULL);
  event_list->events = g_slist_sort (event_list->events, event_compare_func);
  time_t next_reload = event_list->date + 2 * 24 * 60 * 60;
  for (e = event_list->events; e; e = e->next)
    {
      Event *ev = e->data;

      if (! event_list->show_all && ! event_get_visible (ev, NULL))
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
					   (event_list_reload_events),
					   event_list);


      /* If it took more than 10 seconds to do all of this then the
	 machine/process went to sleep between getting the time and
	 setting the timeout.  Try again.  */
      if (time (NULL) - event_list->date > 10)
	goto restart;

      localtime_r (&event_list->date, &event_list->tm);
    }

  event_list->pending_reload = FALSE;
}

static gboolean
event_list_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  EventList *event_list = EVENT_LIST (widget);

  if (event_list->pending_reload)
    event_list_reload_events_hard (event_list);

  return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}

static void
event_list_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  EventList *event_list = EVENT_LIST (widget);

  PangoLayout *pl = gtk_widget_create_pango_layout (widget, NULL);
  PangoContext *context = pango_layout_get_context (pl);
  PangoFontMetrics *metrics
    = pango_context_get_metrics (context,
				 widget->style->font_desc,
				 pango_context_get_language (context));

  int row_height = (pango_font_metrics_get_ascent (metrics)
		    + pango_font_metrics_get_descent (metrics))
    / PANGO_SCALE;

  
  int cw = pango_font_metrics_get_approximate_char_width (metrics);
  event_list->char_width = cw;

  /* We'd like about 6 rows of text and 24 columns but we don't want
     to request more than 25% of the screen.  */  
  int cols = MIN (gdk_screen_width () / (cw / PANGO_SCALE) / 4, 24);
  gtk_widget_set_size_request (event_list->scrolled_window,
			       cols * cw / PANGO_SCALE,
			       row_height * 6);

  g_object_unref (pl);
  pango_font_metrics_unref (metrics);

  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
}

static void
event_list_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  EventList *event_list = EVENT_LIST (widget);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  event_list->narrow
    = allocation->width / (event_list->char_width / PANGO_SCALE) < 24;
}

void
event_list_reload_events (EventList *event_list)
{
  event_list->pending_reload = TRUE;
  gtk_widget_queue_draw (GTK_WIDGET (event_list->scrolled_window));
}

void
event_list_set_show_all (EventList *event_list, gboolean show_all)
{
  if (event_list->show_all == show_all)
    return;
  event_list->show_all = show_all;
  event_list_reload_events (event_list);
}

void
event_list_set_period_box_visible (EventList *event_list, gboolean visible)
{
  if (! visible)
    {
      if (event_list->period_box)
	gtk_widget_hide (GTK_WIDGET (event_list->period_box));
      return;
    }

  if (event_list->period_box)
    {
      gtk_widget_show (GTK_WIDGET (event_list->period_box));
      return;
    }

  /* We need to create it.  */
  GtkBox *box = GTK_BOX (gtk_hbox_new (FALSE, 0));
  event_list->period_box = box;
  gtk_box_pack_start (GTK_BOX (event_list), GTK_WIDGET (box),
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
}

GtkWidget *
event_list_new (EventDB *edb)
{
  GtkWidget *widget = g_object_new (event_list_get_type (), NULL);
  EventList *el = EVENT_LIST (widget);

  g_object_ref (edb);
  el->edb = edb;

  event_list_reload_events (el);

  return widget;
}
