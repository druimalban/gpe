/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <time.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "gtkdatesel.h"
#include <libintl.h>
#include <stdlib.h>

#include "globals.h"

#define _(x) dgettext(PACKAGE, x)

#ifdef IS_HILDON
  #define LABEL_ADD  16
  #define ARROW_SIZE 30
#else
  #define LABEL_ADD   8
  #define ARROW_SIZE 20
#endif

static guint my_signals[1];

struct elem
{
  GtkWidget *container;
  GtkWidget *display;
};

struct _GtkDateSel
{
  GtkHBox hbox;
  GtkWidget *subbox;

  struct elem day, week, month, year;

  gint day_clamped;

  time_t time;

  GtkDateSelMode mode;
  GtkDateSelMonthStyle month_style;
};

struct _GtkDateSelClass
{
  GtkHBoxClass parent_class;
  void (* changed)      (GtkDateSel *sel);
};

static GtkHBoxClass *parent_class = NULL;

void
gtk_date_sel_move_day (GtkDateSel *sel, int d)
{
  sel->time += d * 60 * 60 * 24;
  sel->day_clamped = -1;
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static void
day_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;
  gtk_date_sel_move_day (sel, d);
}

void
gtk_date_sel_move_week (GtkDateSel *sel, int d)
{
  sel->time += d * 60 * 60 * 24 * 7;
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static void
week_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;
  gtk_date_sel_move_week (sel, d);
}

void
gtk_date_sel_move_month (GtkDateSel *sel, int d)
{
  struct tm tm;
  guint mdays;
  localtime_r (&sel->time, &tm);
  if (d < 0)
    {
      if (tm.tm_mon)
        tm.tm_mon--;
      else
        {
          tm.tm_year--;
          tm.tm_mon = 11;
        }
    }
  else
    {
      if (tm.tm_mon < 11)
        tm.tm_mon++;
      else
        {
          tm.tm_year++;
          tm.tm_mon = 0;
        }
    }
  mdays = days_in_month (tm.tm_year + 1900, tm.tm_mon);
  if (tm.tm_mday > mdays)
    {
      sel->day_clamped = tm.tm_mday;
      tm.tm_mday = mdays;
    }
  else
    {
      if (sel->day_clamped != -1)
        {
          tm.tm_mday = mdays;
          if (tm.tm_mday > sel->day_clamped)
            tm.tm_mday = sel->day_clamped;
        }
    }
  sel->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static void
month_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;
  gtk_date_sel_move_month (sel, d);
}

static void
year_click (GtkWidget *w, GtkDateSel *sel)
{
  int d = g_object_get_data (G_OBJECT (w), "direction") ? 1 : -1;
  struct tm tm;
  guint mdays;
  localtime_r (&sel->time, &tm);
  tm.tm_year += d;
  mdays = days_in_month (tm.tm_year + 1900, tm.tm_mon);
  if (tm.tm_mday > mdays)
    {
      sel->day_clamped = tm.tm_mday;
      tm.tm_mday = mdays;
    }
  else
    {
      if (sel->day_clamped != -1)
        {
          tm.tm_mday = mdays;
          if (tm.tm_mday > sel->day_clamped)
            tm.tm_mday = sel->day_clamped;
        }
    }
  sel->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

static gboolean
year_update (GtkWidget *sel, GtkWidget *entry)
{
  struct tm tm;
  gchar *year;

  localtime_r (&GTK_DATE_SEL (sel)->time, &tm);
  year = strftime_strdup_utf8_utf8 (_("%Y"), &tm);
  if (year)
    {
      gtk_entry_set_text (GTK_ENTRY (entry), year);
      g_free (year);
    }

  return FALSE;
}

static gboolean
year_key_press (GtkWidget *entry, GdkEventKey *event, GtkWidget *sel)
{
  int index;
  struct tm tm;

  if (event->keyval != GDK_Return)
    return FALSE;

  index = atoi (gtk_entry_get_text (GTK_ENTRY (entry)));

  localtime_r (&GTK_DATE_SEL (sel)->time, &tm);

  if (index >= 1900)
    tm.tm_year = index - 1900;
  else
    /* If e.g. a two digit year, go to with 2000 + index.  */
    tm.tm_year = 100 + index;

  GTK_DATE_SEL (sel)->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);

  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);

  return FALSE;
}

static gboolean
month_change (GtkComboBox *combo, GtkDateSel *sel)
{
  int index;
  struct tm tm;
  guint mdays;

  index = gtk_combo_box_get_active (combo);
  if (index < 0)
    return FALSE;

  localtime_r (&sel->time, &tm);
  tm.tm_mon = index;
  mdays = days_in_month (tm.tm_year + 1900, tm.tm_mon);
  if (tm.tm_mday > mdays)
    {
      sel->day_clamped = tm.tm_mday;
      tm.tm_mday = mdays;
    }
  else
    {
      if (sel->day_clamped != -1)
        {
          tm.tm_mday = mdays;
          if (tm.tm_mday > sel->day_clamped)
            tm.tm_mday = sel->day_clamped;
        }
    }
  sel->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);

  return FALSE;
}

static void
month_update (GtkDateSel *sel, GtkWidget *combo)
{
  struct tm tm;
  localtime_r (&sel->time, &tm);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), tm.tm_mon);
}

static void
week_update (GtkDateSel *sel, GtkWidget *w)
{
  struct tm tm;
  gchar *sbuf;
  localtime_r (&sel->time, &tm);

  sbuf = strftime_strdup_utf8_utf8 (_("Week %V"), &tm);
  if (sbuf)
    {
      gtk_label_set_text (GTK_LABEL (w), sbuf);
      g_free (sbuf);
    }
}

static gboolean
day_update (GtkWidget *sel, GtkWidget *entry)
{
  struct tm tm;
  gchar *day;

  localtime_r (&GTK_DATE_SEL (sel)->time, &tm);
  day = strftime_strdup_utf8_utf8 (_("%d"), &tm);
  if (day)
    {
      gtk_entry_set_text (GTK_ENTRY (entry), day);
      g_free (day);
    }

  return FALSE;
}

static gboolean
day_key_press (GtkWidget *entry, GdkEventKey *event, GtkWidget *sel)
{
  int index;
  struct tm tm;
  guint mdays;

  if (event->keyval != GDK_Return)
    return FALSE;

  index = atoi (gtk_entry_get_text (GTK_ENTRY (entry)));
  if (index <= 0)
    index = 1;

  localtime_r (&GTK_DATE_SEL (sel)->time, &tm);
  mdays = days_in_month (tm.tm_year + 1900, tm.tm_mon);
  if (index > mdays)
    index = mdays;

  tm.tm_mday = index;
  GTK_DATE_SEL (sel)->time = mktime (&tm);
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);

  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);

  return FALSE;
}

static gboolean
entry_button_press (GtkWidget *entry, GdkEventKey *event, gpointer data)
{
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
  gtk_widget_grab_focus (entry);
  return TRUE;
}

static char *
month (GtkDateSel *sel, int mon)
{
  const char *fmt;
  struct tm tm;

  switch (sel->month_style)
    {
    case GTKDATESEL_MONTH_SHORT:
      fmt = _("%b");
      break;
    case GTKDATESEL_MONTH_LONG:
      fmt = _("%B");
      break;
    case GTKDATESEL_MONTH_NUMERIC:
      fmt = _("%m");
      break;
    case GTKDATESEL_MONTH_ROMAN:
    default:
      return NULL;
    }

  tm.tm_mon = mon;
  return strftime_strdup_utf8_utf8 (fmt, &tm);
}

/* Assumes that E->DISPLAY is valid.  */
static void
make_field (GtkDateSel *sel, struct elem *e,
	    void (*click)(GtkWidget *, GtkDateSel *))
{
  GTK_IS_WIDGET (e->display);

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
  e->container = gtk_hbox_new (FALSE, 0);

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

  g_signal_connect (G_OBJECT (arrow_button_l), "clicked", G_CALLBACK (click), sel);
  g_signal_connect (G_OBJECT (arrow_button_r), "clicked", G_CALLBACK (click), sel);

  gtk_box_pack_start (GTK_BOX (e->container), arrow_button_l, FALSE, FALSE, 0);
  gtk_widget_show (arrow_button_l);
  gtk_box_pack_start (GTK_BOX (e->container), e->display, TRUE, FALSE, 0);
  gtk_widget_show (e->display);
  gtk_box_pack_start (GTK_BOX (e->container), arrow_button_r, FALSE, FALSE, 0);
  gtk_widget_show (arrow_button_r);

  gtk_box_pack_start (GTK_BOX (sel->subbox), e->container, FALSE, FALSE, 0);
}

static void
gtk_date_sel_class_init (GtkDateSelClass * klass)
{
  GObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = gtk_type_class (gtk_hbox_get_type ());
  oclass       = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  my_signals[0] = g_signal_new ("changed",
                                G_OBJECT_CLASS_TYPE (oclass),
                                G_SIGNAL_RUN_LAST,
                                G_STRUCT_OFFSET (GtkDateSelClass, changed),
                                NULL,
                                NULL,
                                g_cclosure_marshal_VOID__VOID,
                                G_TYPE_NONE,
                                0);

}

GType
gtk_date_sel_get_type (void)
{
  static GType date_sel_type = 0;

  if (! date_sel_type)
    {
      static const GTypeInfo date_sel_info =
      {
        sizeof (GtkDateSelClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gtk_date_sel_class_init,
        NULL, /* class finalizer */
        NULL, /* class data */
        sizeof (GtkDateSel),
        0, /* # preallocs */
        (GInstanceInitFunc) NULL,
      };

      date_sel_type = g_type_register_static (GTK_TYPE_HBOX, "GTKDateSel",
                                              &date_sel_info, 0);
    }
  return date_sel_type;
}

void
gtk_date_sel_set_mode (GtkDateSel *datesel, GtkDateSelMode mode)
{
  datesel->mode = mode;

  if (mode == GTKDATESEL_WEEK)
    /* Display the week.  */
    {
      gtk_widget_show (datesel->week.container);
      week_update (datesel, datesel->week.display);
    }
  else
    gtk_widget_hide (datesel->week.container);

  if (mode == GTKDATESEL_FULL)
    /* Display the day.  */
    {
      gtk_widget_show (datesel->day.container);
      day_update (GTK_WIDGET (datesel), datesel->day.display);
    }
  else
    gtk_widget_hide (datesel->day.container);

  if (mode == GTKDATESEL_FULL || mode == GTKDATESEL_MONTH
      || mode == GTKDATESEL_WEEK)
    /* Display the month.  */
    {
      gtk_widget_show (datesel->month.container);
      month_update (datesel, datesel->month.display);
    }
  else
    gtk_widget_hide (datesel->month.container);
}

#define GTK_TYPE_MONTH_CELL_RENDERER (month_cell_renderer_get_type ())
#define MONTH_CELL_RENDERER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_MONTH_CELL_RENDERER, MonthCellRenderer))
#define MONTH_CELL_RENDERER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                            GTK_TYPE_MONTH_CELL_RENDERER, MonthCellRendererClass))
#define MONTH_CELL_RENDERER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              GTK_TYPE_MONTH_CELL_RENDERER, MonthCellRendererClass))

typedef struct
{
  GtkCellRendererClass cell_renderer;
} MonthCellRendererClass;

static GObjectClass *month_parent_class;

typedef struct
{
  GtkCellRenderer cell_renderer;
  char *month;
  gboolean bold;
} MonthCellRenderer;

static void month_cell_renderer_class_init (MonthCellRendererClass *klass);

static GType
month_cell_renderer_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (MonthCellRendererClass),
        NULL,
        NULL,
        month_cell_renderer_class_init,
        NULL,
        NULL,
        sizeof (MonthCellRenderer),
        0,
        NULL
      };

      type = g_type_register_static (gtk_cell_renderer_get_type (),
				     "MonthCellRenderer",
				     &info, 0);
    }

  return type;
}

static void month_cell_renderer_finalize (GObject *object);
static void month_cell_renderer_render (GtkCellRenderer *cell,
					GdkWindow *window,
					GtkWidget *widget,
					GdkRectangle *background_area,
					GdkRectangle *cell_area,
					GdkRectangle *expose_area,
					GtkCellRendererState flags);
static void month_cell_renderer_get_size (GtkCellRenderer *cell,
					  GtkWidget *widget,
					  GdkRectangle *cell_area,
					  gint *x_offset,
					  gint *y_offset,
					  gint *width,
					  gint *height);
static void
month_cell_renderer_class_init (MonthCellRendererClass *klass)
{
  month_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = month_cell_renderer_finalize;
  
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);
  cell_class->get_size = month_cell_renderer_get_size;
  cell_class->render = month_cell_renderer_render;
}

static void
month_cell_renderer_finalize (GObject *object)
{
  g_free (MONTH_CELL_RENDERER (object)->month);

  (* G_OBJECT_CLASS (month_parent_class)->finalize) (object);
}

static PangoLayout*
get_layout (MonthCellRenderer *cell,
            GtkWidget *widget,
	    int force_bold)
{
  PangoLayout *layout;

  if (force_bold || cell->bold)
    {
      char *buf = g_strdup_printf ("<b>%s</b>", cell->month);
      layout = gtk_widget_create_pango_layout (widget, NULL);
      pango_layout_set_markup (layout, buf, -1);
      g_free (buf);
    }
  else
    layout = gtk_widget_create_pango_layout (widget, cell->month);

  return layout;
}

static void
month_cell_renderer_get_size (GtkCellRenderer *cell,
			      GtkWidget *widget,
			      GdkRectangle *cell_area,
			      gint *x_offset,
			      gint *y_offset,
			      gint *width,
			      gint *height)
{
  MonthCellRenderer *month_cell_renderer = MONTH_CELL_RENDERER (cell);

  PangoLayout *pl = get_layout (month_cell_renderer, widget, 1);
  PangoRectangle rect;
  pango_layout_get_pixel_extents (pl, NULL, &rect);
  g_object_unref (pl);

  if (height)
    *height = 2 + rect.height;
  if (width)
    *width = 2 + rect.width;
}

static void
month_cell_renderer_render (GtkCellRenderer *cell,
			    GdkWindow *window,
			    GtkWidget *widget,
			    GdkRectangle *background_area,
			    GdkRectangle *cell_area,
			    GdkRectangle *expose_area,
			    GtkCellRendererState flags)
{
  PangoLayout *pl = get_layout (MONTH_CELL_RENDERER (cell), widget, 0);

  gtk_paint_layout (widget->style, window, GTK_WIDGET_STATE (widget),
                    TRUE, expose_area, widget,
                    "cellrenderertext",
                    cell_area->x + 1,
                    cell_area->y + 1,
                    pl);

  g_object_unref (pl);
}

static GtkCellRenderer *
month_cell_renderer_new (void)
{
  return g_object_new (GTK_TYPE_MONTH_CELL_RENDERER, NULL);
}

static void
month_cell_data_func (GtkCellLayout *cell_layout,
		      GtkCellRenderer *cell_renderer,
		      GtkTreeModel *model,
		      GtkTreeIter *iter,
		      gpointer data)
{
  MonthCellRenderer *cell = MONTH_CELL_RENDERER (cell_renderer);
  GtkDateSel *sel = data;
  int mon;

  gtk_tree_model_get (model, iter, 0, &mon, -1);
  g_free (cell->month);
  cell->month = month (sel, mon);
  cell->bold
    = gtk_combo_box_get_active (GTK_COMBO_BOX (sel->month.display)) == mon;
}

GtkWidget *
gtk_date_sel_new (GtkDateSelMode mode, time_t time)
{
  GtkWidget *w = GTK_WIDGET (gtk_type_new (gtk_date_sel_get_type ()));
  GtkDateSel *sel = GTK_DATE_SEL (w);
  struct elem *e;
  int i;

  sel->time = time;
  sel->day_clamped = -1;

  sel->subbox = gtk_hbox_new (FALSE, 0);

  e = &sel->week;
  e->display = gtk_label_new ("");
  make_field (sel, e, week_click);
  g_signal_connect (G_OBJECT (sel), "changed",
		    G_CALLBACK (week_update), e->display);

  e = &sel->day;
  e->display = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (e->display), 3);
  make_field (sel, e, day_click);
  g_signal_connect (G_OBJECT (sel), "changed",
		    G_CALLBACK (day_update),
		    e->display);
  g_signal_connect (G_OBJECT (e->display), "key-press-event",
		    G_CALLBACK (day_key_press), sel);
  g_signal_connect (G_OBJECT (e->display), "button-press-event",
		    G_CALLBACK (entry_button_press), 0);

  e = &sel->month;
  /* Create the model.  */
  GtkListStore *list = gtk_list_store_new (1, G_TYPE_INT);
  GtkTreeIter iter;
  for (i = 0; i < 12; i ++)
    {
      gtk_list_store_append (list, &iter);
      gtk_list_store_set (list, &iter, 0, i, -1);
    }
  e->display = gtk_combo_box_new_with_model (GTK_TREE_MODEL (list));
  g_object_unref (list);

  gtk_date_sel_set_month_style (sel, GTKDATESEL_MONTH_SHORT);

  /* Set the renderer.  */
  GtkCellRenderer *renderer = month_cell_renderer_new ();

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (e->display));
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (e->display), renderer, FALSE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (e->display),
				      renderer,
				      month_cell_data_func, sel, NULL);
  make_field (sel, &sel->month, month_click);

  /* Three columns.  */
  gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (e->display), 3);

  g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (month_update),
		    e->display);
  g_signal_connect (G_OBJECT (e->display), "changed",
		    G_CALLBACK (month_change), sel);

  e = &sel->year;
  e->display = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (e->display), 5);
  make_field (sel, e, year_click);
  g_signal_connect (G_OBJECT (sel), "changed",
		    G_CALLBACK (year_update),
		    e->display);
  g_signal_connect (G_OBJECT (e->display), "key-press-event",
		    G_CALLBACK (year_key_press), sel);
  g_signal_connect (G_OBJECT (e->display), "button-press-event",
		    G_CALLBACK (entry_button_press), 0);

  year_update (w, e->display);
  gtk_widget_show (e->container);

  gtk_date_sel_set_mode (sel, mode);

  gtk_widget_show (sel->subbox);
  gtk_box_pack_start (GTK_BOX (sel), sel->subbox, TRUE, FALSE, 0);

  return w;
}

time_t
gtk_date_sel_get_time (GtkDateSel *sel)
{
  return sel->time;
}

void
gtk_date_sel_set_time (GtkDateSel *sel, time_t time)
{
  sel->time = time;
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

void
gtk_date_sel_set_day (GtkDateSel *sel, int year, int month, int day)
{
  struct tm tm;
  time_t selected_time;
  localtime_r (&sel->time, &tm);
  tm.tm_year = year;
  tm.tm_mon = month;
  tm.tm_mday = day;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  selected_time = mktime (&tm);
  sel->time = selected_time;
  g_signal_emit (G_OBJECT (sel), my_signals[0], 0);
}

void
gtk_date_sel_set_month_style (GtkDateSel *sel, GtkDateSelMonthStyle style)
{
  sel->month_style = style;
}
