/*
 *  callbacks.h
 *	part of galculator
 *  	(c) 2002 Simon Floery (simon.floery@gmx.at)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>

/* FILE */

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

/* HELP */

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_number_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_main_window_key_press_event         (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_operation_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_main_window_key_press_event         (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_function_button_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_constant_button_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_tbutton_fmod_clicked                (GtkButton       *button,
                                        gpointer         user_data);
void
on_button_sign_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_gfunc_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);
/*THE END*/
void
on_about_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data);
