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
#include <gpe/errorbox.h>

#include "calendars-dialog.h"
#include "calendar-edit-dialog.h"
#include "calendars-widgets.h"
#include "globals.h"

struct _CalendarsDialog
{
  GtkWindow dialog;

  GSList *cals;

  GtkTreeViewColumn *refresh_col;
  GtkTreeViewColumn *delete_col;
  GtkTreeViewColumn *edit_col;
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
calendars_dialog_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;

  calendars_dialog_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = calendars_dialog_finalize;
  object_class->dispose = calendars_dialog_dispose;
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
  G_OBJECT_CLASS (calendars_dialog_parent_class)->finalize (object);
}

static void
new_clicked (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = calendar_edit_dialog_new (NULL);
  gtk_window_set_transient_for (GTK_WINDOW (w),
				GTK_WINDOW (gtk_widget_get_toplevel (widget)));
  gtk_widget_show (w);
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

	  gtk_widget_show (w);
	}
      else if (column == d->refresh_col)
	;
      else if (column == d->delete_col)
	{
	  char *title = event_calendar_get_title (ec);
	  char *s = g_strdup_printf (_("Delete Calendar %s"), title);
	  GtkDialog *d = GTK_DIALOG (gtk_dialog_new_with_buttons
	    (s, GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
	     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	     GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
	     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
	     NULL));
	  g_free (s);

	  GtkWidget *frame = gtk_frame_new (NULL);
	  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	  gtk_box_pack_start (GTK_BOX (d->vbox), frame, FALSE, FALSE, 0);
	  gtk_widget_show (frame);

	  GtkBox *vbox = GTK_BOX (gtk_vbox_new (FALSE, 4));
	  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (vbox));
	  gtk_widget_show (GTK_WIDGET (vbox));

	  s = g_strdup_printf (_("What do you want to do with the events\n"
				 "and calendars which %s contains?"),
			       title);
	  GtkWidget *l = gtk_label_new (s);
	  g_free (s);
	  gtk_box_pack_start (GTK_BOX (vbox), l, FALSE, FALSE, 0);
	  gtk_widget_show (l);

	  GtkWidget *delete_containing = gtk_radio_button_new_with_label
	    (NULL, _("Delete containing events and calendars"));
	  gtk_box_pack_start (GTK_BOX (vbox), delete_containing,
			      FALSE, FALSE, 0);
	  gtk_widget_show (delete_containing);

	  GtkWidget *reparent = gtk_radio_button_new_with_label_from_widget
	    (GTK_RADIO_BUTTON (delete_containing),
	     _("Reparent containing events and calendars"));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (reparent), TRUE);
	  gtk_box_pack_start (GTK_BOX (vbox), reparent, FALSE, FALSE, 0);
	  gtk_widget_show (reparent);

	  GtkWidget *w = calendars_combo_box_new ();
	  gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	  gtk_widget_show (w);

	  EventCalendar *p = event_calendar_get_parent (ec);
	  if (! p)
	    p = event_db_get_default_calendar (event_db, NULL);
	  else
	    g_object_ref (p);
	  calendars_combo_box_set_active (GTK_COMBO_BOX (w), p);
	  g_object_unref (p);

	  g_free (title);

	  gint res = gtk_dialog_run (d);
	  if (res == GTK_RESPONSE_ACCEPT)
	    {
	      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (reparent)))
		{
		  EventCalendar *p = 
		    calendars_combo_box_get_active (GTK_COMBO_BOX (w));

		  if (event_calendar_valid_parent (ec, p))
		    event_calendar_delete (ec, FALSE, p);
		  else
		    {
		      char *ec_title = event_calendar_get_title (ec);
		      char *p_title = event_calendar_get_title (p);

		      gpe_error_box_fmt
			(_("I can't make %s the new parent of %s's "
			   "constituents as it would create a loop.\n"
			   "Please select a calendar "
			   "which is not a decedent of %s."),
			 p_title, ec_title, ec_title);

		      g_free (ec_title);
		      g_free (p_title);
		    }
		}
	      else
		event_calendar_delete (ec, TRUE, NULL);
	    }
	  gtk_widget_destroy (GTK_WIDGET (d));
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


  /* A frame.  */
  GtkWidget *frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (box, GTK_WIDGET (frame), TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* Body box.  */
  GtkBox *body = GTK_BOX (gtk_vbox_new (FALSE, 2));
  gtk_container_set_border_width (GTK_CONTAINER (body), 2);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (body));
  gtk_widget_show (GTK_WIDGET (body));

  /* The view.  */
  GtkTreeModel *model = calendars_tree_model ();
  GtkTreeView *tree_view
  = GTK_TREE_VIEW (gtk_tree_view_new_with_model (model));
  gtk_tree_view_expand_all (tree_view);
  gtk_tree_view_set_rules_hint (tree_view, TRUE);
  gtk_tree_view_set_headers_visible (tree_view, FALSE);
  g_signal_connect (tree_view, "button-press-event",
		    G_CALLBACK (button_press), d);
  gtk_box_pack_start (body, GTK_WIDGET (tree_view), TRUE, TRUE, 0);
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
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
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

  d->refresh_col = col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_refresh_cell_data_func,
				      NULL, NULL);

  d->edit_col = col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (col),
				      renderer,
				      calendar_edit_cell_data_func,
				      NULL, NULL);

  d->delete_col = col = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (tree_view, col);
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
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
}

GtkWidget *
calendars_dialog_new (EventDB *edb)
{
  CalendarsDialog *c
    = CALENDARS_DIALOG (g_object_new (calendars_dialog_get_type (), NULL));

  return GTK_WIDGET (c);
}
