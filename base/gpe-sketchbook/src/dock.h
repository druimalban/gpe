/* 
 * Copyright (C) 2002 Luc Pionchon <luc.pionchon@welho.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef DOCK_H
#define DOCK_H

/******************************************** basic idea:
       01         23       0  1          23
     0 ++---------++     0 +--+----------++
       ||toolbar  ||     1 +--+----------++
     1 ++---------++       |t |          ||
       ||         ||       |o |   area   ||
       ||  area   ||       |o |          ||
       ||         ||       |l |          ||
       ||         ||       |b |          ||
       ||         ||       |a |          ||
       ||         ||       |r |          ||
     2 ++---------++     2 +--+----------++
     3 ++---------++     3 +--+----------++
 ********************************************/

struct _dock {
  GtkWidget  * app_content;
  GtkToolbar * toolbar;

  GtkOrientation toolbar_orientation;

  //--private
  GtkTable   * _table;
};

typedef struct _dock GpeDock;

/** creates a new empty dock */
GpeDock * gpe_dock_new();

/** destroys a dock (and its childs) */
void      gpe_dock_destroy(GpeDock * dock);

/** returns the dock main container */
GtkWidget * gpe_dock(GpeDock * dock);

/** adds the application main "area" to the dock */
void      gpe_dock_add_app_content(GpeDock * dock,
                                   GtkWidget * content);

/** adds the toolbar to the dock */
void      gpe_dock_add_toolbar(GpeDock * dock,
                               GtkToolbar * toolbar,
                               GtkOrientation orientation);

/** changes the orientation of the toolbar */
void      gpe_dock_change_toolbar_orientation(GpeDock * dock,
                                              GtkOrientation orientation);
#endif
