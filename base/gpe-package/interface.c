/*
 * gpe-package
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

#include <libipkg.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "packages.h"
#include "interface.h"
#include "main.h"
#include "filesel.h"

/* --- module global variables --- */
void create_fMain (void);
int sock;
static pkcommand_t running_command = CMD_NONE;
static char *last_pkgname = NULL;
static GtkTreeIter last_pkgiter;
static int pkg_selection_changed = 0;
static int error = 0;

/* --- global widgets --- */
static GtkWidget *notebook;
static GtkWidget *txLog;
static GtkWidget *treeview;
static GtkTreeStore *store = NULL;
static GtkWidget *bUpdate, *bApply, *bSysUpgrade, *bSelectLocal;
static GtkWidget *sbar;
static GtkWidget *fMain;
static GtkWidget *dlgAction = NULL;


struct gpe_icon my_icons[] = {
  { "save" },
  { "cancel" },
  { "delete" },
  { "properties" },
  { "exit" },
  { "refresh" },
  { "local-package", "local-package-16" },
  { "icon", PREFIX "/share/pixmaps/gpe-packages.png" }
};


gboolean get_pending_messages ();


/* dialogs */
#warning todo panel
#warning todo separation

GtkWidget *
progress_dialog (gchar * text, GdkPixbuf * pixbuf)
{
	GtkWidget *window;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *hbox;
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	hbox = gtk_hbox_new (FALSE, 0);
	image = gtk_image_new_from_pixbuf (pixbuf);
	label = gtk_label_new (text);

	gtk_window_set_type_hint (GTK_WINDOW (window),
				  GDK_WINDOW_TYPE_HINT_DIALOG);

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

	gtk_container_set_border_width (GTK_CONTAINER (hbox),
					gpe_get_border ());

	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (window), hbox);

	return window;
}


/* message send and receive */

static void
send_message (pkcontent_t ctype, pkcommand_t command, char* params, char* list)
{
	pkmessage_t msg;
	char *desc;

	msg.type = PK_BACK;
	msg.ctype = ctype;
	
	/* handle commands */
	if (msg.ctype == PK_COMMAND)
	{
		running_command = command;
		switch (command)
		{
			case CMD_LIST:
				desc = _("Reading packages list...");
				gtk_tree_store_clear(GTK_TREE_STORE(store));
			break;
			case CMD_UPDATE:
				desc = _("Updating package lists");
			break;
			case CMD_UPGRADE:
				desc = _("Upgrading installed system");
			break;
			case CMD_INSTALL:
				desc = g_strdup_printf("%s %s", _("Installing"), list);
			break;
			default:
				desc = _("Working...");			
			break;
		}
		if (!dlgAction)
			dlgAction = progress_dialog(desc,gpe_find_icon("icon"));			
		gtk_widget_show_all(dlgAction);
	}
	msg.content.tb.command = command;
	snprintf(msg.content.tb.params,LEN_PARAMS,params);
	snprintf(msg.content.tb.list,LEN_LIST,list);
	if (write (sock, (void *) &msg, sizeof (pkmessage_t)) < 0)
	{
		perror ("ERR: sending data to backend");
	}
}


void printlog(GtkWidget *textview, gchar *str)
{
	GtkTextBuffer* log;
	log = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(log),str,-1);
}


/* --- local intelligence --- */

void 
wait_command_finish()
{
	while (running_command != CMD_NONE)
	{
		gtk_main_iteration();
		gtk_main_iteration();
		usleep(100000);
		get_pending_messages();
	}
}


gboolean
update_check(GtkTreeModel *model, GtkTreePath *path,
             GtkTreeIter *iter,
             gpointer data)
{
	pkg_state_want_t dstate;
	char *name;

	gtk_tree_model_get (GTK_TREE_MODEL(store), iter, 
						COL_DESIREDSTATE, &dstate,
						COL_NAME,&name,	-1);
	switch (dstate)
	{
		case SW_INSTALL:
			send_message(PK_COMMAND,CMD_INSTALL,"",name);
			wait_command_finish();
			gtk_tree_store_set (GTK_TREE_STORE(store), iter, 
                      COL_INSTALLED,TRUE,
                      COL_DESIREDSTATE,SW_UNKNOWN,
                      COL_COLOR,NULL,-1);
		break;
		case SW_DEINSTALL:
			send_message(PK_COMMAND,CMD_REMOVE,"",name);
			wait_command_finish();
			gtk_tree_store_set (GTK_TREE_STORE(store), iter, 
                      COL_INSTALLED,FALSE,
                      COL_DESIREDSTATE,SW_UNKNOWN,
                      COL_COLOR,NULL,-1);
		break;
		default:
		break;
	}	
	return FALSE;
}


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


void do_list(int prio,char* pkgname,char *desc, char *version, pkg_state_status_t status)
{
	GtkTreeIter iter;
	gboolean isinstalled = FALSE;
	gboolean updatev;
	char *color = NULL;
	char *lversion;
	char *nversion = NULL;
	
	updatev = (last_pkgname != NULL) && !strcmp(pkgname,last_pkgname);
	
	if (updatev) 
	{
		iter = last_pkgiter;
		gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,
	                       COL_VERSION, &lversion, 
		                   COL_INSTALLED, &isinstalled, 
		                   COL_COLOR, &color, -1);
		nversion = g_strdup_printf("%s\n%s",lversion,version);
		version = nversion;
	}
	else
	{
		if (last_pkgname != NULL) 
			g_free(last_pkgname);
		last_pkgname = g_strdup(pkgname);
	}
	
	if ((status != SS_INSTALLED) && (status != SS_NOT_INSTALLED))
		color = C_INCOMPLETE;
	isinstalled = isinstalled || (status==SS_INSTALLED);
	
	switch (running_command)
	{
		case CMD_LIST:
			if (!updatev)
			{				
				gtk_tree_store_append (store, &iter, NULL);
				last_pkgiter = iter;
			}
			gtk_tree_store_set (store, &iter,
			    COL_NAME, pkgname,
			    COL_DESCRIPTION, desc,
				COL_VERSION, version,
				COL_INSTALLED, isinstalled,
				COL_DESIREDSTATE, SW_UNKNOWN,
				COL_COLOR, color,
			    -1);
		break;
		default:
			printlog(txLog,pkgname);
	}
	if (updatev) g_free(nversion);
}


void do_message_dlg(int type,char *msg)
{
	GtkWidget *dialog;
	printlog(txLog,msg);
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (fMain),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 type,
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
    gtk_widget_set_sensitive(bUpdate,TRUE);
    gtk_widget_set_sensitive(bApply,TRUE);
	gtk_widget_set_sensitive(bSysUpgrade,TRUE);
    printlog(txLog,_("Command finished. Please check log messages for errors."));
	if (dlgAction) 
	{
		gtk_widget_destroy(dlgAction);
		dlgAction = NULL;
	}
	running_command = CMD_NONE;
}


void do_safe_exit()
{
	GtkWidget *dialog;

	if (pkg_selection_changed > 0)
	{		
		dialog = gtk_message_dialog_new (GTK_WINDOW (fMain),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("Package selection was changed. Really exit?"));
		if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_YES)
		{
			gtk_widget_destroy(dialog);	
			return;
		}
		else
		{
			gtk_widget_destroy(dialog);
		}
	}
	gtk_main_quit();
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
			perror ("ERR: connection lost: ");
			do_message_dlg(GTK_MESSAGE_ERROR,_("IPKG backend failure, cannot continue."));
			close(sock);
			exit(1);
		}
		if (read (sock, (void *) &msg, sizeof (pkmessage_t)) < 0)
		{
			perror ("ERR: receiving data packet");
			close (sock);
			do_message_dlg(GTK_MESSAGE_ERROR,_("Communication error, cannot continue."));
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
				do_message_dlg(GTK_MESSAGE_ERROR,msg.content.tf.str1);
				error++;
			break;	
			case PK_LIST:
				do_list(msg.content.tf.priority, msg.content.tf.str1,
						msg.content.tf.str2,msg.content.tf.str3,
						msg.content.tf.status);
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


void
list_toggle_inst (GtkCellRendererToggle * cellrenderertoggle,
		  gchar * path_str, gpointer model_data)
{
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean inst;
  pkg_state_want_t dstate;
  char *color;
	
  /* get toggled iter and values */
  gtk_tree_model_get_iter (GTK_TREE_MODEL(store), &iter, path);
  gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_INSTALLED, &inst, 
                                                    COL_DESIREDSTATE, &dstate, -1);
  /* What do we want to do with it? */
  if (dstate != SW_UNKNOWN)
  {
  	  pkg_selection_changed--;
	  dstate = SW_UNKNOWN;
	  color = NULL;
  }
  else
  	if (inst)
	{
		pkg_selection_changed++;
		dstate = SW_DEINSTALL;
		color = C_INSTALL;
	}
	else
	{
		pkg_selection_changed++;
		dstate = SW_INSTALL;
		color = C_REMOVE;
	}
 
  /* invert displayed value */
  inst ^= 1;

  /* write values */
  gtk_tree_store_set (GTK_TREE_STORE(store), &iter, 
                      COL_INSTALLED,inst,
                      COL_DESIREDSTATE,dstate,
                      COL_COLOR,color,-1);
	
  /* clean up */
  gtk_tree_path_free (path);
}


void
do_local_install(char *filename,gpointer userdata)
{
	printf(filename);
	if (!access(filename,F_OK))
	{
		send_message(PK_COMMAND,CMD_INSTALL,"",filename);
		wait_command_finish();
	}
	else
		do_message_dlg(GTK_MESSAGE_ERROR,
		_("The selected file could not be accessed."));		
}


/* app mainloop */

int
mainloop (int argc, char *argv[])
{
	struct sockaddr_un name;
	
	sleep(1); /* wait for second process to initialize */
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);
	
	
	/* Create socket from which to read. */
	sock = socket (AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror ("ERR: opening datagram socket");
		exit (1);
	}

	/* Create name. */
	name.sun_family = AF_UNIX;
	strcpy (name.sun_path, PK_SOCKET);
	if (connect (sock, (struct sockaddr *) &name, SUN_LEN (&name)))
	{
		perror ("ERR: connecting to socket");
		exit (1);
	}

	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);

	create_fMain ();
	gtk_widget_show (fMain);
  
	/* get packages list */
	send_message(PK_COMMAND,CMD_LIST,"","");
	
	gtk_timeout_add(500,get_pending_messages,NULL);
	
	gtk_main ();

	close (sock);

	return 0;
}


/* 
 * Checks if the given package is installed.
 */
int 
do_package_check(const char *package)
{
	return 0;
}


void
on_select_local(GtkButton *button, gpointer user_data)
{
	ask_user_a_file("/tmp",_("Select package file"),do_local_install,NULL,NULL);
}


void
on_system_update_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextBuffer *logbuf;
  GtkTextIter start,end;
  gtk_widget_set_sensitive(bUpdate,FALSE);
  gtk_widget_set_sensitive(bApply,FALSE);
  gtk_widget_set_sensitive(bSysUpgrade,FALSE);

	error = 0;
	
  /* clear log */	
  logbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txLog));
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(logbuf),&start);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(logbuf),&end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(logbuf),&start,&end);
  send_message(PK_COMMAND,CMD_UPDATE,"","");
	wait_command_finish();
	send_message(PK_COMMAND,CMD_UPGRADE,"","");
	wait_command_finish();
	send_message(PK_COMMAND,CMD_LIST,"","");
	wait_command_finish();
	if (error)
		do_message_dlg(GTK_MESSAGE_WARNING,_("Some packages could not be updated, " \
						"please check your configuration."));
	else
		do_message_dlg(GTK_MESSAGE_INFO,_("System updated sucessfully."));
}


void
on_packages_update_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextBuffer *logbuf;
  GtkTextIter start,end;
  gtk_widget_set_sensitive(bUpdate,FALSE);
  gtk_widget_set_sensitive(bApply,FALSE);
  gtk_widget_set_sensitive(bSysUpgrade,FALSE);

	error = 0;
	
  /* clear log */	
  logbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txLog));
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(logbuf),&start);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(logbuf),&end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(logbuf),&start,&end);
  send_message(PK_COMMAND,CMD_UPDATE,"","");
	wait_command_finish();
	send_message(PK_COMMAND,CMD_LIST,"","");
	wait_command_finish();
	if (error)
		do_message_dlg(GTK_MESSAGE_WARNING,_("Some lists could not be updated, " \
						"please check your configuration."));
	else
		do_message_dlg(GTK_MESSAGE_INFO,_("Package lists updated successfully."));
}


void 
on_package_install_clicked(GtkButton *button, gpointer user_data)
{
	char *msg;
	error = 0;
	gtk_tree_model_foreach(GTK_TREE_MODEL(store),update_check,NULL);
	if (error)
	{
		msg = g_strdup_printf("%i %s",error,
		_("changes could not be applied."));
		do_message_dlg(GTK_MESSAGE_WARNING,msg);
		g_free(msg);
	}
	else
		do_message_dlg(GTK_MESSAGE_INFO,_("Packages installed sucessfully."));
	pkg_selection_changed = 0;
}


gboolean   
tv_row_clicked(GtkTreeView *treeview, GtkTreePath *arg1, 
	GtkTreeViewColumn *arg2, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	char *version;

	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE) 
		return FALSE;
	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, COL_VERSION, &version, -1);
	gtk_statusbar_push(GTK_STATUSBAR(sbar),0,version);
	return TRUE;
}


/* --- create mainform --- */

void
create_fMain (void)
{
  GtkWidget *vbox;
  GtkWidget *cur;
  GtkWidget *toolbar;
  GtkWidget *pw;
  GtkTooltips *tooltips;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  char *tmp;

	/* init tree storage stuff */
	store = gtk_tree_store_new (N_COLUMNS,
				    G_TYPE_BOOLEAN,
				    G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_INT,
				    G_TYPE_STRING, 
					G_TYPE_STRING,
					G_TYPE_INT
	);

  fMain = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (fMain), _("GPE Package"));
  gtk_window_set_default_size (GTK_WINDOW (fMain), 240, 300);
  gtk_window_set_policy (GTK_WINDOW (fMain), TRUE, TRUE, FALSE);
  gpe_set_window_icon(fMain, "icon");

  vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(fMain),vbox);
	
  tooltips = gtk_tooltips_new ();

  /* toolbar */
	
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  bUpdate = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_DOWN, 
  			_("Update package lists"),	_("Update package lists"),
			   (GtkSignalFunc) on_packages_update_clicked , NULL, -1);
			   
  pw = gtk_image_new_from_pixbuf (gpe_find_icon ("refresh"));
  bSysUpgrade = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Update System"),
			   _("Update System"), _("Update entire system over an internet connection."), pw,
			   (GtkSignalFunc) on_system_update_clicked , NULL);

  bApply = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_APPLY,
			   _("Apply package selection"), _("Apply package selection"),
			   (GtkSignalFunc) on_package_install_clicked , NULL, -1);
			   
  pw = gtk_image_new_from_pixbuf (gpe_find_icon ("local-package"));
  bSelectLocal = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Install package"),
			   _("Install package"), 
			   _("Select a package to install in local filesystem."), pw,
			   (GtkSignalFunc) on_select_local , NULL);
			   
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
  
  pw = gtk_image_new_from_pixbuf(gpe_find_icon ("exit"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Close application"),
			   _("Close application"), _("Close application"), pw,
			   (GtkSignalFunc) do_safe_exit, NULL);
			   
  gtk_box_pack_start(GTK_BOX(vbox),toolbar,FALSE,TRUE,0);

  /* notebook */
  
  notebook = gtk_notebook_new();	
  gtk_box_pack_start(GTK_BOX(vbox),notebook,TRUE,TRUE,0);
	
//  gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_get_border ());
  
  gtk_object_set_data(GTK_OBJECT(notebook),"tooltips",tooltips);
  
  sbar = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(vbox),sbar,FALSE,TRUE,0);

  /* list tab */	
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());

  cur = gtk_label_new(_("Packages"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,cur);

  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Package Selection"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	
	
  cur = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cur),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cur),GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox),cur,TRUE,TRUE,0);	

  /* packages tree */
  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview),TRUE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(treeview),TRUE);
  gtk_container_add(GTK_CONTAINER(cur),treeview);	
  
	g_signal_connect_after (G_OBJECT (treeview), "cursor-changed",
			  G_CALLBACK (tv_row_clicked), NULL);

	renderer = gtk_cell_renderer_toggle_new ();
	gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer),FALSE);
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (list_toggle_inst), store);
	column = gtk_tree_view_column_new_with_attributes (_("Inst."),
							   renderer,
							   "active",
							   COL_INSTALLED,
							   "cell-background",
							   COL_COLOR,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
							   renderer,
							   "text",
							   COL_NAME,
							   "background",
							   COL_COLOR,
							   NULL);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column),TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	
  /* messages tab */
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());

  cur = gtk_label_new(_("Messages"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,cur);

  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Messages Log"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	

  cur = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cur),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cur),GTK_SHADOW_IN);
  gtk_tooltips_set_tip (tooltips, cur, _("This window shows all messages from the packet manager."), NULL);
  
  txLog = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txLog),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(cur),txLog);
  gtk_box_pack_start(GTK_BOX(vbox),cur,TRUE,TRUE,0);	
  
  g_signal_connect(G_OBJECT (fMain),"destroy",gtk_main_quit,NULL);
  
  gtk_widget_show_all(fMain);
}
