/*
 * gpe-appmgr - a program launcher
 *
 * Copyright (c) 2004 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>

/* Gtk etc. */
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

/* I/O */
#include <stdio.h>

/* malloc / free */
#include <stdlib.h>

/* time() */
#include <time.h>

/* i18n */
#include <libintl.h>
#include <locale.h>

/* GPE */
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/spacing.h>
#include <gpe/gpeiconlistview.h>
#include <gpe/tray.h>

/* everything else */
#include "main.h"

#include "cfg.h"
#include "popupmenu.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(x) {fprintf x ;}
#define TRACE(x) {fprintf(stderr,"TRACE: " x "\n");}
#else
#define TRACE(x) ;
#define DBG(x) ;
#endif

GtkWidget *bin;
GtkWidget *table;

static void
run_callback (GObject *obj, GdkEventButton *ev, GnomeDesktopFile *p)
{
  run_package (p);
}

/* Make the contents for a notebook tab.
 * Generally we only want one group, but NULL means
 * ignore group setting (ie. "All").
 */
static GtkWidget *
create_view (GList *all_items)
{
  GtkWidget *view;
  GList *this_item;

  view = gpe_icon_list_view_new ();

  this_item = all_items;
  while (this_item)
    {
      GnomeDesktopFile *p;
      GObject *item;
      gchar *name = NULL;
 
      p = (GnomeDesktopFile *) this_item->data;
     
      gnome_desktop_file_get_string (p, NULL, "Name", &name);
 
      item = gpe_icon_list_view_add_item (GPE_ICON_LIST_VIEW (view),
					  name,
					  get_icon_fn (p, 48),
					  (gpointer)p);

      g_signal_connect (item, "button-release", G_CALLBACK (run_callback), p);
      
      this_item = this_item->next;
    }
  
  gtk_widget_show (view);
  return view;
}

static void 
refresh_callback (void)
{
  GtkWidget *view;
  struct package_group *group = NULL;
  GList *l;

  for (l = groups; l; l = l->next)
    {
      struct package_group *g = l->data;
      if (!strcmp (g->name, only_group))
	{
	  group = g;
	  break;
	}
    }

  if (group)
    {
      view = create_view (group->items);
      gtk_container_add (GTK_CONTAINER (bin), view);
    }
  else
    fprintf (stderr, "requested group \"%s\" does not exist\n", only_group);
}

GtkWidget *
create_single_view (void)
{
  bin = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (bin);

  g_object_set_data (G_OBJECT (bin), "refresh_func", refresh_callback);

  return bin;
}
