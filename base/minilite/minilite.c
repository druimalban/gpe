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

#include <linux/ioctl.h>
#include <sys/ioctl.h>

typedef enum
{
	P_NONE,
	P_IPAQ,
	P_ZAURUS,
	P_CORGI,
	P_INTEGRAL,
	P_SIMPAD,
	P_SIMPAD_NEW
}t_platform;


/* for iPAQ touchscreen */
#define IOC_H3600_TS_MAGIC  'f'
#define TS_GET_BACKLIGHT        _IOR(IOC_H3600_TS_MAGIC, 20, struct h3600_ts_backlight)
#define TS_SET_BACKLIGHT        _IOW(IOC_H3600_TS_MAGIC, 20, struct h3600_ts_backlight)

enum flite_pwr {
        FLITE_PWR_OFF = 0,
        FLITE_PWR_ON  = 1
};

struct h3600_ts_backlight {
        enum flite_pwr power;          /* 0 = off, 1 = on */
        unsigned char  brightness;     /* 0 - 255         */
};

#define TS_DEV "/dev/touchscreen/0raw"
#define H36XX 0
#define H38XX 1
#define H5XXX 2
#define H19XX 3
#define H22XX 4
char IpaqModel = -1;

/* for Integral 90200 */
#define PROC_LIGHT "/proc/hw90200/backlight"

/* for Sharp Corgi */
#define CORGI_FL "/proc/driver/fl/corgi-bl"

/* other Zauri */
#define ZAURUS_FL "/dev/fl"
#define FL_IOCTL_STEP_CONTRAST    100

/* Simpad, a little bit nasty - PWM reg. is set directly */

#define SIMPAD_BACKLIGHT_REG		"/proc/driver/mq200/registers/PWM_CONTROL"
#define SIMPAD_BACKLIGHT_MASK		0x00a10044

/* Simpad, new interface */
#define SIMPAD_BACKLIGHT_REG_NEW	"/proc/driver/mq200/backlight"


GtkWidget *slider_window;
GtkWidget *window, *slider;
GtkWidget *icon;

#define _(_x)  gettext (_x)

#define SLIDER_HEIGHT	96

struct gpe_icon my_icons[] = {
  { "minilite", PREFIX "/share/pixmaps/minilite.png" },
  { NULL, NULL }
};

int window_open;
t_platform platform = P_NONE;

static t_platform
detect_platform(void)
{
	if (!access(TS_DEV,R_OK))
		return P_IPAQ;
	if (!access(PROC_LIGHT,R_OK))
		return P_INTEGRAL;
	if (!access(CORGI_FL,R_OK))
		return P_CORGI;
	if (!access(ZAURUS_FL,R_OK))
		return P_ZAURUS;
	if (!access(SIMPAD_BACKLIGHT_REG_NEW,R_OK)) /* preserve order */
		return P_SIMPAD_NEW;
	if (!access(SIMPAD_BACKLIGHT_REG,R_OK))
		return P_SIMPAD;
	return P_NONE;
}


int 
simpad_new_set_level(int level)
{
  FILE *f_light;
  
  f_light = fopen(SIMPAD_BACKLIGHT_REG_NEW,"w");
  if (f_light != NULL)
  {
    fprintf(f_light,"%d\n", level);
  	fclose(f_light);
	return level;
  }
  else
	  return -1;
}

int 
simpad_new_get_level(void)
{
  FILE *f_light;
  int level;
  
  f_light = fopen(SIMPAD_BACKLIGHT_REG_NEW,"r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"%d", &level);
  	fclose(f_light);
	return level;
  }
  return -1;
}  


int 
simpad_set_level(int level)
{
  int val;
  FILE *f_light;
  
  f_light = fopen(SIMPAD_BACKLIGHT_REG,"w");
  if (f_light != NULL)
  {
	val = 255 - level;
	val = val << 8;
	val += (int)SIMPAD_BACKLIGHT_MASK;
    fprintf(f_light,"0x%x\n", val);
  	fclose(f_light);
	return level;
  }
  else
	  return -1;
}

int 
simpad_get_level(void)
{
  FILE *f_light;
  int level;
  
  f_light = fopen(SIMPAD_BACKLIGHT_REG,"r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"0x%x", &level);
  	fclose(f_light);
	level -= (int)SIMPAD_BACKLIGHT_MASK;
	level = level >> 8;
	return 255 - level;
  }
  return -1;
}  

int 
corgi_set_level(int level)
{
	int fd, val, len, res = -1;
	gchar buf[20];
	
	if ((fd = open(CORGI_FL, O_WRONLY)) >= 0)
	{
		val = (level == 1) ? 1 : level * ( 17.0 / 255.0 );
		len = snprintf(buf, 20, "0x%x\n", val);
		res = (write (fd, buf, len) >= 0);
		close (fd);
    }
	return ((res < 0) ? -1 : level);
}

int 
corgi_get_level(void)
{
	int val, res;
	FILE *fd;
	
	if ((fd = fopen(CORGI_FL, O_RDONLY)) != NULL)
	{
		res = fscanf(fd, "0x%x", &val);
		fclose (fd);
		if (res) 
			return ((val * 255) / 17);
    }
	return (-1);
}

int 
zaurus_set_level(int level)
{
	int fd, val, res = -1;
	
	if ((fd = open(ZAURUS_FL, O_WRONLY)) >= 0)
	{
		val = (level * 4 + 127) / 255;
		if (level && !val)
			val = 1;
            res = ioctl(fd, FL_IOCTL_STEP_CONTRAST, val);
		close (fd);
    }
	return (res != 0) ? -1 : level;
}

int 
zaurus_get_level(void)
{
/*	int fd, val;
	
	if ((fd = open(ZAURUS_FL, O_WRONLY )) >= 0 )
	{
		res = ioctl(fd, FL_IOCTL_GET_STEP_CONTRAST, &val);
		close (fd);
    }
	return (res != 0) ? -1 : ((val*255-127) / 4);
*/
	return 25;	
}


int 
ipaq_set_level(int level)
{
	int light_fd;
	struct h3600_ts_backlight bl;

	light_fd = open (TS_DEV, O_RDONLY);
	if (light_fd > 0) {
  		bl.brightness = level;
  		bl.power = (level > 0) ? FLITE_PWR_ON : FLITE_PWR_OFF;
 		if (ioctl (light_fd, TS_SET_BACKLIGHT, &bl) != 0)
		{
			close(light_fd);
    		return -1;
		}
  		else
		{
			close(light_fd);
    		return level;
		}
	}
	return -1;
}

int 
ipaq_get_level(void)
{
	int light_fd;
	struct h3600_ts_backlight bl;

	light_fd = open (TS_DEV, O_RDONLY);
	if (light_fd > 0) {
 		if (ioctl (light_fd, TS_GET_BACKLIGHT, &bl) != 0)
		{
			close(light_fd);
    		return -1;
		}
  		else
		{
			close(light_fd);
    		return bl.brightness;
		}
	}
	return -1;
}

int 
integral_set_level(int level)
{
  FILE *f_light;
  
  f_light = fopen(PROC_LIGHT,"w");
  if (f_light != NULL)
  {
  	fprintf(f_light,"%i\n", level);
  	fclose(f_light);
	return level;
  }
  else
	  return -1;
}

int 
integral_get_level(void)
{
  FILE *f_light;
  int level;
  
  f_light = fopen(PROC_LIGHT,"r");
  if (f_light != NULL)
  {
  	fscanf(f_light,"%i", &level);
  	fclose(f_light);
	return level;
  }
  return -1;
}  
  
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

int 
set_level (int level)
{
	if (level < 0)
		level = 0;
	if (level > 255)
		level = 255;
	switch (platform)
	{
	case P_IPAQ:
		return ipaq_set_level(level);
	break;
	case P_ZAURUS:
		return zaurus_set_level(level);
	break;
	case P_CORGI:
		return corgi_set_level(level);
	break;
	case P_INTEGRAL:
		return integral_set_level(level);
	break;
	case P_SIMPAD_NEW:
		return simpad_new_set_level(level);
	break;
	case P_SIMPAD:
		return simpad_set_level(level);
	break;
	default:
		return 0;
	break;
	}
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
	switch (platform)
	{
	case P_IPAQ:
		return ipaq_get_level();
	break;
	case P_ZAURUS:
		return zaurus_get_level();
	break;
	case P_CORGI:
		return corgi_get_level();
	break;
	case P_INTEGRAL:
		return integral_get_level();
	break;
	case P_SIMPAD_NEW:
		return simpad_new_get_level();
	break;
	case P_SIMPAD:
		return simpad_get_level();
	break;
	default:
		return 0;
	break;
	}
  return 0;
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


/* handle resizing */
gboolean 
external_event(GtkWindow *window, GdkEventConfigure *event, gpointer user_data)
{
  GdkBitmap *bitmap;
  GdkPixbuf *sbuf, *dbuf;
  int size;

  if (event->type == GDK_CONFIGURE)
  {
    size = (event->width > event->height) ? event->height : event->width;
    sbuf = gpe_find_icon ("minilite");
    dbuf = gdk_pixbuf_scale_simple(sbuf,size, size,GDK_INTERP_HYPER);
    gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &bitmap, 128);
    gtk_widget_shape_combine_mask (GTK_WIDGET(window), NULL, 1, 0);
    gtk_widget_shape_combine_mask (GTK_WIDGET(window), bitmap, 1, 0);
    gdk_bitmap_unref (bitmap);
    gtk_image_set_from_pixbuf(GTK_IMAGE(icon),dbuf);
  }
  return FALSE;
}


int 
main (int argc, char **argv)
{
  GdkBitmap *bitmap;
  GtkTooltips *tooltips;
  GtkAdjustment *adj;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  platform = detect_platform();
  
  if (platform == P_IPAQ)
	  if (get_ipaq_model() == -1) {
      gpe_perror_box (_("Determining iPaq model"));
      exit (1);
    }

  window = gtk_plug_new (0);
  gtk_window_set_resizable(GTK_WINDOW(window),TRUE);
  gtk_widget_set_usize (window, 16, 16);
   /* this makes it scale up to 48pixels child size if possible */	
  gtk_widget_set_size_request(window,50,50);
  gtk_widget_realize (window);

  if (gpe_load_icons (my_icons) == FALSE) {
    gpe_error_box (_("Failed to load icons"));
    exit (1);
  }

  gtk_window_set_title (GTK_WINDOW (window), _("Frontlight control"));
  gpe_set_window_icon (GTK_WIDGET (window), "minilite");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the frontlight control.  Tap here and drag the slider to change the brightness."), NULL);

  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("minilite"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 0, 0);
  gdk_bitmap_unref (bitmap);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon ("minilite"));
  gtk_misc_set_alignment(GTK_MISC(icon),0.0,0.5);
  gtk_container_add (GTK_CONTAINER (window), icon);

  gtk_widget_show_all (window);

  gpe_system_tray_dock (window->window);

  slider_window = gtk_window_new (GTK_WINDOW_POPUP);
  if (platform == P_IPAQ)
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
  else
  	slider = gtk_vscale_new_with_range (0, 255, 1);

  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_widget_set_usize (slider_window, -1, SLIDER_HEIGHT);
  gtk_range_set_inverted (GTK_RANGE (slider), TRUE);

  adj = gtk_range_get_adjustment (GTK_RANGE (slider)); 
  g_signal_connect (G_OBJECT (adj), "value-changed", G_CALLBACK (value_changed), NULL);
  
  gtk_container_add (GTK_CONTAINER (slider_window), slider);

  g_signal_connect (G_OBJECT (window), "configure-event", G_CALLBACK (external_event), NULL);
  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  g_signal_connect (G_OBJECT (slider_window), "button-press-event", G_CALLBACK (slider_clicked), NULL);

  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
gtk_widget_add_events (window, GDK_ALL_EVENTS_MASK);
  gtk_widget_add_events (slider_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gtk_widget_show (slider);

  gtk_main ();

  exit (0);
}
