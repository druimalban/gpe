/*
 * NE PAS ÉDITER CE FICHIER - il est généré par Glade.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include <gpe/picturebutton.h>

#include <pwd.h>
#include <sys/types.h>
#include "../applets.h"

static gchar *listTitles[] = { _("username"),_("User Info"),_("Shell"),_("Home")};
extern GtkStyle *wstyle;
GtkWidget*
Users_Build_Objects (void)
{
  GtkWidget *vbox1;
  GtkWidget *hbuttonbox1;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;
  GtkWidget *list1;
  struct passwd *pwent;
  gchar *entry[4];

  vbox1 = gtk_vbox_new (FALSE, 0);

  hbuttonbox1 = gtk_hbutton_box_new ();

  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, FALSE, 0);

  button1 = gpe_picture_button (wstyle, _("Add"), "new");

  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  button2 = gpe_picture_button (wstyle, _("Edit"), "properties");

  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button2);
  GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);

  button3 = gpe_picture_button (wstyle, _("Delete"), "delete");

  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button3);
  GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

  list1 = gtk_clist_new_with_titles (4,listTitles);
  pwent = getpwent();
 while(pwent)
    {
      if(pwent->pw_uid>500 || pwent->pw_uid ==0)
	{
	  entry[0] = g_strdup_printf("%s",pwent->pw_name);
	  entry[1] = g_strdup_printf("%s",pwent->pw_gecos);
	  entry[2] = g_strdup_printf("%s",pwent->pw_shell);
	  entry[3] = g_strdup_printf("%s",pwent->pw_dir);
	  gtk_clist_append( GTK_CLIST(list1), entry);
	  g_free(entry[0]);
	  g_free(entry[1]);
	  g_free(entry[2]);
	  g_free(entry[3]);
	}
      pwent = getpwent();
    }
  gtk_widget_show (list1);
 
 gtk_box_pack_start (GTK_BOX (vbox1), list1, TRUE, TRUE, 0);

  return vbox1;
}

GtkWidget*
create_userchange (void)
{
  GtkWidget *userchange;
  GtkWidget *vbox2;
  GtkWidget *frame1;
  GtkWidget *table1;
  GtkWidget *self_home;
  GtkWidget *label1;
  GtkWidget *self_username;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *self_userinfo;
  GtkWidget *label4;
  GtkWidget *label5;
  GtkWidget *shellcombo;
  GtkWidget *self_shell;
  GtkWidget *passwd;
  GtkWidget *hbuttonbox2;
  GtkWidget *save;
  GtkWidget *cancel;

  userchange = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (userchange), "userchange", userchange);
  gtk_window_set_title (GTK_WINDOW (userchange), _("Add user"));

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox2);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "vbox2", vbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (userchange), vbox2);

  frame1 = gtk_frame_new (_("Add user"));
  gtk_widget_ref (frame1);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "frame1", frame1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (vbox2), frame1, TRUE, TRUE, 0);

  table1 = gtk_table_new (5, 2, FALSE);
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "table1", table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (frame1), table1);

  self_home = gtk_entry_new ();
  gtk_widget_ref (self_home);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "self_home", self_home,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self_home);
  gtk_table_attach (GTK_TABLE (table1), self_home, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  label1 = gtk_label_new (_("Username"));
  gtk_widget_ref (label1);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "label1", label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

  self_username = gtk_entry_new ();
  gtk_widget_ref (self_username);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "self_username", self_username,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self_username);
  gtk_table_attach (GTK_TABLE (table1), self_username, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label2 = gtk_label_new (_("Passwd"));
  gtk_widget_ref (label2);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "label2", label2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  label3 = gtk_label_new (_("User Info"));
  gtk_widget_ref (label3);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "label3", label3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  self_userinfo = gtk_entry_new ();
  gtk_widget_ref (self_userinfo);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "self_userinfo", self_userinfo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self_userinfo);
  gtk_table_attach (GTK_TABLE (table1), self_userinfo, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label4 = gtk_label_new (_("Shell"));
  gtk_widget_ref (label4);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "label4", label4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label4);
  gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  label5 = gtk_label_new (_("home"));
  gtk_widget_ref (label5);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "label5", label5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label5);
  gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

  shellcombo = gtk_combo_new ();
  gtk_widget_ref (shellcombo);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "shellcombo", shellcombo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (shellcombo);
  gtk_table_attach (GTK_TABLE (table1), shellcombo, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  self_shell = GTK_COMBO (shellcombo)->entry;
  gtk_widget_ref (self_shell);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "self_shell", self_shell,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (self_shell);

  passwd = gtk_button_new_with_label (_("Change"));
  gtk_widget_ref (passwd);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "passwd", passwd,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (passwd);
  gtk_table_attach (GTK_TABLE (table1), passwd, 1, 2, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  hbuttonbox2 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox2);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "hbuttonbox2", hbuttonbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbuttonbox2, FALSE, FALSE, 0);

  save = gtk_button_new_with_label (_("Save"));
  gtk_widget_ref (save);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "save", save,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (save);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), save);
  GTK_WIDGET_SET_FLAGS (save, GTK_CAN_DEFAULT);

  cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_ref (cancel);
  gtk_object_set_data_full (GTK_OBJECT (userchange), "cancel", cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cancel);
  gtk_container_add (GTK_CONTAINER (hbuttonbox2), cancel);
  GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (passwd), "clicked",
                      GTK_SIGNAL_FUNC (on_passwd_clicked),
                      NULL);
  gtk_signal_connect_after (GTK_OBJECT (save), "clicked",
                            GTK_SIGNAL_FUNC (on_save_clicked),
                            NULL);
  gtk_signal_connect_after (GTK_OBJECT (cancel), "clicked",
                            GTK_SIGNAL_FUNC (on_cancel_clicked),
                            NULL);

  return userchange;
}

GtkWidget*
create_passwindow (void)
{
  GtkWidget *passwindow;
  GtkWidget *vbox3;
  GtkWidget *frame2;
  GtkWidget *table3;
  GtkWidget *button7;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *label8;
  GtkWidget *entry4;
  GtkWidget *entry5;
  GtkWidget *entry6;
  GtkWidget *hbuttonbox3;
  GtkWidget *cancel;
  GtkWidget *changepasswd;

  passwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (passwindow), "passwindow", passwindow);
  gtk_window_set_title (GTK_WINDOW (passwindow), _("Change passwd"));

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (passwindow), vbox3);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_ref (frame2);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "frame2", frame2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox3), frame2, TRUE, TRUE, 0);

  table3 = gtk_table_new (3, 3, FALSE);
  gtk_widget_ref (table3);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "table3", table3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table3);
  gtk_container_add (GTK_CONTAINER (frame2), table3);

  button7 = gtk_button_new_with_label (_("button7"));
  gtk_widget_ref (button7);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "button7", button7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button7);
  gtk_table_attach (GTK_TABLE (table3), button7, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label6 = gtk_label_new (_("Old Passwd"));
  gtk_widget_ref (label6);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "label6", label6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table3), label6, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  label7 = gtk_label_new (_("New Passwd"));
  gtk_widget_ref (label7);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "label7", label7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label7);
  gtk_table_attach (GTK_TABLE (table3), label7, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);

  label8 = gtk_label_new (_("Confirm"));
  gtk_widget_ref (label8);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "label8", label8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table3), label8, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);

  entry4 = gtk_entry_new ();
  gtk_widget_ref (entry4);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "entry4", entry4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry4);
  gtk_table_attach (GTK_TABLE (table3), entry4, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (entry4), FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry4), _("gfj"));

  entry5 = gtk_entry_new ();
  gtk_widget_ref (entry5);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "entry5", entry5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry5);
  gtk_table_attach (GTK_TABLE (table3), entry5, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_text (GTK_ENTRY (entry5), _("fgh"));

  entry6 = gtk_entry_new ();
  gtk_widget_ref (entry6);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "entry6", entry6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry6);
  gtk_table_attach (GTK_TABLE (table3), entry6, 2, 3, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_text (GTK_ENTRY (entry6), _("fgh"));

  hbuttonbox3 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox3);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "hbuttonbox3", hbuttonbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox3);
  gtk_box_pack_start (GTK_BOX (vbox3), hbuttonbox3, FALSE, TRUE, 0);

  cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_ref (cancel);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "cancel", cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cancel);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), cancel);
  GTK_WIDGET_SET_FLAGS (cancel, GTK_CAN_DEFAULT);

  changepasswd = gtk_button_new_with_label (_("Ok"));
  gtk_widget_ref (changepasswd);
  gtk_object_set_data_full (GTK_OBJECT (passwindow), "changepasswd", changepasswd,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (changepasswd);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), changepasswd);
  GTK_WIDGET_SET_FLAGS (changepasswd, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (cancel), "clicked",
                      GTK_SIGNAL_FUNC (on_cancel_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (changepasswd), "clicked",
                      GTK_SIGNAL_FUNC (on_changepasswd_clicked),
                      NULL);

  return passwindow;
}

