/*
 * gpe-packages
 *
 * Copyright (C) 2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE package manager module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stropts.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <ipkglib.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "packages.h"
#include "interface.h"
#include "main.h"

/* --- module global variables --- */

static GtkWidget *notebook;
static GtkWidget *txLog, *txLog2;
static GtkWidget *bUpdate, *bInstall, *bRemove;
static GtkWidget *ePackage;
int sock;
void create_fMain (void);
static GtkWidget *fMain;


struct gpe_icon my_icons[] = {
  { "save" },
  { "cancel" },
  { "delete" },
  { "properties" },
  { "new" },
  { "lock" },
  { "exit" }
};

/* message send and receive */

static void
send_message (pkcontent_t ctype, pkcommand_t command, char* params, char* list)
{
	pkmessage_t msg;
	msg.type = PK_BACK;
	msg.ctype = ctype;
	msg.content.tb.command = command;
	snprintf(msg.content.tb.params,LEN_PARAMS,params);
	snprintf(msg.content.tb.list,LEN_LIST,list);
	if (write (sock, (void *) &msg, sizeof (pkmessage_t)) < 0)
	{
		perror ("err sending config data");
	}
}


void printlog(GtkWidget *textview, gchar *str)
{
	GtkTextBuffer* log;
	log = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(log),str,-1);
}


/* --- local intelligence --- */
void do_question(int nr, char *question)
{
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (fMain),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 question);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
		send_message(PK_REPLY,CMD_NONE,"y","");
	else
		send_message(PK_REPLY,CMD_NONE,"n","");
	gtk_widget_destroy(dialog);
}


void do_list(int prio,char* pkg,char *desc)
{
	printlog(txLog,pkg);
}


void do_error(char *msg)
{
	GtkWidget *dialog;
	printlog(txLog,msg);
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (fMain),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 msg);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);	
}


void do_info(int priority, char *str1, char *str2)
{
	printlog(txLog,str1);
	printlog(txLog,str2);	
}


void do_end_command()
{
    gtk_button_set_label(GTK_BUTTON(bUpdate),_("Start"));
    gtk_widget_set_sensitive(bInstall,TRUE);
    gtk_widget_set_sensitive(bUpdate,TRUE);
    gtk_widget_set_sensitive(bRemove,TRUE);
    printlog(txLog,_("Update finished. Please check log messages for errors."));

    gtk_widget_set_sensitive(bInstall,TRUE);
    gtk_widget_set_sensitive(bUpdate,TRUE);
    gtk_widget_set_sensitive(bRemove,TRUE);
    gtk_button_set_label(GTK_BUTTON(bInstall),_("Install"));
    gtk_button_set_label(GTK_BUTTON(bRemove),_("Remove"));
    printlog(txLog2,_("Install/Removal finished. Please check log messages for errors."));
}


gboolean
get_pending_messages ()
{
	static pkmessage_t msg;
	struct pollfd pfd[1];
	pfd[0].fd = sock;
	pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
	while (poll (pfd, 1, 0) > 0)
	{
		if ((pfd[0].revents & POLLERR) || (pfd[0].revents & POLLHUP))
		{
#ifdef DEBUG
			perror ("Err: connection lost: ");
#endif
			return TRUE;
		}
		if (read (sock, (void *) &msg, sizeof (pkmessage_t)) < 0)
		{
			perror ("err receiving data packet");
			close (sock);
#warning todo
			exit (1);
		}
		else
		if (msg.type == PK_FRONT)
		{
			switch (msg.ctype)
			{
			case PK_QUESTION:
				do_question(msg.content.tf.priority, msg.content.tf.str1);
			break;
			case PK_ERROR:
				do_error(msg.content.tf.str1);
			break;	
			case PK_LIST:
				do_list(msg.content.tf.priority, msg.content.tf.str1,msg.content.tf.str2);
			break;	
			case PK_INFO:
				do_info(msg.content.tf.priority, msg.content.tf.str1,msg.content.tf.str2);
			break;	
			case PK_PACKAGESTATE:
//				do_state(msg.content.tf.priority, msg.content.tf.str1,msg.content.tf.str2);
			case PK_FINISHED:
				do_end_command();
			default:
				break;
			}
		}
	}

	return TRUE;
}


/* app mainloop */

int
mainloop (int argc, char *argv[])
{
	struct sockaddr_un name;
sleep(1);
/*	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);
*/
	
	/* Create socket from which to read. */
	sock = socket (AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror ("opening datagram socket");
		exit (1);
	}

	/* Create name. */
	name.sun_family = AF_UNIX;
	strcpy (name.sun_path, PK_SOCKET);
	if (connect (sock, (struct sockaddr *) &name, SUN_LEN (&name)))
	{
		perror ("connecting to socket");
		exit (1);
	}

	printf ("socket -->%s\n", PK_SOCKET);

	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);

	create_fMain ();
	gtk_widget_show (fMain);
	
	gtk_timeout_add(500,get_pending_messages,NULL);
	
	gtk_main ();

	close (sock);

	return 0;
}


/* 
 * Checks if the given package is installed.
 */
int do_package_check(const char *package)
{
/*	char s1[100], s2[100], s3[100];
	char *command = g_strdup_printf("/usr/bin/ipkg status %s",package);
	if (parse_pipe(command,"Status: %s %s %s",s1,s2,s3))
	{
		return FALSE;
	}
	else		
	{
		printf("s1: %s, s2: %s, s3: %s\n",s1,s2,s3);
		if (!strcmp(s1,"install") && !strcmp(s2,"ok") && !strcmp(s3,"installed"))
			return TRUE;
		else
			return FALSE;
	}
*/	
}


void on_network_update_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextBuffer *logbuf;
  GtkTextIter start,end;
	
  gtk_button_set_label(button,_("Running..."));
  gtk_widget_set_sensitive(bUpdate,FALSE);
  gtk_widget_set_sensitive(bInstall,FALSE);
  gtk_widget_set_sensitive(bRemove,FALSE);
	
  /* clear log */	
  logbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txLog));
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(logbuf),&start);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(logbuf),&end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(logbuf),&start,&end);
  send_message(PK_COMMAND,CMD_UPDATE,"","");
  send_message(PK_COMMAND,CMD_UPGRADE,"-force-depends","");
}


void on_package_install_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextBuffer *logbuf;
  GtkTextIter start,end;
  gchar *pname;
	
  gtk_button_set_label(button,_("Running..."));
  gtk_widget_set_sensitive(bInstall,FALSE);
  gtk_widget_set_sensitive(bUpdate,FALSE);
  gtk_widget_set_sensitive(bRemove,FALSE);
	
  /* clear log */	
  logbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txLog2));
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(logbuf),&start);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(logbuf),&end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(logbuf),&start,&end);

  pname = gtk_editable_get_chars(GTK_EDITABLE(ePackage),0,-1);

  if (user_data) // install or remove?
  {
    send_message(PK_COMMAND,CMD_REMOVE,"",pname);
  }
  else
  {	
    send_message(PK_COMMAND,CMD_INSTALL,"",pname);
  }

  g_free(pname);	
}


/* --- gpe-conf interface --- */


void
create_fMain (void)
{
  GtkWidget *vbox;
  GtkWidget *cur;
  GtkTooltips *tooltips;
  char *tmp;

  fMain = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (fMain), _("GPE Package"));
  gtk_window_set_default_size (GTK_WINDOW (fMain), 240, 300);
  gtk_window_set_policy (GTK_WINDOW (fMain), TRUE, TRUE, FALSE);

	
  tooltips = gtk_tooltips_new ();
	
  notebook = gtk_notebook_new();	
  gtk_container_add(GTK_CONTAINER(fMain),notebook);
	
  gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_get_border ());
  
  gtk_object_set_data(GTK_OBJECT(notebook),"tooltips",tooltips);

  /* update tab */	
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());

  cur = gtk_label_new(_("Update"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,cur);

  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Update from network"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	
	
  cur = gtk_button_new_with_label(_("Start"));
  bUpdate = cur;
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,FALSE,gpe_get_boxspacing());	
  g_signal_connect(G_OBJECT (cur), "clicked",G_CALLBACK(on_network_update_clicked),NULL);
  gtk_tooltips_set_tip (tooltips, cur, _("Update entire system over an internet connection."), NULL);
  
  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Activity log"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	
  
  cur = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cur),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cur),GTK_SHADOW_IN);
  gtk_tooltips_set_tip (tooltips, cur, _("This window shows all output from the packet manager that performs the update."), NULL);
  
  txLog = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txLog),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(cur),txLog);
  gtk_box_pack_start_defaults(GTK_BOX(vbox),cur);	
   
  /* install tab */
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());

  cur = gtk_label_new(_("Install/Remove"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,cur);

  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Install/Remove a package"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	

  ePackage = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(vbox),ePackage,FALSE,TRUE,0);	
  gtk_tooltips_set_tip (tooltips, ePackage, _("Enter name of package to be installed here."), NULL);
  
  cur = gtk_button_new_with_label(_("Install"));
  bInstall = cur;
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,FALSE,gpe_get_boxspacing());	
  g_signal_connect(G_OBJECT (cur), "clicked",G_CALLBACK(on_package_install_clicked),(gpointer)FALSE);
  gtk_tooltips_set_tip (tooltips, cur, _("Install a package from a configured source."), NULL);
  
  cur = gtk_button_new_with_label(_("Remove"));
  bRemove = cur;
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,FALSE,gpe_get_boxspacing());	
  g_signal_connect(G_OBJECT (cur), "clicked",G_CALLBACK(on_package_install_clicked),(gpointer)TRUE);
  gtk_tooltips_set_tip (tooltips, cur, _("Remove the package."), NULL);
  
  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Activity log"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	
  
  cur = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cur),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cur),GTK_SHADOW_IN);
  gtk_tooltips_set_tip (tooltips, cur, _("This window shows all output from the packet manager installing the package."), NULL);
  
  txLog2 = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txLog2),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(cur),txLog2);
  gtk_box_pack_start_defaults(GTK_BOX(vbox),cur);	
  
  g_signal_connect(G_OBJECT (fMain),"destroy",gtk_main_quit,NULL);
  
  gtk_widget_show_all(fMain);
}
