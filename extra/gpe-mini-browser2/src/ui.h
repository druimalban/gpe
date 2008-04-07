/* 
 * gpe-mini-browser2 
 * - Basic web browser for GPE and small screen GTK devices based on WebKitGtk -
 * - This file contains the declarations of the main UI functions 
 *
 * Dedicated to my dear Nóra.
 *
 * Author: Philippe De Swert <philippe.deswert@scarlet.be>
 *
 * Copyright (C) 2008 Philippe De Swert
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


/* For the UI creation */

GtkWidget * create_toolbar(void);

/* urlbox will be used on really small screens, urlbar will be used on bigger screens
   - urlbox uses a button to the toolbar to show a box to input an url 
     (this is also used in total fullscreen mode)
   - urlbar is a regular urlbar as in any other web browser 
*/
GtkWidget * create_urlbox(void);
GtkWidget * add_urlbox_button(GtkWidget *toolbar);
GtkWidget * create_urlbar(void);
GtkWidget * create_htmlview(void);
GtkWidget * create_tabs(void);



