/*
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
 
#define GPE_OWNERINFO_DONTSHOW_FILE "/etc/gpe/gpe-ownerinfo.dontshow"
#define GPE_LOGIN_BG_LINKED_FILE    "/etc/gpe/gpe-login-bg.png"
#define GPE_LOGIN_BG_DONTSHOW_FILE  "/etc/gpe/gpe-login-bg.dontshow"
#define GPE_LOGIN_LOCK_SCRIPT "/etc/apm/resume.d/S98lock-display"

GtkWidget *Login_Setup_Build_Objects();
void Login_Setup_Free_Objects();
void Login_Setup_Save();
void Login_Setup_Restore();

void
choose_login_bg_file (GtkWidget *button,
		      gpointer  user_data);

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
