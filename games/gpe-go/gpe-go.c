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
#include <gtk/gtk.h>

#include <stdlib.h> //exit()
#include <stdio.h>  //sscanf()
#include <string.h> //strlen()

#include "model.h"
#include "board.h"
#include "sgf-handler.h"

//--GPE libs
#include "gpe/init.h"
#include "gpe/pixmaps.h"
#include "gpe/popup_menu.h"
#include "gpe/picturebutton.h"

//--i18n
#include <libintl.h>
#define _(_x) gettext(_x)

#ifdef DEBUG
#define TRACE(message...) {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...) 
#endif

static struct gpe_icon my_icons[] = {
  { "this_app_icon", PREFIX "/share/pixmaps/gpe-go.png" },

  { NULL, NULL }
};

enum page_names{
  PAGE_BOARD = 0,
  PAGE_GAME_SETTINGS,
  PAGE_COMMENT_EDITOR
};

void app_init(int argc, char ** argv){
  int size = 19;//default value

  if(argc > 1){//first arg is board size if numeric
    sscanf(argv[1], "%d", &size);
  }
  init_new_game(size);
}

void app_quit(){
  gtk_exit (0);
}

void gui_init();

int main (int argc, char ** argv){

  if (gpe_application_init (&argc, &argv) == FALSE) exit (1);
  if (gpe_load_icons (my_icons) == FALSE) exit (1);

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
  setlocale (LC_ALL, "");

  gui_init ();
  load_graphics();
  app_init (argc, argv);

  gtk_main ();

  return 0;
}

void _remove_duplicated(char * string, char c){
  char *m, *s, *t;
  m = s = t = string;
  while(*t){
    t++;    
    if(*t != c || *s != c){
      *m = *s;
      m++;
    }
    s++;
  }
  *m = '\0';//FIXME: resize the string!
}

void status_update(char * message){
  g_strdelimit(message, "\n\t\r", ' ');//make message a "flat" string
  _remove_duplicated(message, ' ');

  gtk_statusbar_pop (GTK_STATUSBAR(go.status), 0);
  gtk_statusbar_push(GTK_STATUSBAR(go.status), 0, message);

  {//if the message is longer than the label, alert the user
    GtkWidget * label;
    PangoLayout * layout;
    int text_width;
    int label_width;

    label = (GTK_STATUSBAR(go.status))->label;//NOTE: access to private field
    label_width = label->allocation.width;
    layout = gtk_label_get_layout(GTK_LABEL(label));
    pango_layout_get_pixel_size (layout, &text_width, NULL);
    TRACE("GOT width %d / %d", text_width, label_width);

    if(text_width > label_width){
      gtk_label_set_text(GTK_LABEL(go.status_expander), "(...)");
    }
    else{
      /*TRANSLATORS: initial of "Comments"
        => Small labeled button to open the comment editor.
        The surrounding spaces are desired.*/
      gtk_label_set_text(GTK_LABEL(go.status_expander), _(" C "));
    }
  }
}

#include <stdarg.h>
void status_update_fmt(const char * format, ...){
  gchar * message;
  va_list ap;
  va_start (ap, format);
  vasprintf (&message, format, ap);
  va_end (ap);

  status_update(message);
}

void status_update_current(){
  Hitem * hitem;
  char item;
  gchar * message;

  hitem = go.history->data;
  if(!hitem){
    return;
  }

  switch(hitem->item){
    case BLACK_STONE: item = 'B'; break;
    case WHITE_STONE: item = 'W'; break;
    default: item = '-';
  }

  if(hitem->action == PASS){
    message = g_strdup_printf(" %c PASS", item);
  }
  else /*if(hitem->action == PLAY)*/{
    message = g_strdup_printf(" %c (%d,%d) ", item, hitem->row, hitem->col);
  }

  if(hitem->comment){
    status_update_fmt("%s - %s", message, hitem->comment);
  }
  else{
    status_update(message);
  }
}

void update_capture_label(){
  free(go.capture_string);
  go.capture_string = (char *) malloc (20 * sizeof(char));
  /* TRANSLATORS: B = Black stones, W = White stones */
  sprintf(go.capture_string, _("B:%d  W:%d "), go.black_captures, go.white_captures);
  gtk_label_set_text (GTK_LABEL (go.capture_label), go.capture_string);
}


void on_file_selection_ok(GtkWidget *widget, gpointer file_selector){
  if(go.save_game) save_game();
  else load_game();
}

//CLEAN: connect directly to the called function
void on_button_first_pressed(GtkWidget *widget, gpointer unused){
  goto_first();
}

void on_button_last_pressed(GtkWidget *widget, gpointer unused){
  goto_last();
}

void on_button_prev_pressed (void){
  undo_turn();
}
void on_button_next_pressed (void){
  redo_turn();
}

void on_button_pass_clicked (void){
  if(go.lock_variation_choice) return;
  pass_turn();
}

void on_button_newgame_cancel_clicked (void){
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_BOARD);
}

void on_button_newgame_ok_clicked (void){
  init_new_game(go.selected_game_size);
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_BOARD);
}

void on_button_game_new_clicked(GtkButton *button, gpointer unused){
  popup_menu_close(go.game_popup_button);
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_GAME_SETTINGS);
}
void on_button_game_save_clicked(GtkButton *button, gpointer unused){
  popup_menu_close(go.game_popup_button);
  go.save_game = TRUE;
  gtk_window_set_title( GTK_WINDOW(go.file_selector), _("Save game as..."));
  gtk_widget_show (go.file_selector);
}
void on_button_game_load_clicked(GtkButton *button, gpointer unused){
  popup_menu_close(go.game_popup_button);//FIXME: connect function on "clicked"
  go.save_game = FALSE;
  gtk_window_set_title( GTK_WINDOW(go.file_selector), _("Load game..."));
  gtk_widget_show (go.file_selector);
}

void on_radiobutton_size_clicked (GtkButton *button, gpointer size){
  if(size == NULL){
    go.selected_game_size =
      gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(go.game_size_spiner));
  }
  else{
    go.selected_game_size = GPOINTER_TO_INT(size);
  }
}

void on_spinbutton_value_changed(GtkSpinButton *spinbutton,
                                 gpointer radiobutton){
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton), TRUE);
  go.selected_game_size = gtk_spin_button_get_value_as_int (spinbutton);
}

GtkWidget * build_new_game_dialog(){
  GtkWidget * top_level_container;

  //containers
  GtkWidget * title;
  GtkWidget * game_size;
  GtkWidget * buttons;

  //
  GtkWidget * label;
  GtkWidget * vbox;
  GtkWidget * radiobutton1;
  GtkWidget * radiobutton3;
  GtkWidget * radiobutton4;
  GSList    * group_game_size_group = NULL;
  GtkWidget * hbox;
  GtkWidget * radiobutton2;
  GtkObject * spinbutton1_adj;
  GtkWidget * spinbutton1;

  GtkWidget * frame;
  GtkWidget * image;
  GdkPixbuf * pixbuf;

  //--title
  label = gtk_label_new (NULL);
  /* TRANSLATORS: keep the <big><b> tags */
  gtk_label_set_markup (GTK_LABEL (label), _("<big><b>New Game</b></big>"));

  //image
  pixbuf = gpe_find_icon ("this_app_icon");
  image = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(G_OBJECT(pixbuf));
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (frame), image);

  //packing
  hbox = gtk_hbox_new (FALSE, 20);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  title = hbox;


  //--size choice
  vbox = gtk_vbox_new (FALSE, 5);

  label = gtk_label_new (NULL);
  /* TRANSLATORS: keep the <b> tags */
  gtk_label_set_markup (GTK_LABEL (label), _("<b>Game Size</b>"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  radiobutton1 = gtk_radio_button_new_with_label (group_game_size_group, "9");
  group_game_size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton1));
  gtk_box_pack_start (GTK_BOX (vbox), radiobutton1, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radiobutton1), "clicked",
                    G_CALLBACK (on_radiobutton_size_clicked), GINT_TO_POINTER(9));

  radiobutton3 = gtk_radio_button_new_with_label (group_game_size_group, "13");
  group_game_size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton3));
  gtk_box_pack_start (GTK_BOX (vbox), radiobutton3, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radiobutton3), "clicked",
                    G_CALLBACK (on_radiobutton_size_clicked), GINT_TO_POINTER(13));

  radiobutton4 = gtk_radio_button_new_with_label (group_game_size_group, "19");
  group_game_size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton4));
  gtk_box_pack_start (GTK_BOX (vbox), radiobutton4, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radiobutton4), "clicked",
                    G_CALLBACK (on_radiobutton_size_clicked), GINT_TO_POINTER(19));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  radiobutton2 = gtk_radio_button_new_with_label (group_game_size_group, "");
  group_game_size_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton2));
  gtk_box_pack_start (GTK_BOX (hbox), radiobutton2, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radiobutton2), "clicked",
                    G_CALLBACK (on_radiobutton_size_clicked), NULL);

  spinbutton1_adj = gtk_adjustment_new (21, 4, 26, 1, 10, 10);
  spinbutton1 = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton1_adj), 1, 0);
  g_signal_connect (G_OBJECT (spinbutton1), "value-changed",
                    G_CALLBACK (on_spinbutton_value_changed), radiobutton2);

  gtk_box_pack_start (GTK_BOX (hbox), spinbutton1, FALSE, FALSE, 0);

  //default
  go.selected_game_size = 19;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton4), TRUE);

  go.game_size_spiner = spinbutton1;

  game_size = vbox;

  //[OK] button
  {
    GtkWidget * button;
    GtkWidget * hbox;

    hbox = gtk_hbox_new(TRUE, 0);
    
    button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_newgame_cancel_clicked), NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 4);

    button = gtk_button_new_from_stock (GTK_STOCK_OK);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_newgame_ok_clicked), NULL);
    
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 4);

    buttons = hbox;
  }
    
  //--Packing
  top_level_container = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (top_level_container), 5);

  gtk_box_pack_start (GTK_BOX (top_level_container), title, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (top_level_container), game_size, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (top_level_container), gtk_hseparator_new (), FALSE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX (top_level_container), buttons, FALSE, FALSE, 4);

  return top_level_container;
}

void on_button_edit_comment_clicked(){
  Hitem * hitem;

  if(go.lock_variation_choice) return;

  hitem = go.history->data;
  if(hitem && hitem->comment){
    gtk_text_buffer_set_text (go.comment_buffer, hitem->comment, -1);
  }
  else{
    gtk_text_buffer_set_text (go.comment_buffer, "", -1);
  }
  go.comment_edited = FALSE;
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_COMMENT_EDITOR);
  gtk_widget_grab_focus(go.comment_text_view);
}

void on_textbuffer_changed (GtkTextBuffer * textbuffer, gpointer unused){
  go.comment_edited = TRUE;
}

void on_button_comment_cancel_clicked (void){
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_BOARD);
}

void on_button_comment_ok_clicked (void){
  if(go.comment_edited){
    gchar * s;
    GtkTextIter it_start;
    GtkTextIter it_end;
    
    gtk_text_buffer_get_bounds   (go.comment_buffer, &it_start, &it_end);
    s = gtk_text_buffer_get_text (go.comment_buffer, &it_start, &it_end, FALSE);
    TRACE("Got -->%s<--", s);

    g_strstrip(s);

    if( strlen(s) > 0 && go.history->data ){
      Hitem * hitem;
      hitem = go.history->data;
      if(hitem->comment) free(hitem->comment);
      hitem->comment = s;
    }
    status_update_current();
  }
  gtk_notebook_set_page(GTK_NOTEBOOK(go.notebook), PAGE_BOARD);
}

GtkWidget * build_comment_editor(){
  GtkWidget * top_level_container;

  //containers
  GtkWidget * title;
  GtkWidget * buttons;

  //
  GtkWidget * label;
  GtkWidget * hbox;

  GtkWidget * frame;
  GtkWidget * image;
  GdkPixbuf * pixbuf;

  GtkWidget * comment_text_view;
  GtkWidget * scrolled_window;

  //--title
  label = gtk_label_new (NULL);
  /* TRANSLATORS: keep the <big><b> tags */
  gtk_label_set_markup (GTK_LABEL (label), _("<big><b>Comment editor</b></big>"));

  //image
  pixbuf = gpe_find_icon ("this_app_icon");
  image = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(G_OBJECT(pixbuf));
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (frame), image);

  //packing
  hbox = gtk_hbox_new (FALSE, 20);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  title = hbox;

  //--Text editor
  comment_text_view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(comment_text_view), GTK_WRAP_CHAR);
  go.comment_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (comment_text_view));
  g_signal_connect (G_OBJECT (go.comment_buffer), "changed",
                    G_CALLBACK (on_textbuffer_changed), NULL);
  go.comment_text_view = comment_text_view;

  //scrolled window
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolled_window), comment_text_view);

  //--[CANCEL][OK] buttons
  {
    GtkWidget * button;
    GtkWidget * hbox;

    hbox = gtk_hbox_new(TRUE, 0);
    
    button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_comment_cancel_clicked), NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 4);

    button = gtk_button_new_from_stock (GTK_STOCK_OK);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_button_comment_ok_clicked), NULL);
    
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 4);

    buttons = hbox;
  }

  //--Packing
  top_level_container = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (top_level_container), 5);

  gtk_box_pack_start (GTK_BOX (top_level_container), title, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (top_level_container), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (top_level_container), buttons, FALSE, FALSE, 4);

  return top_level_container;
}

GtkWidget * _game_popup_menu_new (GtkWidget *parent_button){
  GtkWidget *vbox;//to return

  GtkWidget * button_new;
  GtkWidget * button_load;
  GtkWidget * button_save;

  GtkStyle * style = go.window->style;

  button_new  = gpe_picture_button_aligned (style, _("New...") , "!gtk-new" , GPE_POS_LEFT);
  button_save = gpe_picture_button_aligned (style, _("Save..."), "!gtk-save", GPE_POS_LEFT);
  button_load = gpe_picture_button_aligned (style, _("Load..."), "!gtk-open", GPE_POS_LEFT);

#define _BUTTON_SETUP(action) \
              gtk_button_set_relief (GTK_BUTTON (button_ ##action), GTK_RELIEF_NONE);\
              g_signal_connect (G_OBJECT (button_ ##action), "clicked",\
              G_CALLBACK (on_button_game_ ##action ##_clicked), NULL);

  _BUTTON_SETUP(new);
  _BUTTON_SETUP(save);
  _BUTTON_SETUP(load);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_new,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_save, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_load, FALSE, FALSE, 0);

  return vbox;
}


void gui_init(){
  GtkWidget * widget;

  GtkWidget * window;
  GtkWidget * vbox;

  GtkWidget * new_game_dialog;
  GtkWidget * comment_editor;

  GtkWidget * toolbar;
  GtkWidget * image;

  GtkWidget * capture_label;

  //--toplevel window
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  go.window = window;

#ifdef DESKTOP
  gtk_window_set_default_size (GTK_WINDOW (window), 240, 280);
#endif
  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (app_quit), NULL);

  gpe_set_window_icon (window, "this_app_icon");


  //--toolbar
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation(GTK_TOOLBAR (toolbar),
                              GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style      (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  //[game] popup menu
  widget = popup_menu_button_new_from_stock (GTK_STOCK_NEW,
                                             _game_popup_menu_new, NULL);
  gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);
  gtk_toolbar_append_widget(GTK_TOOLBAR (toolbar), widget,
                            _("Game menu"), _("Game menu"));
  go.game_popup_button = widget;

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  //[FIRST] button
  image = gtk_image_new_from_stock (GTK_STOCK_GOTO_FIRST, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("First"),
                           _("Go to the beginning of the game"),
                           _("Go to the beginning of the game"),
                           image, GTK_SIGNAL_FUNC (on_button_first_pressed), NULL);

  //[PREV] button
  image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Prev"),
                           _("Go to the previous step"),
                           _("Go to the previous step"),
                           image, GTK_SIGNAL_FUNC (on_button_prev_pressed), NULL);

  //[NEXT] button
  image = gtk_image_new_from_stock (GTK_STOCK_REDO, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Next"),
                           _("Go to the next step"),
                           _("Go to the next step"),
                           image, GTK_SIGNAL_FUNC (on_button_next_pressed), NULL);

  //[LAST] button
  image = gtk_image_new_from_stock (GTK_STOCK_GOTO_LAST, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Last"),
                           _("Go to the end of the game"),
                           _("Go to the end of the game"),
                           image, GTK_SIGNAL_FUNC (on_button_last_pressed), NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));


  //Capture label, 
  //let put it in the toolbar. Will find a better place later
  capture_label = gtk_label_new("");
  
  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), capture_label, NULL, NULL);
  go.capture_label = capture_label;

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  // [PASS] button
  image = gtk_image_new_from_stock (GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
                           _("Pass"), _("Pass your turn"), _("Pass your turn"),
                           image, GTK_SIGNAL_FUNC (on_button_pass_clicked), NULL);

  //--file selector
  widget = gtk_file_selection_new (NULL);

  g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (widget)->ok_button),
                    "clicked",
                    G_CALLBACK (on_file_selection_ok),
                    (gpointer) widget);

  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (widget)->ok_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_hide), 
                            (gpointer) widget); 

  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (widget)->cancel_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_hide),
                            (gpointer) widget);

  //gtk_file_selection_complete(GTK_FILE_SELECTION(widget), "*.sgf");
  go.file_selector = widget;

  //--Go Board
  go.drawing_area = go_board_new();

  //--Status bar
  widget = gtk_statusbar_new();
  gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(widget), FALSE);
  go.status = widget;

  {//[Comments] button / Status expander
    GtkWidget * event_box;

    event_box = gtk_event_box_new();
    gtk_widget_set_events (event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect (G_OBJECT (event_box), "button_press_event",
                      G_CALLBACK (on_button_edit_comment_clicked), NULL);

    widget = gtk_label_new("");
    gtk_container_add (GTK_CONTAINER (event_box), widget);
    gtk_box_pack_start (GTK_BOX (go.status), event_box, FALSE, FALSE, 0);

    go.status_expander = widget;
  }

  //--Comment editor
  comment_editor = build_comment_editor();

  //--New game settings page
  new_game_dialog = build_new_game_dialog();


  //--packing

  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar,      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), go.drawing_area, TRUE,  TRUE,  0);
  gtk_box_pack_start (GTK_BOX (vbox), go.status,    FALSE, FALSE, 0);

  widget = gtk_notebook_new();
  go.notebook = widget;
  gtk_notebook_set_show_border(GTK_NOTEBOOK(widget), FALSE);
  gtk_notebook_set_show_tabs  (GTK_NOTEBOOK(widget), FALSE);

  gtk_notebook_insert_page(GTK_NOTEBOOK(widget), vbox,  NULL, PAGE_BOARD);
  gtk_notebook_insert_page(GTK_NOTEBOOK(widget), new_game_dialog, NULL, PAGE_GAME_SETTINGS);
  gtk_notebook_insert_page(GTK_NOTEBOOK(widget), comment_editor, NULL, PAGE_COMMENT_EDITOR);

  gtk_container_add (GTK_CONTAINER (window), widget);

  gtk_widget_show_all (window);
  gtk_notebook_set_page(GTK_NOTEBOOK(widget), PAGE_BOARD);

}//gui_init()

