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
	{ "line" , PREFIX "/share/gpe-mixer/line.png"},
	{ "line1" , PREFIX "/share/gpe-mixer/line.png"},
	{ "cd" , PREFIX "/share/gpe-mixer/cd.png"},
	{ "bass" , PREFIX "/share/gpe-mixer/bass.png"},
	{ "vol" , PREFIX "/share/gpe-mixer/volume.png"},
	{ "unkn" , PREFIX "/share/gpe-mixer/unkn.png"},
	{ "treble" , PREFIX "/share/gpe-mixer/treble.png"},
	{ "synth" , PREFIX "/share/gpe-mixer/synth.png"},
	{ "speaker" , PREFIX "/share/gpe-mixer/speaker.png"},
	{ "pcm" , PREFIX "/share/gpe-mixer/pcm.png"},
	{ "pcm2" , PREFIX "/share/gpe-mixer/pcm.png"},
	{ "mic" , PREFIX "/share/gpe-mixer/mic.png"},
	{ NULL, NULL }
};


/* GTK UI */
GtkWidget *window;				/* Main window */
GtkStyle *style;
GtkObject **MixerAdjuster;

/* helper for i8n */
#define _(x) gettext(x)

/* mixer handling */
char *mixer_labels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
char *mixer_names[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;
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
  	gpe_perror_box("WRITE_MIXER");
}


void create_mixers(GtkWidget *table)
{
int i,n=0;
int mval;
GtkWidget *slider;
GtkWidget *label;
GtkWidget *w;
int *devnum;
GtkTooltips *mixer_tips;
gchar *err = NULL;
GdkPixbuf *pbuf = NULL;
	
	/* count mixers */
	for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
		if ((1 << i) & devmask)
			n++;
	}

	/* create table for all mixers */
	table=gtk_table_new(2, n, FALSE);
	style = gtk_style_copy (table->style);
	gtk_container_add (GTK_CONTAINER (window), table);
	MixerAdjuster = (GtkObject **)malloc(n * sizeof (GtkObject));

	mixer_tips = gtk_tooltips_new ();

	/* fill table with mixers */
	n=0;
	for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
		if ((1 << i) & devmask) {
			devnum=(int *)malloc(sizeof(int));
			*devnum=i;
			MixerAdjuster[n] = gtk_adjustment_new (101.0, 0.0, 101.0, 1.0, 10.0, 1.0);
			slider = gtk_vscale_new (GTK_ADJUSTMENT (MixerAdjuster[n]));
			gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
			gtk_widget_set_style (slider, style);
			gtk_table_attach(GTK_TABLE(table), slider, n, n+1, 1, 2, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
			gtk_tooltips_set_tip(GTK_TOOLTIPS(mixer_tips),GTK_WIDGET(slider),mixer_names[i],mixer_names[i]);
			if (ioctl(mixfd, MIXER_READ(i),&mval)== -1) /* get initial setting */
				gpe_perror_box("MIXER_READ");
			else
				gtk_adjustment_set_value(GTK_ADJUSTMENT(MixerAdjuster[n]), 100 - (gdouble)(mval & 0x7f));
			g_signal_connect (G_OBJECT (MixerAdjuster[n]), "value-changed",G_CALLBACK (set_volume), devnum);
			gtk_widget_show(slider);
			pbuf = gpe_try_find_icon (mixer_names[i], &err);
			if (!pbuf)
				pbuf = gpe_try_find_icon ("unkn", &err);
			w = gtk_image_new_from_pixbuf (pbuf);
			if (w) {
				gtk_widget_set_style (w, style);
				gtk_table_attach(GTK_TABLE(table), w, n, n+1, 0, 1, 0, 0, 1, 0);
				gtk_widget_show(w);
				gtk_tooltips_set_tip(GTK_TOOLTIPS(mixer_tips),GTK_WIDGET(w),mixer_names[i],mixer_names[i]);
			} else {
				label=gtk_label_new(_(mixer_labels[i]));
				gtk_widget_set_style (label, style);
				gtk_table_attach(GTK_TABLE(table), label, n, n+1, 0, 1, 0, 0, 1, 0);
				gtk_widget_show(label);
				gtk_tooltips_set_tip(GTK_TOOLTIPS(mixer_tips),GTK_WIDGET(label),mixer_names[i],mixer_names[i]);
			}
			n++;
		}
	}
	gtk_widget_show (table);
}


gboolean mixer_idle (gpointer data)
{
unsigned int i, n;
int mval;

	n=0;
	for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
		if ((1 << i) & devmask) {
			if (ioctl(mixfd, MIXER_READ(i),&mval)== -1) /* get setting */
				perror("MIXER_READ");
			else
				gtk_adjustment_set_value(GTK_ADJUSTMENT(MixerAdjuster[n]), 100 - (gdouble)(mval & 0x7f));
			n++;
		}
	}

return TRUE;
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
		gpe_perror_box("opening mixer");
		exit(1);
	}
	if (ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1) {
		gpe_perror_box("SOUND_MIXER_READ_DEVMASK");
		exit(1);
	}
	if (ioctl(mixfd, SOUND_MIXER_READ_RECMASK, &recmask) == -1) {
		gpe_perror_box("SOUND_MIXER_READ_RECMASK");
		exit(1);
	}
	if (ioctl(mixfd, SOUND_MIXER_READ_RECSRC, &recsrc) == -1) {
		gpe_perror_box("SOUND_MIXER_READ_RECSRC");
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

	/* Create the table of mixer widgets */
	table = NULL;
	create_mixers(table);

#ifdef USE_GPE
	gpe_set_window_icon (window, "icon");
#endif

	/* Auto update mixer settings */
	gtk_timeout_add(500 /*ms*/, mixer_idle, NULL);

	gtk_widget_show (window);

	gtk_main ();

return (0);
}
