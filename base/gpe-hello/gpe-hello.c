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
#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h> 

#define _(x) gettext(x)
//#define DEBUG /* uncomment this if you want debug output*/

struct gpe_icon my_icons[] = 
{
  { "hello", PREFIX "/share/pixmaps/gpe-hello.png" },
  {NULL, NULL}
};

static void hello_box (void)
{
 gpe_error_box(_("Welcome to GPE!"));
}

int
main (int argc, char *argv[])
{
  GtkWidget *app;
  GtkWidget *contentbox, *quit_button, *hello_button;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  //create application window
  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (app), "delete-event", gtk_main_quit, NULL);
  gtk_window_set_title (GTK_WINDOW (app), "GPE HELLO");
  
  gtk_widget_realize (app);

  //show icon in window decoration
  gpe_set_window_icon (app, "hello");

  //set fullscreen (uncomment the following line if you want a fullscreen application)
  //gtk_window_fullscreen(GTK_WINDOW(app));

  //create boxes
  contentbox = gtk_hbox_new (FALSE, 0);

  //add a hello & quit button with a gtk stock icon
  hello_button = gpe_button_new_from_stock(GTK_STOCK_ABOUT, GPE_BUTTON_TYPE_BOTH);
  quit_button = gpe_button_new_from_stock(GTK_STOCK_QUIT, GPE_BUTTON_TYPE_BOTH);

  //attach signals to the buttons
  g_signal_connect(GTK_OBJECT(hello_button), "clicked",
		   G_CALLBACK(hello_box), NULL);
  g_signal_connect(GTK_OBJECT(quit_button), "clicked",
		   G_CALLBACK(gtk_main_quit), NULL);

  //add buttons to the box
  gtk_box_pack_start(GTK_BOX(contentbox), hello_button, TRUE, TRUE, 5);
  gtk_box_pack_start(GTK_BOX(contentbox), quit_button, TRUE, TRUE, 5);

  //make everything viewable
  gtk_container_add (GTK_CONTAINER (app), contentbox);

  gtk_widget_show_all (app);
  gtk_main ();

  exit (0);
}
