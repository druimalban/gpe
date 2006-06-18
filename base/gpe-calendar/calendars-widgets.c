/* calendars-widgets.c - Calendars widgets implementation.
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

#include <gtk/gtk.h>
#include <gpe/event-db.h>

#include "calendars-widgets.h"
#include "globals.h"

struct _CalendarTextCellRenderer
{
  GtkCellRenderer cell_renderer;
  char *text;
  gboolean bold;
};

struct _CalendarTextCellRendererClass
{
  GtkCellRendererClass cell_renderer;
};

static GObjectClass *text_parent_class;

static void calendar_text_cell_renderer_class_init (gpointer klass,
						    gpointer data);

static GType
calendar_text_cell_renderer_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (CalendarTextCellRendererClass),
        NULL,
        NULL,
        calendar_text_cell_renderer_class_init,
        NULL,
        NULL,
        sizeof (CalendarTextCellRenderer),
        0,
        NULL
      };

      type = g_type_register_static (gtk_cell_renderer_get_type (),
				     "CalendarTextCellRenderer",
				     &info, 0);
    }

  return type;
}

static void calendar_text_cell_renderer_finalize (GObject *object);
static void calendar_text_cell_renderer_render (GtkCellRenderer *cell,
						GdkWindow *window,
						GtkWidget *widget,
						GdkRectangle *background_area,
						GdkRectangle *cell_area,
						GdkRectangle *expose_area,
						GtkCellRendererState flags);
static void calendar_text_cell_renderer_get_size (GtkCellRenderer *cell,
						  GtkWidget *widget,
						  GdkRectangle *cell_area,
						  gint *x_offset,
						  gint *y_offset,
						  gint *width,
						  gint *height);

static void
calendar_text_cell_renderer_class_init (gpointer klass, gpointer data)
{
  text_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = calendar_text_cell_renderer_finalize;
  
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);
  cell_class->get_size = calendar_text_cell_renderer_get_size;
  cell_class->render = calendar_text_cell_renderer_render;
}

static void
calendar_text_cell_renderer_finalize (GObject *object)
{
  g_free (CALENDAR_TEXT_CELL_RENDERER (object)->text);

  (* G_OBJECT_CLASS (text_parent_class)->finalize) (object);
}

static PangoLayout*
get_layout (CalendarTextCellRenderer *cell,
            GtkWidget *widget,
	    int force_bold)
{
  PangoLayout *layout;

  if (force_bold || cell->bold)
    {
      char *buf = g_strdup_printf ("<b>%s</b>", cell->text);
      layout = gtk_widget_create_pango_layout (widget, NULL);
      pango_layout_set_markup (layout, buf, -1);
      g_free (buf);
    }
  else
    layout = gtk_widget_create_pango_layout (widget, cell->text);

  return layout;
}

static void
calendar_text_cell_renderer_get_size (GtkCellRenderer *cell,
				      GtkWidget *widget,
				      GdkRectangle *cell_area,
				      gint *x_offset,
				      gint *y_offset,
				      gint *width,
				      gint *height)
{
  CalendarTextCellRenderer *calendar_text_cell_renderer
    = CALENDAR_TEXT_CELL_RENDERER (cell);

  PangoLayout *pl = get_layout (calendar_text_cell_renderer, widget, 1);
  PangoRectangle rect;
  pango_layout_get_pixel_extents (pl, NULL, &rect);
  g_object_unref (pl);

  if (height)
    *height = 2 + rect.height;
  if (width)
    *width = 2 + rect.width;
}

static void
calendar_text_cell_renderer_render (GtkCellRenderer *cell,
				    GdkWindow *window,
				    GtkWidget *widget,
				    GdkRectangle *background_area,
				    GdkRectangle *cell_area,
				    GdkRectangle *expose_area,
				    GtkCellRendererState flags)
{
  CalendarTextCellRenderer *renderer = CALENDAR_TEXT_CELL_RENDERER (cell);
  PangoLayout *pl = get_layout (renderer, widget, 0);

  gtk_paint_layout (widget->style, window, GTK_WIDGET_STATE (widget),
                    TRUE, expose_area, widget,
                    "cellrenderertext",
                    cell_area->x + 1,
                    cell_area->y + 1,
                    pl);

  g_object_unref (pl);
}

GtkCellRenderer *
calendar_text_cell_renderer_new (void)
{
  return g_object_new (TYPE_CALENDAR_TEXT_CELL_RENDERER, NULL);
}

GtkWidget *
calendars_combo_box_new (EventDB *edb)
{
  GtkWidget *combo
    = gtk_combo_box_new_with_model (calendars_tree_model (edb));

  /* Set the renderer.  */
  GtkCellRenderer *renderer = calendar_text_cell_renderer_new ();

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo),
				      renderer,
				      calendar_text_cell_data_func, combo,
				      NULL);

  EventCalendar *ec = event_db_get_default_calendar (edb, NULL);
  calendars_combo_box_set_active (GTK_COMBO_BOX (combo), ec);
  g_object_unref (ec);

  return combo;
}

EventCalendar *
calendars_combo_box_get_active (GtkComboBox *combo)
{
  GtkTreeIter iter;
  
  if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      EventCalendar *ec;
      gtk_tree_model_get (gtk_combo_box_get_model (combo),
			  &iter, COL_CALENDAR, &ec, -1);
      return ec;
    }

  return NULL;
}

static gboolean
search (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent,
	EventCalendar *ec)
{
  int valid = gtk_tree_model_iter_children (model, iter, parent);
  while (valid)
    {
      EventCalendar *i;
      gtk_tree_model_get (model, iter, COL_CALENDAR, &i, -1);
      if (i == ec)
	return TRUE;

      GtkTreeIter child;
      if (gtk_tree_model_iter_children (model, &child, iter))
	if (search (model, &child, iter, ec))
	  {
	    *iter = child;
	    return TRUE;
	  }

      valid = gtk_tree_model_iter_next (model, iter);
    }

  return FALSE;
}


void
calendars_combo_box_set_active (GtkComboBox *combo, EventCalendar *ec)
{
  if (! ec)
    gtk_combo_box_set_active (combo, -1);

  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  GtkTreeIter iter;

  if (search (model, &iter, NULL, ec))
    gtk_combo_box_set_active_iter (combo, &iter);
  else
    gtk_combo_box_set_active (combo, -1);
}

void
calendar_visible_toggle_cell_data_func (GtkCellLayout *cell_layout,
					GtkCellRenderer *cell_renderer,
					GtkTreeModel *model,
					GtkTreeIter *iter,
					gpointer data)
{
  EventCalendar *ec;
  gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);
  if (! ec)
    return;

  g_object_set (cell_renderer,
		"active", event_calendar_get_visible (ec), NULL);

  if (data && GTK_IS_TREE_VIEW (data))
    /* XXX: This looks really back in a combo box.  */
    {
      GdkColor color;
      if (event_calendar_get_color (ec, &color))
	g_object_set (cell_renderer,
		      "cell-background-gdk", &color, NULL);
      else
	g_object_set (cell_renderer,
		      "cell-background-gdk", NULL, NULL);
    }
}

void
calendar_text_cell_data_func (GtkCellLayout *cell_layout,
			      GtkCellRenderer *cell_renderer,
			      GtkTreeModel *model,
			      GtkTreeIter *iter,
			      gpointer data)
{
  CalendarTextCellRenderer *renderer
    = CALENDAR_TEXT_CELL_RENDERER (cell_renderer);

  EventCalendar *ec;
  gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);
  if (! ec)
    return;

  g_free (renderer->text);
  renderer->text = event_calendar_get_title (ec);

  if (data && GTK_IS_COMBO_BOX (data))
    {
      GtkComboBox *combo = GTK_COMBO_BOX (data);
      GtkTreeIter active_iter;
      if (gtk_combo_box_get_active_iter (combo, &active_iter))
	{
	  EventCalendar *active_ec;
	  gtk_tree_model_get (model, &active_iter,
			      COL_CALENDAR, &active_ec, -1);
	  renderer->bold = active_ec == ec;
	}
    }

  if (data && GTK_IS_TREE_VIEW (data))
    /* XXX: This looks really bad in a combo box.  */
    {
      GdkColor color;
      if (event_calendar_get_color (ec, &color))
	g_object_set (cell_renderer,
		      "cell-background-gdk", &color, NULL);
      else
	g_object_set (cell_renderer,
		      "cell-background-gdk", NULL, NULL);
    }
}

void
calendar_description_cell_data_func (GtkCellLayout *cell_layout,
				     GtkCellRenderer *cell_renderer,
				     GtkTreeModel *model,
				     GtkTreeIter *iter,
				     gpointer data)
{
  EventCalendar *ec;
  gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);
  if (! ec)
    return;

  char *s = event_calendar_get_description (ec);
  g_object_set (cell_renderer, "text", s, NULL);
  g_free (s);

  GdkColor color;
  if (event_calendar_get_color (ec, &color))
    g_object_set (cell_renderer,
		  "cell-background-gdk", &color, NULL);
  else
    g_object_set (cell_renderer,
		  "cell-background-gdk", NULL, NULL);
}

void
calendar_last_update_cell_data_func (GtkCellLayout *cell_layout,
				     GtkCellRenderer *cell_renderer,
				     GtkTreeModel *model,
				     GtkTreeIter *iter,
				     gpointer data)
{
  EventCalendar *ec;
  gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);
  if (! ec)
    return;

  switch (event_calendar_get_mode (ec))
    {
    case 0:
    default:
      g_object_set (cell_renderer, "text", "", NULL);
      break;
    case 1:
      {
	time_t now = time (NULL);
	time_t t = event_calendar_get_last_pull (ec);
	time_t diff = now - t;
	char *s;
	if (diff < 60 * 60)
	  s = g_strdup_printf (_("%ld minutes ago"), diff / 60);
	else if (diff < 2 * 24 * 60 * 60)
	  s = g_strdup_printf (_("%ld hours ago"), diff / 60 / 60);
	else if (diff < 14 * 24 * 60 * 60)
	  s = g_strdup_printf (_("%ld days ago"), diff / 24 / 60 / 60);
	else if (t == 0)
	  s = g_strdup ("Never");
	else
	  {
	    struct tm tm;
	    localtime_r (&t, &tm);
	    s = strftime_strdup_utf8_locale (_("%x"), &tm);
	  }

	g_object_set (cell_renderer, "text", s, NULL);
	g_free (s);
	break;
      }
    case 2:
      {
	time_t t = event_calendar_get_last_push (ec);
	time_t m = event_calendar_get_last_modification (ec);

	if (m <= t)
	  g_object_set (cell_renderer, "text", _("Up to date"), NULL);
	else
	  g_object_set (cell_renderer, "text", _("Modified"), NULL);
	break;
      }
    }

  GdkColor color;
  if (event_calendar_get_color (ec, &color))
    g_object_set (cell_renderer,
		  "cell-background-gdk", &color, NULL);
  else
    g_object_set (cell_renderer,
		  "cell-background-gdk", NULL, NULL);
}

static void
pixmap_it (GtkCellRenderer *cell_renderer,
	   EventCalendar *ec,
	   const char *pixmap,
	   gboolean set)
{
  if (set)
    g_object_set (cell_renderer, "stock-id", pixmap, NULL);
  else
    g_object_set (cell_renderer, "stock-id", NULL, NULL);

  GdkColor color;
  if (event_calendar_get_color (ec, &color))
    g_object_set (cell_renderer,
		  "cell-background-gdk", &color, NULL);
  else
    g_object_set (cell_renderer,
		  "cell-background-gdk", NULL, NULL);
}

void
calendar_refresh_cell_data_func (GtkCellLayout *cell_layout,
				 GtkCellRenderer *cell_renderer,
				 GtkTreeModel *model,
				 GtkTreeIter *iter,
				 gpointer data)
{
  EventCalendar *ec;
  gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);
  if (! ec)
    return;

  char *image;
  gboolean set;
  switch (event_calendar_get_mode (ec))
    {
    case 1:
      image = GTK_STOCK_GO_DOWN;
      set = TRUE;
      break;
    case 2:
      image = GTK_STOCK_GO_UP;
      set = TRUE;
      break;
    default:
      image = NULL;
      set = FALSE;
    }

  pixmap_it (cell_renderer, ec, image, set);
}

void
calendar_edit_cell_data_func (GtkCellLayout *cell_layout,
			      GtkCellRenderer *cell_renderer,
			      GtkTreeModel *model,
			      GtkTreeIter *iter,
			      gpointer data)
{
  EventCalendar *ec;
  gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);
  if (! ec)
    return;

  pixmap_it (cell_renderer, ec, GTK_STOCK_EDIT, TRUE);
}

void
calendar_delete_cell_data_func (GtkCellLayout *cell_layout,
				GtkCellRenderer *cell_renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer data)
{
  EventCalendar *ec;
  gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);
  if (! ec)
    return;

  pixmap_it (cell_renderer, ec, GTK_STOCK_DELETE, TRUE);
}

struct callback
{
  CalendarMenuSelected cb;
  EventCalendar *ec;
  gpointer user_data;
};

static void
menu_item_activated (GtkWidget *menu_item, gpointer user_data)
{
  struct callback *cb = user_data;

  cb->cb (cb->ec, cb->user_data);
}

static void
build_menu (CalendarMenuSelected cb, gpointer user_data,
	    GtkMenu *menu, int *row, int indent,
	    GtkTreeModel *model, GtkTreeIter *iter)
{
  int ret;

  do
    {
      EventCalendar *ec;
      gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);

      char *title = event_calendar_get_title (ec);
      char *s = g_strdup_printf ("%*s%s", indent * 2, "", title);
      g_free (title);
      GtkWidget *item = gtk_menu_item_new_with_label (s);
      g_free (s);
      gtk_widget_show (item);
      gtk_menu_attach (menu, item, 0, 1, *row, *row + 1);
      (*row) ++;

      struct callback *cb_info = g_malloc (sizeof (*cb_info));
      cb_info->cb = cb;
      cb_info->ec = ec;
      cb_info->user_data = user_data;

      g_signal_connect_data (G_OBJECT (item), "activate",
			     G_CALLBACK (menu_item_activated),
			     (gpointer) cb_info,
			     (GClosureNotify) g_free, 0);

      GtkTreeIter child;
      if (gtk_tree_model_iter_children (model, &child, iter))
	build_menu (cb, user_data, menu, row, indent + 1, model, &child);

      ret = gtk_tree_model_iter_next (model, iter);
    }
  while (ret);
}

GtkWidget *
calendars_menu (EventDB *edb, CalendarMenuSelected cb, gpointer data)
{
  GtkTreeModel *model = calendars_tree_model (edb);
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      int row = 0;
      GtkMenu *menu = GTK_MENU (gtk_menu_new ());
      build_menu (cb, data, menu, &row, 0, model, &iter);
      g_object_unref (model);
      return GTK_WIDGET (menu);
    }

  g_object_unref (model);
  return NULL;
}

static void
populate_store (EventDB *edb, GtkTreeStore *tree_store)
{
  GSList *l = event_db_list_event_calendars (edb);
  int n = g_slist_length (l);
  struct info
  {
    EventCalendar *ec;
    EventCalendar *ecp;
    gboolean inserted;
    GtkTreeIter iter;
  };
  struct info info[n];
  int count = n;

  GSList *a;
  int i;
  for (i = 0, a = l; i < n; i ++, a = a->next)
    {
      info[i].ec = a->data;
      info[i].ecp = event_calendar_get_parent (a->data);
      if (! info[i].ecp)
	{
	  gtk_tree_store_insert (tree_store, &info[i].iter, NULL, 0);
	  gtk_tree_store_set (tree_store, &info[i].iter,
			      COL_CALENDAR, info[i].ec, -1);
	  info[i].inserted = TRUE;
	  count --;
	}
      else
	info[i].inserted = FALSE;
    }

  while (count > 0)
    {
      int saved_count = count;

      for (i = 0; i < n; i ++)
	if (! info[i].inserted)
	  {
	    int j;
	    for (j = 0; j < n; j ++)
	      if (info[j].inserted && info[j].ec == info[i].ecp)
		{
		  gtk_tree_store_insert (tree_store, &info[i].iter,
					 &info[j].iter, 0);
		  gtk_tree_store_set (tree_store, &info[i].iter,
				      COL_CALENDAR, info[i].ec, -1);
		  info[i].inserted = TRUE;
		  count --;
		}
	  }

      if (count == saved_count)
	/* Cycle!  */
	g_critical ("%s: calendar hierarchy contains a cycle", __func__); 
    }
}

static void
calendar_new (EventDB *edb, EventCalendar *ec, GtkTreeStore *tree_store)
{
  GtkTreeModel *model = GTK_TREE_MODEL (tree_store);

  /* Find the ancestry.  */
  GSList *ancestry = NULL;
  EventCalendar *p = ec;
  while ((p = event_calendar_get_parent (p)))
    ancestry = g_slist_prepend (ancestry, p);

  /* Walk the tree from the top towards where the insertion needs to
     occur.  */
  EventCalendar *c = NULL;
  GtkTreeIter child;
  GtkTreeIter parent;
  while (ancestry)
    {
      if (! gtk_tree_model_iter_children (model, &child, p ? &parent : NULL))
	goto fail;

      while (1)
	{
	  gtk_tree_model_get (model, &child, COL_CALENDAR, &c, -1);

	  if (c == ancestry->data)
	    {
	      g_object_unref (c);
	      ancestry = g_slist_delete_link (ancestry, ancestry);
	      parent = child;
	      break;
	    }

	  if (! gtk_tree_model_iter_next (model, &child))
	    goto fail;
	}
    }

  gtk_tree_store_append (GTK_TREE_STORE (model), &child, c ? &parent : NULL);
  gtk_tree_store_set (GTK_TREE_STORE (model), &child, COL_CALENDAR, ec, -1);

  return;

 fail:
  {
    char *s = event_calendar_get_title (ec);
    g_critical ("%s: could not find %s in tree!", __func__, s);
    g_free (s);
    while (ancestry)
      {
	g_object_unref (ancestry->data);
	ancestry = g_slist_delete_link (ancestry, ancestry);
      }
  }
}

static void
calendar_deleted (EventDB *edb, EventCalendar *ec, GtkTreeStore *tree_store)
{
  GtkTreeIter iter;
  if (search (GTK_TREE_MODEL (tree_store), &iter, NULL, ec))
    {
      g_object_unref (ec);
      gtk_tree_store_remove (tree_store, &iter);
    }
}

static void
unref_model (GtkTreeStore *tree_store)
{
  gboolean deref (GtkTreeModel *model, GtkTreePath *path,
		  GtkTreeIter *iter, gpointer data)
    {
      EventCalendar *ec;
      gtk_tree_model_get (model, iter, COL_CALENDAR, &ec, -1);
      g_object_unref (ec);

      return FALSE;
    }

  gtk_tree_model_foreach (GTK_TREE_MODEL (tree_store), deref, NULL);
}

static void
calendar_reparented (EventDB *edb, EventCalendar *ec,
		     GtkTreeStore *tree_store)
{
  /* We can't simply call calendar_deleted and then calendar_new: both
     assume that EC has no children.  Trying to be smart just isn't
     gpointer to pay: just clear the tree and rebuild it from
     scratch.  */
  unref_model (tree_store);
  gtk_tree_store_clear (tree_store);
  populate_store (edb, tree_store);
}

struct info
{
  EventDB *edb;
  GtkTreeStore *tree_store;

  guint calendar_new_signal;
  guint calendar_deleted_signal;
  guint calendar_reparented_signal;
};

static GSList *tree_stores;

static void
info_destroy (struct info *info)
{
  g_assert (g_slist_find (tree_stores, info));
  tree_stores = g_slist_remove (tree_stores, info);

  g_object_unref (info->edb);
  g_signal_handler_disconnect (info->edb, info->calendar_new_signal);
  g_signal_handler_disconnect (info->edb, info->calendar_deleted_signal);
  g_signal_handler_disconnect (info->edb, info->calendar_reparented_signal);
  g_free (info);
}

GtkTreeModel *
calendars_tree_model (EventDB *edb)
{
  GSList *l;
  for (l = tree_stores; l; l = l->next)
    {
      struct info *info = l->data;

      if (info->edb == edb)
	{
	  g_object_ref (info->tree_store);
	  return GTK_TREE_MODEL (info->tree_store);
	}
    }

  struct info *info = g_malloc (sizeof (*info));
  g_object_ref (edb);
  info->edb = edb;

  /* Create a new tree store.  */
  info->tree_store = gtk_tree_store_new (1, G_TYPE_POINTER);
  g_object_weak_ref (G_OBJECT (info->tree_store), info_destroy, info);

  populate_store (edb, info->tree_store);

  info->calendar_new_signal =
    g_signal_connect (edb, "calendar-new",
		      G_CALLBACK (calendar_new), info->tree_store);
  info->calendar_deleted_signal =
    g_signal_connect (edb, "calendar-deleted",
		      G_CALLBACK (calendar_deleted), info->tree_store);
  info->calendar_reparented_signal =
    g_signal_connect (edb, "calendar-reparented",
		      G_CALLBACK (calendar_reparented), info->tree_store);

  return GTK_TREE_MODEL (info->tree_store);
}
