/*
 * gpe-hello v0.1
 *
 * Tutorial program to learn programming for GPE 
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* the usual suspects */
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

/* gtk includes */
#include <gtk/gtk.h>
#include <glib.h>

/* internationalisation aka i8n */
#include <libintl.h>
#include <locale.h> 

/* gpe includes */
#include <gpe/init.h>           /* gpe_application_init */
#include <gpe/gpedialog.h>       /* gpe_info_dialog */
#include <gpe/pixmaps.h>        /* gpe_load_icons */
#include <gpe/picturebutton.h>  /* gpe_button_new_from_stock */
#include <gpe/spacing.h>        /* gpe_get_border, gpe_get_boxspacing */

#define _(x) gettext(x)
//#define DEBUG /* uncomment this if you want debug output*/

/*
 * This struct holds all pixmaps handled by libgpewidget functions.
 * It consists of a set of identifiers and icon files. For files in the
 * default GPE icon location you can omit the path and the extension.
 */
struct gpe_icon my_icons[] = 
{
  { "hello", PREFIX "/share/pixmaps/gpe-hello.png" },
  {NULL, NULL}
};

/* Just do someting... this displays a nasty error box */
static void 
hello_box (void)
{
 gpe_info_dialog(_("Welcome to GPE!"));
}


/* 
 * This creates the toolbar, it is not essential to have this code in a 
 * separate function, but it makes the code easier to read.
 */
static GtkWidget*
toolbar_create(void)
{
  GtkWidget *toolbar;
  GtkToolItem *item;
  
  /* Create a toolbar widget and define its orientation. The size and layout
     is defined externally via xsettings. */
  toolbar = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);

  /* Create a tool button and add it to the leftmost free position.*/
  item = gtk_tool_button_new_from_stock(GTK_STOCK_ABOUT);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  /* Connect "clicked" signal of the toolbutton. */
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(hello_box), NULL);

  /* Create a separator taking up the free space to the end of the toolbar. */
  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_tool_item_set_expand(item, TRUE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    
  /* Create the close button and add it to the leftmost free position.*/
  item = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  /* Connect "clicked" signal of the toolbutton to close the application. */
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(gtk_main_quit), NULL);
  return toolbar;
}

int
main (int argc, char *argv[])
{
  GtkWidget *app;
  GtkWidget *contentbox, *quit_button, *hello_button;
  GtkWidget *mainbox, *toolbar, *label;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  /* create application window */
  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  /* gtk_window_set_type_hint(GTK_WINDOW(app), GDK_WINDOW_TYPE_HINT_DIALOG); */
  
  g_signal_connect (G_OBJECT (app), "delete-event", gtk_main_quit, NULL);
  gtk_window_set_title (GTK_WINDOW (app), "GPE HELLO");
  
  gtk_widget_realize (app);

  /* show icon in window decoration */
  gpe_set_window_icon (app, "hello");

  /* set fullscreen (uncomment the following line if you want a fullscreen application)
   * On PDAs we ususall have Matchbox doing this for use */
  /* gtk_window_fullscreen(GTK_WINDOW(app)); */

  /* call function creating main window toolbar, see above */
  toolbar = toolbar_create();
  
  /* the label  is an example widget taking up the major part of the window */
  label = gtk_label_new("Welcome to GPE!");

  /* create boxes */
  contentbox = gtk_hbox_new (FALSE, gpe_get_boxspacing());
  mainbox = gtk_vbox_new (FALSE, gpe_get_boxspacing());

  /* set border with */
  gtk_container_set_border_width(GTK_CONTAINER(contentbox), gpe_get_border());

  /* add a hello & quit button with a gtk stock icon */
  hello_button = gpe_button_new_from_stock(GTK_STOCK_ABOUT, GPE_BUTTON_TYPE_BOTH);
  quit_button = gpe_button_new_from_stock(GTK_STOCK_QUIT, GPE_BUTTON_TYPE_BOTH);

  /* attach signals to the buttons */
  g_signal_connect(GTK_OBJECT(hello_button), "clicked",
		   G_CALLBACK(hello_box), NULL);
  g_signal_connect(GTK_OBJECT(quit_button), "clicked",
		   G_CALLBACK(gtk_main_quit), NULL);

  /* add buttons to the box */
  gtk_box_pack_start(GTK_BOX(contentbox), hello_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(contentbox), quit_button, TRUE, TRUE, 0);

  /* pack the box with buttons and and the toolbar to the main box */
  gtk_box_pack_start(GTK_BOX(mainbox), toolbar, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(mainbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(mainbox), contentbox, FALSE, TRUE, 0);

  /* add the main box to the window */
  gtk_container_add (GTK_CONTAINER (app), mainbox);

  /* make everything visible and start the main loop */
  gtk_widget_show_all (app);
  gtk_main ();

  exit (0);
}
