/*
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

GtkWidget *Login_Setup_Build_Objects();
void Login_Setup_Free_Objects();
void Login_Setup_Save();
void Login_Setup_Restore();

void
choose_login_bg_file (GtkWidget *button,
		      gpointer  user_data);

static void
File_Selected (char *file,
	       gpointer data);

void
get_initial_values ();

void 
update_login_bg_show ();

void 
update_ownerinfo_show ();

void
on_login_bg_file_button_size_allocate (GtkWidget       *widget,
                                       GtkAllocation   *allocation,
                                       gpointer         user_data);
