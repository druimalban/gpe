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

#include "gtkminifilesel.h"

static GtkWindowClass *parent_class;

static void
gtk_mini_file_selection_init (GtkMiniFileSelection *fs)
{
}

static void
gtk_mini_file_selection_destroy (GtkObject *fs)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (fs);
}

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
gtk_mini_file_selection_show (GtkWidget *widget)
{
  GtkMiniFileSelection *fs;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MINI_FILE_SELECTION (widget));

  GTK_WIDGET_CLASS (parent_class)->show (widget);
  
  fs = GTK_MINI_FILE_SELECTION (widget);
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
      mini_file_selection_type = gtk_type_unique (gtk_hbox_get_type (), 
						  &mini_file_selection_info);
    }
  return mini_file_selection_type;
}

GtkWidget *
gtk_mini_file_selection_new (void)
{
  return GTK_WIDGET (gtk_type_new (gtk_mini_file_selection_get_type ()));
}



