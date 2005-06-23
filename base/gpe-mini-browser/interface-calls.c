/*
 * gpe-mini-browser v0.1
 *
 * Basic web browser based on gtk-webcore 
 * 
 * Interface calls.
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

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <glib.h>

#include <gpe/init.h>
#include "gpe/pixmaps.h"
#include "gpe/init.h"
#include "gpe/picturebutton.h"
#include <gpe/errorbox.h>

#include "gpe-mini-browser.h"

#define HOME_PAGE "file:///usr/share/doc/gpe/mini-browser-index.html"

#define DEBUG /* uncomment this if you want debug output*/

/* makes the engine go forward one page */
void
forward_func (GtkWidget * forward, GtkWidget * html)
{

  if (webi_can_go_forward (WEBI (html)))
    webi_go_forward (WEBI (html));
  else
    gpe_error_box ("no more pages forward!");

}

/* makes the engine go back one page */
void
back_func (GtkWidget * back, GtkWidget * html)
{
  if (webi_can_go_back (WEBI (html)))
    webi_go_back (WEBI (html));
  else
    gpe_error_box ("No more pages back!");


}

/* makes the engine load the home page, if none exists default to gpe.handhelds.org :-) */
void
home_func (GtkWidget * home, GtkWidget * html)
{
  if (access (HOME_PAGE, F_OK))  
	fetch_url ("http://gpe.handhelds.org", GTK_WIDGET (html));
  else
  	fetch_url (HOME_PAGE, GTK_WIDGET (html));
}

/* tell the engine to reload the current page */
void
reload_func (GtkWidget * reload, GtkWidget * html)
{
	webi_refresh(WEBI (html));
}

/* tell the engine to stop loading */
void
stop_func (GtkWidget * stop, GtkWidget * html)
{
	webi_stop_load(WEBI(html));
}

/* pop up a window to enter an URL */
void 
show_url_window (GtkWidget * show, GtkWidget * html)
{
	GtkWidget *url_window, *entry;
	GtkWidget *hbox, *vbox;
        GtkWidget *label;
        GtkWidget *buttonok, *buttoncancel;
	struct url_data *data;

        /* create dialog window */
	url_window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(url_window),("Where to go to?"));
  
	hbox = gtk_hbox_new (FALSE, 0);
	entry = gtk_entry_new ();
	label = gtk_label_new (("Enter url:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	gtk_container_set_border_width (GTK_CONTAINER (url_window), gpe_get_border ());

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

 	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->vbox),
        		      hbox, FALSE, FALSE, 0);

	/* add the buttons */
	buttonok = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
	buttoncancel = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->action_area),
        	      buttoncancel, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->action_area),
        	      buttonok, TRUE, TRUE, 0);

	GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (buttonok);

	data = malloc(sizeof(struct url_data));
	data->html = html;
	data->entry = entry;
	data->window = url_window;

	/* add button callbacks */
	g_signal_connect (GTK_OBJECT (buttonok), "clicked", G_CALLBACK(load_text_entry) , (gpointer *) data);
        g_signal_connect (GTK_OBJECT (buttoncancel), "clicked", G_CALLBACK(destroy_window), (gpointer *)url_window);
        g_signal_connect (GTK_OBJECT (buttonok), "clicked", G_CALLBACK(destroy_window), (gpointer *)url_window);

	gtk_widget_show_all (url_window);
        gtk_widget_grab_focus (entry);
}

void destroy_window (GtkButton * button, gpointer * window)
{
	gtk_widget_destroy (GTK_WIDGET(window));
}
