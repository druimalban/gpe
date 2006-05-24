/* calendars-widgets.h - Calendars widgets interface.
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

#ifndef CALENDARS_WIDGETS
#define CALENDARS_WIDGETS

#include <gtk/gtk.h>

enum
{
  COL_CALENDAR,
  NUM_COLS
};

extern GtkWidget *calendars_combo_box_new (void);

/* Does not add a reference to the returned calendar.  The reference
   is valid as long as COMBO is live.  */
extern EventCalendar *calendars_combo_box_get_active (GtkComboBox *combo);

extern void calendars_combo_box_set_active (GtkComboBox *combo,
					    EventCalendar *ec);

extern GtkTreeModel *calendars_tree_model (void);

#define TYPE_CALENDAR_TEXT_CELL_RENDERER \
  (calendar_text_cell_renderer_get_type ())
#define CALENDAR_TEXT_CELL_RENDERER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CALENDAR_TEXT_CELL_RENDERER, \
                               CalendarTextCellRenderer))
#define CALENDAR_TEXT_CELL_RENDERER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CALENDAR_TEXT_CELL_RENDERER, \
                            CalendarTextCellRendererClass))
#define IS_CALENDAR_TEXT_CELL_RENDERER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CALENDAR_TEXT_CELL_RENDERER))
#define IS_CALENDAR_TEXT_CELL_RENDERER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_CALENDAR_TEXT_CELL_RENDERER))
#define CALENDAR_TEXT_CELL_RENDERER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CALENDAR_TEXT_CELL_RENDERER, \
                              CalendarTextCellRendererClass))

struct _CalendarTextCellRendererClass;
typedef struct _CalendarTextCellRendererClass CalendarTextCellRendererClass;

struct _CalendarTextCellRenderer;
typedef struct _CalendarTextCellRenderer CalendarTextCellRenderer;

extern GtkCellRenderer *calendar_text_cell_renderer_new (void);

extern void calendar_text_cell_data_func (GtkCellLayout *cell_layout,
					  GtkCellRenderer *cell_renderer,
					  GtkTreeModel *model,
					  GtkTreeIter *iter,
					  gpointer data);

extern void calendar_description_cell_data_func (GtkCellLayout *cell_layout,
						 GtkCellRenderer
						  *cell_renderer,
						 GtkTreeModel *model,
						 GtkTreeIter *iter,
						 gpointer data);

extern void calendar_visible_toggle_cell_data_func (GtkCellLayout *cell_layout,
						    GtkCellRenderer
						     *cell_renderer,
						    GtkTreeModel *model,
						    GtkTreeIter *iter,
						    gpointer data);
extern void calendar_refresh_cell_data_func (GtkCellLayout *cell_layout,
					     GtkCellRenderer *cell_renderer,
					     GtkTreeModel *model,
					     GtkTreeIter *iter,
					     gpointer data);
extern void calendar_edit_cell_data_func (GtkCellLayout *cell_layout,
					  GtkCellRenderer *cell_renderer,
					  GtkTreeModel *model,
					  GtkTreeIter *iter,
					  gpointer data);
extern void calendar_delete_cell_data_func (GtkCellLayout *cell_layout,
					    GtkCellRenderer *cell_renderer,
					    GtkTreeModel *model,
					    GtkTreeIter *iter,
					    gpointer data);

typedef void (*CalendarMenuSelected) (EventCalendar *, gpointer);
extern GtkWidget *calendars_menu (CalendarMenuSelected cb, gpointer data);

#endif /* CALENDARS_WIDGETS */
