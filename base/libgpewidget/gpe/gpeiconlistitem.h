/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GPE_ICON_LIST_ITEM_H
#define GPE_ICON_LIST_ITEM_H

#include <gtk/gtk.h>
#include <glib-object.h>

#define GPE_TYPE_ICON_LIST_ITEM           (gpe_icon_list_item_get_type ())
#define GPE_ICON_LIST_ITEM(obj)           G_TYPE_CHECK_INSTANCE_CAST ((obj), gpe_icon_list_item_get_type(), GPEIconListItem)
#define GPE_ICON_LIST_ITEM_CONST(obj)	  G_TYPE_CHECK_INSTANCE_CAST ((obj), gpe_icon_list_item_get_type(), GPEIconListItem const)
#define GPE_ICON_LIST_ITEM_CLASS(klass)	  G_TYPE_CHECK_CLASS_CAST ((klass), gpe_icon_list_item_get_type(), GPEIConListItemClass)
#define GPE_IS_ICON_LIST_ITEM(obj)	  G_TYPE_CHECK_INSTANCE_TYPE ((obj), gpe_icon_list_item_get_type ())

#define GPE_ICON_LIST_ITEM_GET_CLASS(obj) G_TYPE_INSTANCE_GET_CLASS ((obj), gpe_icon_list_item_get_type(), GPEIconListItemClass)

struct _GPEIconListItem
{
  GObject class;

  char *title;
  char *icon;
  gpointer udata;
  GdkPixbuf *pb;
  GdkPixbuf *pb_scaled;  
};

typedef struct _GPEIconListItem	        GPEIconListItem;
typedef struct _GPEIconListItemClass    GPEIconListItemClass;

GType		gpe_icon_list_item_get_type (void);

GObject        *gpe_icon_list_item_new ();

void           gpe_icon_list_item_button_press (GPEIconListItem *i, GdkEventButton *b);

void           gpe_icon_list_item_button_release (GPEIconListItem *i, GdkEventButton *b);

#endif
