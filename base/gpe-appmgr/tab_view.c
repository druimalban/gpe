/* gpe-appmgr - a program launcher

   Copyright 2002 Robert Mibus;

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
#include <gpe/gpe-iconlist.h>
#include <gpe/tray.h>
#include <gpe/launch.h>

/* everything else */
#include "main.h"

#include "cfg.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(x) {fprintf x ;}
#define TRACE(x) {fprintf(stderr,"TRACE: " x "\n");}
#else
#define TRACE(x) ;
#define DBG(x) ;
#endif

/* For not starting an app twice after a double click */
static int ignore_press = 0;

static void 
cb_clicked (GtkWidget *il, gpointer udata, gpointer data) 
{
  GnomeDesktopFile *p;
  p = udata;
  
  run_package (p);
}

static void 
autohide_labels (int page) 
{
  GtkWidget *hb;
  int i=0;
  int pagenum = page == -1 ? gtk_notebook_get_current_page (GTK_NOTEBOOK(notebook)) : page;

  while (1)
    {
      GtkWidget *page_contents;
      GList *children;
      
      page_contents = gtk_notebook_get_nth_page (GTK_NOTEBOOK(notebook), i);
      if (!page_contents)
	break;
      
      hb = gtk_notebook_get_tab_label (GTK_NOTEBOOK(notebook), page_contents);
      if (!hb)
	continue;
      
      children = gtk_container_children (GTK_CONTAINER(hb));
      while (children)
	{
	  if (GTK_IS_LABEL(children->data))
	    {
	      if (!cfg_options.auto_hide_group_labels || i == pagenum)
		gtk_widget_show (GTK_WIDGET(children->data));
	      else
		gtk_widget_hide (GTK_WIDGET(children->data));
	    }
	  children = children->next;
	}
      
      i++;
    }
}

static void 
nb_switch (GtkNotebook *nb, GtkNotebookPage *page, guint pagenum)
{
  autohide_labels (pagenum);
}

static gint 
unignore_press (gpointer data)
{
  ignore_press = 0;
  return FALSE;
}

/* Remove the appmgr (not plugin) tabs from the notebook */
static void 
clear_appmgr_tabs (void)
{
	int i=0;
	while (1) {
		GtkWidget *page;
		page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i);
		if (page == NULL)
			return;
		if (!GTK_IS_SOCKET(page))
			gtk_notebook_remove_page (GTK_NOTEBOOK(notebook), i);
		else
			i++;
	}
}

/* Make the contents for a notebook tab.
 * Generally we only want one group, but NULL means
 * ignore group setting (ie. "All").
 */
static GtkWidget *
create_tab (GList *all_items, char *current_group, tab_view_style style)
{
	GtkWidget *il;
	GList *this_item;
	//char *icon_file;

	il = gpe_iconlist_new ();

	gtk_signal_connect (GTK_OBJECT (il), "clicked", (GtkSignalFunc)cb_clicked, NULL);

	this_item = all_items;
	while (this_item)
	{
	  GnomeDesktopFile *p;
	  gchar *name;

	  p = (GnomeDesktopFile *) this_item->data;
	  
	  gnome_desktop_file_get_string (p, NULL, "Name", &name);
	  
	  gpe_iconlist_add_item (GPE_ICONLIST (il),
				 name,
				 get_icon_fn (p, 48),
				 (gpointer)p);
	  
	  this_item = this_item->next;
	}

	gtk_widget_show (il);
	return il;
}

/* Creates the image/label combo for a tab.
 */
static GtkWidget *
create_tab_label (char *name, char *icon_file, GtkStyle *style)
{
	GtkWidget *img=NULL,*lbl,*hb;

	img = create_icon_pixmap (style, icon_file, 18);
	if (!img)
		img = create_icon_pixmap (style, "/usr/share/pixmaps/menu_unknown_group16.png", 16);

	lbl = gtk_label_new (name);

	hb = gtk_hbox_new (FALSE, 0);
	if (img)
		gtk_box_pack_start_defaults (GTK_BOX(hb), img);
	gtk_box_pack_start (GTK_BOX(hb), lbl, FALSE, FALSE, gpe_get_boxspacing());

	gtk_widget_show_all (hb);
	if (cfg_options.auto_hide_group_labels)
		gtk_widget_hide (lbl);

	return hb;
}

/* Creates the image/label combo for the tab
 * of a specified group.
 */
static GtkWidget *
create_group_tab_label (char *group, GtkStyle *style)
{
	GtkWidget *hb;
	char *icon_file;

	icon_file = g_strdup_printf ("/usr/share/pixmaps/group_%s.png", group);
	hb = create_tab_label (group, icon_file, style);
	g_free (icon_file);

	return hb;
}

static void 
create_all_tab ()
{
	TRACE ("create_all_tab");
	DBG((stderr, "Show 'All' group? %s\n", cfg_options.show_all_group ? "Yes" : "No"));
	/* Create the 'All' tab if wanted */
	if (cfg_options.show_all_group)
		gtk_notebook_prepend_page (GTK_NOTEBOOK(notebook),
					   create_tab (items, NULL, cfg_options.tab_view),
					   create_group_tab_label("All", notebook->style));
}

/* Wipe the old tabs / icons and replace them with whats
 * currently supposed to be there */
static void 
refresh_callback (void)
{
  GList *l;
  int old_tab;

  old_tab = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

  gtk_widget_hide (notebook);

  clear_appmgr_tabs ();

  create_all_tab ();

  /* Create the normal tabs if wanted */
  for (l = groups; l; l = l->next)
    {
      struct package_group *g = l->data;
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				create_tab (g->items, g->name, cfg_options.tab_view),
				create_group_tab_label (g->name, notebook->style));
      
    }
  
  if (old_tab != -1)
    gtk_notebook_set_page (GTK_NOTEBOOK(notebook), old_tab);
  
  gtk_widget_show_all (notebook);
  
  autohide_labels (0);
}

static void 
icons_page_up_down (int down)
{
  GtkWidget *sw;
  GtkAdjustment *adj;
  int page;
  gfloat newval;
  
  page = gtk_notebook_get_current_page (GTK_NOTEBOOK(notebook));
  sw = gtk_notebook_get_nth_page (GTK_NOTEBOOK(notebook), page);
  
  adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(sw));
  
  if (down)
    newval = adj->value + adj->page_increment;
  else
    newval = adj->value - adj->page_increment;
  
  if (newval < adj->lower)
    newval = adj->lower;
  else if (newval > adj->upper-adj->page_size)
    newval = adj->upper-adj->page_size;
  
  gtk_adjustment_set_value(adj, newval);
}

gint keysnoop (GtkWidget *grab_widget, GdkEventKey *event, gpointer func_data)
{
  if (event->type != GDK_KEY_PRESS)
    return 1;

  switch (event->keyval) {
  case GDK_Up:
    icons_page_up_down (0);
    break;
  case GDK_Down:
    icons_page_up_down (1);
    break;
  case GDK_Left:
    gtk_notebook_prev_page (GTK_NOTEBOOK(notebook));
    break;
  case GDK_Right:
    gtk_notebook_next_page (GTK_NOTEBOOK(notebook));
    break;
  default:
    DBG ((stderr, "Unhandled key: %d\n", event->keyval));
    DBG ((stderr, "     (%s)\n", gdk_keyval_name(event->keyval)));
    return 0;
  }

  return 1;
}

GtkWidget *
create_tab_view (void)
{
  notebook = gtk_notebook_new ();
  gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(notebook), FALSE);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK(notebook), TRUE);
  gtk_notebook_set_tab_border (GTK_NOTEBOOK(notebook), 0);
  gtk_signal_connect (GTK_OBJECT(notebook), "switch_page",
		      (GtkSignalFunc)nb_switch, NULL);

  /* Send all key events to the one place */
  gtk_key_snooper_install ((GtkKeySnoopFunc)keysnoop,NULL);

  g_object_set_data (G_OBJECT (notebook), "refresh_func", refresh_callback);

  return notebook;
}
