/*
 * gpe-conf
 *
 * Copyright (C) 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE sound settings module, frontend.
 *
 */

/*
  ToDo
  - move mixer icons to gpe-icons
  - Filtering for different platforms.
  - Additional settings.
  - Update settings from system.
  - percentage to tooltips
  - replace wav test code by player
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>

#include "sound.h"
#include "alarmctrl.h"
#include "soundctrl.h"
#define MAX_CHANNELS 24

#warning needs gpe-mixer
struct gpe_icon mixer_icons[] = 
{
	{ "line" , PREFIX "/share/gpe-mixer/line.png"},
	{ "line1" , PREFIX "/share/gpe-mixer/line.png"},
	{ "cd" , PREFIX "/share/gpe-mixer/cd.png"},
	{ "bass" , PREFIX "/share/gpe-mixer/bass.png"},
	{ "vol" , PREFIX "/share/gpe-mixer/volume.png"},
	{ "treble" , PREFIX "/share/gpe-mixer/treble.png"},
	{ "synth" , PREFIX "/share/gpe-mixer/synth.png"},
	{ "speaker" , PREFIX "/share/gpe-mixer/speaker.png"},
	{ "phout" , PREFIX "/share/gpe-mixer/speaker.png"},
	{ "pcm" , PREFIX "/share/gpe-mixer/pcm.png"},
	{ "pcm2" , PREFIX "/share/gpe-mixer/pcm.png"},
	{ "mic" , PREFIX "/share/gpe-mixer/mic.png"},
	{ "unkn" , PREFIX "/share/gpe-mixer/unkn.png"},
	{NULL, NULL}
};


struct
{
	gint num_channels;
	GtkWidget *cMute;
	GtkWidget *slMain;
	GtkWidget *slAlarm;
	t_mixer *channels;
	GtkWidget *slider[MAX_CHANNELS];
}
self;

static gboolean level_changed = FALSE;


static gboolean
timeout_callback(void)
{
	if (level_changed)
	{
		play_sample(SOUND_SAMPLE);
		level_changed = FALSE;
	}
	return TRUE;
}


static void
do_change_alarm(GtkRange *range, gpointer data)
{
	set_alarm_level((int)gtk_range_get_value(range));
}


static void
on_alarm_toggled(GtkToggleButton *tb, gpointer userdata)
{
	int alarm_enable = gtk_toggle_button_get_active(tb);
	gtk_widget_set_sensitive(self.slAlarm, alarm_enable);
	set_alarm_enabled(alarm_enable);
}


static void
do_change_channel(GtkRange *range, gpointer data)
{
	int cn = (int) data;
	
	self.channels[cn].value = (int)gtk_range_get_value(range);
	set_volume (self.channels[cn].nr, self.channels[cn].value);
	level_changed = TRUE;
}

static void
on_mute_toggled(GtkToggleButton *tb, gpointer userdata)
{
	int i;
	gboolean set_state = (gboolean)userdata;
	
	if (gtk_toggle_button_get_active(tb))
	{
		for (i = 0; i < self.num_channels; i++)
		{
			gtk_widget_set_sensitive(self.slider[i], FALSE);
		}
		if (set_state)
			set_mute_status(TRUE);
	}
	else
	{
		if (set_state)
			set_mute_status(FALSE);
		for (i = 0; i < self.num_channels; i++)
		{
 			gtk_widget_set_sensitive(self.slider[i], TRUE);
			gtk_range_set_value(GTK_RANGE(self.slider[i]), 
		                        self.channels[i].value);
		}
	}		
}

/* gpe-conf applet interface */

void
Sound_Free_Objects ()
{
}

void
Sound_Save ()
{
	sound_save_settings();
	alarm_save_settings();
}

void
Sound_Restore ()
{
	sound_restore_settings();
	alarm_restore_settings();
}

GtkWidget *
Sound_Build_Objects (void)
{
	GtkWidget *table;
	GtkWidget *tw, *slider, *label;
	GtkWidget *image;
	GdkPixbuf *pbuf = NULL;
	gchar *ts = NULL;
	int i;
	gboolean mute;
	gchar *err = NULL;
	
	gpe_load_icons(mixer_icons);
	
	/* init devices */
	sound_init();
	alarm_init();
	self.num_channels = sound_get_channels(&self.channels);
	if (self.num_channels > MAX_CHANNELS)
		self.num_channels = MAX_CHANNELS;
	mute = get_mute_status();
	
	/* build gui */
	
	table = gtk_table_new(6, 3, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), gpe_get_border());
	gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
	
	/* audio settings section */ 
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("Audio Settings"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 0, 1, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	
	tw = gtk_check_button_new_with_label(_("Mute everything"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, 1, 2, GTK_FILL | GTK_EXPAND,
                     GTK_FILL, 0, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tw), mute);
	g_signal_connect_after(G_OBJECT(tw), "toggled", 
	                       G_CALLBACK(on_mute_toggled), (gpointer)TRUE);
	
	/* create widgets for channels */
	
	for (i = 0; i < self.num_channels; i++)
	{
		slider = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
		label  = gtk_label_new(self.channels[i].label);

		pbuf = gpe_try_find_icon (self.channels[i].name, &err);
		if (!pbuf)
			pbuf = gpe_try_find_icon ("unkn", &err);
		image = gtk_image_new_from_pixbuf(pbuf);
		
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
		gtk_range_set_value(GTK_RANGE(slider), 
		                    mute ? self.channels[i].backupval 
		                         : self.channels[i].value);
				
		g_signal_connect (G_OBJECT(slider), "value-changed",
		                  G_CALLBACK (do_change_channel), (gpointer)i);
		self.slider[i] = slider;
		
		gtk_table_attach(GTK_TABLE(table), image, 0, 1, i + 2, i + 3, 
		                 GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach(GTK_TABLE(table), label, 1, 2, i + 2, i + 3, 
		                 GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach(GTK_TABLE(table), slider, 2, 3, i + 2, i + 3, 
		                 GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	}
	
	on_mute_toggled(GTK_TOGGLE_BUTTON(tw), FALSE);
	
	/* alarm settings section */ 
	tw = gtk_label_new(NULL);
	ts = g_strdup_printf("<b>%s</b>",_("Alarm Settings"));
	gtk_label_set_markup(GTK_LABEL(tw),ts);
	gtk_misc_set_alignment(GTK_MISC(tw),0,0.5);
	g_free(ts);
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, i + 3, i + 4, 
	                 GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	
	tw = gtk_check_button_new_with_label(_("Enable Alarm"));
	gtk_table_attach(GTK_TABLE(table), tw, 0, 3, i + 4, i + 5, 
	                 GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tw), get_alarm_enabled());
	g_signal_connect_after(G_OBJECT(tw), "toggled", 
	                       G_CALLBACK(on_alarm_toggled), NULL);
	
	slider = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
	self.slAlarm = slider;
	gtk_widget_set_sensitive(slider, get_alarm_enabled());
	label  = gtk_label_new(_("Volume"));
	
	pbuf = gpe_try_find_icon ("alarm", &err);
	if (!pbuf)
		pbuf = gpe_try_find_icon ("unkn", &err);
	image = gtk_image_new_from_pixbuf(pbuf);
		
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
	gtk_range_set_value(GTK_RANGE(slider), get_alarm_level());
				
	g_signal_connect (G_OBJECT(slider), "value-changed",
	                  G_CALLBACK (do_change_alarm), NULL);
	
	gtk_table_attach(GTK_TABLE(table), image, 0, 1, i + 5, i + 6, 
	                 GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, i + 5, i + 6, 
	                 GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), slider, 2, 3, i + 5, i + 6, 
	                 GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
	
	g_timeout_add(1000, (GSourceFunc)timeout_callback, NULL);
		
	return table;
}
