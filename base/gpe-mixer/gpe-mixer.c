/*
 * Copyright (C) 2003 Nils Faerber <nils@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>    
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <libintl.h>

/* OSS sound specific */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <gtk/gtk.h>

/* GPE specific */
#ifdef USE_GPE
#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/render.h>
#include <gpe/picturebutton.h>
#include <gpe/errorbox.h>
#endif

static struct gpe_icon my_icons[] = {
	{ "icon", PREFIX "/share/pixmaps/gpe-mixer.png" },
	{ NULL, NULL }
};


/* GTK UI */
GtkWidget *window;				/* Main window */
GtkStyle *style;

/* helper for i8n */
#define _(x) gettext(x)

/* mixer handling */
char *names[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;
int devmask = 0, recmask = 0, recsrc = 0;
int mixfd;

static void
set_volume (GtkObject *o, void *t)
{
  GtkAdjustment *a = GTK_ADJUSTMENT (o);
  int volume = 100 - (int)a->value;

#ifdef DEBUG
  g_print("vol=(%d)%d\n", *(int *)t, volume);
#endif
  volume |= (volume << 8);
  if (ioctl(mixfd, MIXER_WRITE(*(int *)t), &volume) == -1)
  	perror("WRITE_MIXER");
}

void create_mixers(GtkWidget *table, int mdev)
{
int i,n=0;
int mval;
GtkWidget *slider;
GtkObject *adjuster;
GtkWidget *label;
int *devnum;

	/* count mixers */
	for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
		if ((1 << i) & devmask)
			n++;
	}

	/* create table for all mixers */
	table=gtk_table_new(2,n,FALSE);
	style = gtk_style_copy (table->style);
	gtk_container_add (GTK_CONTAINER (window), table);

	/* fill table with mixers */
	for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
		if ((1 << i) & devmask) {
			devnum=(int *)malloc(sizeof(int));
			*devnum=i;
			adjuster = gtk_adjustment_new (101.0, 0.0, 101.0, 1.0, 10.0, 1.0);
			slider = gtk_vscale_new (GTK_ADJUSTMENT (adjuster));
			gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
			gtk_widget_set_style (slider, style);
			gtk_table_attach(GTK_TABLE(table), slider, n,n+1,1,2,(GtkAttachOptions) (GTK_FILL),(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
			if (ioctl(mdev, MIXER_READ(i),&mval)== -1) /* get initial setting */
				perror("MIXER_READ");
			else
				gtk_adjustment_set_value(GTK_ADJUSTMENT(adjuster), 100 - (gdouble)(mval & 0x7f));
			g_signal_connect (G_OBJECT (adjuster), "value-changed",G_CALLBACK (set_volume), devnum);
			gtk_widget_show(slider);
			label=gtk_label_new(_(names[i]));
			gtk_widget_set_style (label, style);
			gtk_table_attach(GTK_TABLE(table), label, n,n+1,0,1, (GtkAttachOptions) (0),(GtkAttachOptions) (0), 1, 0);
			gtk_widget_show(label);
			n++;
		}
	}
	gtk_widget_show (table);
}


int
main (int argc, char *argv[])
{
/* GTK Widgets */
GtkWidget *table;
GdkColor col;
gchar *color = "gray80";

#ifdef USE_GPE
	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);
#else
	gtk_set_locale ();
	gtk_init (&argc, &argv);
#endif

	gdk_color_parse (color, &col);
  
	/* mixer init */
	if ((mixfd = open("/dev/mixer", O_RDWR)) < 0) {
		perror("opening mixer");
		exit(1);
	}
	if (ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1) {
		perror("SOUND_MIXER_READ_DEVMASK");
		exit(1);
	}
	if (ioctl(mixfd, SOUND_MIXER_READ_RECMASK, &recmask) == -1) {
		perror("SOUND_MIXER_READ_RECMASK");
		exit(1);
	}
	if (ioctl(mixfd, SOUND_MIXER_READ_RECSRC, &recsrc) == -1) {
		perror("SOUND_MIXER_READ_RECSRC");
		exit(1);
	}

	/* GTK Window stuff */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Mixer");
	gtk_window_set_default_size(GTK_WINDOW(window), 240, 100);
	gtk_widget_realize (window);
	gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_TOOLBAR);

	/* Destroy handler */
	g_signal_connect (G_OBJECT (window), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);

	table = NULL;
	create_mixers(table, mixfd);

#ifdef USE_GPE
	gpe_set_window_icon (window, "icon");
#endif

	gtk_widget_show (window);

	gtk_main ();

return (0);
}
