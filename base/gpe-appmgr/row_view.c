/* gpe-appmgr - a program launcher

   Copyright (c) 2004 Phil Blundell

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

/* Gtk etc. */
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

/* directory listing */
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

/* catch SIGHUP */
#include <signal.h>

/* I/O */
#include <stdio.h>

/* malloc / free */
#include <stdlib.h>

/* time() */
#include <time.h>

#include <X11/Xatom.h>

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
#include "package.h"
#include "plugin.h"
#include "popupmenu.h"
#include "xsi.h"
#include <dlfcn.h>
#include "tray.h"

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
run_callback (GObject *obj, GdkEventButton *ev, struct package *p)
{
  run_package (p);
}

/* Make the contents for a notebook tab.
 * Generally we only want one group, but NULL means
 * ignore group setting (ie. "All").
 */
GtkWidget *
create_row (GList *all_items, char *current_group)
{
  GtkWidget *il;
  GList *this_item;
  //char *icon_file;

  il = gpe_icon_list_view_new ();

  gpe_icon_list_view_set_rows (GPE_ICON_LIST_VIEW (il), 1);
  
  this_item = all_items;
  while (this_item)
    {
      struct package *p;
      GObject *item;
 
      p = (struct package *) this_item->data;
      
      if (!current_group || (current_group && !strcmp (current_group, package_get_data (p, "section")))) {
	item = gpe_icon_list_view_add_item (GPE_ICON_LIST_VIEW (il),
					    package_get_data (p, "title"),
					    get_icon_fn (p, 48),
					    (gpointer)p);

	g_signal_connect (item, "button-release", G_CALLBACK (run_callback), p);
      }
      
      this_item = this_item->next;
      
    }
  
  gtk_widget_show (il);
  return il;
}

void 
refresh_tabs (void)
{
  GList *l;
  GSList *rows = NULL, *p;
  int n_rows = 0;
  int i;
  GtkWidget *sep;

  for (l = groups; l; l = l->next)
    {
      gchar *group = l->data;
      GtkWidget *row;

      row = create_row (items, group);

      rows = g_slist_append (rows, row);
      n_rows++;
    }

  table = gtk_table_new (3, n_rows, FALSE);
  gtk_widget_show (table);

  for (i = 0, p = rows, l = groups; i < n_rows; i++)
    {
      GtkWidget *label;
      label = gtk_label_new (l->data);
      gtk_widget_show (label);

      gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (label), 0, 1, i, i + 1, 0, 0, 0, 0);
      gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (p->data), 2, 3, i, i + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
      p = p->next;
      l = l->next;
    }
  g_slist_free (rows);

  sep = gtk_vseparator_new ();
  gtk_widget_show (sep);
  gtk_table_attach (GTK_TABLE (table), sep, 1, 2, 0, n_rows, 0, 0, 0, 0);

  gtk_container_add (GTK_CONTAINER (bin), table);
}

GtkWidget *
create_row_view (void)
{
  bin = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (bin);

  return bin;
}
