/*
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>
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
#include <stdlib.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/errorbox.h>

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "home", "home" },
  { "zoom_in", "gpe-julia/zoom_in" },
  { "zoom_out", "gpe-julia/zoom_out" },
  { "icon", PREFIX "/share/pixmaps/gpe-julia.png" },
  { "exit", "exit" },
  { NULL, NULL }
};

#define _(x) gettext(x)

GtkWidget *window = NULL, *statusbar = NULL;
GdkPixbuf *pixbuf = NULL;
GtkWidget *image_widget = NULL;
gint width = 320;
gint height = 320;
gint iterations = 20;

guchar *colour[3];
gint *xcoords = NULL;
gint *ycoords = NULL;
gdouble xcentre = 0;
gdouble ycentre = 0;
gdouble zoom = 1;
signed long juliax, juliay;
guint timeout = 0;
gint zoomx, zoomy;
gboolean already_zoomed = FALSE;

enum mode_type {
  mode_julia,
  mode_zoom_in,
  mode_zoom_out
};

enum mode_type current_mode = mode_julia;

enum fractal_type {
  fractal_julia,
  fractal_mandelbrot
};

enum fractal_type current_fractal = fractal_mandelbrot;

void statusbar_update (void)
{
  if (statusbar)
    {
      switch (current_mode)
	{
	case mode_julia:
	  gtk_label_set_label (GTK_LABEL (GTK_STATUSBAR (statusbar)->label), _("Click to draw a julia set"));
	  break;
	case mode_zoom_in:
	  gtk_label_set_label (GTK_LABEL (GTK_STATUSBAR (statusbar)->label), _("Click to zoom in"));
	  break;
	case mode_zoom_out:
	  gtk_label_set_label (GTK_LABEL (GTK_STATUSBAR (statusbar)->label), _("Click to zoom out"));
	  break;
	}
    }
}

void draw_init (void)
{
  gint i, x, y, colours = 20;

  if (zoom >= 200)
    {
      /* insufficient precision */
      zoom = 200;
    }

  iterations = 30 + pow (zoom, 1.2) * 10;
  if (iterations > 2000)
    iterations = 2000;

  if (xcoords)
    g_free (xcoords);

  if (ycoords)
    g_free (ycoords);

  for (i = 0; i < 3; i++)
    {
      if (colour[i])
	g_free (colour[i]);
      colour[i] = g_malloc (sizeof(gchar) * (iterations + 1));
    }

  xcoords = g_malloc (sizeof(gint) * width);
  ycoords = g_malloc (sizeof(gint) * height);

  colour[0][iterations] = 0;
  colour[1][iterations] = 0;
  colour[2][iterations] = 0;

  for (i = 0; i < iterations; i++)
    {
      colour[0][i] = ((256 - (i * 256) / colours)*2/3) % 256;
      colour[1][i] = 0;
      colour[2][i] = abs((i*256)/colours - 128)*2 % 256;
    }

  for (x = 0; x < width; x++)
	  xcoords[x] = ((x*(1<<15)/width) - (1<<14))/zoom + xcentre;

  for (y = 0; y < height; y++)
	  ycoords[y] = ((y*(1<<15)/height) - (1<<14))/zoom + ycentre;
}

void choose_julia (gint pixelx, gint pixely)
{
  juliax = ((pixelx*(1<<16)/width) - (1<<15))/zoom + xcentre;
  juliay = ((pixely*(1<<16)/height) - (1<<15))/zoom + ycentre;
}

void choose_centre (gint pixelx, gint pixely)
{
  xcentre = ((pixelx*(1<<16)/width) - (1<<15))/zoom + xcentre;
  ycentre = ((pixely*(1<<16)/height) - (1<<15))/zoom + ycentre;
}

void
draw_mandelbrot (void)
{
  signed int rowstride;
  guchar *image;
  signed long x, y;

  image = gdk_pixbuf_get_pixels (GDK_PIXBUF(pixbuf));
  rowstride = gdk_pixbuf_get_rowstride (GDK_PIXBUF(pixbuf));

  for (x = 0; x < width; x++)
    {
      for (y = 0; y < height; y++)
	{
       	  signed long ca, cb, a, b, newa, i;

	  ca = xcoords[x];
	  cb = ycoords[y];

	  a = ca;
	  b = cb;

	  for (i = 0; (i < iterations) && ((a * a + b * b) < (4 << 26)); i++)
	    {
	      newa = ((a * a) >> 13) - ((b * b) >> 13) + ca;
	      b = ((a * b) >> 12) + cb;
	      a = newa;
	    }

	  image[y*rowstride+x*3] = colour[0][i];
	  image[y*rowstride+x*3+1] = colour[1][i];
	  image[y*rowstride+x*3+2] = colour[2][i];
	}
    }
}

void
draw_julia (void)
{
  signed int rowstride;
  guchar *image;
  signed long x, y;

  image = gdk_pixbuf_get_pixels (GDK_PIXBUF(pixbuf));
  rowstride = gdk_pixbuf_get_rowstride (GDK_PIXBUF(pixbuf));

  for (x = 0; x < width; x++)
    {
      for (y = 0; y < height; y++)
	{
       	  signed long a, b, newa, i;

	  a = xcoords[x];
	  b = ycoords[y];

	  for (i = 0; (i < iterations) && ((a * a + b * b) < (4 << 26)); i++)
	    {
	      newa = ((a * a) >> 13) - ((b * b) >> 13) + juliax;
	      b = ((a * b) >> 12) + juliay;
	      a = newa;
	    }

	  image[y*rowstride+x*3] = colour[0][i];
	  image[y*rowstride+x*3+1] = colour[1][i];
	  image[y*rowstride+x*3+2] = colour[2][i];
	}
    }
}

void draw (void)
{
  if (current_fractal == fractal_mandelbrot)
    {
      draw_mandelbrot ();
    }
  else
    {
      draw_julia ();
    }
}

void redraw (void)
{
  gtk_widget_queue_draw (image_widget);
  gdk_window_process_updates (GTK_WIDGET(image_widget)->window, FALSE);
}

void zoom_in (gint pixelx, gint pixely, double zoomfactor)
{
  zoom = zoom * zoomfactor;
  choose_centre (pixelx, pixely);
  draw_init ();
  draw ();
  redraw ();
}

void zoom_out (gint pixelx, gint pixely, double zoomfactor)
{
  zoom = zoom / zoomfactor;
  if (zoom <= 1.01)
    {
      zoom = 1;
      xcentre = 0;
      ycentre = 0;
    }
  choose_centre (pixelx, pixely);
  draw_init ();
  draw ();
  redraw ();
}

void
motion_notify (GtkWidget *widget, GdkEventMotion *motion, gpointer *data)
{
  gint x, y;

  gdk_window_get_pointer (widget->window, &x, &y, NULL);

  if (current_mode == mode_julia)
    {
      choose_julia (x, y);
      current_fractal = fractal_julia;
      draw_julia ();
      redraw ();
      gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
    }
}

gboolean timeout_event (gpointer data)
{
  gint x, y;
  GtkWidget *widget = GTK_WIDGET (data);

  gdk_window_get_pointer (widget->window, &x, &y, NULL);

  already_zoomed = TRUE;

  if (current_mode == mode_zoom_in)
    {
      zoom_in (x, y, 1.02);
    }
  else if (current_mode == mode_zoom_out)
    {
      zoom_out (x, y, 1.02);
    }

#ifdef DEBUG
  printf ("zooming after timeout\n");
#endif

  return TRUE;
}

void
button_press_event (GtkWidget *widget, GdkEventButton *button, gpointer *data)
{
  if (current_mode == mode_julia)
    {
      gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
    }
  else if ((current_mode == mode_zoom_in) || (current_mode == mode_zoom_out))
    {
      zoomx = button->x;
      zoomy = button->y;
/*      choose_centre (zoomx, zoomy); */
      already_zoomed = FALSE;
      if (timeout == 0)
        timeout = g_timeout_add (100, timeout_event, widget);
#ifdef DEBUG
      printf ("adding timeout\n");
#endif
    }
}

void
button_release_event (GtkWidget *widget, GdkEventButton *button, gpointer *data)
{
  if (current_mode == mode_julia)
    {
      choose_julia (button->x, button->y);
      current_fractal = fractal_julia;
      draw_julia ();
      redraw ();
    }
  else if (!already_zoomed && (current_mode == mode_zoom_in))
    {
      zoom_in (button->x, button->y, 1.3);
    }
  else if (!(already_zoomed) && (current_mode == mode_zoom_out))
    {
      zoom_out (button->x, button->y, 1.3);
    }
  if ((current_mode == mode_zoom_in) || (current_mode == mode_zoom_out))
    {
      if (timeout != 0)
      {
        g_source_remove (timeout);
        timeout = 0;
#ifdef DEBUG
        printf ("removing timeout\n");
#endif
      }
    }
}

void
home                  (void)
{
  current_mode = mode_julia;
  current_fractal = fractal_mandelbrot;

  xcentre = 0;
  ycentre = 0;
  zoom = 1;
  juliax = 0;
  juliay = 0;

  draw_init ();
  draw_mandelbrot ();
  redraw ();

  statusbar_update ();
}

void
zoom_in_button                  (void)
{
  current_mode = mode_zoom_in;
  statusbar_update ();
}

void
zoom_out_button                  (void)
{
  current_mode = mode_zoom_out;
  statusbar_update ();
}

void
exit_button               (void)
{
  gtk_exit (0);
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
  GtkWidget *image_event_box, *toolbar, *icon, *vbox;
  GdkPixbuf *iconpixbuf;

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  if (gpe_application_init (&argc, &argv) == FALSE)
    {
      exit (1);
    }

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Julia");
  gpe_set_window_icon (window, "icon");
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (on_window_destroy),
                      NULL);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  iconpixbuf = gpe_find_icon ("home");
  if (!iconpixbuf)
    exit (1);
  icon = gtk_image_new_from_pixbuf (iconpixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("Home"),
			   _("Home"), _("Home"),
			   icon, home, NULL);

  iconpixbuf = gpe_find_icon ("zoom_in");
  if (!iconpixbuf)
    exit (1);
  icon = gtk_image_new_from_pixbuf (iconpixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("In"),
			   _("Zoom in"), _("Zoom in"),
			   icon, zoom_in_button, NULL);

  iconpixbuf = gpe_find_icon ("zoom_out");
  if (!iconpixbuf)
    exit (1);
  icon = gtk_image_new_from_pixbuf (iconpixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("Out"),
			   _("Zoom out"), _("Zoom out"),
			   icon, zoom_out_button, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  iconpixbuf = gpe_find_icon ("exit");
  if (!iconpixbuf)
    exit (1);
  icon = gtk_image_new_from_pixbuf (iconpixbuf);
  gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("Exit"),
			   _("Exit"), _("Exit"),
			   icon, exit_button, NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                           FALSE,
                           8,
                           width,
                           height );

  gdk_pixbuf_fill(pixbuf, 0);

  image_widget = gtk_image_new_from_pixbuf (pixbuf);
  image_event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (image_event_box), image_widget);
  gtk_container_add (GTK_CONTAINER (vbox), image_event_box);

  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-press-event", GTK_SIGNAL_FUNC (button_press_event), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-release-event", GTK_SIGNAL_FUNC (button_release_event), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "motion-notify-event", GTK_SIGNAL_FUNC (motion_notify), NULL);

  gtk_widget_add_events (GTK_WIDGET (image_event_box), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_HINT_MASK);

  //  gtk_widget_set_usize (image_widget, width, height*2);
  gtk_widget_set_size_request (image_widget, width, height);

  statusbar = gtk_statusbar_new ();
  statusbar_update ();
  gtk_container_add (GTK_CONTAINER (vbox), statusbar);

  gtk_widget_show_all (window);

  home ();

  gtk_main ();

  return 0;
}
