/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003-2005  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * User manager module.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include "ucallbacks.h"
#include "uinterface.h"
#include <gpe/picturebutton.h>
#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>

#include "../../applets.h"
#include "../../suid.h"

static gchar *listTitles[3];
extern gboolean set_own_password;

pwlist *pwroot = NULL;

GtkAttachOptions table_attach_left_col_x;
GtkAttachOptions table_attach_left_col_y;
GtkAttachOptions table_attach_right_col_x;
GtkAttachOptions table_attach_right_col_y;
GtkJustification table_justify_left_col;
GtkJustification table_justify_right_col;
guint border_width;
guint col_spacing;
guint row_spacing;
guint widget_padding_x;
guint widget_padding_y;
guint widget_padding_y_even;
guint widget_padding_y_odd;
GtkWidget *user_list;
gboolean have_access = FALSE;

void
InitSpacings () {
  
  table_attach_left_col_x = GTK_FILL; 
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;
  
  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_RIGHT;

  border_width = 6;
  col_spacing = 6;
  row_spacing = 6;
  widget_padding_x = 0; /* add space with col_spacing */
  widget_padding_y = 0; /* add space with row_spacing */
  widget_padding_y_even = 6; /* padding in y direction for widgets in an even row */
  widget_padding_y_odd  = 6; /* padding in y direction for widgets in an odd row  */
}


int IsHidden(pwlist *cur)
{
  return  !((cur->pw.pw_uid>=MINUSERUID 
	         && cur->pw.pw_uid<65534) 
	         || cur->pw.pw_uid ==0);
}

void InitPwList()
{
  struct passwd *pwent;
  pwlist **prec = &pwroot;
  pwlist *cur;

  setpwent();
  if(!pwroot)
    {
      pwent = getpwent();
      while(pwent)
	{
	  cur = malloc(sizeof(pwlist));
	  cur->pw = *pwent;
	  pwent = &cur->pw;
	  pwent->pw_name = strdup(pwent->pw_name);
	  pwent->pw_passwd = strdup(pwent->pw_passwd);
	  pwent->pw_gecos = strdup(pwent->pw_gecos);
	  pwent->pw_dir = strdup(pwent->pw_dir);
	  pwent->pw_shell = strdup(pwent->pw_shell);
	  cur->next = 0;
	  *prec=cur;
	  prec=&cur->next;
	  pwent = getpwent();
	}
      endpwent();
    }
  
}

 
void ReloadList()
{
  pwlist *cur = pwroot;
  gchar *entry[3];
  gtk_clist_clear(GTK_CLIST(user_list));
  while(cur != NULL)
  {
    
    if(! IsHidden(cur))
	{
	  entry[0] = g_strdup_printf("%s",cur->pw.pw_name);
	  entry[1] = g_strdup_printf("%s",cur->pw.pw_gecos);
	  entry[2] = g_strdup_printf("%s",cur->pw.pw_dir);
	  gtk_clist_append( GTK_CLIST(user_list), entry);
	  g_free(entry[0]);
	  g_free(entry[1]);
	  g_free(entry[2]);
	}
      cur = cur->next;
    }
  gtk_clist_columns_autosize      ((GtkCList *)user_list);
}

void Users_Free_Objects()
{
  pwlist *cur = pwroot,*next;
  while(cur != NULL)
    {
      next=cur->next;
      free(cur->pw.pw_name);
      free(cur->pw.pw_passwd);
      free(cur->pw.pw_gecos);
      free(cur->pw.pw_dir);
      free(cur->pw.pw_shell);
      free(cur);
      cur = next;
    }
  pwroot = 0;
}

void Users_Save()
{
  pwlist *cur = pwroot;
  gchar *tmp;
  FILE *f;

  if (!have_access) 
	  return;

  f = fopen("/tmp/passwd","w");

  while(cur != NULL)
    {
      fprintf(f,"%s:%s:%d:%d:%s:%s:%s\n",
	      cur->pw.pw_name,
	      cur->pw.pw_passwd,
	      cur->pw.pw_uid,
	      cur->pw.pw_gid,
	      cur->pw.pw_gecos,
	      cur->pw.pw_dir,
	      cur->pw.pw_shell);
      cur = cur->next;
    }
  fclose(f);
  
  suid_exec("CPPW"," ");
  sleep(1);
  cur = pwroot;
  while(cur != NULL)
    {
      /* check users homedir */
		if (access(cur->pw.pw_dir,F_OK))
		{			
			tmp = malloc(sizeof(char)*(strlen(cur->pw.pw_dir)+strlen(cur->pw.pw_name)+2));
			sprintf(tmp,"%s\n%s",cur->pw.pw_dir,cur->pw.pw_name);
			suid_exec("CRHD",tmp);
			free(tmp);
		}
      cur = cur->next;
    }
}

void Users_Restore()
{
  Users_Free_Objects();
  InitPwList();
  ReloadList();
}

GtkWidget*
Users_Build_Objects (gboolean password_only, GtkWidget *toolbar)
{
  GtkWidget *vbox1 = NULL;
  GtkWidget *pw = NULL;
  GtkToolItem *button1 = NULL;
  GtkToolItem *button2 = NULL;
  GtkToolItem *button3 = NULL;
  GtkToolItem *item;

  listTitles[0] = _("User Name");
  listTitles[1] = _("User Info");
  listTitles[2] = _("Home");

  InitSpacings ();

  if (!password_only)
  {
    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox1), border_width);
	
	pw = gtk_image_new_from_pixbuf (gpe_find_icon ("lock16"));
	item = gtk_tool_button_new(pw, _("Password"));
	g_signal_connect(G_OBJECT(item), "clicked", 
	                 G_CALLBACK(password_change_clicked), NULL);
//	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
//	                     _("Change oassword."), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
    gtk_widget_show_all(GTK_WIDGET(item));

	button1 = item = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("Delete User"));
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(users_on_delete_clicked), 
	                 NULL);
//	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
//	                     _("Add a new user."), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
    gtk_widget_show(GTK_WIDGET(item));

	button2 = item = gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("Edit User"));
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(users_on_edit_clicked), 
	                 NULL);
//	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
//	                     _("Edit existing user"), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
    gtk_widget_show(GTK_WIDGET(item));
	
	button3 = item = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), _("New User"));
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(users_on_new_clicked), 
	                 NULL);
//	gtk_tooltips_set_tip(tooltips, GTK_WIDGET(item), 
//	                     _("Add a new user."), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
    gtk_widget_show(GTK_WIDGET(item));
	  
    user_list = gtk_clist_new_with_titles (3, listTitles);
    pw = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pw),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (pw), user_list);
  }
  
  InitPwList();
  if (!password_only)
    {
      ReloadList();

      gtk_widget_show (user_list);
      gtk_box_pack_start (GTK_BOX (vbox1), pw, TRUE, TRUE, 0);
  
      /* check if we have the permissions to change users */
      if (suid_exec("CHEK"," "))
        {
	      gtk_widget_set_sensitive(user_list, FALSE);
	      gtk_widget_set_sensitive(GTK_WIDGET(button1), FALSE);
	      gtk_widget_set_sensitive(GTK_WIDGET(button2), FALSE);
	      gtk_widget_set_sensitive(GTK_WIDGET(button3), FALSE);
	      have_access = FALSE;
        }
      else
        {
	      have_access = TRUE;
        }
    }
  else
	have_access = FALSE;
  
  if (password_only)
    {
      gtk_widget_show(create_passwindow(pwroot, NULL));
      return NULL; 
    }
  else
  	return vbox1;
}

GtkWidget*
create_userchange (pwlist *init,GtkWidget *parent)
{
  GtkWidget *userchange;
  GtkWidget *vbox2;
  GtkWidget *table1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *label5;
  GtkWidget *shellcombo;
  GtkWidget *passwd;
  GtkWidget *hbuttonbox2;
  GtkWidget *save;
  GtkWidget *cancel;
  userw     *self = malloc (sizeof(userw));

  /* ======================================================================== */
  /* draw the GUI */
  InitSpacings ();
  
  self->cur = init;
  self->w = userchange = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(userchange), GTK_WINDOW(parent));
  gtk_window_set_title (GTK_WINDOW (userchange), _("User settings"));
  gtk_window_set_modal (GTK_WINDOW (userchange), TRUE);

  vbox2 = GTK_DIALOG (userchange)->vbox;


  table1 = gtk_table_new (5, 2, FALSE);

  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (vbox2), table1);
  gtk_container_set_border_width (GTK_CONTAINER (table1), border_width);
  gtk_table_set_row_spacings (GTK_TABLE (table1), row_spacing);
  gtk_table_set_col_spacings (GTK_TABLE (table1), col_spacing);

  label1 = gtk_label_new (_("User Name:"));

  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

  self->username = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(self->username),init->pw.pw_name);

  gtk_widget_show (self->username);
  gtk_table_attach (GTK_TABLE (table1), self->username, 1, 2, 0, 1,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  label2 = gtk_label_new (_("Password:"));

  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  label3 = gtk_label_new (_("User Info:"));

  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  self->gecos = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(self->gecos),init->pw.pw_gecos);

  gtk_widget_show (self->gecos);
  gtk_table_attach (GTK_TABLE (table1), self->gecos, 1, 2, 2, 3,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (0), 0, 0);

  label4 = gtk_label_new (_("Shell:"));

  gtk_widget_show (label4);
  gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 3, 4,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  shellcombo = gtk_combo_new ();

  gtk_widget_show (shellcombo);
  gtk_table_attach (GTK_TABLE (table1), shellcombo, 1, 2, 3, 4,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  self->shell = GTK_COMBO (shellcombo)->entry;
  gtk_entry_set_text(GTK_ENTRY(self->shell),init->pw.pw_shell);


  label5 = gtk_label_new (_("Home:"));

  gtk_widget_show (label5);
  gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 4, 5,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

  self->home = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(self->home),init->pw.pw_dir);

  gtk_widget_show (self->home);
  gtk_table_attach (GTK_TABLE (table1), self->home, 1, 2, 4, 5,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);
  gtk_widget_show (self->shell);

  
  passwd = gpe_picture_button (NULL,
                               strlen(init->pw.pw_passwd) ? _("Change") 
							     : _("Set"), "lock16");

  gtk_widget_show (passwd);
  gtk_table_attach (GTK_TABLE (table1), passwd, 1, 2, 1, 2,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);

  hbuttonbox2 =  GTK_DIALOG (userchange)->action_area;

  cancel = gpe_button_new_from_stock(GTK_STOCK_CANCEL,GPE_BUTTON_TYPE_BOTH);
  gtk_widget_show (cancel);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), cancel);

  save = gpe_button_new_from_stock(GTK_STOCK_OK,GPE_BUTTON_TYPE_BOTH);
  GTK_WIDGET_SET_FLAGS(save, GTK_CAN_DEFAULT);
  gtk_widget_show (save);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), save);

  if(strcmp(init->pw.pw_name,"newuser"))
    {
      gtk_widget_set_sensitive(self->username,FALSE);
      gtk_widget_set_sensitive(self->home,FALSE);
    }

  gtk_widget_grab_default(save);
	
  g_signal_connect (G_OBJECT (passwd), "clicked",
                      G_CALLBACK(users_on_passwd_clicked),
                      (gpointer) self);
  g_signal_connect (G_OBJECT (save), "clicked",
                            G_CALLBACK (users_on_save_clicked),
                            (gpointer) self);
  g_signal_connect (G_OBJECT (cancel), "clicked",
                            G_CALLBACK(users_on_cancel_clicked),
                            (gpointer) self);

  /* in case of destruction by close (X) button */
  g_signal_connect (G_OBJECT(userchange) , "destroy", 
		      G_CALLBACK(freedata), (gpointer)self);
  return userchange;
}

GtkWidget*
create_passwindow (pwlist *init, GtkWidget *parent)
{
  GtkWidget *passwindow;
  GtkWidget *vbox3;
  GtkWidget *table3;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *label8;
  GtkWidget *hbuttonbox3;
  GtkWidget *cancel;
  GtkWidget *changepasswd;
  passw     *self = malloc(sizeof(passw));
  GdkPixbuf *p = gpe_find_icon ("lock");

  /* ======================================================================== */
  /* draw the GUI */
  InitSpacings ();

  self->cur = init;
  self->w = passwindow = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(passwindow), GTK_WINDOW(parent));
  gtk_window_set_title (GTK_WINDOW (passwindow), _("Change Password"));
  gtk_window_set_modal (GTK_WINDOW (passwindow), TRUE);
  vbox3 = GTK_DIALOG (passwindow)->vbox;
  
  table3 = gtk_table_new (3, 3, FALSE);

  gtk_widget_show (table3);
  gtk_container_add (GTK_CONTAINER (vbox3), table3);
  gtk_container_set_border_width (GTK_CONTAINER (table3), border_width);
  gtk_table_set_row_spacings (GTK_TABLE (table3), row_spacing);
  gtk_table_set_col_spacings (GTK_TABLE (table3), col_spacing);

  /* 1st column: */
  if(p)
    {
      GtkWidget *pixmap1 = gtk_image_new_from_pixbuf (p);
      gtk_widget_show (pixmap1);

      /* span all table rows from 0 to 3: */
      gtk_table_attach (GTK_TABLE (table3), pixmap1, 0, 1, 0, 3,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
      gtk_misc_set_alignment (GTK_MISC (pixmap1), 0.5, 0.5);
    }

  /* 2nd column: */
  label6 = gtk_label_new (_("Old Password:"));
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table3), label6, 1, 2, 0, 1,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  label7 = gtk_label_new (_("New Password:"));

  gtk_widget_show (label7);
  gtk_table_attach (GTK_TABLE (table3), label7, 1, 2, 1, 2,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);

  label8 = gtk_label_new (_("Confirm:"));

  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table3), label8, 1, 2, 2, 3,
                    (GtkAttachOptions) (table_attach_left_col_x),
                    (GtkAttachOptions) (table_attach_left_col_y), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);

  /* 3rd column: */
  self->oldpasswd = gtk_entry_new ();

  gtk_widget_show (self->oldpasswd);
  gtk_table_attach (GTK_TABLE (table3), self->oldpasswd, 2, 3, 0, 1,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (self->oldpasswd), FALSE);
  gtk_widget_set_sensitive(self->oldpasswd, strlen(init->pw.pw_passwd));

  self->newpasswd = gtk_entry_new ();

  gtk_widget_show (self->newpasswd);
  gtk_table_attach (GTK_TABLE (table3), self->newpasswd, 2, 3, 1, 2,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (self->newpasswd), FALSE);

  self->newpasswd2 = gtk_entry_new ();

  gtk_widget_show (self->newpasswd2);
  gtk_table_attach (GTK_TABLE (table3), self->newpasswd2, 2, 3, 2, 3,
                    (GtkAttachOptions) (table_attach_right_col_x),
                    (GtkAttachOptions) (table_attach_right_col_y), 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (self->newpasswd2), FALSE);



  hbuttonbox3 = GTK_DIALOG (passwindow)->action_area;

  cancel = gpe_button_new_from_stock(GTK_STOCK_CANCEL,GPE_BUTTON_TYPE_BOTH);

  gtk_widget_show (cancel);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), cancel);
  GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);

  changepasswd = gpe_button_new_from_stock(GTK_STOCK_OK,GPE_BUTTON_TYPE_BOTH);

  gtk_widget_show (changepasswd);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), changepasswd);
  GTK_WIDGET_SET_FLAGS (changepasswd, GTK_CAN_DEFAULT);

  gtk_widget_grab_default(changepasswd);
  g_signal_connect (G_OBJECT (cancel), "clicked",
                      G_CALLBACK (users_on_passwdcancel_clicked),
                      (gpointer)self);
  g_signal_connect (G_OBJECT (changepasswd), "clicked",
                      G_CALLBACK (users_on_changepasswd_clicked),
                      (gpointer)self);

  if (parent)
 	 g_signal_connect (G_OBJECT(passwindow) , "destroy", 
		      G_CALLBACK(freedata), (gpointer)self);
  else
  {
     set_own_password = TRUE;
 	 g_signal_connect (G_OBJECT(passwindow) , "destroy", 
		      G_CALLBACK(gtk_main_quit), NULL);
  }
  return passwindow;
}
