/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
#ifdef GST_NMF
#include <gst/gst.h>
#endif

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/render.h>
#include <gpe/picturebutton.h>
#include <gpe/errorbox.h>
#include <gpe/gtkminifilesel.h>

#include "frontend.h"

static struct gpe_icon my_icons[] = {
  { "close" },
  { "media-rew" },
  { "media-fwd" },
  { "media-prev" },
  { "media-next" },
  { "media-play" },
  { "media-pause" },
  { "media-stop" },
  { "media-eject" },
  { "media-playlist", "open" },
  { "new" },
  { "open" },
  { "delete" },
  { "save" },
  { "dir-up" },
  { "dir-closed" },
  { NULL, NULL }
};

/* GTK */
GtkWidget *window;				/* Main window */
GtkWidget *buttons_hbox;			/* Container for playback buttons */

#define _(x) gettext(x)

void
update_track_info (struct nmf_frontend *fe, struct playlist *p)
{
  if (p)
    {
      assert (p->type == ITEM_TYPE_TRACK);
      
      gtk_label_set_text (GTK_LABEL (fe->title_label), p->title);
      gtk_label_set_text (GTK_LABEL (fe->artist_label), p->data.track.artist);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (fe->title_label), "");
      gtk_label_set_text (GTK_LABEL (fe->artist_label), "");
    }
}

static void
eject_clicked (GtkWidget *w, struct nmf_frontend *fe)
{
  if (GTK_WIDGET_VISIBLE (fe->playlist_widget))
    gtk_widget_hide (fe->playlist_widget);
  else
    gtk_widget_show (fe->playlist_widget);
}

static void
play_clicked (GtkWidget *w, struct nmf_frontend *fe)
{
  if (!fe->playing)
    {
      struct player_status ps;
      player_play (fe->player);
      player_status (fe->player, &ps);
      update_track_info (fe, ps.item);
      fe->playing = TRUE;
    }
}

static void
stop_clicked (GtkWidget *w, struct nmf_frontend *fe)
{
  player_stop (fe->player);
  update_track_info (fe, NULL);
  fe->playing = FALSE;
}

static void
next_clicked (GtkWidget *w, struct nmf_frontend *fe)
{
  struct player_status ps;
  player_next_track (fe->player);
  player_status (fe->player, &ps);
  update_track_info (fe, ps.item);
}

static void
prev_clicked (GtkWidget *w, struct nmf_frontend *fe)
{
  struct player_status ps;
  player_prev_track (fe->player);
  player_status (fe->player, &ps);
  update_track_info (fe, ps.item);
}

static void
set_volume (GtkObject *o, player_t p)
{
  GtkAdjustment *a = GTK_ADJUSTMENT (o);
  int volume = 256 - (a->value * 256);
  player_set_volume (p, volume);
}

static void
update_time (struct nmf_frontend *fe, struct player_status *ps)
{
  char buf[32];

  unsigned long seconds = ps->time / GST_SECOND;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;

  snprintf (buf, sizeof (buf)-1, "%02d:%02d", minutes, seconds);
  buf[sizeof (buf)-1] = 0;
  gtk_label_set_text (GTK_LABEL (fe->time_label), buf);
 
#if 0 
  if (ps->total_time)
    {
      double d = (double)ps->time / (double)ps->total_time;
      gtk_adjustment_set_value (fe->progress_adjustment, d);
      gtk_widget_draw (fe->progress_slider, NULL);
    }
#endif
}

static gboolean
player_poll_func (struct nmf_frontend *fe)
{
  struct player_status ps;
  player_status (fe->player, &ps);
  if (ps.changed)
    update_track_info (fe, ps.item);
  update_time (fe, &ps);
  return TRUE;
}

int
main (int argc, char *argv[])
{
  /* GTK Widgets */
  GtkWidget *w;
  GtkWidget *hbox2, *hbox3, *hbox4;
  GtkWidget *vbox, *vbox2, *vbox3;
  GdkColor col;
  GtkStyle *style;
  GtkWidget *prev_button, *play_button, *pause_button, *stop_button, *next_button, *eject_button, *exit_button;
  GtkWidget *rewind_button, *forward_button;
  GtkWidget *vol_slider;
  GtkObject *vol_adjust;
  struct nmf_frontend *fe = g_malloc (sizeof (struct nmf_frontend));
  gint button_height = 20;
  gchar *color = "gray80";

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

#ifdef GST_NMF
  gst_init (&argc, &argv);
#endif

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  gdk_color_parse (color, &col);
  
  /* GTK Window stuff */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "NMF");
  gtk_widget_realize (window);
  gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_TOOLBAR);

  fe->player = player_new ();
  if (fe->player == NULL)
    {
      gpe_error_box (_("Unable to initialise player"));
      exit (1);
    }

  player_play (fe->player);

  fe->playlist = playlist_new_list ();
#ifndef GST_NMF
  player_error_handler (fe->player, gpe_error_box);
#endif
  g_timeout_add (250, (GSourceFunc)player_poll_func, fe);

  fe->playlist_widget = playlist_edit (fe, NULL);

  /* Destroy handler */

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (gtk_main_quit), NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 0);
  vbox3 = gtk_vbox_new (FALSE, 0);
  buttons_hbox = gtk_hbox_new (FALSE, 0);
  hbox2 = gtk_hbox_new (FALSE, 0);
  hbox3 = gtk_hbox_new (FALSE, 0);
  hbox4 = gtk_hbox_new (FALSE, 0);

  style = gtk_style_copy (buttons_hbox->style);
  style->bg[0] = col;
  
  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-prev"));
  gtk_widget_show (w);
  prev_button = gtk_button_new ();
  gtk_widget_show (prev_button);
  gtk_widget_set_style (prev_button, style);
  gtk_widget_set_usize (prev_button, -1, button_height);
  gtk_container_add (GTK_CONTAINER (prev_button), w);
  g_signal_connect (G_OBJECT (prev_button), "clicked",
		      G_CALLBACK (prev_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-rew"));
  gtk_widget_show (w);
  rewind_button = gtk_button_new ();
  gtk_widget_show (rewind_button);
  gtk_widget_set_style (rewind_button, style);
  gtk_widget_set_usize (rewind_button, -1, button_height);
  gtk_container_add (GTK_CONTAINER (rewind_button), w);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-play"));
  play_button = gtk_button_new ();
  gtk_widget_show (play_button);
  gtk_widget_set_style (play_button, style);
  gtk_container_add (GTK_CONTAINER (play_button), w);
  gtk_widget_set_usize (play_button, -1, button_height);
  gtk_widget_show (w);
  g_signal_connect (G_OBJECT (play_button), "clicked", 
		    G_CALLBACK (play_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-pause"));
  pause_button = gtk_button_new ();
  gtk_widget_show (pause_button);
  gtk_widget_set_style (pause_button, style);
  gtk_container_add (GTK_CONTAINER (pause_button), w);
  gtk_widget_set_usize (pause_button, -1, button_height);
  gtk_widget_show (w);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-stop"));
  stop_button = gtk_button_new ();
  gtk_widget_show (stop_button);
  gtk_widget_set_style (stop_button, style);
  gtk_container_add (GTK_CONTAINER (stop_button), w);
  gtk_widget_set_usize (stop_button, -1, button_height);
  gtk_widget_show (w);
  g_signal_connect (G_OBJECT (stop_button), "clicked", 
		    G_CALLBACK (stop_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-fwd"));
  forward_button = gtk_button_new ();
  gtk_widget_show (forward_button);
  gtk_widget_set_style (forward_button, style);
  gtk_widget_set_usize (forward_button, -1, button_height);
  gtk_container_add (GTK_CONTAINER (forward_button), w);
  gtk_widget_show (w);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-next"));
  next_button = gtk_button_new ();
  gtk_widget_show (next_button);
  gtk_widget_set_style (next_button, style);
  gtk_container_add (GTK_CONTAINER (next_button), w);
  gtk_widget_set_usize (next_button, -1, button_height);
  gtk_widget_show (w);
  g_signal_connect (G_OBJECT (next_button), "clicked",
		    G_CALLBACK (next_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-eject"));
  eject_button = gtk_button_new ();
  gtk_widget_show (eject_button);
  gtk_widget_set_style (eject_button, style);
  gtk_container_add (GTK_CONTAINER (eject_button), w);
  gtk_widget_set_usize (eject_button, -1, button_height);
  gtk_widget_show (w);
  g_signal_connect (G_OBJECT (eject_button), "clicked", 
		    G_CALLBACK (eject_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("close"));
  exit_button = gtk_button_new ();
  gtk_widget_show (exit_button);
  gtk_widget_set_style (exit_button, style);
  gtk_container_add (GTK_CONTAINER (exit_button), w);
  gtk_widget_show (w);
  g_signal_connect (G_OBJECT (exit_button), "clicked",
		    G_CALLBACK (gtk_main_quit), NULL);
  gtk_button_set_relief (GTK_BUTTON (exit_button), GTK_RELIEF_NONE);
  gtk_widget_set_usize (exit_button, 18, 18);

  fe->time_label = gtk_label_new ("00:00");
  fe->artist_label = gtk_label_new ("");
  fe->title_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (fe->artist_label), 0.0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (fe->title_label), 0.0, 0.5);

  vol_adjust = gtk_adjustment_new (0.0, 0.0, 1.0, 0.1, 0.2, 0.2);
  vol_slider = gtk_vscale_new (GTK_ADJUSTMENT (vol_adjust));
  gtk_scale_set_draw_value (GTK_SCALE (vol_slider), FALSE);
  gtk_widget_set_style (vol_slider, style);
  g_signal_connect (G_OBJECT (vol_adjust), "value-changed", 
		    G_CALLBACK (set_volume), fe->player);

  fe->progress_adjustment = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 
								  0.1, 0.2, 0.2);

  fe->progress_slider = gtk_hscale_new (GTK_ADJUSTMENT (fe->progress_adjustment));
  gtk_scale_set_draw_value (GTK_SCALE (fe->progress_slider), FALSE);
  gtk_widget_set_style (fe->progress_slider, style);
  gtk_widget_set_usize (fe->progress_slider, -1, 16);

  gtk_container_add (GTK_CONTAINER (window), hbox2);

  gtk_box_pack_start (GTK_BOX (hbox2), vol_slider, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), vbox, TRUE, TRUE, 0);

#if SLIDER_WORKS
  gtk_box_pack_start (GTK_BOX (hbox4), fe->progress_slider, TRUE, TRUE, 0);
#endif
  gtk_box_pack_start (GTK_BOX (hbox4), fe->time_label, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox3, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox4, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), buttons_hbox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox3), vbox2, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox3), vbox3, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), fe->title_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), fe->artist_label, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox3), exit_button, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (buttons_hbox), prev_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), play_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), pause_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), stop_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), eject_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), next_button, TRUE, TRUE, 0);

  gtk_widget_show (fe->time_label);
  gtk_widget_show (fe->artist_label);
  gtk_widget_show (fe->title_label);
#if SLIDER_WORKS
  gtk_widget_show (fe->progress_slider);
#endif
  gtk_widget_show (vol_slider);

  gtk_widget_show (vbox);
  gtk_widget_show (vbox2);
  gtk_widget_show (vbox3);
  gtk_widget_show (buttons_hbox);
  gtk_widget_show (hbox2);
  gtk_widget_show (hbox3);
  gtk_widget_show (hbox4);

  gtk_widget_show (window);

  gtk_widget_set_usize (fe->time_label, fe->time_label->allocation.width, -1);
  
  gtk_main ();

  exit (0);
}
