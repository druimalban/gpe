/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <gtk/gtk.h>
#include "gtksimplemenu.h"
#include <libintl.h>

#include "link-warning.h"

static guint my_signals[1];

struct _GtkSimpleMenu
{
  GtkOptionMenu optionmenu;
  GtkWidget *menu;
  guint nr;
};

struct _GtkSimpleMenuClass 
{
  GtkOptionMenuClass parent_class;
  void (* changed)      (GtkSimpleMenu *sm);
};

static GtkOptionMenuClass *parent_class = NULL;

static void
gtk_simple_menu_init (GtkSimpleMenu *sm)
{
  sm->menu = gtk_menu_new ();
  sm->nr = 0;

  gtk_option_menu_set_menu (GTK_OPTION_MENU (sm), sm->menu);
}

static void
gtk_simple_menu_show (GtkWidget *widget)
{
  GtkSimpleMenu *sm;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_SIMPLE_MENU (widget));

  GTK_WIDGET_CLASS (parent_class)->show (widget);

  sm = GTK_SIMPLE_MENU (widget);
  gtk_widget_show_all (sm->menu);
}

static void
gtk_simple_menu_class_init (GtkSimpleMenuClass * klass)
{
  GtkObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = gtk_type_class (gtk_option_menu_get_type ());
  oclass = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  widget_class->show = gtk_simple_menu_show;

  my_signals[0] = gtk_signal_new ("item-changed",
				  GTK_RUN_LAST,
				  GTK_CLASS_TYPE (oclass),				  
				  GTK_SIGNAL_OFFSET (GtkSimpleMenuClass, changed),
				  gtk_marshal_NONE__NONE,
				  GTK_TYPE_NONE, 0);

}

GtkType
gtk_simple_menu_get_type (void)
{
  static guint simple_menu_type = 0;

  if (! simple_menu_type)
    {
      static const GtkTypeInfo simple_menu_info =
      {
	"GtkSimpleMenu",
	sizeof (GtkSimpleMenu),
	sizeof (GtkSimpleMenuClass),
	(GtkClassInitFunc) gtk_simple_menu_class_init,
	(GtkObjectInitFunc) gtk_simple_menu_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      simple_menu_type = gtk_type_unique (gtk_option_menu_get_type (), 
					  &simple_menu_info);
    }
  return simple_menu_type;
}

GtkWidget *
gtk_simple_menu_new (void)
{
  return GTK_WIDGET (gtk_type_new (gtk_simple_menu_get_type ()));
}

guint
gtk_simple_menu_get_result (GtkSimpleMenu *sm)
{
  return gtk_option_menu_get_history (GTK_OPTION_MENU (sm));
}
link_warning(gtk_simple_menu_get_result, "warning: gtk_simple_menu_get_result is obsolescent; use gtk_option_menu_get_history instead.");

void
gtk_simple_menu_append_item (GtkSimpleMenu *sm, const gchar *s)
{
  GtkWidget *mi = gtk_menu_item_new_with_label (s);
  gtk_widget_show (mi);
  gtk_object_set_data (GTK_OBJECT (mi), "item", (gpointer)sm->nr++);
  gtk_menu_append (GTK_MENU (sm->menu), mi);
  if (sm->nr == 1)
    gtk_option_menu_set_history (GTK_OPTION_MENU (sm), 0);
}

void
gtk_simple_menu_flush (GtkSimpleMenu *sm)
{
  gtk_simple_menu_init (sm);
}
