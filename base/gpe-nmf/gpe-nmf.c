/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 * Changes (C) 2004 Chris Lord <cwiiis@handhelds.org> :
 * - Enable progress slider
 * - Fix time display
 * - Add seeking
 * Several UI updates and fixes 
 *      (c) 2005 Florian Boor <florian@kernelconcepts.de>
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
#include <gst/gst.h>

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>

#include "frontend.h"

static struct gpe_icon my_icons[] = {
  { "media-rew" },
  { "media-fwd" },
  { "media-prev" },
  { "media-next" },
  { "media-play" },
  { "media-pause" },
  { "media-stop" },
  { "media-eject" },
  { "media-playlist", "open" },
  { "icon", PREFIX "/share/pixmaps/gpe-nmf.png" },
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
      if (player_play (fe->player))
        {
          player_status (fe->player, &ps);
          update_track_info (fe, ps.item);
          fe->playing = TRUE;
        }
      else
        {
          GtkWidget *message;
          message = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_WARNING, 
                                           GTK_BUTTONS_CLOSE,
                                           _("Unable to start playback."));
          gtk_dialog_run(GTK_DIALOG(message));
          gtk_widget_destroy(message);  
          fe->player->state = PLAYER_STATE_NULL;
          fe->playing = FALSE;
        }
    }
    else
    	if( fe->player->state == PLAYER_STATE_PAUSED )
    		player_play (fe->player);
  gtk_range_set_value(GTK_RANGE(fe->volume_slider),
                      player_get_volume(fe->player));
}

static void
pause_clicked (GtkWidget *w, struct nmf_frontend *fe)
{
    if( fe->player->state != PLAYER_STATE_PAUSED )
      {
	    if(fe->playing)
		  player_pause (fe->player);
      }
    else
    	player_play (fe->player);
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

static gboolean
set_position (GtkObject *o, GtkScrollType scroll, gdouble value, 
              struct nmf_frontend *fe)
{
  GtkAdjustment *a = GTK_ADJUSTMENT (GTK_RANGE(o)->adjustment);
  fe->position = a->value;
  return FALSE;
}

static void
update_time (struct nmf_frontend *fe, struct player_status *ps)
{
  char buf[32];
  
  snprintf (buf, sizeof (buf)-1, "%02d:%02d", 
            (int)ps->time / 60, (int)ps->time % 60 );
  buf[sizeof (buf)-1] = 0;
  gtk_label_set_text (GTK_LABEL (fe->time_label), buf);
  
  if (ps->total_time)
    {
      if (fe->position >= 0)
        {
          player_seek(fe->player, fe->position);
          fe->position = -1;            
        }
        else
        {
          double d = ps->time / ps->total_time;
          gtk_adjustment_set_value (fe->progress_adjustment, d);
          gtk_widget_draw (fe->progress_slider, NULL);
        }
    }
  if( ps->state == PLAYER_STATE_NEXT_TRACK )
    {
      update_track_info (fe, ps->item);
    }
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

static void 
toggle_shuffle (GtkToggleButton *tb, struct nmf_frontend *fe)
{
  player_set_shuffle (fe->player, gtk_toggle_button_get_active (tb));
}

static void
toggle_loop (GtkToggleButton *tb, struct nmf_frontend *fe)
{
  player_set_loop (fe->player, gtk_toggle_button_get_active (tb));
}

int
main (int argc, char *argv[])
{
  /* GTK Widgets */
  GtkWidget *w, *table;
  GtkWidget *prev_button, *play_button, *pause_button, *stop_button, *next_button, *eject_button, *exit_button;
  GtkWidget *rewind_button, *forward_button;
  GtkWidget *shuffle_button, *loop_button;
  GtkWidget *vol_slider;
  GtkObject *vol_adjust;
  struct nmf_frontend *fe = g_malloc0 (sizeof (struct nmf_frontend));
  gint button_height = 20;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  gst_init (&argc, &argv);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  /* GTK Window stuff */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "NMF");
  gtk_widget_realize (window);
  gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_TOOLBAR);

  fe->position = -1;
  fe->current_path = NULL;
  fe->player = player_new ();
  if (fe->player == NULL)
    {
      gpe_error_box (_("Unable to initialise player"));
      exit (1);
    }

  player_play (fe->player);

  fe->playlist = playlist_new_list ();
  g_timeout_add (500, (GSourceFunc)player_poll_func, fe);

  fe->playlist_widget = playlist_edit (fe, NULL);

  /* Destroy handler */

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  buttons_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(buttons_hbox), gpe_get_border());

  table = gtk_table_new(5, 5, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table), 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), gpe_get_boxspacing());

  /* Main controls */
  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-prev"));
  prev_button = gtk_button_new ();
  gtk_widget_set_usize (prev_button, -1, button_height);
  gtk_container_add (GTK_CONTAINER (prev_button), w);
  g_signal_connect (G_OBJECT (prev_button), "clicked",
                    G_CALLBACK (prev_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-rew"));
  rewind_button = gtk_button_new ();
  gtk_widget_set_usize (rewind_button, -1, button_height);
  gtk_container_add (GTK_CONTAINER (rewind_button), w);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-play"));
  play_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (play_button), w);
  gtk_widget_set_usize (play_button, -1, button_height);
  g_signal_connect (G_OBJECT (play_button), "clicked", 
                    G_CALLBACK (play_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-pause"));
  pause_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (pause_button), w);
  gtk_widget_set_usize (pause_button, -1, button_height);
  g_signal_connect (G_OBJECT (pause_button), "clicked", 
                    G_CALLBACK (pause_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-stop"));
  stop_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (stop_button), w);
  gtk_widget_set_usize (stop_button, -1, button_height);
  g_signal_connect (G_OBJECT (stop_button), "clicked", 
                    G_CALLBACK (stop_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-fwd"));
  forward_button = gtk_button_new ();
  gtk_widget_set_usize (forward_button, -1, button_height);
  gtk_container_add (GTK_CONTAINER (forward_button), w);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-next"));
  next_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (next_button), w);
  gtk_widget_set_usize (next_button, -1, button_height);
  g_signal_connect (G_OBJECT (next_button), "clicked",
                    G_CALLBACK (next_clicked), fe);

  w = gtk_image_new_from_pixbuf (gpe_find_icon ("media-eject"));
  eject_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (eject_button), w);
  gtk_widget_set_usize (eject_button, -1, button_height);
  g_signal_connect (G_OBJECT (eject_button), "clicked", 
                    G_CALLBACK (eject_clicked), fe);

  /* Shuffle / Loop */
  shuffle_button = gtk_check_button_new_with_label (_("Shuffle"));
  g_signal_connect (G_OBJECT (shuffle_button), "toggled",
                    G_CALLBACK (toggle_shuffle), fe);

  loop_button = gtk_check_button_new_with_label ("Loop");
  g_signal_connect (G_OBJECT (loop_button), "toggled",
                    G_CALLBACK (toggle_loop), fe);

  /* Itty-bitty close button */
  w = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_SMALL_TOOLBAR);
  exit_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (exit_button), w);
  g_signal_connect (G_OBJECT (exit_button), "clicked",
		    G_CALLBACK (gtk_main_quit), NULL);
  gtk_button_set_relief (GTK_BUTTON (exit_button), GTK_RELIEF_NONE);
  gtk_widget_set_usize (exit_button, 18, 18);

  fe->time_label = gtk_label_new ("00:00");
  fe->artist_label = gtk_label_new (_("Artist"));
  fe->title_label = gtk_label_new (_("Title"));
  gtk_misc_set_alignment (GTK_MISC (fe->artist_label), 0.0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (fe->title_label), 0.0, 0.5);

  vol_adjust = gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.02, 0.02);
  vol_slider = gtk_vscale_new (GTK_ADJUSTMENT (vol_adjust));
  fe->volume_slider = vol_slider;
  gtk_scale_set_draw_value (GTK_SCALE (vol_slider), FALSE);
  g_signal_connect (G_OBJECT (vol_adjust), "value-changed", 
                    G_CALLBACK (set_volume), fe->player);

  fe->progress_adjustment = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 
                                                                  1.0, 0.01, 
                                                                  0.02, 0.02);

  fe->progress_slider = gtk_hscale_new (GTK_ADJUSTMENT (fe->progress_adjustment));
  gtk_scale_set_draw_value (GTK_SCALE (fe->progress_slider), FALSE);
  gtk_widget_set_usize (fe->progress_slider, -1, 16);
  
  g_signal_connect (G_OBJECT (fe->progress_slider), "change-value", 
		    G_CALLBACK (set_position), fe);
  
  gtk_container_add (GTK_CONTAINER (window), table);
  gtk_table_attach_defaults(GTK_TABLE(table), vol_slider, 0, 1, 0, 5);
  gtk_table_attach_defaults(GTK_TABLE(table), fe->title_label, 1, 4, 0, 1);
  /* currently not used:
  gtk_table_attach_defaults(GTK_TABLE(table), fe->artist_label, 1, 5, 1, 2);
  */ 
  gtk_table_attach(GTK_TABLE(table), exit_button, 4, 5, 0, 1, 0, 0, 0, 0);
  
  gtk_table_attach_defaults(GTK_TABLE(table), fe->progress_slider, 1, 5, 2, 3);
  gtk_table_attach(GTK_TABLE(table), fe->time_label, 3, 5, 3, 4, 
                   GTK_FILL, GTK_FILL, gpe_get_boxspacing(), 0);

  gtk_table_attach_defaults(GTK_TABLE(table), shuffle_button, 1, 2, 3, 4);
  gtk_table_attach_defaults(GTK_TABLE(table), loop_button, 2, 3, 3, 4);
  
  gtk_table_attach_defaults(GTK_TABLE(table), buttons_hbox, 1, 5, 4, 5);

  gtk_box_pack_start (GTK_BOX (buttons_hbox), prev_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), play_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), pause_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), stop_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), eject_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), next_button, TRUE, TRUE, 0);
  
  gpe_set_window_icon (window, "icon");
  gtk_widget_show_all (window);

  gtk_widget_set_usize (fe->time_label, fe->time_label->allocation.width, -1);
  
  gtk_main ();

  exit (0);
}
