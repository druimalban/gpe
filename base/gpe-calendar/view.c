/* view.c - View widget implementation.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include "view.h"

#define _(x) gettext(x)

static void gtk_view_class_init (gpointer klass, gpointer klass_data);
static void gtk_view_init (GTypeInstance *instance, gpointer klass);
static void gtk_view_dispose (GObject *obj);
static void gtk_view_finalize (GObject *object);

static GtkWidgetClass *parent_class;

GType
gtk_view_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (GtkViewClass),
	NULL,
	NULL,
	gtk_view_class_init,
	NULL,
	NULL,
	sizeof (struct _GtkView),
	0,
	gtk_view_init
      };

      type = g_type_register_static (gtk_vbox_get_type (),
				     "GtkView", &info, 0);
    }

  return type;
}

static void
gtk_view_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkViewClass *view_class;

  parent_class = g_type_class_ref (gtk_vbox_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gtk_view_finalize;
  object_class->dispose = gtk_view_dispose;

  widget_class = (GtkWidgetClass *) klass;

  view_class = (GtkViewClass *) klass;
  view_class->time_changed_signal
    = g_signal_new ("time-changed",
		    G_OBJECT_CLASS_TYPE (object_class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (GtkViewClass, time_changed),
		    NULL,
		    NULL,
		    g_cclosure_marshal_VOID__ULONG,
		    G_TYPE_NONE,
		    1,
		    G_TYPE_POINTER);

}

static void
gtk_view_init (GTypeInstance *instance, gpointer klass)
{
  GtkView *view = GTK_VIEW (instance);

  view->date = (time_t) -1;
}

static void
gtk_view_dispose (GObject *obj)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gtk_view_finalize (GObject *object)
{
  GtkView *view;

  g_return_if_fail (object);
  g_return_if_fail (GTK_IS_VIEW (object));

  view = (GtkView *) object;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

time_t
gtk_view_get_time (GtkView *view)
{
  return view->date;
}

void
gtk_view_set_time (GtkView *view, time_t time)
{
  GtkViewClass *view_class = (GtkViewClass *) G_OBJECT_GET_CLASS (view);

  time_t old = view->date;
  view->date = time;

  g_return_if_fail (view_class->set_time);
  view_class->set_time (view, old);

  GValue args[2];
  GValue rv;

  args[0].g_type = 0;
  g_value_init (&args[0], G_TYPE_FROM_INSTANCE (G_OBJECT (view)));
  g_value_set_instance (&args[0], view);

  args[1].g_type = 0;
  g_value_init (&args[1], G_TYPE_ULONG);
  g_value_set_ulong (&args[1], view->date);

  g_signal_emitv (args, view_class->time_changed_signal, 0, &rv);
}

void
gtk_view_reload_events (GtkView *view)
{
  GtkViewClass *view_class = (GtkViewClass *) G_OBJECT_GET_CLASS (view);

  g_return_if_fail (view_class->reload_events);
  view_class->reload_events (view);
}
