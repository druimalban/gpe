/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
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

#include "gpeiconlistitem.h"
#include "gpeiconlistview.h"

static guint my_signals[2];

struct _GPEIconListItemClass 
{
  GObjectClass parent_class;
  void (*button_press)      (GPEIconListItem *sm, GdkEventButton *);
  void (*button_release)    (GPEIconListItem *sm, GdkEventButton *);
};

extern void _gpe_icon_list_view_queue_redraw (GPEIconListView *view, GPEIconListItem *i);
extern void _gpe_icon_list_view_check_icon_size (GPEIconListView *il, GObject *obj);

static void
gpe_icon_list_item_init (GPEIconListItem *item)
{
}


/**
 * gpe_icon_list_item_button_press:
 * @i: Item
 * @b: Button event to emit
 * 
 * Emit a button press event on a #GPEIconListItem.
 */
void
gpe_icon_list_item_button_press (GPEIconListItem *i, GdkEventButton *b)
{
  g_signal_emit (G_OBJECT (i), my_signals[0], 0, b);
}

/**
 * gpe_icon_list_item_button_release:
 * @i: Item
 * @b: Button event to emit
 * 
 * Emit a button release event on a #GPEIconListItem.
 */
void
gpe_icon_list_item_button_release (GPEIconListItem *i, GdkEventButton *b)
{
  g_signal_emit (G_OBJECT (i), my_signals[1], 0, b);
}

static void
gpe_icon_list_item_class_init (GPEIconListItemClass * klass)
{
  my_signals[0] = g_signal_new ("button-press",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GPEIconListItemClass, button_press),
				NULL, NULL,
				gtk_marshal_VOID__BOXED,
				G_TYPE_NONE, 1,
				GDK_TYPE_EVENT);

  my_signals[1] = g_signal_new ("button-release",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (struct _GPEIconListItemClass, button_release),
				NULL, NULL,
				gtk_marshal_VOID__BOXED,
				G_TYPE_NONE, 1,
				GDK_TYPE_EVENT);
}

static void
gpe_icon_list_item_fini (GPEIconListItem *item)
{
  if (item->pb)
    g_object_unref (item->pb);
  if (item->pb_scaled)
    g_object_unref (item->pb_scaled);
}

GType
gpe_icon_list_item_get_type (void)
{
  static GType item_type = 0;

  if (! item_type)
    {
      static const GTypeInfo info =
      {
	sizeof (GPEIconListItemClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) gpe_icon_list_item_fini,
	(GClassInitFunc) gpe_icon_list_item_class_init,
	(GClassFinalizeFunc) NULL,
	NULL /* class_data */,
	sizeof (GPEIconListItem),
	0 /* n_preallocs */,
	(GInstanceInitFunc) gpe_icon_list_item_init,
      };

      item_type = g_type_register_static (G_TYPE_OBJECT, "GPEIconListItem", &info, (GTypeFlags)0);
    }

  return item_type;
}

/**
 * gpe_icon_list_item_get_pixbuf:
 * @i: Item to get icon pixbuf from.
 * 
 * Returns the currently diplayed icon from a #GPEIconListItem.
 * This function alsways returns the icon with the currently displayed size.
 *
 * Returns: Pointer to GdkPixbuf containing the current icon.
 */
GdkPixbuf *
gpe_icon_list_item_get_pixbuf (GPEIconListItem *i)
{
  return i->pb_scaled ? i->pb_scaled : i->pb;
}

/**
 * gpe_icon_list_item_set_pixbuf:
 * @i: Item to alter.
 * @pixbuf: New icon to associate with item.
 * 
 * Set the icon displayed by the item. 
 * The pixbuf will be resized to the icon size defined by the #GPEIconListView
 * the item belongs to. If an old icon exists its reference will be released by 
 * g_object_unref(). 
 *
 */
void
gpe_icon_list_item_set_pixbuf (GPEIconListItem *i, GdkPixbuf *pixbuf)
{
  if (i->pb)
    g_object_unref (i->pb);
  if (i->pb_scaled)
    g_object_unref (i->pb_scaled);
  
  g_object_ref (pixbuf);
  i->pb = pixbuf;

  if (i->parent)
    {
      _gpe_icon_list_view_check_icon_size (i->parent, G_OBJECT (i));
      _gpe_icon_list_view_queue_redraw (i->parent, i);
    }
}

/**
 * gpe_icon_list_item_set_parent:
 * @i: Item to move.
 * @view: #GPEIconListView to move the item to.
 * 
 * Change the #GPEIconListView the item belongs to.
 *
 */
void
gpe_icon_list_item_set_parent (GPEIconListItem *item, GPEIconListView *parent)
{
  item->parent = parent;
}

/**
 * gpe_icon_list_item_new:
 * 
 * Create a new item widget.
 *
 * Returns: New widget.
 */
GObject *
gpe_icon_list_item_new (void)
{
  return g_object_new (gpe_icon_list_item_get_type (), NULL);
}
