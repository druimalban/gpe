/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define MAX_SCALE 5

#define DEBUG	1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <libintl.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/gpe-iconlist.h>

#define WINDOW_NAME "Gallery"
#define _(_x) gettext (_x)

int MAXX[MAX_SCALE],MAXY[MAX_SCALE];

GtkWidget *window, *toolbar, *scrolled_window, *table;
GtkWidget *image_widget[3][3], *image_event_box;
GdkPixbuf *image_pixbuf[3][3];
gint image_x_number, image_y_number, current_scale;
GtkWidget *west, *east, *north, *south, *zoomin, *zoomout;
GtkAdjustment *h_adjust, *v_adjust;
gchar *map;
guint x_start, y_start, x_max, y_max;
double xadj_start, yadj_start;
gboolean confine_pointer_to_window;

struct gpe_icon my_icons[] = {
  { "left", "left" },
  { "right", "right" },
  { "up", "gpe-mapviewer/up" },
  { "down", "gpe-mapviewer/down" },
  { "zoom_in", "gpe-mapviewer/zoom_in" },
  { "zoom_out", "gpe-mapviewer/zoom_out" },
  { "icon", PREFIX "/share/pixmaps/gpe-mapviewer.png" },
  { "exit", },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

void
recenter()
{
  x_max = 3*(gdk_pixbuf_get_width (GDK_PIXBUF (image_pixbuf[1][1]))) - (scrolled_window->allocation.width - 4);
  y_max = 3*(gdk_pixbuf_get_height (GDK_PIXBUF (image_pixbuf[1][1]))) - (scrolled_window->allocation.height - 4);

  gtk_adjustment_set_value (h_adjust, 0.5*x_max);
  gtk_adjustment_set_value (v_adjust, 0.5*y_max);

}

void
pan_button_down (GtkWidget *w, GdkEventButton *b)
{
  x_start = b->x_root;
  y_start = b->y_root;

  xadj_start = gtk_adjustment_get_value (h_adjust);
  yadj_start = gtk_adjustment_get_value (v_adjust);

  x_max = 3*(gdk_pixbuf_get_width (GDK_PIXBUF (image_pixbuf[1][1]))) - (scrolled_window->allocation.width - 4);
  y_max = 3*(gdk_pixbuf_get_height (GDK_PIXBUF (image_pixbuf[1][1]))) - (scrolled_window->allocation.height - 4);

  gdk_pointer_grab (w->window, 
		    FALSE, GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK,
		    confine_pointer_to_window ? w->window : NULL, NULL, b->time);
}

void
button_down (GtkWidget *w, GdkEventButton *b)
{
  pan_button_down (w, b);
}

void
button_up (GtkWidget *w, GdkEventButton *b)
{
  gdk_pointer_ungrab (b->time);
}

void
pan (GtkWidget *w, GdkEventMotion *m)
{
  gint dx = m->x_root - x_start, dy = m->y_root - y_start;
  double x, y;

  x = xadj_start - dx;
  y = yadj_start - dy;

  if (x > x_max)
    x = x_max;

  if (y > y_max)
    y = y_max;

  gtk_adjustment_set_value (h_adjust, x);
  gtk_adjustment_set_value (v_adjust, y);

  gdk_window_get_pointer (w->window, NULL, NULL, NULL);
}

void
motion_notify (GtkWidget *w, GdkEventMotion *m, GdkPixbuf *pixbuf)
{
  pan (w, m);
}

void
get_scales (GtkWidget *widget, gchar *filename_root)
{
  int i, maxx, maxy, scale;
  
  FILE *fp=NULL;
  gchar *filename;
  
  filename = g_strdup_printf ("%s/.maps/%s.dat", getenv("HOME"), filename_root);
  if((fp=fopen(filename, "r"))==NULL)
  {
    printf("Cannot Open File %s\n", filename);
    exit(1);	
  }   
      
  for (i=0;i<MAX_SCALE;i++) {
  	fscanf(fp, "%d %d %d\n", &(scale), &(maxx), &(maxy));
	MAXX[scale]=maxx+1;
	MAXY[scale]=maxy+1;
  }
   
  fclose(fp);
  
}

void
show_image (GtkWidget *widget, gchar *filename_root)
{
  int i, j;
  gchar *filename;
  
  for (i=0;i<3;i++) {
  	for (j=0;j<3;j++) {
		filename = g_strdup_printf ("%s/.maps/%s-%d-%d-%d.gif", getenv("HOME"), filename_root, current_scale,
			image_x_number+i-1, image_y_number+j-1);
  		image_pixbuf[i][j] = gdk_pixbuf_new_from_file (filename, NULL);
  		if (DEBUG) printf("%s\n", filename);
  
  		gtk_image_set_from_pixbuf (GTK_IMAGE (image_widget[i][j]), image_pixbuf[i][j]);
	}
  }

  gtk_widget_show (toolbar);
  gtk_widget_show (image_event_box);
  
  for (i=0;i<3;i++) {
  	for (j=0;j<3;j++) gtk_widget_show (image_widget[i][j]);
  }
  
  gtk_widget_show (table);
  gtk_widget_show (scrolled_window);
  recenter();

}

void
recalc_sensitivy (GtkWidget *widget, gpointer user_data)
{
  int i;
  
  	for (i=0;i<MAX_SCALE;i++) {
		if ((image_x_number==(MAXX[i]-2) && current_scale==i)) {
				gtk_widget_set_sensitive (east, FALSE);
				break;
		}
		else	gtk_widget_set_sensitive (east, TRUE);
	}
	
	if (image_x_number==1) 
			gtk_widget_set_sensitive (west, FALSE);
	else	gtk_widget_set_sensitive (west, TRUE);
	
	for (i=0;i<MAX_SCALE;i++) {
		if ((image_y_number==(MAXY[i]-2) && current_scale==i)) {
				gtk_widget_set_sensitive (north, FALSE);
				break;
		}
		else	gtk_widget_set_sensitive (north, TRUE);
	}

	if (image_y_number==1) 
			gtk_widget_set_sensitive (south, FALSE);
	else	gtk_widget_set_sensitive (south, TRUE);
	
	if (current_scale==1) 
			gtk_widget_set_sensitive (zoomin, FALSE);
	else	gtk_widget_set_sensitive (zoomin, TRUE);
	
	if (current_scale==MAX_SCALE) 
			gtk_widget_set_sensitive (zoomout, FALSE);
	else	gtk_widget_set_sensitive (zoomout, TRUE);
				
}

void
go_east (GtkWidget *widget, gpointer user_data)
{
	if (GTK_WIDGET_SENSITIVE(east))
	{
		image_x_number++;
		recalc_sensitivy (NULL, NULL);
		show_image (NULL, map);
	}
	else if(DEBUG) printf("cannot do it! (east) = %d > %d\n", image_x_number, MAXX[current_scale]-2);
}

void
go_west (GtkWidget *widget, gpointer user_data)
{
	if (GTK_WIDGET_SENSITIVE(west))
	{
		image_x_number--;
		recalc_sensitivy (NULL, NULL);
		show_image (NULL, map);
	}
	else if(DEBUG) printf("cannot do it! (west) = %d <= %d\n", image_x_number, 1);
}

void
go_north (GtkWidget *widget, gpointer user_data)
{
	if (GTK_WIDGET_SENSITIVE(north))
	{
		image_y_number++;
		recalc_sensitivy (NULL, NULL);
		show_image (NULL, map);
	}
	else if(DEBUG) printf("cannot do it! (north) = %d > %d\n", image_x_number, MAXY[current_scale]-2);
}

void
go_south (GtkWidget *widget, gpointer user_data)
{
	if (GTK_WIDGET_SENSITIVE(south))
	{
		image_y_number--;
		recalc_sensitivy (NULL, NULL);
		show_image (NULL, map);
	}
	else if(DEBUG) printf("cannot do it! (south) = %d <= %d\n", image_x_number, 1);
}

void
zoom_out (GtkWidget *widget, gpointer user_data)
{
	if (GTK_WIDGET_SENSITIVE(zoomout))
	{
		int old_scale=current_scale;
		double scalerx, scalery;
		
		current_scale++;
		scalerx=(double)image_x_number/MAXX[old_scale];
		scalery=(double)image_y_number/MAXY[old_scale];
		if (DEBUG) printf("%lf %d\n", scalerx, old_scale);
		image_x_number = (int)(scalerx*MAXX[current_scale]);
		image_y_number = (int)(scalery*MAXY[current_scale]);
		if (DEBUG) printf("%d\n", current_scale);
		recalc_sensitivy (NULL, NULL);
		show_image (NULL, map);
	}
}

void
zoom_in (GtkWidget *widget, gpointer user_data)
{
	if (GTK_WIDGET_SENSITIVE(zoomin))
	{
		int old_scale=current_scale;
		double scalerx, scalery;
		
		current_scale--;
		scalerx=(double)(image_x_number)/MAXX[old_scale];
		scalery=(double)(image_y_number)/MAXY[old_scale];
		image_x_number = (int)(scalerx*MAXX[current_scale])+(int)(0.5*(double)MAXX[current_scale]/(double)MAXX[old_scale]);
		image_y_number = (int)(scalery*MAXY[current_scale])+(int)(0.5*(double)MAXY[current_scale]/(double)MAXY[old_scale]);
		if (DEBUG) printf("%lf %d %d %d %d %d\n", scalerx, (int)MAXX[current_scale]/MAXX[old_scale],
		MAXX[current_scale], MAXX[old_scale], MAXY[current_scale], MAXY[old_scale]);
		if (DEBUG) printf("%d\n", current_scale);
		recalc_sensitivy (NULL, NULL);
		show_image (NULL, map);
	}
}

int
main (int argc, char *argv[])
{
  int i, j;
  GtkWidget *vbox;
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkWidget *align;
  
  image_x_number=0;
  image_y_number=0;
  current_scale=5;
  
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");
  
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);
  gpe_set_window_icon (window, "icon");
  gtk_window_set_title (GTK_WINDOW (window), _("Map Viewer"));

  vbox = gtk_vbox_new (FALSE, 0);
  
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);  

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

  p = gpe_find_icon ("left");
  west = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("West"), _("Scroll West"), _("Tap here to Scroll West."),
			   west, GTK_SIGNAL_FUNC (go_west), NULL);
  gtk_widget_set_sensitive (west, FALSE);
  
  p = gpe_find_icon ("right");
  east = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("East"), _("Scroll East"), _("Tap here to Scroll East."),
			   east, GTK_SIGNAL_FUNC (go_east), NULL);
  gtk_widget_set_sensitive (east, FALSE);
  
  p = gpe_find_icon ("up");
  north = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("North"), _("Scroll North"), _("Tap here to Scroll North."),
			   north, GTK_SIGNAL_FUNC (go_north), NULL);
  gtk_widget_set_sensitive (north, FALSE);
  
  p = gpe_find_icon ("down");
  south = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("South"), _("Scroll South"), _("Tap here to Scroll South."),
			   south, GTK_SIGNAL_FUNC (go_south), NULL);
  gtk_widget_set_sensitive (south, FALSE);
  
  p = gpe_find_icon ("zoom_in");
  zoomin = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Zoom In"), _("Zoom In"), _("Tap here to Zoom In."),
			   zoomin, GTK_SIGNAL_FUNC (zoom_in), NULL);
  
  p = gpe_find_icon ("zoom_out");
  zoomout = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Zoom Out"), _("Zoom Out"), _("Tap here to Zoom Out."),
			   zoomout, GTK_SIGNAL_FUNC (zoom_out), NULL);
  gtk_widget_set_sensitive (zoomout, FALSE);
  
  p = gpe_find_icon ("exit");
  pw = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"), 
			   _("Exit"), _("Exit the program."), pw, 
			   GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

  h_adjust = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
  v_adjust = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window));

  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  
  table = gtk_table_new (3, 3, TRUE);
  gtk_table_set_row_spacings(GTK_TABLE (table), 0);
  gtk_table_set_row_spacings(GTK_TABLE (table), 0);
  
  for (i=0;i<3;i++) {
  	for (j=0;j<3;j++) 
		image_widget[i][j] = gtk_image_new_from_pixbuf (image_pixbuf[i][j]);
  }	
    
  for (i=0;i<3;i++) {
  	for (j=0;j<3;j++) 
		gtk_table_attach_defaults (GTK_TABLE (table), image_widget[i][2-j], i, i+1, j, j+1);
  }	
    
  image_event_box = gtk_event_box_new ();
  
  gtk_container_add (GTK_CONTAINER (image_event_box), table);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), image_event_box);
  
  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-press-event", GTK_SIGNAL_FUNC (button_down), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "button-release-event", GTK_SIGNAL_FUNC (button_up), NULL);
  gtk_signal_connect (GTK_OBJECT (image_event_box), "motion-notify-event", GTK_SIGNAL_FUNC (motion_notify), NULL);

  gtk_widget_add_events (GTK_WIDGET (image_event_box), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));

  recalc_sensitivy (NULL, NULL);
  
  gtk_widget_show_all (window);
  
  if (argc > 1) map=argv[1];
  else map = "map";
  
  get_scales (NULL, map);
  show_image (NULL, map);
  
  gtk_main();
  return 0;
}
