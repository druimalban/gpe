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

struct _GPEIconListItemClass 
{
  GObjectClass parent_class;
};

static void
gpe_icon_list_item_init (GPEIconListItem *item)
{
}

static void
gpe_icon_list_item_class_init (GPEIconListItemClass * klass)
{
}

static void
gpe_icon_list_item_fini (GPEIconListItem *item)
{
  if (item->pb)
    gdk_pixbuf_unref (item->pb);
  if (item->pb_scaled)
    gdk_pixbuf_unref (item->pb_scaled);
}

GtkType
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
