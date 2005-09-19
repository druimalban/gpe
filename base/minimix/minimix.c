/*
 * minimix -- volume control
 *
 * Copyright (c) 2002, 2003, 2004 Phil Blundell
 *               2005 Rene Wagner
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


GtkWidget *slider_window;
GtkWidget *window, *slider;
GtkWidget *icon;
gint icon_size;
gboolean configure_done = FALSE;
guint timeout;

int mixerfd;

#define _(_x)  gettext (_x)

#define SLIDER_HEIGHT	116
#define SLIDER_MAX	100

struct gpe_icon my_icons[] = 
  {
    { "stock_volume", PREFIX "/share/pixmaps/stock_volume.png" },
    { "stock_volume-mute", PREFIX "/share/pixmaps/stock_volume-mute.png" },
    { "stock_volume-0", PREFIX "/share/pixmaps/stock_volume-0.png" },
    { "stock_volume-min", PREFIX "/share/pixmaps/stock_volume-min.png" },
    { "stock_volume-med", PREFIX "/share/pixmaps/stock_volume-med.png" },
    { "stock_volume-max", PREFIX "/share/pixmaps/stock_volume-max.png" },
    { NULL, NULL }
  };

enum
{
  VOL_ICON_0,
  VOL_ICON_MIN,
  VOL_ICON_MED,
  VOL_ICON_MAX,
  NUM_VOL_ICONS
};

GdkPixbuf *vol_icons[NUM_VOL_ICONS];
GdkPixbuf *current_vol_icon;


void
refresh (int level)
{
  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (slider));

  if (level <= 0)
    {
      level = 0;
      current_vol_icon = vol_icons[0];
    }
  else
    {
      current_vol_icon = vol_icons[level < SLIDER_MAX ? level / (SLIDER_MAX / (NUM_VOL_ICONS - 1)) + 1 : VOL_ICON_MAX];
    }

  /* avoid loops... */
  if (level != gtk_adjustment_get_value (adj))
    gtk_adjustment_set_value (adj, level);
}

void
render_icon ()
{
  GdkBitmap *tmp;
  GdkPixbuf *dbuf;

  dbuf = gdk_pixbuf_scale_simple(current_vol_icon, icon_size, icon_size, GDK_INTERP_HYPER);
  gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &tmp, 12);
  gtk_widget_shape_combine_mask (GTK_WIDGET(window), NULL, 1, 0);
  gtk_widget_shape_combine_mask (GTK_WIDGET(window), tmp, 1, 0);
  gdk_bitmap_unref (tmp);
  gtk_image_set_from_pixbuf(GTK_IMAGE(icon), dbuf);
}

gboolean
timeout_cb (gpointer data)
{
  int level = read_volume_level ();
  GdkPixbuf *old_icon = current_vol_icon;

  refresh (level);
  if (configure_done && current_vol_icon != old_icon)
    render_icon ();

  return TRUE;
}

void
value_changed (GtkAdjustment *adj)
{
  int value;
  GdkPixbuf *old_icon = current_vol_icon;
  
  value = gtk_adjustment_get_value (adj);
  refresh (value);

  if (configure_done && current_vol_icon != old_icon)
    render_icon ();

  value |= (value << 8);
  ioctl (mixerfd, SOUND_MIXER_WRITE_VOLUME, &value);
}

int
read_volume_level (void)
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
  
  level = read_volume_level ();

  if (level != -1)
    gtk_adjustment_set_value (gtk_range_get_adjustment (GTK_RANGE (slider)), level);
  
  gtk_widget_show (slider_window);
  
  gdk_pointer_grab (slider_window->window, TRUE, GDK_BUTTON_PRESS_MASK, NULL, NULL, ev->time);

  return TRUE;
}

gboolean 
configure_event (GtkWidget *window, GdkEventConfigure *event, gpointer user_data)
{
  /* resize icon pixmap */
  if (event->type == GDK_CONFIGURE)
  {
    /* we use a square icon. take the window height and resize the width
       accordingly. */
    icon_size = event->height + 2;
    gtk_widget_set_size_request (GTK_WIDGET (window), icon_size, icon_size);

    render_icon ();
    configure_done = TRUE;
  }

  return FALSE;
}

int 
main (int argc, char **argv)
{
  GtkTooltips *tooltips;
  GtkAdjustment *adj;
  GtkWidget *box;
  GtkWidget *pluslabel, *minuslabel;

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
  vol_icons[0] = gpe_find_icon ("stock_volume-0");
  vol_icons[1] = gpe_find_icon ("stock_volume-min");
  vol_icons[2] = gpe_find_icon ("stock_volume-med");
  vol_icons[3] = gpe_find_icon ("stock_volume-max");

  /* will be updated when refresh is called. */
  current_vol_icon = vol_icons[0];

  mixerfd = open ("/dev/mixer", O_RDWR);
  if (mixerfd < 0)
    mixerfd = open ("/dev/sound/mixer", O_RDWR);
  if (mixerfd < 0)
    {
      gpe_perror_box ("Could not open /dev/sound/mixer");
      exit (1);
    }

  gtk_window_set_title (GTK_WINDOW (window), _("Volume control"));
  gpe_set_window_icon (GTK_WIDGET (window), "stock_volume");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the volume control.  Tap here and drag the slider to change the speaker volume."), NULL);

  icon = gtk_image_new_from_pixbuf (current_vol_icon);
  gtk_container_add (GTK_CONTAINER (window), icon);

  gtk_widget_show_all (window);

  gpe_system_tray_dock (window->window);

  slider_window = gtk_window_new (GTK_WINDOW_POPUP);
  slider = gtk_vscale_new_with_range (0, SLIDER_MAX, 1);

  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_widget_set_usize (slider_window, -1, SLIDER_HEIGHT);
  gtk_range_set_inverted (GTK_RANGE (slider), TRUE);

  adj = gtk_range_get_adjustment (GTK_RANGE (slider)); 
  g_signal_connect (G_OBJECT (adj), "value-changed", G_CALLBACK (value_changed), NULL);
  
  box = gtk_vbox_new (FALSE, 0);
  pluslabel = gtk_label_new ("+");
  minuslabel = gtk_label_new ("-");
  gtk_box_pack_start (GTK_BOX (box), pluslabel, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (box), minuslabel, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (box), slider, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (slider_window), box);

  g_signal_connect (G_OBJECT (window), "configure-event", G_CALLBACK (configure_event), NULL);
  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  g_signal_connect (G_OBJECT (slider_window), "button-press-event", G_CALLBACK (slider_clicked), NULL);

  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_add_events (slider_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  refresh (read_volume_level ());
  timeout = g_timeout_add (200,
                           (GSourceFunc) timeout_cb,
                           NULL);

  gtk_widget_show_all (box);

  gtk_main ();

  return 0;
}
