/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003,2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * User manager module.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h>

#include "callbacks.h"
#include "interface.h"
#include <gpe/errorbox.h>
#include <gpe/question.h>
#include "../applets.h"
#include <unistd.h>
#include <crypt.h>
#include <time.h>

extern GtkWidget *user_list;
gboolean set_own_password = FALSE;

/************** main dialog *****************/

void
users_on_new_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  pwlist **prec = &pwroot;
  pwlist *cur = pwroot;
  int id=MINUSERUID;

  while(cur)
    {
      if(id == cur->pw.pw_uid)
	id = cur->pw.pw_uid + 1;

      prec = &cur->next;
      cur = cur ->next;
    }
  cur = malloc(sizeof(pwlist));
  *prec = cur;
  cur->pw.pw_name = strdup("newuser");
  cur->pw.pw_uid = id;
  cur->pw.pw_gid = id;
  cur->pw.pw_passwd = strdup("");
  cur->pw.pw_gecos = strdup("User Name");
  cur->pw.pw_dir = strdup("/home/newuser");
  cur->pw.pw_shell = strdup("/bin/sh");
  cur->next = 0;
  gtk_widget_show(create_userchange (cur,mainw));
}

void
users_on_edit_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  GList *tmp;
  for (tmp = GTK_CLIST(user_list)->selection; tmp; tmp = tmp->next)
  {
    pwlist *cur = pwroot;
    uint i=GPOINTER_TO_UINT(tmp->data);
    while(IsHidden(cur))
	cur =cur->next;
    while(i>0)
      {
        cur =cur->next;
        while(IsHidden(cur))
        cur =cur->next;
        i--;
      }
    gtk_widget_show(create_userchange (cur,mainw));
  }
}

void
users_on_delete_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  GList *tmp;
  for (tmp = GTK_CLIST(user_list)->selection; tmp; tmp = tmp->next)
  {
    pwlist *cur = pwroot;
    pwlist **prec = &pwroot;
    uint i=GPOINTER_TO_UINT(tmp->data);
	uint usercount = 0;
	
	while (cur)
	 {
		 if ((cur->pw.pw_uid >= MINUSERUID) && (cur->pw.pw_uid < MAXUSERID))
		   usercount++;
	     cur = cur->next;
	 }

	if (usercount < 2)
	  {
        gpe_error_box(_("You need at least one user account!"));
		return;
	  }
	cur = pwroot;
	  
    while(IsHidden(cur))
      {
        prec = &cur->next;
        cur =cur->next;
      }
    while(i>0)
      {
        prec = &cur->next;
        cur =cur->next;
        while(IsHidden(cur))
		{
	      prec = &cur->next;
	      cur =cur->next;
        }
       i--;
      }
	  
	if (!strcmp(cur->pw.pw_name, "lx"))
	  {
        gpe_error_box(_("You can't remove this user!"));
		return;
	  }
	  
    if(cur->pw.pw_uid < MINUSERUID)
      gpe_error_box(_("You can't remove\n system users!"));
    else
  		if (gpe_question_ask (_("Delete user and all its data?\nThis action is not revertable\nand is executed instantly."), _("Question"), "question",
		   "!gtk-no", NULL, "!gtk-yes", NULL, NULL))
      	{
			*prec = cur->next;
     		suid_exec("DHDI",strrchr(cur->pw.pw_dir,'/'));
			#warning todo: queue this and wait until user accepts changes.
			free(cur);
      	}
  }
  ReloadList();
}



void
users_on_passwd_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  userw *self=(userw *)user_data;
  gtk_widget_show(create_passwindow (self->cur,mainw));
}


void
users_on_save_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
  userw *self=(userw *)user_data;
  
  gchar *tmp = NULL;
  const gchar *homedir;
  const gchar *user = gtk_entry_get_text(GTK_ENTRY(self->username));
  
  if(strcmp(user,"newuser")==0)
    {
      gpe_error_box(_("Please choose a user name."));
      return ;
    }
  
  free(self->cur->pw.pw_name);
  self->cur->pw.pw_name = strdup(user); 

  tmp = strdup(gtk_entry_get_text(GTK_ENTRY(self->gecos)));
  free(self->cur->pw.pw_gecos);
  self->cur->pw.pw_gecos = tmp;

  tmp = strdup(gtk_entry_get_text(GTK_ENTRY(self->shell)));
  free(self->cur->pw.pw_shell);
  self->cur->pw.pw_shell = tmp;

  homedir = gtk_entry_get_text(GTK_ENTRY(self->home));
  if(strcmp(homedir,"/home/newuser")==0)
    {
      gpe_error_box(_("Please choose a home directory."));
      return ;
    }
  else // create dir later, user maybe doesn't exist
    {
    }

  free(self->cur->pw.pw_dir);
  self->cur->pw.pw_dir = strdup(homedir);

  gtk_widget_destroy(self->w);
  ReloadList();
}

void
delete_newuser(void)
{
  pwlist *cur = pwroot;
  pwlist *prec = NULL;

  while(cur)
    {
	  if (!strcmp(cur->pw.pw_name, "newuser"))
	    {
			if (prec) 
				prec->next = cur->next;
			free(cur);
			break;
		}
	  prec = cur;
      cur = cur ->next;
    }
}


void
users_on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  userw *self=(userw *)user_data;
  delete_newuser();
  gtk_widget_destroy(self->w);
}


#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

void
users_on_changepasswd_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

  passw *self=(passw *)user_data;
  gchar *oldpasswd = g_strdup(gtk_entry_get_text(GTK_ENTRY(self->oldpasswd)));
  gchar *newpasswd2 = g_strdup(gtk_entry_get_text(GTK_ENTRY(self->newpasswd2)));
  gchar *newpasswd = g_strdup(gtk_entry_get_text(GTK_ENTRY(self->newpasswd)));
  char salt[2];
  time_t tm;
  char *old_crypted_pass = self->cur->pw.pw_passwd; 

  time (&tm);
  salt[0] = bin_to_ascii (tm & 0x3f);
  salt[1] = bin_to_ascii ((tm >> 6) & 0x3f);

  if ((old_crypted_pass[0]==0 && oldpasswd[0]==0) // there was no pass!
      || strcmp(crypt(oldpasswd,old_crypted_pass ), old_crypted_pass) == 0)
    {
      if(strcmp(newpasswd, newpasswd2) == 0)
		{
			if ((newpasswd[0]) && strlen(newpasswd)) /* we have a passwd */
				self->cur->pw.pw_passwd = strdup(crypt(newpasswd,salt));
			else
				self->cur->pw.pw_passwd = strdup("");
			free(old_crypted_pass);
			if (set_own_password)
			{
				gchar *pwds = g_strdup_printf("%s %s\n",
				                              g_get_user_name(),
				                              strlen(self->cur->pw.pw_passwd)
				                                ? self->cur->pw.pw_passwd 
				                                : "*");
				suid_exec("CHPW", pwds);
				g_free(pwds);
				sleep(5);
			}
			gtk_widget_destroy(self->w);
		}
      else
        gpe_error_box(_("The two new pass are different!\n Please try again!"));
    }
  else
    gpe_error_box(_("Wrong password\n Please try again!"));
}


void
users_on_passwdcancel_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  passw *self=(passw *)user_data;
  gtk_widget_destroy(self->w);
}

void
password_change_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	pwlist *cur = pwroot;
 	while (cur)
 	{
		if (!strcmp(g_get_user_name(),cur->pw.pw_name))
		{
			set_own_password = TRUE;
    		gtk_widget_show(create_passwindow (cur, mainw));
			break;
		}
		cur = cur->next;
	}
}
