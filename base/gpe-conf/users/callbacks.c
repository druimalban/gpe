#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include <gpe/errorbox.h>




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
  gtk_widget_show(create_userchange (cur));
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
    gtk_widget_show(create_userchange (cur));
    
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
  gtk_widget_show(create_passwindow (self->cur));
}


void
users_on_save_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
  userw *self=(userw *)user_data;

}


void
users_on_cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
  userw *self=(userw *)user_data;

}


void
users_on_changepasswd_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

  passw *self=(passw *)user_data;

}


void
users_on_passwdcancel_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  passw *self=(passw *)user_data;

}

