/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "pixmaps.h"
#include "init.h"
#include "render.h"
#include "picturebutton.h"

static struct gpe_icon my_icons[] = {
  { "ok" },
  { "cancel" },
  { "media-prev" },
  { "media-next" },
  { "media-play" },
  { "media-pause" },
  { "media-stop" },
  { NULL, NULL }
};

#define _(x) gettext(x)

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEventExpose *event,
		   gpointer user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  guint width, height;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  
  gdk_gc_set_clip_rectangle (black_gc, &event->area);
  gdk_gc_set_clip_rectangle (gray_gc, &event->area);
  gdk_gc_set_clip_rectangle (white_gc, &event->area);
  
  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_draw_rectangle (drawable, white_gc, 1,
		      0, 0, width, height);

  gdk_draw_string (drawable, widget->style->font, black_gc, 
		   4, 16, "Track information here");

  gdk_gc_set_clip_rectangle (black_gc, NULL);
  gdk_gc_set_clip_rectangle (gray_gc, NULL);
  gdk_gc_set_clip_rectangle (white_gc, NULL);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  GdkPixbuf *p;
  GtkWidget *window, *w;
  GtkWidget *hbox, *hbox2;
  GdkColor col;
  GtkStyle *style;
  GtkWidget *prev, *play, *pause, *stop, *next;
  GtkWidget *rewind, *forward;
  GtkWidget *vbox, *vol_slider;
  GtkWidget *draw, *progress;
  GtkObject *vol_adjust;
  GtkWidget *hboxlist;
  GtkWidget *labellist, *buttonlist, *arrowlist;

  gchar *color = "gray80";
  Atom window_type_atom, window_type_toolbar_atom;
  Display *dpy;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (argc == 2)
    color = argv[1];

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  gdk_color_parse (color, &col);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (window);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  window_type_atom =
    XInternAtom (dpy, "_NET_WM_WINDOW_TYPE" , False);
  window_type_toolbar_atom =
    XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR",False);

  XChangeProperty (dpy, GDK_WINDOW_XWINDOW (window->window), 
		   window_type_atom, XA_ATOM, 32, PropModeReplace, 
		  (unsigned char *) &window_type_toolbar_atom, 1);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);

  style = gtk_style_copy (hbox->style);
  style->bg[0] = col;
  
  p = gpe_find_icon ("media-prev");
  w = gpe_render_icon (window->style, p);
  gtk_widget_show (w);
  prev = gtk_button_new ();
  gtk_widget_show (prev);
  gtk_widget_set_style (prev, style);
  gtk_container_add (GTK_CONTAINER (prev), w);
  gtk_box_pack_start (GTK_BOX (hbox), prev, TRUE, TRUE, 0);

  p = gpe_find_icon ("media-rew");
  w = gpe_render_icon (window->style, p);
  gtk_widget_show (w);
  rewind = gtk_button_new ();
  gtk_widget_show (rewind);
  gtk_widget_set_style (rewind, style);
  gtk_container_add (GTK_CONTAINER (rewind), w);
  gtk_box_pack_start (GTK_BOX (hbox), rewind, TRUE, TRUE, 0);

  p = gpe_find_icon ("media-play");
  w = gpe_render_icon (window->style, p);
  play = gtk_button_new ();
  gtk_widget_show (play);
  gtk_widget_set_style (play, style);
  gtk_container_add (GTK_CONTAINER (play), w);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), play, TRUE, TRUE, 0);

  p = gpe_find_icon ("media-pause");
  w = gpe_render_icon (window->style, p);
  pause = gtk_button_new ();
  gtk_widget_show (pause);
  gtk_widget_set_style (pause, style);
  gtk_container_add (GTK_CONTAINER (pause), w);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), pause, TRUE, TRUE, 0);

  p = gpe_find_icon ("media-stop");
  w = gpe_render_icon (window->style, p);
  stop = gtk_button_new ();
  gtk_widget_show (stop);
  gtk_widget_set_style (stop, style);
  gtk_container_add (GTK_CONTAINER (stop), w);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), stop, TRUE, TRUE, 0);

  p = gpe_find_icon ("media-fwd");
  w = gpe_render_icon (window->style, p);
  forward = gtk_button_new ();
  gtk_widget_show (forward);
  gtk_widget_set_style (forward, style);
  gtk_container_add (GTK_CONTAINER (forward), w);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), forward, TRUE, TRUE, 0);

  p = gpe_find_icon ("media-next");
  w = gpe_render_icon (window->style, p);
  next = gtk_button_new ();
  gtk_widget_show (next);
  gtk_widget_set_style (next, style);
  gtk_container_add (GTK_CONTAINER (next), w);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), next, TRUE, TRUE, 0);

  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  vol_adjust = gtk_adjustment_new (0.0, 0.0, 1.0, 0.1, 0.2, 0.2);
  vol_slider = gtk_vscale_new (GTK_ADJUSTMENT (vol_adjust));
  gtk_scale_set_draw_value (GTK_SCALE (vol_slider), FALSE);
  gtk_widget_show (vol_slider);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);

  draw = gtk_drawing_area_new ();
  gtk_widget_show (draw);
  gtk_signal_connect (GTK_OBJECT (draw), "expose-event",
		      draw_expose_event, NULL);

  hboxlist = gtk_hbox_new (FALSE, 0);
  labellist = gtk_label_new ("name of playlist here");
  buttonlist = gtk_button_new ();
  arrowlist = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (buttonlist), arrowlist);
  gtk_widget_show (arrowlist);
  gtk_widget_show (buttonlist);
  gtk_widget_show (labellist);
  gtk_widget_show (hboxlist);
  gtk_box_pack_end (GTK_BOX (hboxlist), buttonlist, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hboxlist), labellist, TRUE, TRUE, 0);
  
  gtk_box_pack_start (GTK_BOX (vbox), hboxlist, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), draw, TRUE, TRUE, 0);

  progress = gtk_progress_bar_new ();
  gtk_widget_show (progress);
  gtk_widget_set_usize (progress, -1, 8);
  gtk_box_pack_start (GTK_BOX (vbox), progress, FALSE, FALSE, 0);

  gtk_box_pack_end (GTK_BOX (hbox2), vol_slider, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox2), vbox, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), hbox2);

  gtk_widget_show (window);

  gtk_main ();

  exit (0);
}
