/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GPE_ICON_LIST_VIEW_H
#define GPE_ICON_LIST_VIEW_H

#include <gtk/gtk.h>
#include <glib-object.h>

#define GPE_TYPE_ICON_LIST_VIEW           (gpe_icon_list_view_get_type ())
#define GPE_ICON_LIST_VIEW(obj)           G_TYPE_CHECK_INSTANCE_CAST ((obj), gpe_icon_list_view_get_type(), GPEIconListView)
#define GPE_ICON_LIST_VIEW_CONST(obj)	  G_TYPE_CHECK_INSTANCE_CAST ((obj), gpe_icon_list_view_get_type(), GPEIconListView const)
#define GPE_ICON_LIST_VIEW_CLASS(klass)	  G_TYPE_CHECK_CLASS_CAST ((klass), gpe_icon_list_view_get_type(), GPEIConListViewClass)
#define GPE_IS_ICON_LIST_VIEW(obj)	  G_TYPE_CHECK_INSTANCE_TYPE ((obj), gpe_icon_list_view_get_type ())

#define GPE_ICON_LIST_VIEW_GET_CLASS(obj) G_TYPE_INSTANCE_GET_CLASS ((obj), gpe_icon_list_view_get_type(), GPEIconListViewClass)

struct _GPEIconListView
{
  GtkDrawingArea class;
  
  /* private */
  GList * icons;
  GdkPixbuf * bgpixbuf;
  guint32 bgcolor;
  int rows;
  int cols;
  int mcol;
  int mrow;
  int popup_timeout;
  gboolean flag_embolden;
  gboolean flag_show_title;
  guint icon_size;
  guint icon_xmargin;
  guint label_height;
};

typedef struct _GPEIconListView	        GPEIconListView;
typedef struct _GPEIconListViewClass    GPEIconListViewClass;

GType		gpe_icon_list_view_get_type (void);

GtkWidget       *gpe_icon_list_view_new ();

/* Set the background */
void gpe_icon_list_view_set_bg (GPEIconListView *self, char *bg);
void gpe_icon_list_view_set_bg_pixmap (GPEIconListView *self, GdkPixbuf *bg);
void gpe_icon_list_view_set_bg_color (GPEIconListView *self, guint32 color);

void gpe_icon_list_view_remove_item_with_udata (GPEIconListView *self, gpointer udata);
void gpe_icon_list_view_update_icon_item_with_udata (GPEIconListView *self, GdkPixbuf * pixbuf, gpointer udata);
GObject *gpe_icon_list_view_add_item (GPEIconListView *self, char *title, char *icon, gpointer udata);
GObject *gpe_icon_list_view_add_item_pixbuf (GPEIconListView *self, char *title, GdkPixbuf *icon, gpointer udata);
void gpe_icon_list_view_remove_item (GPEIconListView *self, GObject *item);
void gpe_icon_list_view_set_item_icon (GPEIconListView *self, GObject *item, GdkPixbuf *new_pixbuf);

void gpe_icon_list_view_set_embolden (GPEIconListView *self, gboolean yes);
void gpe_icon_list_view_set_show_title (GPEIconListView *self, gboolean yes);
void gpe_icon_list_view_set_icon_xmargin (GPEIconListView *self, guint margin);
void gpe_icon_list_view_clear (GPEIconListView *self);
void gpe_icon_list_view_set_icon_size (GPEIconListView *self, guint size);

void gpe_icon_list_view_popup_removed (GPEIconListView *self);

#endif
