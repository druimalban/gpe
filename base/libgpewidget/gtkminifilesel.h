/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GTK_MINI_FILE_SEL_H
#define GTK_MINI_FILE_SEL_H

#include <gtk/gtk.h>

#define GTK_TYPE_MINI_FILE_SELECTION         (gtk_mini_file_selection_get_type ())
#define GTK_MINI_FILE_SELECTION(obj)         (GTK_CHECK_CAST ((obj), GTK_TYPE_MINI_FILE_SELECTION, GtkMiniFileSelection))
#define GTK_MINI_FILE_SELECTION__CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_MINI_FILE_SELECTION, GtkMiniFileSelectionClass))
#define GTK_IS_MINI_FILE_SELECTION(obj)      (GTK_CHECK_TYPE ((obj), GTK_TYPE_MINI_FILE_SELECTION))
#define GTK_IS_MINI_FILE_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MINI_FILE_SELECTION))

struct _GtkMiniFileSelection
{
  GtkWindow window;
  GtkWidget *clist;
  GtkWidget *cancel_button;
  GtkWidget *ok_button;
  GtkWidget *entry;
  GtkWidget *option;

  char *directory;
};

typedef struct _GtkMiniFileSelection	   GtkMiniFileSelection;

struct _GtkMiniFileSelectionClass 
{
  GtkWindowClass parent_class;
  void (* completed)  (GtkMiniFileSelection *fs);
};

typedef struct _GtkMiniFileSelectionClass  GtkMiniFileSelectionClass;

GtkType		gtk_mini_file_selection_get_type	   (void);
GtkWidget      *gtk_mini_file_selection_new (const gchar *title);
gchar * gtk_mini_file_selection_get_filename (GtkMiniFileSelection *fs);

#endif
