/*
 * minimix -- volume control
 *
 * Copyright (c) 2002, 2003, 2004 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/tray.h>
#include <gpe/popup.h>

#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "config.h"

GtkWidget *slider_window;
GtkWidget *window, *slider;
GtkWidget *icon;
GdkBitmap *bitmap;

int mixerfd;

#define _(_x)  gettext (_x)

#define SLIDER_HEIGHT	96

struct gpe_icon my_icons[] = 
  {
    { "minimix", PREFIX "/share/pixmaps/minimix.png" },
    { NULL, NULL }
  };

void
value_changed (GtkAdjustment *adj)
{
  int value;
  
  value = gtk_adjustment_get_value (adj);
  value |= (value << 8);

  ioctl (mixerfd, SOUND_MIXER_WRITE_VOLUME, &value);
}

int
read_old_level (void)
{
  int orig_vol;

  if (ioctl (mixerfd, SOUND_MIXER_READ_VOLUME, &orig_vol) < 0)
    return 0;

  orig_vol &= 0x7f;

  return orig_vol;
}

static gboolean
slider_clicked (GtkWidget *w, GdkEventButton *ev)
{
  gdk_pointer_ungrab (ev->time);

  gtk_widget_hide (slider_window);

  return FALSE;
}

static gboolean
clicked (GtkWidget *w, GdkEventButton *ev)
{
  int level;
  int x, y;

  gpe_get_win_position (GDK_WINDOW_XDISPLAY (w->window), GDK_WINDOW_XWINDOW (w->window), &x, &y);
  
  gtk_widget_set_uposition (GTK_WIDGET (slider_window), x, y - SLIDER_HEIGHT);
  
  level = read_old_level ();

  if (level != -1)
    gtk_adjustment_set_value (gtk_range_get_adjustment (GTK_RANGE (slider)), level);
  
  gtk_widget_show (slider_window);
  
  gdk_pointer_grab (slider_window->window, TRUE, GDK_BUTTON_PRESS_MASK, NULL, NULL, ev->time);

  return TRUE;
}

gboolean 
configure_event (GtkWidget *window, GdkEventConfigure *event)
{
  GdkPixbuf *buf;
  int xoff, yoff;

  xoff = (event->width - 30) / 2;
  yoff = (event->height - 32) / 2;

  gtk_widget_shape_combine_mask (window, bitmap, xoff, yoff);
 
  return FALSE;
}

int 
main (int argc, char **argv)
{
  GtkTooltips *tooltips;
  GtkAdjustment *adj;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  window = gtk_plug_new (0);
  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
  gtk_widget_set_usize (window, 30, 32);
  gtk_widget_realize (window);

  if (gpe_load_icons (my_icons) == FALSE) 
    {
      gpe_error_box (_("Failed to load icons"));
      exit (1);
    }

  mixerfd = open ("/dev/mixer", O_RDWR);
  if (mixerfd < 0)
    mixerfd = open ("/dev/sound/mixer", O_RDWR);
  if (mixerfd < 0)
    {
      gpe_perror_box ("Could not open /dev/sound/mixer");
      exit (1);
    }

  gtk_window_set_title (GTK_WINDOW (window), _("Volume control"));
  gpe_set_window_icon (GTK_WIDGET (window), "minimix");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the volume control.  Tap here and drag the slider to change the speaker volume."), NULL);

  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("minimix"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 0, 0);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon ("minimix"));
  gtk_container_add (GTK_CONTAINER (window), icon);

  gtk_widget_show_all (window);

  gpe_system_tray_dock (window->window);

  slider_window = gtk_window_new (GTK_WINDOW_POPUP);
  slider = gtk_vscale_new_with_range (0, 100, 1);

  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_widget_set_usize (slider_window, -1, SLIDER_HEIGHT);
  gtk_range_set_inverted (GTK_RANGE (slider), TRUE);

  adj = gtk_range_get_adjustment (GTK_RANGE (slider)); 
  g_signal_connect (G_OBJECT (adj), "value-changed", G_CALLBACK (value_changed), NULL);
  
  gtk_container_add (GTK_CONTAINER (slider_window), slider);

  g_signal_connect (G_OBJECT (window), "configure-event", G_CALLBACK (configure_event), NULL);
  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  g_signal_connect (G_OBJECT (slider_window), "button-press-event", G_CALLBACK (slider_clicked), NULL);

  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_add_events (slider_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gtk_widget_show (slider);

  gtk_main ();

  exit (0);
}
