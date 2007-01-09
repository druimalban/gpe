/*
 * Copyright (C) 2003 Moray Allan <moray@sermisy.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <fcntl.h>
#include <libintl.h>
#include <locale.h>
#include <signal.h>
#include <math.h>

#include <gtk/gtk.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
//  { "icon", PREFIX "/share/pixmaps/gpe-handwriting.png" },
  { NULL, NULL }
};

#define _(x) gettext(x)

GtkWidget *window = NULL;
GdkPixmap *pixmap = NULL;
GtkWidget *image_widget = NULL;
gint width = 240;
gint height = 38*3/2;
gint ruleheight[3] = { 7*3/2, 16*3/2, 27*3/2 };
#define maxpoints 10240
gint points[maxpoints][2];
gint currentpoint = -1;
gboolean pendown = FALSE;

void redraw (void)
{
/*  gtk_image_set_from_pixmap (GTK_IMAGE(image_widget), pixmap, NULL);
  gdk_window_process_updates (GTK_WIDGET(image_widget)->window, FALSE); */
  gtk_widget_queue_draw (image_widget);
}

void
draw_init (void)
{
  gint i;
//  signed int rowstride;
//  guchar *image;

//  image = gdk_pixbuf_get_pixels (GDK_PIXBUF(pixbuf));
//  rowstride = gdk_pixbuf_get_rowstride (GDK_PIXBUF(pixbuf));

//	  image[y*rowstride+x*3] = colour[0][i];
//	  image[y*rowstride+x*3+1] = colour[1][i];
//	  image[y*rowstride+x*3+2] = colour[2][i];

  gdk_draw_rectangle (pixmap, window->style->white_gc, TRUE, 0, 0, width, height);

  for (i = 0; i < 3; i++)
    gdk_draw_line (pixmap, window->style->dark_gc[GTK_STATE_NORMAL], 0, ruleheight[i], width, ruleheight[i]);

  redraw ();
}

void
add_point (gint x, gint y)
{
  currentpoint++;
  if (currentpoint == 0)
    {
      points[0][0] = x;
      points[0][1] = y;
      gdk_draw_point (pixmap, window->style->black_gc, x, y);
    }
  else
    {
      points[currentpoint][0] = x;
      points[currentpoint][1] = y;
      gdk_draw_line (pixmap, window->style->black_gc,
                     points[currentpoint - 1][0], points[currentpoint - 1][1],
                     x, y);
    }
  redraw ();
}

void
motion_notify (GtkWidget *widget, GdkEventMotion *motion, gpointer *data)
{
  if (pendown)
  {
    add_point (motion->x, motion->y);
  }
}

void
button_press_event (GtkWidget *widget, GdkEventButton *button, gpointer *data)
{
  add_point (button->x, button->y);
  pendown = TRUE;
}

void
button_release_event (GtkWidget *widget, GdkEventButton *button, gpointer *data)
{
  if ((points[currentpoint][0] < 40) && (points[0][0] > 200))
  {
    draw_init ();
  }
  currentpoint = -1;
  pendown = FALSE;
}

void
on_window_destroy                  (GtkObject       *object,
				    gpointer         user_data)
{
  gtk_exit (0);
}

int
main(int argc, char *argv[])
{
  GtkWidget *image_event_box;
  GdkAtom window_type_atom, window_type_toolbar_atom;

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (gpe_application_init (&argc, &argv) == FALSE)
    {
      exit (1);
    }

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  window_type_atom =
    gdk_atom_intern ("_NET_WM_WINDOW_TYPE" , FALSE);
  window_type_toolbar_atom =
    gdk_atom_intern ("_NET_WM_WINDOW_TYPE_TOOLBAR", FALSE);

  gtk_widget_realize (window);

  gdk_property_change (window->window,
		       window_type_atom, (GdkAtom) XA_ATOM,
		       32, GDK_PROP_MODE_REPLACE,
		       (guchar *) &window_type_toolbar_atom,
		       1);

  gtk_window_set_title (GTK_WINDOW (window), "Handwriting");
//  gpe_set_window_icon (window, "icon");
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (on_window_destroy),
                      NULL);

  pixmap = gdk_pixmap_new (window->window, width, height, -1);

  image_widget = gtk_image_new_from_pixmap (pixmap, NULL);
  image_event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (image_event_box), image_widget);
  gtk_container_add (GTK_CONTAINER (window), image_event_box);

  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-press-event", GTK_SIGNAL_FUNC (button_press_event), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-release-event", GTK_SIGNAL_FUNC (button_release_event), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "motion-notify-event", GTK_SIGNAL_FUNC (motion_notify), NULL);

  gtk_widget_add_events (GTK_WIDGET (image_event_box), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

  //  gtk_widget_set_usize (image_widget, width, height*2);
  gtk_widget_set_size_request (image_widget, width, height);

  gtk_widget_show_all (window);

  draw_init ();

  gtk_main ();

  return 0;
}
