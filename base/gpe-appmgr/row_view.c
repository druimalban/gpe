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
  run_package (p, obj);
}

/* Make the contents for a notebook tab.
 * Generally we only want one group, but NULL means
 * ignore group setting (ie. "All").
 */
static GtkWidget *
create_row (GList *all_items, char *current_group)
{
  GtkWidget *view;
  GList *this_item;

  view = gpe_icon_list_view_new ();

  gpe_icon_list_view_set_icon_size (GPE_ICON_LIST_VIEW (view), 32);
  gpe_icon_list_view_set_icon_xmargin (GPE_ICON_LIST_VIEW (view), 32);
  
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
  GList *l;
  GSList *rows = NULL, *p;
  int n_rows = 0;
  int i;
  GtkWidget *sep;

  for (l = groups; l; l = l->next)
    {
      struct package_group *group = l->data;
      GtkWidget *row;

      if (group->hide)
	continue;

      row = create_row (group->items, group->name);
      g_object_set_data (G_OBJECT (row), "group", group);

      rows = g_slist_append (rows, row);
      n_rows++;
    }

  table = gtk_table_new (3, n_rows, FALSE);
  gtk_widget_show (table);

  for (i = 0, p = rows; i < n_rows; i++)
    {
      GtkWidget *label;
      struct package_group *g;
      GtkWidget *w;

      w = p->data;
      g = g_object_get_data (G_OBJECT (w), "group");

      label = gtk_label_new (g->name);
      gtk_widget_show (label);

      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);

      gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (label), 0, 1, i, i + 1, 0, GTK_FILL | GTK_SHRINK, 0, 0);
      gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (w), 2, 3, i, i + 1, GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
      p = p->next;
    }

  g_slist_free (rows);

  sep = gtk_vseparator_new ();
  gtk_widget_show (sep);
  gtk_table_attach (GTK_TABLE (table), sep, 1, 2, 0, n_rows, 0, 0, 0, 0);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (bin), table);
}

static void
realize_callback (GtkWidget *widget)
{
  pid_t pid;
  gchar *id;

  id = g_strdup_printf ("%d", gtk_socket_get_id (GTK_SOCKET (widget)));
  pid = vfork ();
  if (pid == 0) {
    execlp ("gpe-today", "gpe-today", "-s", id, NULL);
    perror ("gpe-today");
    exit (1);
  }
  g_free (id);
}

GtkWidget *
create_row_view (void)
{
  GtkWidget *socket;
  GtkWidget *html;

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_RIGHT);

  socket = gtk_socket_new ();
  g_signal_connect (G_OBJECT (socket), "realize", G_CALLBACK (realize_callback), NULL);
  gtk_widget_show (socket);

  bin = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (bin);

  html = gtk_html_new ();
  gtk_widget_show (html);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), socket, gtk_label_new ("Today"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), bin, gtk_label_new ("Programs"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), html, gtk_label_new ("Wizards"));

  gtk_widget_show (notebook);

  g_object_set_data (G_OBJECT (notebook), "refresh_func", refresh_callback);

  return notebook;
}
