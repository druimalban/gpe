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

static guint my_signals[2];

struct _GPEIconListItemClass 
{
  GObjectClass parent_class;
  void (*button_press)      (GPEIconListItem *sm, GdkEventButton *);
  void (*button_release)    (GPEIconListItem *sm, GdkEventButton *);
};

static void
gpe_icon_list_item_init (GPEIconListItem *item)
{
}

void
gpe_icon_list_item_button_press (GPEIconListItem *i, GdkEventButton *b)
{
  g_signal_emit (G_OBJECT (i), my_signals[0], 0, b);
}

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
    gdk_pixbuf_unref (item->pb);
  if (item->pb_scaled)
    gdk_pixbuf_unref (item->pb_scaled);
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

GObject *
gpe_icon_list_item_new (void)
{
  return g_object_new (gpe_icon_list_item_get_type (), NULL);
}
