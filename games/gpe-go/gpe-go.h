/* gpe-go, a GO board for GPE
 *
 * $Id$
 *
 * Copyright (C) 2003-2004 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef GPE_GO_H
#define GPE_GO_H

#include <gtk/gtk.h>

#include "model.h"
#include "board.h"

struct {
  GoGame  game;
  GoBoard board;

  struct {
    GtkWidget * window;
    GtkWidget * notebook;
    
    GtkWidget * status;
    GtkWidget * status_expander;

    GtkWidget * capture_label;
    char      * capture_string;

    GtkWidget * game_menu_popup_button;

    /* New game dialog */
    int selected_game_size;
    GtkWidget * game_size_spiner;

    /* Save/Load game dialog */
    GtkWidget * file_selector;
    gboolean save_game;

    /* Comment editor */
    GtkWidget * comment_text_view;
    GtkTextBuffer * comment_buffer;
    gboolean comment_edited;

  } ui;

} go;


void status_update_fmt(const char * format, ...);
void status_update_current();
void update_capture_label();

#endif
