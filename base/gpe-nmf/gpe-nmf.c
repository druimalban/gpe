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

#include <gtk/gtk.h>

#include "pixmaps.h"
#include "init.h"
#include "render.h"
#include "picturebutton.h"

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "media-play", "media-play" },
  { "media-stop", "media-stop" },
  { "media-rew", "media-rew" },
  { "media-fwd", "media-fwd" },
  { NULL, NULL }
};

#define _(x) gettext(x)

int
main (int argc, char *argv[])
{
  GdkPixbuf *p;
  GtkWidget *window, *w;
  GtkWidget *hbox;
  GdkColor col;
  GtkStyle *style;
  gchar *color = "gray80";

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

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);

  style = gtk_style_copy (hbox->style);
  style->bg[0] = col;
  gtk_widget_set_style (window, style);
  
  p = gpe_find_icon ("media-rew");
  w = gpe_render_icon (window->style, p);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, FALSE, 8);

  p = gpe_find_icon ("media-stop");
  w = gpe_render_icon (window->style, p);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, FALSE, 8);

  p = gpe_find_icon ("media-play");
  w = gpe_render_icon (window->style, p);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, FALSE, 8);

  p = gpe_find_icon ("media-fwd");
  w = gpe_render_icon (window->style, p);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, FALSE, 8);

  gtk_container_add (GTK_CONTAINER (window), hbox);

  gtk_widget_show (window);

  gtk_main ();

  exit (0);
}
