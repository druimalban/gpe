
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include <gpe/errorbox.h>
#include "../applets.h"
#define _XOPEN_SOURCE_
#include <unistd.h>




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
  ReloadList();

}
extern GtkWidget *user_list;

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
    if(cur->pw.pw_uid < MINUSERUID)
      gpe_error_box("Dont remove\n system users!");
    else
      {
	*prec = cur->next;
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
  
  
  gchar *tmp = gtk_entry_get_text(GTK_ENTRY(self->username));
  
  if(strcmp(tmp,"newuser")==0)
    {
      gpe_error_box(_("please choose a user name."));
      return ;
    }
  
  free(self->cur->pw.pw_name);
  self->cur->pw.pw_name = strdup(tmp); 

  tmp = gtk_entry_get_text(GTK_ENTRY(self->gecos));
  free(self->cur->pw.pw_gecos);
  self->cur->pw.pw_gecos = strdup(tmp);

  tmp = gtk_entry_get_text(GTK_ENTRY(self->shell));
  free(self->cur->pw.pw_shell);
  self->cur->pw.pw_shell = strdup(tmp);

  tmp = gtk_entry_get_text(GTK_ENTRY(self->home));
  if(strcmp(tmp,"/home/newuser")==0)
    {
      gpe_error_box(_("please choose a home."));
      return ;
    }

  free(self->cur->pw.pw_dir);
  self->cur->pw.pw_dir = strdup(tmp);

  gtk_widget_destroy(self->w);
  ReloadList();

}


void
users_on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  userw *self=(userw *)user_data;

  gtk_widget_destroy(self->w);

}


#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

void
users_on_changepasswd_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

  passw *self=(passw *)user_data;
  gchar *oldpasswd = gtk_entry_get_text(GTK_ENTRY(self->oldpasswd));
  gchar *newpasswd2 = gtk_entry_get_text(GTK_ENTRY(self->newpasswd2));
  gchar *newpasswd = gtk_entry_get_text(GTK_ENTRY(self->newpasswd));
  char salt[2];
  time_t tm;
  char *old_crypted_pass = self->cur->pw.pw_passwd; 

  time (&tm);
  salt[0] = bin_to_ascii (tm & 0x3f);
  salt[1] = bin_to_ascii ((tm >> 6) & 0x3f);

  if ((old_crypted_pass[0]==0 && oldpasswd[0]==0) // there was no pass!
      || strcmp(crypt(oldpasswd,old_crypted_pass ), old_crypted_pass) == 0)
    {
      if(strcmp(newpasswd,newpasswd2)==0)
	{
	  if(newpasswd[0])// we have a passwd
	    self->cur->pw.pw_passwd = strdup(crypt(newpasswd,salt));
	  else
	      self->cur->pw.pw_passwd = strdup(newpasswd); // keep it clear

	  free(old_crypted_pass);
	  gtk_widget_destroy(self->w);
	  
	}
      else
	gpe_error_box(_("The two new pass are different!\n try again!"));
    }
  else
    gpe_error_box(_("Wrong password\n try again!"));

}


void
users_on_passwdcancel_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  passw *self=(passw *)user_data;
  gtk_widget_destroy(self->w);

}

