/*
 * minilite -- frontlight control
 *
 * Copyright 2002, 2003 Phil Blundell
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

#ifdef IPAQ
/* for iPAQ touchscreen */
#include <linux/h3600_ts.h>
#define TS_DEV "/dev/touchscreen/0raw"
#define H36XX 0
#define H38XX 1
#define H5XXX 2
#define H19XX 3
#define H22XX 4
char IpaqModel = -1;
#endif

#ifdef INTEGRAL
#define PROC_LIGHT "/proc/hw90200/backlight"
#endif

GtkWidget *slider_window;
GtkWidget *window, *slider;

#define _(_x)  gettext (_x)

#define SLIDER_HEIGHT	96

struct gpe_icon my_icons[] = {
  { "minilite", PREFIX "/share/pixmaps/minilite.png" },
  { NULL, NULL }
};

int window_open;
int light_fd=-1;	/* filedescriptor to backlight device */


#ifdef IPAQ
int
get_ipaq_model (void)
{
  int fd;
  char buf[16];

  fd = open ("/proc/hal/model", O_RDONLY);
  if (fd <= 0)
    return -1;
  if ( read( fd, buf, 16) < 4)
    return -1;
  close(fd);

  if ((strncmp (buf, "36", 2) == 0) || (strncmp (buf, "37", 2) == 0)) {
    g_print ("model is H36xx\n");
    IpaqModel = H36XX;
  } else if ((strncmp (buf, "38", 2) == 0) || (strncmp (buf, "39", 2) == 0)) {
    g_print ("model is H38xx\n");
    IpaqModel = H38XX;
  } else if ((strncmp (buf, "54", 2) == 0) || (strncmp (buf, "55", 2) == 0)) {
    g_print ("model is H5xxx\n");
    IpaqModel = H5XXX;
  } else
    IpaqModel = -1;

return IpaqModel;
}
#endif

int 
set_level (int level)
{
#ifdef IPAQ
  struct h3600_ts_backlight bl;

  bl.brightness = level;
  bl.power = (level > 0) ? FLITE_PWR_ON : FLITE_PWR_OFF;

  if (ioctl (light_fd, TS_SET_BACKLIGHT, &bl) != 0)
    return -1;
  else
    return level;
#else
#ifdef INTEGRAL
  FILE *f_light;
  
  f_light = fopen(PROC_LIGHT,"w");
  if (f_light >= 0)
  {
  	fprintf(f_light,"%i\n", level);
  	fclose(f_light);
	return level;
  }
  return -1;
#endif  
return level;
#endif
}

void
value_changed (GtkAdjustment *adj)
{
  int value;
  
  value = gtk_adjustment_get_value (adj);

  set_level (value);
}

int
read_old_level (void)
{
#ifdef IPAQ
  struct h3600_ts_backlight bl;

  if (ioctl (light_fd, TS_GET_BACKLIGHT, &bl) != 0)
    return -1;
  else
    return bl.brightness;
#else 
#ifdef INTEGRAL
  FILE *f_light;
  int level;
  
  f_light = fopen(PROC_LIGHT,"r");
  if (f_light >= 0)
  {
  	fscanf(f_light,"%i", &level);
  	fclose(f_light);
	return level;
  }
  return -1;
#endif
  return 0;
#endif
}

static void
slider_clicked (GtkWidget *w, GdkEventButton *ev)
{
  gdk_pointer_ungrab (ev->time);

  gtk_widget_hide (slider_window);
}

static void
clicked (GtkWidget *w, GdkEventButton *ev)
{
  int level;
  int x, y;

  gpe_get_win_position (GDK_WINDOW_XDISPLAY (w->window), GDK_WINDOW_XWINDOW (w->window), &x, &y);
  
  gtk_widget_set_uposition (GTK_WIDGET (slider_window), x, y - SLIDER_HEIGHT);
  
  level = read_old_level ();

  if (level != -1)
    gtk_adjustment_set_value(gtk_range_get_adjustment(GTK_RANGE(slider)), level);
  
  gtk_widget_show (slider_window);
  
  gdk_pointer_grab (slider_window->window, TRUE, GDK_BUTTON_PRESS_MASK, NULL, NULL, ev->time);
}

int 
main (int argc, char **argv)
{
  GdkBitmap *bitmap;
  GtkTooltips *tooltips;
  GtkWidget *icon;
  GtkAdjustment *adj;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

#ifdef IPAQ
  light_fd = open (TS_DEV, O_RDONLY);
  if (light_fd <= 0) {
    gpe_perror_box (_("Opening touchscreen device" TS_DEV));
    exit (1);
  }
  if (get_ipaq_model() == -1) {
    gpe_perror_box (_("Determining iPaq model"));
    exit (1);
  }
#endif

  window = gtk_plug_new (0);
  gtk_widget_set_usize (window, 16, 16);
  gtk_widget_realize (window);

  if (gpe_load_icons (my_icons) == FALSE) {
    gpe_error_box (_("Failed to load icons"));
    exit (1);
  }

  gtk_window_set_title (GTK_WINDOW (window), _("Frontlight control"));
  gpe_set_window_icon (GTK_WIDGET (window), "minilite");

  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("minilite"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 2, 0);
  gdk_bitmap_unref (bitmap);

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the frontlight control.  Tap here and drag the slider to change the brightness."), NULL);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon ("minilite"));

  gtk_container_add (GTK_CONTAINER (window), icon);

  gtk_widget_show_all (window);

  gpe_system_tray_dock (window->window);

  slider_window = gtk_window_new (GTK_WINDOW_POPUP);
#ifdef IPAQ
  switch (IpaqModel) {
    case H36XX:
      slider = gtk_vscale_new_with_range (0, 255, 1);
      break;
    case H38XX:
      slider = gtk_vscale_new_with_range (0, 64, 1);
      break;
    case H5XXX:
      slider = gtk_vscale_new_with_range (0, 255, 1);
      break;
    case H19XX:
      slider = gtk_vscale_new_with_range (0, 255, 1);
      break;
    case H22XX:
      slider = gtk_vscale_new_with_range (0, 255, 1);
      break;
    default:
      slider = gtk_vscale_new_with_range (0, 255, 1);
      break;
  }
#else
  slider = gtk_vscale_new_with_range (0, 255, 1);
#endif

  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_widget_set_usize (slider_window, -1, SLIDER_HEIGHT);
  gtk_range_set_inverted (GTK_RANGE (slider), TRUE);

  adj = gtk_range_get_adjustment (GTK_RANGE (slider)); 
  g_signal_connect (G_OBJECT (adj), "value-changed", G_CALLBACK (value_changed), NULL);
  
  gtk_container_add (GTK_CONTAINER (slider_window), slider);

  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  g_signal_connect (G_OBJECT (slider_window), "button-press-event", G_CALLBACK (slider_clicked), NULL);

  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_add_events (slider_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gtk_widget_show (slider);

  gtk_main ();

  exit (0);
}
