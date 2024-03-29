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
#include <gpe/picturebutton.h>
#include <gpe/errorbox.h>
#endif


static struct gpe_icon my_icons[] = {
	{ "icon", PREFIX "/share/pixmaps/gpe-mixer.png" },
	{ "line" , "line"},
	{ "line1" , "line"},
	{ "cd" , "cd"},
	{ "bass" , "bass"},
	{ "vol" , "volume"},
	{ "unkn" , "unkn"},
	{ "treble" , "treble"},
	{ "synth" , "synth"},
	{ "speaker" , "speaker"},
	{ "phout" , "speaker"},
	{ "pcm" , "pcm"},
	{ "pcm2" , "pcm"},
	{ "mic" , "mic"},
	{ NULL, NULL }
};


/* GTK UI */
GtkWidget *window;				/* Main window */
GtkObject **MixerAdjuster;
gchar **channels = NULL;

/* helper for i8n */
#define _(x) gettext(x)

/* mixer handling */
char *mixer_labels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
char *mixer_names[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;
int devmask = 0;
int mixfd;

static gboolean 
channel_active(const gchar *name)
{
	int i = 0;
	
	if (!channels)
		return TRUE;
	
	while (channels[i])
		if (!strcmp(channels[i++], name))
			return TRUE;
		
	return FALSE;
}

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
		if (((1 << i) & devmask) && channel_active(mixer_names[i]))
			n++;
	}

	/* create table for all mixers */
	table=gtk_table_new(2, n, FALSE);
	gtk_container_add (GTK_CONTAINER (window), table);
	MixerAdjuster = (GtkObject **)g_malloc0(n * sizeof (GtkObject *));

	mixer_tips = gtk_tooltips_new ();

	/* fill table with mixers */
	n=0;
	for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
		if (((1 << i) & devmask) && channel_active(mixer_names[i])) {
			devnum=(int *)malloc(sizeof(int));
			*devnum=i;
			MixerAdjuster[n] = gtk_adjustment_new (101.0, 0.0, 101.0, 1.0, 10.0, 1.0);
			slider = gtk_vscale_new (GTK_ADJUSTMENT (MixerAdjuster[n]));
			gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
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
				gtk_table_attach(GTK_TABLE(table), w, n, n+1, 0, 1, 0, 0, 1, 0);
				gtk_widget_show(w);
				gtk_tooltips_set_tip(GTK_TOOLTIPS(mixer_tips),GTK_WIDGET(w),mixer_names[i],mixer_names[i]);
			} else {
				label=gtk_label_new(_(mixer_labels[i]));
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
		if (((1 << i) & devmask) && channel_active(mixer_names[n])){
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
int arg;
gchar *cstr = NULL;
	
#ifdef USE_GPE
	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);
#else
	gtk_set_locale ();
	gtk_init (&argc, &argv);
#endif

	/* command line parameters */
	while ((arg = getopt(argc, argv, "c:")) > 0) {
		switch (arg) {
			case 'c':
				cstr = g_strdup(optarg);
			break;
			default:
				fprintf(stderr, _("Unknown argument: %c\n"),arg);
		}
	}
	
	/* get list of selected channels */
	if (cstr) {
		int i = 0;
		gchar *j = cstr;
		gchar *p = cstr;
		
		while (j) {
			channels = realloc(channels, sizeof (gchar*) * (i + 1));
			if ((j = strstr(p, ","))) {
				channels[i] = malloc(j - p + 1);
				snprintf(channels[i], j - p + 1, "%s", p);
				p = j + 1;
			}
			else {
				channels = realloc(channels, sizeof (gchar*) * (i + 2));
				channels[i] = g_strdup(p);
				channels[i + 1] = NULL;
				j = NULL;
			}
			i++;
		}
		g_free(cstr);
		i = 0;
		while (channels[i])
			printf("DC: %s\n",channels[i++]);
	}
	
	/* mixer init */
	if ((mixfd = open("/dev/mixer", O_RDWR)) < 0) {
		gpe_perror_box("opening mixer");
		exit(1);
	}
	if (ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1) {
		gpe_perror_box("SOUND_MIXER_READ_DEVMASK");
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
