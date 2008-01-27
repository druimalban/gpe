/*
 *
 * gpe-mini-browser2 v0.0.1
 * - Basic web browser for GPE and small screen GTK devices based on WebKitGtk -
 * - This file contains the browser wide includes
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

/*****************************************************************
                        GENERAL INCLUDES
*****************************************************************/

struct browser_window
{
  WebKitWebView* web_view;
  GtkWidget *url_entry;
};

