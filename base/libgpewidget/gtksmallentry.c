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

#define MIN_ENTRY_WIDTH  50
#define INNER_BORDER     2

static void
gtk_small_entry_size_request (GtkWidget      *widget,
			      GtkRequisition *requisition)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_ENTRY (widget));
  g_return_if_fail (requisition != NULL);

  requisition->width = MIN_ENTRY_WIDTH + (widget->style->klass->xthickness + INNER_BORDER) * 2;
  requisition->height = (widget->style->font->ascent +
			 widget->style->font->descent +
			 (widget->style->klass->ythickness + INNER_BORDER) * 2);
}

static void
gtk_small_entry_init (GtkEntry *entry)
{
}

static void
gtk_small_entry_class_init (GtkEntryClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass*) class;

  widget_class->size_request = gtk_small_entry_size_request;
}

guint
gtk_small_entry_get_type (void)
{
  static guint type = 0;

  if (! type)
    {
      static const GtkTypeInfo info =
      {
	"GtkSmallEntry",
	sizeof (GtkEntry),
	sizeof (GtkEntryClass),
	(GtkClassInitFunc) gtk_small_entry_class_init,
	(GtkObjectInitFunc) gtk_small_entry_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      type = gtk_type_unique (gtk_entry_get_type (), 
			      &info);
    }
  return type;
}

GtkWidget *
gtk_entry_new (void)
{
  return GTK_WIDGET (gtk_type_new (gtk_small_entry_get_type ()));
}
