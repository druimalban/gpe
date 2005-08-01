/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include "playlist_db.h"
#include "player.h"

struct nmf_frontend
{
  player_t player;
  struct playlist *playlist;
  GtkWidget *playlist_widget;
  gboolean playing;
  GtkAdjustment *progress_adjustment;
  GtkWidget *progress_slider;
  GtkWidget *volume_slider;
  GtkWidget *time_label, *artist_label, *title_label;
  GtkTreeModel *model;
  GtkTreeView *view;
  gboolean fs_open;
  gchar *current_path;
  gdouble position;
};

void update_track_info (struct nmf_frontend *fe, struct playlist *p);
void playlist_edit_push (GtkWidget *w, struct playlist *p);
GtkWidget *playlist_edit (struct nmf_frontend *fe, struct playlist *p);
