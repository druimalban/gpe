/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
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
#ifndef GPE_SKETCHBOOK_H
#define GPE_SKETCHBOOK_H

//--main
extern gchar * sketchdir;
void app_quit();

////--about
//extern GtkWidget *window_about;

////--dialog
//extern GtkWidget *window_dialog;
//void dialog_set_text(gchar * text);

//extern gint dialog_action;
//#define DELETE 1
//#define SAVE   2
//#define dialog_set_action(action) (dialog_action = action)
//extern gint dialog_type;
//#define DIALOG_CANCEL_OK 0
//#define DIALOG_OK        1
//#define dialog_set_type(type) (dialog_type = type)

#endif
