/*
 * NE PAS ÉDITER CE FICHIER - il est généré par Glade.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include <gpe/picturebutton.h>

#include "../applets.h"
#include "gpe/pixmaps.h"
#include "gpe/render.h"

static gchar *listTitles[] = { _("username"),_("User Info"),_("Shell"),_("Home")};

pwlist *pwroot = NULL;
int IsHidden(pwlist *cur)
{
  return     !((cur->pw.pw_uid>=MINUSERUID && cur->pw.pw_uid<65534) || cur->pw.pw_uid ==0);

}
void InitPwList()
{
  struct passwd *pwent;
  pwlist **prec = &pwroot;
  pwlist *cur;

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
}

GtkWidget *user_list;
 
void ReloadList()
{
  pwlist *cur = pwroot;
  gchar *entry[4];
  gtk_clist_clear(GTK_CLIST(user_list));
  while(cur != NULL)
  {
    
    if(! IsHidden(cur))
	{
	  entry[0] = g_strdup_printf("%s",cur->pw.pw_name);
	  entry[1] = g_strdup_printf("%s",cur->pw.pw_gecos);
	  entry[2] = g_strdup_printf("%s",cur->pw.pw_shell);
	  entry[3] = g_strdup_printf("%s",cur->pw.pw_dir);
	  gtk_clist_append( GTK_CLIST(user_list), entry);
	  g_free(entry[0]);
	  g_free(entry[1]);
	  g_free(entry[2]);
	  g_free(entry[3]);
	}
      cur = cur->next;
    }
  gtk_clist_columns_autosize      ((GtkCList *)user_list);
}

GtkWidget*
Users_Build_Objects (void)
{
  GtkWidget *vbox1;
  GtkWidget *hbuttonbox1;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;
  GtkWidget *scrw;

  vbox1 = gtk_vbox_new (FALSE, 0);

  hbuttonbox1 = gtk_hbutton_box_new ();

  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, FALSE, 0);

  button1 = gpe_picture_button (wstyle, _("Add"), "new");

  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button1);

  button2 = gpe_picture_button (wstyle, _("Edit"), "properties");

  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button2);

  button3 = gpe_picture_button (wstyle, _("Delete"), "delete");

  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button3);

  scrw = gtk_scrolled_window_new(NULL,NULL);

  user_list = gtk_clist_new_with_titles (4,listTitles);
  InitPwList();
  ReloadList();

 gtk_widget_show (user_list);
 
 gtk_box_pack_start (GTK_BOX (vbox1), scrw, TRUE, TRUE, 0);
 gtk_container_add (GTK_CONTAINER (scrw), user_list);

 gtk_signal_connect_after (GTK_OBJECT (button1), "clicked",
			   GTK_SIGNAL_FUNC (users_on_new_clicked),
			   NULL);

 gtk_signal_connect_after (GTK_OBJECT (button2), "clicked",
			   GTK_SIGNAL_FUNC (users_on_edit_clicked),
			   NULL);
 gtk_signal_connect_after (GTK_OBJECT (button3), "clicked",
			   GTK_SIGNAL_FUNC (users_on_delete_clicked),
			   NULL);

  return vbox1;
}

GtkWidget*
create_userchange (pwlist *init)
{
  GtkWidget *userchange;
  GtkWidget *vbox2;
  GtkWidget *frame1;
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

  userchange = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (userchange), _("Add user"));
  gtk_window_set_modal (GTK_WINDOW (userchange), TRUE);

  vbox2 = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (userchange), vbox2);

  frame1 = gtk_frame_new (_("Add user"));

  gtk_widget_show (frame1);

  gtk_box_pack_start (GTK_BOX (vbox2), frame1, TRUE, TRUE, 0);

  table1 = gtk_table_new (5, 2, FALSE);

  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (frame1), table1);

  label1 = gtk_label_new (_("Username"));

  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

  self->username = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(self->username),init->pw.pw_name);

  gtk_widget_show (self->username);
  gtk_table_attach (GTK_TABLE (table1), self->username, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label2 = gtk_label_new (_("Passwd"));

  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  label3 = gtk_label_new (_("User Info"));

  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  self->gecos = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(self->gecos),init->pw.pw_gecos);

  gtk_widget_show (self->gecos);
  gtk_table_attach (GTK_TABLE (table1), self->gecos, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label4 = gtk_label_new (_("Shell"));

  gtk_widget_show (label4);
  gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  shellcombo = gtk_combo_new ();

  gtk_widget_show (shellcombo);
  gtk_table_attach (GTK_TABLE (table1), shellcombo, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  self->shell = GTK_COMBO (shellcombo)->entry;
  gtk_entry_set_text(GTK_ENTRY(self->shell),init->pw.pw_shell);


  label5 = gtk_label_new (_("home"));

  gtk_widget_show (label5);
  gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

  self->home = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(self->home),init->pw.pw_dir);

  gtk_widget_show (self->home);
  gtk_table_attach (GTK_TABLE (table1), self->home, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_widget_show (self->shell);

  passwd = gpe_picture_button (wstyle,_("Change"),"lock");

  gtk_widget_show (passwd);
  gtk_table_attach (GTK_TABLE (table1), passwd, 1, 2, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  hbuttonbox2 = gtk_hbutton_box_new ();

  gtk_widget_show (hbuttonbox2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbuttonbox2, FALSE, FALSE, 0);

  save = gpe_picture_button (wstyle,_("Ok"),"ok");

  gtk_widget_show (save);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), save);
  GTK_WIDGET_SET_FLAGS (save, GTK_CAN_DEFAULT);

  cancel = gpe_picture_button (wstyle,_("Cancel"),"cancel");

  gtk_widget_show (cancel);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), cancel);

  if(strcmp(init->pw.pw_name,"newuser"))
    {
      gtk_entry_set_editable(GTK_ENTRY(self->username),FALSE);
      gtk_entry_set_editable(GTK_ENTRY(self->home),FALSE);
    }

  gtk_signal_connect (GTK_OBJECT (passwd), "clicked",
                      GTK_SIGNAL_FUNC (users_on_passwd_clicked),
                      (gpointer) self);
  gtk_signal_connect (GTK_OBJECT (save), "clicked",
                            GTK_SIGNAL_FUNC (users_on_save_clicked),
                            (gpointer) self);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked",
                            GTK_SIGNAL_FUNC (users_on_cancel_clicked),
                            (gpointer) self);

  gtk_signal_connect (GTK_OBJECT(userchange) , "destroy", 
		      (GtkSignalFunc) freedata, (gpointer)self); // in case of destruction by close (X) button

  return userchange;
}

GtkWidget*
create_passwindow (pwlist *init)
{
  GtkWidget *passwindow;
  GtkWidget *vbox3;
  GtkWidget *frame2;
  GtkWidget *table3;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *label8;
  GtkWidget *hbuttonbox3;
  GtkWidget *cancel;
  GtkWidget *changepasswd;
  passw     *self = malloc(sizeof(passw));
  GdkPixbuf *p = gpe_find_icon ("lock");

  passwindow = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (passwindow), _("Change passwd"));
  gtk_window_set_modal (GTK_WINDOW (passwindow), TRUE);

  vbox3 = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (passwindow), vbox3);

  frame2 = gtk_frame_new (NULL);

  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox3), frame2, TRUE, TRUE, 0);

  table3 = gtk_table_new (3, 3, FALSE);

  gtk_widget_show (table3);
  gtk_container_add (GTK_CONTAINER (frame2), table3);

  if(p)
    {
      GtkWidget *pixmap1 = gpe_render_icon (wstyle, p);
      gtk_widget_show (pixmap1);

      gtk_table_attach (GTK_TABLE (table3), pixmap1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
    }
  label6 = gtk_label_new (_("Old Passwd"));

  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table3), label6, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  label7 = gtk_label_new (_("New Passwd"));

  gtk_widget_show (label7);
  gtk_table_attach (GTK_TABLE (table3), label7, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);

  label8 = gtk_label_new (_("Confirm"));

  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table3), label8, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);

  self->oldpasswd = gtk_entry_new ();

  gtk_widget_show (self->oldpasswd);
  gtk_table_attach (GTK_TABLE (table3), self->oldpasswd, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (self->oldpasswd), FALSE);

  self->newpasswd = gtk_entry_new ();

  gtk_widget_show (self->newpasswd);
  gtk_table_attach (GTK_TABLE (table3), self->newpasswd, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  self->newpasswd2 = gtk_entry_new ();

  gtk_widget_show (self->newpasswd2);
  gtk_table_attach (GTK_TABLE (table3), self->newpasswd2, 2, 3, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  hbuttonbox3 = gtk_hbutton_box_new ();

  gtk_widget_show (hbuttonbox3);
  gtk_box_pack_start (GTK_BOX (vbox3), hbuttonbox3, FALSE, TRUE, 0);

  cancel = gpe_picture_button (wstyle,_("Cancel"),"cancel");

  gtk_widget_show (cancel);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), cancel);
  GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);

  changepasswd = gpe_picture_button (wstyle,_("Ok"),"ok");

  gtk_widget_show (changepasswd);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), changepasswd);
  GTK_WIDGET_SET_FLAGS (changepasswd, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (cancel), "clicked",
                      GTK_SIGNAL_FUNC (users_on_passwdcancel_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (changepasswd), "clicked",
                      GTK_SIGNAL_FUNC (users_on_changepasswd_clicked),
                      NULL);

  gtk_signal_connect (GTK_OBJECT(passwindow) , "destroy", 
		      (GtkSignalFunc) freedata, (gpointer)self); // in case of destruction by close (X) button

  return passwindow;
}

