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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "pixmaps.h"
#include "init.h"
#include "render.h"
#include "picturebutton.h"
#include "errorbox.h"
#include "gtkminifilesel.h"

#include "playlist_db.h"
#include "player.h"
#include "decoder.h"

static struct gpe_icon my_icons[] = {
  { "ok" },
  { "cancel" },
  { "media-prev" },
  { "media-next" },
  { "media-play" },
  { "media-pause" },
  { "media-stop" },
  { "media-eject" },
  { "media-playlist", "open" },
  { "new" },
  { "delete" },
  { "exit" },
  { NULL, NULL }
};

/* GTK */
GtkWidget *window;				/* Main window */
GtkWidget *slider;				/* Playback slider */
GtkAdjustment *slider_adjustment;		/* Slider position indication */
float upper, step, page, pos;		/* Slider adjustment values */
GtkWidget *buttons_hbox;			/* Container for playback buttons */

GtkWidget *time_label, *artist_label, *title_label;

#define _(x) gettext(x)

static void
update_track_info (struct playlist *p)
{
  if (p)
    {
      assert (p->type == ITEM_TYPE_TRACK);
      
      gtk_label_set_text (GTK_LABEL (title_label), p->title);
      gtk_label_set_text (GTK_LABEL (artist_label), p->data.track.artist);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (title_label), "");
      gtk_label_set_text (GTK_LABEL (artist_label), "");
    }
}

static void
playlist_toggle (GtkWidget *w, gpointer d)
{
}

static void
close_file_sel (GtkWidget *w, gpointer d)
{
  gtk_widget_destroy (GTK_WIDGET (d));
}

static void
select_file_done (GtkWidget *fs, gpointer d)
{
  char *s = gtk_mini_file_selection_get_filename (fs);
  player_t player = (player_t)d;

  if (strstr (s, ".npl") || strstr (s, ".xml"))
    {
      struct playlist *p = playlist_xml_load (s);
      if (p)
	player_set_playlist (player, p);
    }
  else
    {
      struct playlist *p = playlist_new_list ();
      struct playlist *t = playlist_new_track ();
      t->data.track.url = s;
      if (strstr (t->data.track.url, ".mp3"))
	playlist_fill_id3_data (t);
      else if (strstr (t->data.track.url, ".ogg"))
	playlist_fill_ogg_data (t);

      if (t->title == NULL)
	t->title = t->data.track.url;

      p->data.list = g_slist_append (p->data.list, t);

      player_set_playlist (player, p);
    }

  gtk_widget_destroy (fs);
}

static void
eject_clicked (GtkWidget *w, gpointer d)
{
  GtkWidget *filesel = gtk_mini_file_selection_new (_("Select file"));
  gtk_signal_connect (GTK_OBJECT (GTK_MINI_FILE_SELECTION (filesel)->cancel_button), 
		      "clicked", close_file_sel, filesel);
  gtk_signal_connect (GTK_OBJECT (filesel), "completed", select_file_done, d);
  gtk_widget_show (filesel);
}

static void
play_clicked (GtkWidget *w, player_t p)
{
  struct player_status ps;
  player_play (p);
  player_status (p, &ps);
  update_track_info (ps.item);
}

int
main (int argc, char *argv[])
{
  /* GTK Widgets */
  GdkPixbuf *p;
  GtkWidget *w;
  GtkWidget *hbox2, *hbox3, *hbox4;
  GtkWidget *vbox, *vbox2, *vbox3;
  GdkColor col;
  GtkStyle *style;
  GtkWidget *prev_button, *play_button, *pause_button, *stop_button, *next_button, *eject_button, *exit_button, *playlist_button;
  GtkWidget *rewind_button, *forward_button;
  GtkWidget *vol_slider;
  GtkObject *vol_adjust;
  player_t player;

  gchar *color = "gray80";
  Atom window_type_atom, window_type_toolbar_atom;
  Display *dpy;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  gdk_color_parse (color, &col);
  
  /* GTK Window stuff */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GPE Media");
  gtk_widget_set_usize (GTK_WIDGET (window), 240, 71);
  gtk_widget_realize(window);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  window_type_atom =
    XInternAtom (dpy, "_NET_WM_WINDOW_TYPE" , False);
  window_type_toolbar_atom =
    XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR",False);

  XChangeProperty (dpy, GDK_WINDOW_XWINDOW (window->window), 
		   window_type_atom, XA_ATOM, 32, PropModeReplace, 
		  (unsigned char *) &window_type_toolbar_atom, 1);

  decoder_init ();
  player = player_new ();

  /* Destroy handler */

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 0);
  vbox3 = gtk_vbox_new (FALSE, 0);
  buttons_hbox = gtk_hbox_new (FALSE, 0);
  hbox2 = gtk_hbox_new (FALSE, 0);
  hbox3 = gtk_hbox_new (FALSE, 0);
  hbox4 = gtk_hbox_new (FALSE, 0);

  style = gtk_style_copy (buttons_hbox->style);
  style->bg[0] = col;
  
  p = gpe_find_icon ("media-prev");
  w = gpe_render_icon (window->style, p);
  gtk_widget_show (w);
  prev_button = gtk_button_new ();
  gtk_widget_show (prev_button);
  gtk_widget_set_style (prev_button, style);
  gtk_container_add (GTK_CONTAINER (prev_button), w);

  p = gpe_find_icon ("media-rew");
  w = gpe_render_icon (window->style, p);
  gtk_widget_show (w);
  rewind_button = gtk_button_new ();
  gtk_widget_show (rewind_button);
  gtk_widget_set_style (rewind_button, style);
  gtk_container_add (GTK_CONTAINER (rewind_button), w);

  p = gpe_find_icon ("media-play");
  w = gpe_render_icon (window->style, p);
  play_button = gtk_button_new ();
  gtk_widget_show (play_button);
  gtk_widget_set_style (play_button, style);
  gtk_container_add (GTK_CONTAINER (play_button), w);
  gtk_widget_show (w);
  gtk_signal_connect (GTK_OBJECT (play_button), "clicked", 
		      GTK_SIGNAL_FUNC (play_clicked), player);

  p = gpe_find_icon ("media-pause");
  w = gpe_render_icon (window->style, p);
  pause_button = gtk_button_new ();
  gtk_widget_show (pause_button);
  gtk_widget_set_style (pause_button, style);
  gtk_container_add (GTK_CONTAINER (pause_button), w);
  gtk_widget_show (w);

  p = gpe_find_icon ("media-stop");
  w = gpe_render_icon (window->style, p);
  stop_button = gtk_button_new ();
  gtk_widget_show (stop_button);
  gtk_widget_set_style (stop_button, style);
  gtk_container_add (GTK_CONTAINER (stop_button), w);
  gtk_widget_show (w);

  p = gpe_find_icon ("media-fwd");
  w = gpe_render_icon (window->style, p);
  forward_button = gtk_button_new ();
  gtk_widget_show (forward_button);
  gtk_widget_set_style (forward_button, style);
  gtk_container_add (GTK_CONTAINER (forward_button), w);
  gtk_widget_show (w);

  p = gpe_find_icon ("media-next");
  w = gpe_render_icon (window->style, p);
  next_button = gtk_button_new ();
  gtk_widget_show (next_button);
  gtk_widget_set_style (next_button, style);
  gtk_container_add (GTK_CONTAINER (next_button), w);
  gtk_widget_show (w);

  p = gpe_find_icon ("media-eject");
  w = gpe_render_icon (window->style, p);
  eject_button = gtk_button_new ();
  gtk_widget_show (eject_button);
  gtk_widget_set_style (eject_button, style);
  gtk_container_add (GTK_CONTAINER (eject_button), w);
  gtk_widget_show (w);
  gtk_signal_connect (GTK_OBJECT (eject_button), "clicked", 
		      GTK_SIGNAL_FUNC (eject_clicked), player);

  p = gpe_find_icon ("media-exit");
  w = gpe_render_icon (window->style, p);
  exit_button = gtk_button_new ();
  gtk_widget_show (exit_button);
  gtk_widget_set_style (exit_button, style);
  gtk_container_add (GTK_CONTAINER (exit_button), w);
  gtk_widget_show (w);
  gtk_signal_connect (GTK_OBJECT (exit_button), "clicked",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  p = gpe_find_icon ("media-playlist");
  w = gpe_render_icon (window->style, p);
  playlist_button = gtk_button_new ();
  gtk_widget_show (playlist_button);
  gtk_widget_set_style (playlist_button, style);
  gtk_container_add (GTK_CONTAINER (playlist_button), w);
  gtk_widget_show (w);
  gtk_signal_connect (GTK_OBJECT (playlist_button), "clicked",
		      GTK_SIGNAL_FUNC (playlist_toggle), NULL);

  time_label = gtk_label_new ("");
  artist_label = gtk_label_new ("");
  title_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (artist_label), 0.0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (title_label), 0.0, 0.5);

  vol_adjust = gtk_adjustment_new (0.0, 0.0, 1.0, 0.1, 0.2, 0.2);
  vol_slider = gtk_vscale_new (GTK_ADJUSTMENT (vol_adjust));
  gtk_scale_set_draw_value (GTK_SCALE (vol_slider), FALSE);
  gtk_widget_set_style (vol_slider, style);

  slider_adjustment = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 0.0, 
						     0.0, 0.0, 0.0);

  slider = gtk_hscale_new(GTK_ADJUSTMENT(slider_adjustment));
  gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
  gtk_widget_set_style (slider, style);
  GTK_WIDGET_UNSET_FLAGS (slider, GTK_CAN_FOCUS);
  gtk_range_set_update_policy(GTK_RANGE(slider), GTK_UPDATE_CONTINUOUS);

  gtk_container_add (GTK_CONTAINER (window), hbox2);

  gtk_box_pack_start (GTK_BOX (hbox2), vol_slider, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), vbox, TRUE, TRUE, 0);
  
  gtk_box_pack_start (GTK_BOX (vbox), hbox3, TRUE, TRUE, 0);
  //  gtk_box_pack_start (GTK_BOX (vbox), slider, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), buttons_hbox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox3), vbox2, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox3), vbox3, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), title_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), artist_label, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox3), hbox4, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox3), time_label, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox4), playlist_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox4), exit_button, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (buttons_hbox), prev_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), rewind_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), play_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), pause_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), stop_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), forward_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), eject_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttons_hbox), next_button, TRUE, TRUE, 0);

  gtk_widget_show (time_label);
  gtk_widget_show (artist_label);
  gtk_widget_show (title_label);
  gtk_widget_show (slider);
  gtk_widget_show (vol_slider);

  gtk_widget_show (vbox);
  gtk_widget_show (vbox2);
  gtk_widget_show (vbox3);
  gtk_widget_show (buttons_hbox);
  gtk_widget_show (hbox2);
  gtk_widget_show (hbox3);
  gtk_widget_show (hbox4);

  gtk_widget_show (window);
  
  gtk_main ();

  exit (0);
}
