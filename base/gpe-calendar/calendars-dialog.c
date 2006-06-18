/* calendars-dialog.h - Calendars dialog implementation.
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

#include <gpe/event-db.h>
#include <gpe/colordialog.h>
#include <gpe/pixmaps.h>

#include "calendars-dialog.h"
#include "calendar-edit-dialog.h"
#include "calendar-delete-dialog.h"
#include "calendars-widgets.h"
#include "calendar-update.h"
#include "globals.h"

struct _CalendarsDialog
{
  GtkWindow dialog;

  GSList *cals;

  GtkScrolledWindow *scrolled_window;
  GtkTreeView *tree_view;

  GtkTreeViewColumn *refresh_col;
  GtkTreeViewColumn *delete_col;
  GtkTreeViewColumn *edit_col;

  guint edb_modified_signal;
};

struct _CalendarsDialogClass
{
  GtkWindowClass gtk_window_class;
};

static void calendars_dialog_class_init (gpointer klass, gpointer klass_data);
static void calendars_dialog_dispose (GObject *obj);
static void calendars_dialog_finalize (GObject *object);
static void calendars_dialog_init (GTypeInstance *instance, gpointer klass);

static GtkWidgetClass *calendars_dialog_parent_class;

GType
calendars_dialog_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (CalendarsDialogClass),
	NULL,
	NULL,
	calendars_dialog_class_init,
	NULL,
	NULL,
	sizeof (struct _CalendarsDialog),
	0,
	calendars_dialog_init
      };

      type = g_type_register_static (gtk_window_get_type (),
				     "CalendarsDialog", &info, 0);
    }

  return type;
}

static void
size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  CalendarsDialog *d = CALENDARS_DIALOG (widget);

  GTK_WIDGET_CLASS (calendars_dialog_parent_class)
    ->size_request (widget, requisition);

  GtkRequisition s;
  gtk_widget_size_request (GTK_WIDGET (d->scrolled_window), &s);
  GtkRequisition t;
  gtk_widget_size_request (GTK_WIDGET (d->tree_view), &t);

  requisition->height = MIN (gdk_screen_height () * 7 / 8,
			     requisition->height - s.height + t.height);
  requisition->width = MIN (gdk_screen_width () * 7 / 8,
			    requisition->width - s.width + t.width);
}

static void
calendars_dialog_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;

  calendars_dialog_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = calendars_dialog_finalize;
  object_class->dispose = calendars_dialog_dispose;

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->size_request = size_request;
}

static void
calendars_dialog_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (calendars_dialog_parent_class)->dispose (obj);
}

static void
calendars_dialog_finalize (GObject *object)
{
  CalendarsDialog *c = CALENDARS_DIALOG (object);

  g_signal_handler_disconnect (event_db, c->edb_modified_signal);

  G_OBJECT_CLASS (calendars_dialog_parent_class)->finalize (object);
}

static void
new_clicked (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = calendar_edit_dialog_new (NULL);
  gtk_window_set_transient_for (GTK_WINDOW (w),
				GTK_WINDOW (gtk_widget_get_toplevel (widget)));
  g_signal_connect (G_OBJECT (w), "response",
		    G_CALLBACK (gtk_widget_destroy), NULL);
  gtk_widget_show (w);
}

static void
refresh_all_clicked (GtkWidget *widget, gpointer d)
{
  GSList *l = event_db_list_event_calendars (event_db);
  GSList *i;
  GSList *next = l;
  while (next)
    {
      i = next;
      next = next->next;

      EventCalendar *ec = i->data;
      if (event_calendar_get_mode (ec) == 1)
	calendar_pull (ec);
      g_object_unref (ec);
    }
}

static void
show_all_clicked (GtkWidget *widget, gpointer d)
{
  GSList *l = event_db_list_event_calendars (event_db);
  GSList *i;
  GSList *next = l;
  while (next)
    {
      i = next;
      next = next->next;

      EventCalendar *ec = i->data;
      event_calendar_set_visible (ec, TRUE);
      g_object_unref (ec);
    }
}

static void
visible_toggled (GtkCellRendererToggle *cell_renderer,
		 gchar *p, gpointer user_data)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (p);
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      EventCalendar *ec;

      gtk_tree_model_get (model, &iter, COL_CALENDAR, &ec, -1);
      event_calendar_set_visible (ec, ! event_calendar_get_visible (ec));
    }
}

static gboolean
button_press (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  GtkTreeView *view = GTK_TREE_VIEW (widget);
  CalendarsDialog *d = CALENDARS_DIALOG (user_data);

  if (event->type != GDK_BUTTON_PRESS)
    /* We only catch single clicks.  */
    return FALSE;

  if (event->button == 1)
    {
      GtkTreePath *path;
      GtkTreeViewColumn *column;
      if (! gtk_tree_view_get_path_at_pos (view, event->x, event->y,
					   &path, &column, NULL, NULL))
	/* No row at curser.  */
	return FALSE;

      GtkTreeModel *model;
      model = gtk_tree_view_get_model (view);

      GtkTreeIter iter;
      if (! gtk_tree_model_get_iter (model, &iter, path))
	{
	  gtk_tree_path_free (path);
	  return FALSE;
	}
      gtk_tree_path_free (path);

      EventCalendar *ec;
      gtk_tree_model_get (model, &iter, COL_CALENDAR, &ec, -1);

      if (column == d->edit_col)
	{
	  GtkWidget *w = calendar_edit_dialog_new (ec);
	  gtk_window_set_transient_for
	    (GTK_WINDOW (w),
	     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))));
	  g_signal_connect (G_OBJECT (w), "response",
			    G_CALLBACK (gtk_widget_destroy), NULL);
	  gtk_widget_show (w);
	}
      else if (column == d->refresh_col)
	{
	  switch (event_calendar_get_mode (ec))
	    {
	    case 1:
	      calendar_pull (ec);
	      break;
	    case 2:
	      calendar_push (ec);
	      break;
	    default:
	      ;
	    }
	}
      else if (column == d->delete_col)
	{
	  GtkWidget *w = calendar_delete_dialog_new (ec);
	  gtk_window_set_transient_for
	    (GTK_WINDOW (w),
	     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))));
	  gtk_widget_show (w);
	}
    }

  return FALSE;
}

static void
row_inserted (GtkTreeModel *treemodel, GtkTreePath *path, GtkTreeIter *iter,
	      gpointer user_data)
{
  GtkTreeView *tree_view = GTK_TREE_VIEW (user_data);

  gtk_tree_view_expand_to_path (tree_view, path);
}

struct sig_info
{
  gpointer instance;
  gulong sig;
};

static void
disconnect_sig (gpointer data, GObject *tree_view)
{
  struct sig_info *info = data;
  g_signal_handler_disconnect (info->instance, info->sig);
  g_free (info);
}

static void
calendars_dialog_init (GTypeInstance *instance, gpointer klass)
{
  CalendarsDialog *d = CALENDARS_DIALOG (instance);
  GtkBox *hbox;

  gtk_window_set_title (GTK_WINDOW (instance), _("Calendars"));
  gtk_window_set_position (GTK_WINDOW (instance),
			   GTK_WIN_POS_CENTER_ON_PARENT);

  /* Main vbox.  */
  GtkBox *box = GTK_BOX (gtk_vbox_new (FALSE, 0));
  gtk_container_set_border_width (GTK_CONTAINER (box), 2);
  gtk_container_add (GTK_CONTAINER (instance), GTK_WIDGET (box));
  gtk_widget_show (GTK_WIDGET (box));

  /* Tool bar.  */
  GtkWidget *toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  GTK_WIDGET_UNSET_FLAGS (toolbar, GTK_CAN_FOCUS);
  gtk_box_pack_start (box, toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  /* New calendar.  */
  GtkToolItem *item = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (new_clicked), NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_widget_show (GTK_WIDGET (item));

  /* Refresh all calendars.  */
  item = gtk_tool_button_new_from_stock (GTK_STOCK_REFRESH);
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (refresh_all_clicked), NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_widget_show (GTK_WIDGET (item));

  /* Show all calendars.  */
  item = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_FIT);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), _("Show All"));
  g_signal_connect (G_OBJECT (item), "clicked",
		    G_CALLBACK (show_all_clicked), NULL);
  GTK_WIDGET_UNSET_FLAGS (item, GTK_CAN_FOCUS);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_widget_show (GTK_WIDGET (item));


  /* A frame.  */
  GtkWidget *frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (box, GTK_WIDGET (frame), TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* The view.  */
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  d->scrolled_window = GTK_SCROLLED_WINDOW (scrolled_window);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (scrolled_window));
  gtk_widget_show (GTK_WIDGET (scrolled_window));

  GtkTreeModel *model = calendars_tree_model (event_db);
  GtkTreeView *tree_view
    = GTK_TREE_VIEW (gtk_tree_view_new_with_model (model));
  /* XXX: Ugly maemo hack.  See:
     https://maemo.org/bugzilla/show_bug.cgi?id=538 . */
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (tree_view),
				    "allow-checkbox-mode"))
    g_object_set (tree_view, "allow-checkbox-mode", FALSE, NULL);
  d->tree_view = tree_view;
  gtk_tree_view_expand_all (tree_view);
  gtk_tree_view_set_rules_hint (tree_view, TRUE);
  gtk_tree_view_set_headers_visible (tree_view, FALSE);
  g_signal_connect (tree_view, "button-press-event",
		    G_CALLBACK (button_press), d);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
		     GTK_WIDGET (tree_view));
  gtk_widget_show (GTK_WIDGET (tree_view));

  struct sig_info *info = g_malloc (sizeof (*info));
  info->instance = model;
  info->sig = g_signal_connect (G_OBJECT (model), "row-inserted",
				G_CALLBACK (row_inserted), tree_view);
  g_object_weak_ref (G_OBJECT (tree_view), disconnect_sig, info);

  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (visible_toggled), tree_view);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_visible_toggle_cell_data_func,
				      tree_view,
				      NULL);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = calendar_text_cell_renderer_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_text_cell_data_func, tree_view,
				      NULL);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_description_cell_data_func,
				      NULL, NULL);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_last_update_cell_data_func,
				      NULL, NULL);

  d->refresh_col = col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_refresh_cell_data_func,
				      NULL, NULL);

  d->edit_col = col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_edit_cell_data_func,
				      NULL, NULL);

  d->delete_col = col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_delete_cell_data_func,
				      NULL, NULL);

  /* A separator.  */
  GtkWidget *sep = gtk_hseparator_new ();
  gtk_box_pack_start (box, sep, FALSE, TRUE, 0);
  gtk_widget_show (sep);


  /* A button box at the bottom.  */
  hbox = GTK_BOX (gtk_hbox_new (FALSE, 0));
  gtk_box_pack_start (box, GTK_WIDGET (hbox), FALSE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (hbox));

  GtkBox *buttons = GTK_BOX (gtk_hbox_new (TRUE, 4));
  gtk_container_set_border_width (GTK_CONTAINER (buttons), 2);
  gtk_box_pack_end (hbox, GTK_WIDGET (buttons), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (buttons));

  GtkWidget *button = gtk_button_new_with_label (_("Dismiss"));
  g_signal_connect_swapped (G_OBJECT (button), "clicked",
			    G_CALLBACK (gtk_widget_destroy), d);
  gtk_box_pack_start (buttons, button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  d->edb_modified_signal = g_signal_connect_swapped
    (event_db, "calendar-modified",
     G_CALLBACK (gtk_widget_queue_draw), tree_view);
}

GtkWidget *
calendars_dialog_new (EventDB *edb)
{
  CalendarsDialog *c
    = CALENDARS_DIALOG (g_object_new (calendars_dialog_get_type (), NULL));

  return GTK_WIDGET (c);
}
