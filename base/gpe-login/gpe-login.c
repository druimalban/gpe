/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <pwd.h>
#include <sys/types.h>
#include <shadow.h>
#include <crypt.h>
#include <unistd.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "gpe.xpm"

static const char *current_username;
static GtkWidget *label_result;
static gboolean have_users;

static void
set_username(GtkWidget *widget,
	     gpointer data)
{
  current_username = (const char *)data;
}

static void
slurp_passwd(GtkWidget *menu)
{
  struct passwd *pw;
  while (pw = getpwent(), pw != NULL)
    {
      const char *name;
      GtkWidget *item;
      if (pw->pw_uid < 100 || pw->pw_uid >= 65534)
	continue;
      have_users = TRUE;
      name = g_strdup(pw->pw_name);
      item = gtk_menu_item_new_with_label(name);
      gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(set_username), (gpointer)name);
      gtk_menu_append(GTK_MENU(menu), item);
      if (current_username == NULL)
	current_username = name;
    }
}

static void
enter_callback(GtkWidget *widget,
	       GtkWidget *entry)
{
  gchar *pwstr;
  struct passwd *pwe;
  struct spwd *spe;
  char *p;

  if (current_username == NULL)
    return;

  pwstr = gtk_entry_get_text(GTK_ENTRY(entry));
  gtk_entry_set_text(GTK_ENTRY(entry), "");

  pwe = getpwnam(current_username);
  if (pwe == NULL)
    goto login_incorrect;

  spe = getspnam(current_username);
  if (spe)
    pwe->pw_passwd = spe->sp_pwdp;

  p = crypt(pwstr, pwe->pw_passwd);
  if (strcmp(p, pwe->pw_passwd))
    goto login_incorrect;

  setuid(pwe->pw_uid);
  setgid(pwe->pw_gid);
  setenv("HOME", pwe->pw_dir, 1);
  chdir(pwe->pw_dir);

  execl("/etc/X11/Xsession", "/etc/X11/Xsession", NULL);

  return;

 login_incorrect:
  gtk_label_set_text(GTK_LABEL(label_result), "Login incorrect");
}

int
main(int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *option, *menu;
  GtkWidget *vbox, *hbox, *vbox2;
  GtkWidget *next_button;
  GtkWidget *hbox_user, *hbox_password;
  GtkWidget *frame;
  GtkWidget *logo;
  GtkWidget *login_label, *password_label;
  GtkWidget *entry;

  GdkPixmap *gpe_pix;
  GdkBitmap *gpe_pix_mask;

  gtk_init(&argc, &argv);
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gpe_pix = gdk_pixmap_create_from_xpm_d (window->window, &gpe_pix_mask, NULL, 
					  gpe_login_xpm);
  logo = gtk_pixmap_new (gpe_pix, gpe_pix_mask);

  frame = gtk_frame_new ("Log in");

  next_button = gtk_button_new_with_label ("OK");
  login_label = gtk_label_new ("Username:");
  password_label = gtk_label_new ("Password:");
  label_result = gtk_label_new ("");

  gtk_widget_set_usize (next_button, 48, -1);

  option = gtk_option_menu_new();
  menu = gtk_menu_new();
  slurp_passwd(menu);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);

  gtk_widget_set_usize (entry, 120, -1);
  gtk_widget_set_usize (option, 120, -1);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  hbox_user = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_user), login_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_user), option, TRUE, TRUE, 0);

  hbox_password = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_password), password_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_password), entry, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox_user, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox_password, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label_result, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  gtk_box_pack_end (GTK_BOX (hbox), next_button, FALSE, FALSE, 5);

  gtk_signal_connect(GTK_OBJECT (entry), "activate",
		     GTK_SIGNAL_FUNC (enter_callback), entry);
  gtk_signal_connect(GTK_OBJECT (next_button), "clicked",
		     GTK_SIGNAL_FUNC (enter_callback), entry);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), logo, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 5);

  gtk_container_add (GTK_CONTAINER (window), vbox2);

  gtk_widget_grab_focus (entry);

  gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_widget_show_all (window);

  gtk_main();

  return 0;
}
