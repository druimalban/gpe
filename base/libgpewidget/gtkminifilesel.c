/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

#include "gtkminifilesel.h"

#define _(x) gettext(x)

static GtkWindowClass *parent_class;

static void set_directory (GtkMiniFileSelection *fs, char *directory);

static void
gtk_mini_file_selection_size_allocate (GtkWidget     *widget,
				       GtkAllocation *allocation)
{
  GtkMiniFileSelection *fs;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MINI_FILE_SELECTION (widget));
  g_return_if_fail (allocation != NULL);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
  
  fs = GTK_MINI_FILE_SELECTION (widget);
}

static void
gtk_mini_file_selection_size_request (GtkWidget     *widget,
				      GtkRequisition *requisition)
{
  GtkMiniFileSelection *fs;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MINI_FILE_SELECTION (widget));
  g_return_if_fail (requisition != NULL);

  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);

  requisition->height = 240;  
}

static void
gtk_mini_file_selection_show (GtkWidget *widget)
{
  GtkMiniFileSelection *fs;
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MINI_FILE_SELECTION (widget));

  GTK_WIDGET_CLASS (parent_class)->show (widget);

  fs = GTK_MINI_FILE_SELECTION (widget);
  
  gtk_widget_grab_focus (fs->entry);
}

static void
menu_change_directory (GtkWidget *w, gpointer p)
{
  GtkMiniFileSelection *fs = GTK_MINI_FILE_SELECTION (p);
  char *s = strdup (gtk_object_get_data (GTK_OBJECT (w), "text"));
  
  set_directory (p, s);
}

static void
free_string (GtkObject *o, gpointer p)
{
  g_free (p);
}

static void
add_directory (GtkWidget *menu, char *name, GtkMiniFileSelection *fs)
{
  GtkWidget *item = gtk_menu_item_new_with_label (name);
  gchar *p;
  gtk_widget_show (item);
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
		      menu_change_directory, fs);
  p = g_strdup (name);
  gtk_object_set_data (GTK_OBJECT (item), "text", p);
  gtk_signal_connect (GTK_OBJECT (item), "destroy",
		      free_string, p);
}

static void
set_directory_menu (GtkWidget *option, char *dir, GtkMiniFileSelection *fs)
{
  GtkWidget *menu = gtk_menu_new ();
  char *buf = alloca (strlen (dir) + 1);
  char *p, *n;
  guint nr = 1;

  strcpy (buf, dir);

  add_directory (menu, "/", fs);

  n = buf;
  if (*n == '/')
    n++;
  for (; p = strchr (n, '/'), p; n = p + 1)
    {
      *p = 0;
      add_directory (menu, buf, fs);
      *p = '/';
      nr++;
    }
 
  add_directory (menu, buf, fs);

  gtk_widget_show (menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option), nr);
}

static void
set_members (GtkWidget *clist, char *directory)
{
  struct dirent *d;
  DIR *dir;
  GSList *files = NULL, *subdirs = NULL, *iter;
  gchar *buf = g_malloc (strlen (directory) + 256 + 2);
  gchar *fp;
  gchar *text[2];
 
  gtk_clist_clear (GTK_CLIST (clist));

  sprintf (buf, "%s/", directory);
  fp = buf + strlen (directory) + 1;

  dir = opendir (directory);
  if (dir)
    {
      while (d = readdir (dir), d != NULL)
	{
	  struct stat s;
	  strcpy (fp, d->d_name);
	  if (stat (buf, &s) == 0)
	    {
	      if (S_ISDIR (s.st_mode))
		{
		  if (strcmp (d->d_name, "..") && strcmp (d->d_name, "."))
		    {
		      gchar *fn = g_malloc (strlen (d->d_name) + 3);
		      sprintf (fn, "[%s]", d->d_name);
		      subdirs = g_slist_append (subdirs, fn);
		    }
		}
	      else
		{
		  gchar *fn = g_strdup (d->d_name);
		  files = g_slist_append (files, fn);
		}
	    }
	}
      closedir (dir);
    }

  g_slist_sort (subdirs, (GCompareFunc)strcoll);
  g_slist_sort (files, (GCompareFunc)strcoll);

  if (strcmp (directory, "/") != 0)
    {
      guint row;
      text[0] = NULL;
      text[1] = "[..]";
      row = gtk_clist_append (GTK_CLIST (clist), text);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, (gpointer)2);
    }

  for (iter = subdirs; iter; iter = iter->next)
    {
      guint row;
      text[1] = iter->data;
      text[0] = NULL;
      row = gtk_clist_append (GTK_CLIST (clist), text);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, (gpointer)1);
      g_free (iter->data);
    }
  for (iter = files; iter; iter = iter->next)
    {
      guint row;
      text[1] = iter->data;
      text[0] = NULL;
      row = gtk_clist_append (GTK_CLIST (clist), text);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, (gpointer)0);
      g_free (iter->data);
    }

  g_free (buf);
}

static void
set_directory (GtkMiniFileSelection *fs, char *directory)
{
  if (fs->directory)
    free (fs->directory);
  fs->directory = directory;

  set_directory_menu (fs->option, fs->directory, fs);
  set_members (fs->clist, fs->directory);
}

static void 
selection_made(GtkWidget      *clist,
	       gint            row,
	       gint            column,
	       GdkEventButton *event,
	       GtkWidget      *widget)
{
  guint type = (guint)gtk_clist_get_row_data (GTK_CLIST (clist), row);
  gchar *text;
  GtkMiniFileSelection *fs = GTK_MINI_FILE_SELECTION (widget);
  gtk_clist_get_text (GTK_CLIST (clist), row, 1, &text);
 
  if (event->type == GDK_2BUTTON_PRESS)
    {
      switch (type)
	{
	case 1:
	  {
	    size_t l = strlen (text);
	    if (text[0] == '[' && text[l - 1] == ']')
	      {
		size_t dl = strlen (fs->directory);
		char *s = malloc (l + dl);
		strcpy (s, fs->directory);
		s[dl] = '/';
		strncpy (s + dl + 1, text + 1, l - 2);
		s[dl + l - 1] = 0;
		set_directory (fs, s);
	      }
	    break;
	  }
	case 2:
	  {
	    char *p = alloca (strlen (fs->directory) + 1);
	    char *d;
	    strcpy (p, fs->directory);
	    d = dirname (p);
	    set_directory (fs, strdup (d));
	    break;
	  }
	case 0:
	  {
	    gtk_entry_set_text (GTK_ENTRY (fs->entry), text);
	    break;
	  }
	}
    }
  else
    {
      if (type == 0)
	gtk_entry_set_text (GTK_ENTRY (fs->entry), text);
    }
}

static void
gtk_mini_file_selection_init (GtkMiniFileSelection *fs)
{
  GtkWidget *vbox, *hbox, *scrolled;
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (fs), vbox);

  fs->option = gtk_option_menu_new ();
  gtk_widget_show (fs->option);
  gtk_box_pack_start (GTK_BOX (vbox), fs->option, FALSE, FALSE, 0);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  fs->clist = gtk_clist_new (2);
  gtk_widget_show (fs->clist);
  gtk_container_add (GTK_CONTAINER (scrolled), fs->clist);

  gtk_signal_connect (GTK_OBJECT (fs->clist), "select_row",
		      GTK_SIGNAL_FUNC (selection_made),
		      fs);

  fs->entry = gtk_entry_new ();
  gtk_widget_show (fs->entry);
  gtk_box_pack_start (GTK_BOX (vbox), fs->entry, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  fs->cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_show (fs->cancel_button);
  gtk_box_pack_start (GTK_BOX (hbox), fs->cancel_button, TRUE, TRUE, 0);

  fs->ok_button = gtk_button_new_with_label (_("OK"));
  gtk_widget_show (fs->ok_button);
  gtk_box_pack_start (GTK_BOX (hbox), fs->ok_button, TRUE, TRUE, 0);

  fs->directory = NULL;
  set_directory (fs, get_current_dir_name ());
}

static void
gtk_mini_file_selection_destroy (GtkObject *fs)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (fs);
}

static void
gtk_mini_file_selection_class_init (GtkMiniFileSelectionClass * klass)
{
  GtkObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = gtk_type_class (gtk_window_get_type ());
  oclass = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  oclass->destroy = gtk_mini_file_selection_destroy;
  
  widget_class->size_request = gtk_mini_file_selection_size_request;
  widget_class->size_allocate = gtk_mini_file_selection_size_allocate;
  widget_class->show = gtk_mini_file_selection_show;
}

guint
gtk_mini_file_selection_get_type (void)
{
  static guint mini_file_selection_type;

  if (! mini_file_selection_type)
    {
      static const GtkTypeInfo mini_file_selection_info =
      {
	"GtkMiniFileSelection",
	sizeof (GtkMiniFileSelection),
	sizeof (GtkMiniFileSelectionClass),
	(GtkClassInitFunc) gtk_mini_file_selection_class_init,
	(GtkObjectInitFunc) gtk_mini_file_selection_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      mini_file_selection_type = gtk_type_unique (gtk_window_get_type (), 
						  &mini_file_selection_info);
    }
  return mini_file_selection_type;
}

GtkWidget *
gtk_mini_file_selection_new (const gchar *title)
{
  GtkWidget *w = GTK_WIDGET (gtk_type_new (gtk_mini_file_selection_get_type ()));
  gtk_window_set_title (GTK_WINDOW (w), title);
  return w;
}
