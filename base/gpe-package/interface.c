/*
 * gpe-package
 *
 * Copyright (C) 2003 - 2005  Florian Boor <florian.boor@kernelconcepts.de>
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
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/un.h>

#include <locale.h>
#include <libintl.h>

#ifdef ENABLE_PCRE
#include <pcre.h>
#endif

#define _(x) gettext(x)
#define N_(_x) (_x)

#include <gtk/gtk.h>

#include <libipkg.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>
#include <gpe/gpehelp.h>

#include "packages.h"
#include "interface.h"
#include "main.h"
#include "filechooser.h"
#include "feededit.h"

#define MI_FILE             1
#define MI_FILE_INSTALL     2
#define MI_FILE_CLOSE       3
#define MI_PACKAGES         4
#define MI_PACKAGES_UPDATE  5
#define MI_PACKAGES_UPGRADE 6
#define MI_PACKAGES_APPLY   7
#define MI_FILTER_INST		8
#define MI_FILTER_NOTINST	9
#define MI_FILTER_SEARCH	10
#define MI_PACKAGES_INFO    11

#define HELPMESSAGE "GPE-Package\nVersion " VERSION \
		"\nGPE frontend for ipkg\n\nflorian@handhelds.org"

#define NOHELPMESSAGE N_("Help for this application is not installed.")

/* --- module global variables --- */


typedef struct {
	char *name;
	char *version;
	char *description;
	char *color;
	pkg_state_status_t status;
} description_t;

static description_t *pkg_info = NULL;
static int pkg_count = 0;

int sock;
static pkcommand_t running_command = CMD_NONE;
static int pkg_selection_changed = 0;
static int error = 0;
static gboolean ErrorDialogOpen = FALSE;

/* --- global widgets --- */
static GtkWidget *notebook;
static GtkWidget *txLog;
static GtkWidget *treeview;
static GtkTreeModel *filter;
static gchar *filter_term = NULL;
static GtkTreeStore *store = NULL;
static GtkToolItem *bApply;
static GtkWidget *miUpdate, *miSysUpgrade, *miSelectLocal, *miApply;
static GtkWidget *miFilterInst, *miFilterNotInst, *miFilterSearch;
static GtkWidget *sbar;
GtkWidget *fMain;
static GtkWidget *dlgAction = NULL;
static GtkWidget *dlgInfo = NULL;
static GtkTextBuffer *infobuffer = NULL;
static GtkWidget *mMain;

#ifdef ENABLE_PCRE
static gboolean is_regexp;
#endif

/* some forwards */
gboolean get_pending_messages ();
void on_tree_filter_changed(GtkCheckMenuItem *menuitem, gpointer user_data);
void on_tree_filter_search_changed (GtkCheckMenuItem *menuitem, gpointer user_data);
gboolean filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
void set_filter_term (const gchar *text, gboolean regexp);
void search_entry_activated (GtkEntry *entry, gpointer user_data);
void create_fMain (void);
void on_select_local(GtkButton *button, gpointer user_data);
void on_packages_update_clicked(GtkButton *button, gpointer user_data);
void on_system_update_clicked(GtkButton *button, gpointer user_data);
void on_package_install_clicked(GtkButton *button, gpointer user_data);
void on_about_clicked (GtkWidget * w);
void on_help_clicked (GtkWidget * w);
void on_package_info_clicked(GtkButton *button, gpointer user_data);


static GtkItemFactoryEntry mMain_items[] = {
  { N_("/_File"),         NULL,         NULL, MI_FILE, "<Branch>" },
  { N_("/File/_Install file"), "", on_select_local, MI_FILE_INSTALL, "<StockItem>", GTK_STOCK_OPEN},
  { N_("/_File/s1"), NULL , NULL,    0, "<Separator>"},
  { N_("/File/_Close"),  NULL, do_safe_exit, MI_FILE_CLOSE, "<StockItem>", GTK_STOCK_QUIT },
  { N_("/_Packages"),         NULL,         NULL, MI_PACKAGES, "<Branch>" },
  { N_("/Packages/Show Insta_lled"), "", on_tree_filter_changed, MI_FILTER_INST , "<CheckItem>"},
  { N_("/Packages/Show _Not Installed"), "", on_tree_filter_changed, MI_FILTER_NOTINST, "<CheckItem>"},
  { N_("/_Packages/s2"), NULL , NULL,    0, "<Separator>"},
  { N_("/Packages/_Search"), "<Control> I", on_tree_filter_search_changed, MI_FILTER_SEARCH , "<CheckItem>"},
  { N_("/_Packages/s3"), NULL , NULL,    0, "<Separator>"},
  { N_("/Packages/Show _Info"), "<Control> I", on_package_info_clicked, MI_PACKAGES_INFO , "<Item>"},
  { N_("/_Packages/s4"), NULL , NULL,    0, "<Separator>"},
  { N_("/Packages/_Update lists"), "<Control> U", on_packages_update_clicked, MI_PACKAGES_UPDATE , "<Item>"},
  { N_("/Packages/Upgrade _System"), "", on_system_update_clicked, MI_PACKAGES_UPGRADE, "<Item>"},
  { N_("/_Packages/s3"), NULL , NULL,    0, "<Separator>"},
  { N_("/Packages/_Apply"), "", on_package_install_clicked, MI_PACKAGES_APPLY, "<StockItem>", GTK_STOCK_APPLY},
  { N_("/_Help"),         NULL,         NULL,           0, "<Branch>" },
  { N_("/_Help/Index"),   NULL,         on_help_clicked,    0, "<StockItem>",GTK_STOCK_HELP },
  { N_("/_Help/About"),   NULL,         on_about_clicked,    0, "<Item>" }
};

int mMain_items_count = sizeof(mMain_items) / sizeof(GtkItemFactoryEntry);


struct gpe_icon my_icons[] = {
  { "local-package", PREFIX "/share/pixmaps/local-package-16.png" },
  { "icon", PREFIX "/share/pixmaps/gpe-package.png" },
  { NULL, NULL}
};



/* dialogs */
#warning todo: status icons, verbose dialogs

void
destroy_package_list(description_t *list, int len)
{
	while (len) {
		if (list[len-1].name)
			g_free(list[len-1].name);
		if (list[len-1].description)
			g_free(list[len-1].description);
		if (list[len-1].version)
			g_free(list[len-1].version);
		len--;
	}
	if (list)
		g_free(list);
}


gboolean dialog_destroy()
{
	gtk_widget_destroy(dlgInfo);
	dlgInfo = NULL;
	infobuffer = NULL;
	return TRUE;
}


void
do_package_info(const char* name, const char *info)
{
GtkWidget *sw;
GtkWidget *textview;
GtkTextBuffer *buffer;
	
	if (dlgInfo == NULL) {
		dlgInfo = gtk_dialog_new_with_buttons(_("Package information"),GTK_WINDOW(fMain),
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_STOCK_OK,
				   GTK_RESPONSE_ACCEPT,
				   NULL);
		sw = gtk_scrolled_window_new(NULL,NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
			GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
		buffer = gtk_text_buffer_new(NULL);
		infobuffer = buffer;
		gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(buffer),info,strlen(info));
		textview = gtk_text_view_new_with_buffer(buffer);
		gtk_text_view_set_editable(GTK_TEXT_VIEW(textview),FALSE);
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview),GTK_WRAP_WORD);
		gtk_container_add(GTK_CONTAINER(sw),textview);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlgInfo)->vbox),sw,TRUE,TRUE,0);
		gtk_widget_set_size_request(textview,220,250);
		gtk_widget_show_all(dlgInfo);
		g_signal_connect_swapped(dlgInfo,"response",G_CALLBACK(dialog_destroy),NULL);
	} else {
		if (!infobuffer)
			return;
		gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(infobuffer),
			info,strlen(info));
	}
}


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
	gtk_window_set_title (GTK_WINDOW (window), _("Working"));

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

	gtk_container_set_border_width (GTK_CONTAINER (hbox),
					gpe_get_border ());

	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (window), hbox);

	return window;
}


void
show_message(GtkMessageType type, char* message)
{
GtkWidget* dialog;
	
	dialog = gtk_message_dialog_new (GTK_WINDOW(fMain),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 type,
					 GTK_BUTTONS_OK,
					 message);
	gtk_dialog_run (GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
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
	if (msg.ctype == PK_COMMAND) {
		running_command = command;
		switch (command) {
			case CMD_LIST:
				desc = _("Reading packages list...");
				destroy_package_list(pkg_info,pkg_count);
				pkg_count = 0;
				pkg_info = NULL;			
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
			case CMD_INFO:
				desc = g_strdup_printf("%s", _("Retrieving package information"));
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
	if (write (sock, (void *) &msg, sizeof (pkmessage_t)) < 0) {
		perror ("ERR: sending data to backend");
	}
}


void printlog(GtkWidget *textview, gchar *str)
{
GtkTextBuffer* log;

	log = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(log),str,-1);
}


void wait_command_finish()
{
	while (running_command != CMD_NONE) {
		gtk_main_iteration();
		gtk_main_iteration();
		usleep (100000);
		get_pending_messages();
	}
}


/* --- local intelligence --- */

gboolean update_check(GtkTreeModel *model, GtkTreePath *path,
             GtkTreeIter *iter,
             gpointer data)
{
pkg_state_want_t dstate;
char *name;

	gtk_tree_model_get (GTK_TREE_MODEL(model), iter, 
				COL_DESIREDSTATE, &dstate,
				COL_NAME, &name, -1);
	switch (dstate) {
		case SW_INSTALL:
			send_message(PK_COMMAND,CMD_INSTALL,"",name);
			wait_command_finish();
			gtk_tree_store_set (GTK_TREE_STORE(model), iter, 
				COL_INSTALLED,TRUE, COL_DESIREDSTATE,SW_UNKNOWN,
				COL_COLOR,NULL,-1);
			break;
		case SW_DEINSTALL:
			send_message(PK_COMMAND,CMD_REMOVE,"",name);
			wait_command_finish();
			gtk_tree_store_set (GTK_TREE_STORE(model), iter, 
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


void update_tree(void)
{
	guint id;
	int i;
	GtkTreeIter iter;

	id = gtk_statusbar_get_context_id(GTK_STATUSBAR(sbar),"upd");
	gtk_statusbar_push(GTK_STATUSBAR(sbar),
		id,_("Updating views, please wait..."));
	
	gtk_tree_store_clear(GTK_TREE_STORE(store));
	
	for (i=0; i<pkg_count; i++) {
		gtk_tree_store_append (store, &iter, NULL);
	
		gtk_tree_store_set (store, &iter,
			COL_NAME, pkg_info[i].name,
			COL_DESCRIPTION, pkg_info[i].description,
			COL_VERSION, pkg_info[i].version,
			COL_INSTALLED, pkg_info[i].status == SS_INSTALLED,
			COL_DESIREDSTATE, SW_UNKNOWN,
			COL_COLOR, pkg_info[i].color, -1);
	}
	gtk_statusbar_pop(GTK_STATUSBAR(sbar),id);	
}


void on_tree_filter_changed(GtkCheckMenuItem *menuitem, gpointer user_data)
{
	if (filter)
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter));
}

gboolean filter_visible_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	gboolean installed;
	gboolean not_installed;
	GValue value_installed = { 0, };
	GValue value_name = { 0, };
	const gchar *pkgname;

	installed = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(miFilterInst));
	not_installed = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(miFilterNotInst));
        
	gtk_tree_model_get_value (model, iter, COL_INSTALLED, &value_installed);
        
	if (g_value_get_boolean (&value_installed)) {
		/* Package is installed */
		if (!installed)
			return FALSE;
		} else {
		/* Package is not installed */
		if (!not_installed)
			return FALSE;
	}

	if (!filter_term)
		return TRUE;

	gtk_tree_model_get_value (model, iter, COL_NAME, &value_name);

	pkgname = g_value_get_string (&value_name);

#ifdef ENABLE_PCRE
	pcre *re;
	gint ret;
	const gchar *error;
	gint error_offset;

	if (is_regexp) {
		re = pcre_compile (filter_term, 0, &error, &error_offset, NULL);

		if (re) {
			ret = pcre_exec (re, NULL, pkgname, strlen (pkgname), 0, 0, NULL, 0); 

			g_free (re);

			if (ret >= 0) {
				return TRUE;
			}
		}

		return FALSE;
	}
#endif

	if (strstr (pkgname, filter_term))
		return TRUE;

	return FALSE;
}

void set_filter_term (const gchar *text, gboolean regexp)
{
	if (filter_term) {
		g_free (filter_term);
	}
	
	filter_term = g_strdup (text);

#ifdef ENABLE_PCRE
	is_regexp = regexp;
#endif

	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter));
}

void search_entry_activated (GtkEntry *entry, gpointer user_data)
{
	GtkDialog *dialog = user_data;

	gtk_dialog_response (dialog, GTK_RESPONSE_ACCEPT);
}

void on_tree_filter_search_changed (GtkCheckMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	const gchar *text;
#ifdef ENABLE_PCRE
	GtkWidget *checkbutton;
#endif

	if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (miFilterSearch))) {
		dialog = gtk_dialog_new_with_buttons (_("Search term:"),
			GTK_WINDOW (fMain),
			GTK_DIALOG_DESTROY_WITH_PARENT,
		        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
			NULL);
		entry = gtk_entry_new ();
		g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (search_entry_activated), dialog);
		gtk_widget_show (entry);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), entry);
#ifdef ENABLE_PCRE
		checkbutton = gtk_check_button_new_with_label (_("Regular expression"));
		gtk_widget_show (checkbutton);
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), checkbutton);
#endif
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
			text = gtk_entry_get_text (GTK_ENTRY (entry));
			if (strlen (text)) {
#ifdef ENABLE_PCRE			
				set_filter_term (text, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)));
#else
				set_filter_term (text, FALSE);
#endif
			} else {
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (miFilterSearch), FALSE);
			}
		} else {
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (miFilterSearch), FALSE);
                }
		gtk_widget_destroy (dialog);
	} else {
		set_filter_term (NULL, FALSE);
	}
		
        
}

void on_about_clicked (GtkWidget * w)
{
	show_message(GTK_MESSAGE_INFO, HELPMESSAGE);
}


void on_help_clicked (GtkWidget * w)
{
	if (gpe_show_help("gpe-package", NULL))
		show_message(GTK_MESSAGE_INFO, NOHELPMESSAGE);
}


void do_list(int prio,char* pkgname,char *desc, char *version, pkg_state_status_t status)
{
gboolean updatev;
char *nversion = NULL;

	updatev = (pkg_count && !strcmp(pkgname,pkg_info[pkg_count-1].name));

	if (!updatev) {
		pkg_info = realloc(pkg_info,sizeof(description_t) * (pkg_count + 1));		
		pkg_info[pkg_count].name = g_strdup(pkgname);
		pkg_info[pkg_count].version = g_strdup(version);
		pkg_info[pkg_count].description = g_strdup(desc);
		pkg_info[pkg_count].status = status;
		pkg_info[pkg_count].color = NULL;
		if ((status != SS_INSTALLED) && (status != SS_NOT_INSTALLED))
			pkg_info[pkg_count].color = C_INCOMPLETE;
		pkg_count++;
	} else {
		nversion = pkg_info[pkg_count-1].version;
		pkg_info[pkg_count-1].version = g_strdup_printf("%s\n%s",nversion,version);
		if (pkg_info[pkg_count-1].status != SS_INSTALLED)
			pkg_info[pkg_count-1].status = status;
		if (nversion) g_free(nversion);
		if ((status != SS_INSTALLED) && (status != SS_NOT_INSTALLED))
			pkg_info[pkg_count-1].color = C_INCOMPLETE;
	}		
	
		
	switch (running_command) {
		case CMD_LIST:
			break;
		default:
			printlog(txLog,pkgname);
			break;
	}
}


void do_message_dlg(int type,char *msg)
{
GtkWidget *dialog;
printlog(txLog,msg);
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (fMain),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 type, GTK_BUTTONS_CLOSE,
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
	gtk_widget_set_sensitive(miUpdate, TRUE);
	gtk_widget_set_sensitive(miApply, TRUE);
	gtk_widget_set_sensitive(miSysUpgrade, TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(bApply), TRUE);
	printlog(txLog,_("Command finished. Please check log messages for errors."));
	if (dlgAction) {
		gtk_widget_destroy(dlgAction);
		dlgAction = NULL;
	}
	if (running_command == CMD_LIST) 
		update_tree();
	running_command = CMD_NONE;
}


void do_safe_exit(int sig)
{
GtkWidget *dialog;

	if (pkg_selection_changed > 0) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (fMain),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("Package selection was changed. Really exit?"));
		if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_YES) {
			gtk_widget_destroy(dialog);	
			return;
		} else {
			gtk_widget_destroy(dialog);
		}
	}
	gtk_main_quit();
}


void error_dialog_close(void *data)
{
	ErrorDialogOpen = FALSE;
	gtk_widget_destroy(GTK_WIDGET(data));
}


gboolean get_pending_messages ()
{
static pkmessage_t msg;
struct pollfd pfd[1];

	pfd[0].fd = sock;
	pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
	while (poll (pfd, 1, 0) > 0) {
		if ((pfd[0].revents & POLLERR) || (pfd[0].revents & POLLHUP)) {
			perror ("ERR: connection lost: ");
			do_message_dlg(GTK_MESSAGE_ERROR,_("IPKG backend failure, cannot continue."));
			close(sock);
			exit(1);
		}
		if (read (sock, (void *) &msg, sizeof (pkmessage_t)) < 0) {
			perror ("ERR: receiving data packet");
			close (sock);
			do_message_dlg(GTK_MESSAGE_ERROR,_("Communication error, cannot continue."));
			exit (1);
		} else if (msg.type == PK_FRONT) {
			switch (msg.ctype) {
				case PK_QUESTION:
					do_question(msg.content.tf.priority, msg.content.tf.str1);
					break;
				case PK_ERROR:
					if (!ErrorDialogOpen) {
						GtkWidget *dialog;
						dialog = gtk_message_dialog_new (GTK_WINDOW(fMain),
		                                  GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                		                  GTK_MESSAGE_ERROR,
                        		          GTK_BUTTONS_CLOSE,
                        		          _("An error occurred.\nPlease consult the message log\nfor hints what went wrong."));
                        		        g_signal_connect_swapped (dialog, "response", G_CALLBACK (error_dialog_close), dialog);
						gtk_widget_show(dialog);
						ErrorDialogOpen = TRUE;
					}
					printlog(txLog, msg.content.tf.str1);
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
				case PK_PKGINFO:
					do_package_info(msg.content.tf.str1, msg.content.tf.str2);
					break;	
				case PK_PACKAGESTATE:
//					do_state(msg.content.tf.priority, msg.content.tf.str1,msg.content.tf.str2);
					break;
				case PK_FINISHED:
#ifdef DEBUG				
					printf("finished\n");
#endif			
					do_end_command();
					break;
				default:
					break;
			} /* switch */
		}
	}
	return TRUE;
}


void list_toggle_inst (GtkCellRendererToggle * cellrenderertoggle,
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
	if (dstate != SW_UNKNOWN) {
		pkg_selection_changed--;
		dstate = SW_UNKNOWN;
		color = NULL;
	} else if (inst) {
		pkg_selection_changed++;
		dstate = SW_DEINSTALL;
		color = C_INSTALL;
	} else {
		pkg_selection_changed++;
		dstate = SW_INSTALL;
		color = C_REMOVE;
	}
 
	/* invert displayed value */
	inst ^= 1;

	/* write values */
	gtk_tree_store_set (GTK_TREE_STORE(store), &iter, 
				COL_INSTALLED, inst,
				COL_DESIREDSTATE, dstate,
				COL_COLOR,color, -1);
	
	/* clean up */
	gtk_tree_path_free (path);
}


void do_local_install(char *filename,gpointer userdata)
{
	if (!access(filename,F_OK)) {
		send_message(PK_COMMAND,CMD_INSTALL,"",filename);
		wait_command_finish();
	} else {
		do_message_dlg(GTK_MESSAGE_WARNING,
		_("The selected file could not be accessed."));
	}
}


/* app mainloop */

int mainloop (int argc, char *argv[])
{
struct sockaddr_un name;
int opt;
gboolean mode_upgrade = FALSE;

	sleep(1); /* wait for second process to initialize */
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);
	
	signal (SIGINT, do_safe_exit);
	signal (SIGTERM, do_safe_exit);
 	
	while ((opt = getopt(argc, argv, "u")) > 0)
	{
		if (opt == 'u')
			mode_upgrade = TRUE;
	}

	/* Create socket from which to read. */
	sock = socket (AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror ("ERR: opening datagram socket");
		exit (1);
	}

	/* Create name. */
	name.sun_family = AF_UNIX;
	strcpy (name.sun_path, PK_SOCKET);
	if (connect (sock, (struct sockaddr *) &name, SUN_LEN (&name))) {
		perror ("ERR: connecting to socket");
		exit (1);
	}

	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);

	create_fMain ();
	gtk_widget_show (fMain);
  
	/* do initial actions */
	if (mode_upgrade)
		on_system_update_clicked(NULL, NULL);
	else
		send_message(PK_COMMAND,CMD_LIST,NULL,NULL);
	
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
	return 0;
}


void on_select_local(GtkButton *button, gpointer user_data)
{
	package_choose(fMain, do_local_install);
}


void on_package_info_clicked(GtkButton *button, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeSelection *sel;
	char *name = NULL;
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if (gtk_tree_selection_get_selected(sel, (GtkTreeModel**)(&store), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_NAME, &name, -1);
		send_message(PK_COMMAND,CMD_INFO,name,name);
	}
}


void on_system_update_clicked(GtkButton *button, gpointer user_data)
{
	GtkTextBuffer *logbuf;
	GtkTextIter start,end;
	
	gtk_widget_set_sensitive(miUpdate, FALSE);
	gtk_widget_set_sensitive(miApply, FALSE);
	gtk_widget_set_sensitive(miSysUpgrade, FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(bApply), FALSE);

	error = 0;
	
	show_message(GTK_MESSAGE_INFO, 
	             _("Make sure your internet connection is up " \
	               "or your update source is available and " \
	               "press OK to continue."));
	
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


void on_packages_update_clicked(GtkButton *button, gpointer user_data)
{
GtkTextBuffer *logbuf;
GtkTextIter start,end;

	gtk_widget_set_sensitive(miUpdate, FALSE);
	gtk_widget_set_sensitive(miApply, FALSE);
	gtk_widget_set_sensitive(miSysUpgrade, FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(bApply), FALSE);

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
	if (error) {
		msg = g_strdup_printf("%i %s",error,
		_("changes could not be applied."));
		do_message_dlg(GTK_MESSAGE_WARNING,msg);
		g_free(msg);
	} else
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
	gtk_tree_model_get (GTK_TREE_MODEL (filter), &iter, COL_VERSION, &version, -1);
	gtk_statusbar_push(GTK_STATUSBAR(sbar),0,version);

	return TRUE;
}


/* create menus from description */
GtkWidget *
create_mMain(GtkWidget  *window)
{
GtkItemFactory *itemfactory;
GtkAccelGroup *accelgroup;

	accelgroup = gtk_accel_group_new ();

	itemfactory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
                                       accelgroup);
	gtk_item_factory_create_items (itemfactory, mMain_items_count, 
		mMain_items, NULL);
	gtk_window_add_accel_group (GTK_WINDOW (window), accelgroup);

	miUpdate = gtk_item_factory_get_item_by_action(itemfactory,
		MI_PACKAGES_UPDATE);
	miSysUpgrade = gtk_item_factory_get_item_by_action(itemfactory,
		MI_PACKAGES_UPGRADE);
	miSelectLocal = gtk_item_factory_get_item_by_action(itemfactory,
		MI_FILE_INSTALL);
	miApply = gtk_item_factory_get_item_by_action(itemfactory,
		MI_PACKAGES_APPLY);
	miFilterInst = gtk_item_factory_get_item_by_action(itemfactory,
		MI_FILTER_INST);
	miFilterNotInst = gtk_item_factory_get_item_by_action(itemfactory,
		MI_FILTER_NOTINST);
	miFilterSearch = gtk_item_factory_get_item_by_action(itemfactory,
		MI_FILTER_SEARCH);
	
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(miFilterInst),TRUE); 
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(miFilterNotInst),TRUE); 
	
return (gtk_item_factory_get_widget (itemfactory, "<main>"));
}


static void 
on_select_page (GtkNotebook *notebook, GtkNotebookPage *pp, 
                gint page, gpointer user_data) 
{
  feed_edit_set_active ((page == 2) ? TRUE : FALSE);
}


/* --- create mainform --- */

void create_fMain (void)
{
GtkWidget *vbox;
GtkWidget *cur;
GtkWidget *toolbar;
GtkTooltips *tooltips;
GtkCellRenderer *renderer;
GtkTreeViewColumn *column;
GtkToolItem *sep, *bExit;
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
	gtk_window_set_title (GTK_WINDOW (fMain), _("Package Manager"));
	gtk_window_set_default_size (GTK_WINDOW (fMain), 240, 300);
	gtk_window_set_policy (GTK_WINDOW (fMain), TRUE, TRUE, FALSE);
	gpe_set_window_icon(fMain, "icon");

	vbox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(fMain),vbox);
	
	tooltips = gtk_tooltips_new ();
  
	sbar = gtk_statusbar_new();
	gtk_box_pack_end(GTK_BOX(vbox),sbar,FALSE,TRUE,0);

	/* main menu */ 
	mMain = create_mMain(fMain);
	gtk_box_pack_start(GTK_BOX(vbox),mMain,FALSE,TRUE,0);
  
	/* toolbar */
	
	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
	                             GTK_ORIENTATION_HORIZONTAL);

	bApply = gtk_tool_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(bApply), -1);
	g_signal_connect(G_OBJECT(bApply), "clicked", 
	                 G_CALLBACK(on_package_install_clicked), NULL);
			   
	sep = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(sep), -1);
     
	gtk_box_pack_start(GTK_BOX(vbox),toolbar, FALSE, TRUE, 0);

	/* notebook */
  
	notebook = gtk_notebook_new();	
	gtk_box_pack_start(GTK_BOX(vbox),notebook,TRUE,TRUE,0);
	
	gtk_object_set_data(GTK_OBJECT(notebook),"tooltips",tooltips);
  
	/* installed tab */	
	vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());

	cur = gtk_label_new(_("List"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,cur);

	cur = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
	tmp = g_strdup_printf("<b>%s</b>",_("Package List"));
	gtk_label_set_markup(GTK_LABEL(cur),tmp);
	free(tmp);
	gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	
	
	cur = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cur),
  		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cur), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox), cur, TRUE, TRUE, 0);	

	/* packages tree */
	filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter), filter_visible_func, NULL, NULL);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (filter));
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview),FALSE);
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
	vbox = gtk_vbox_new(FALSE, gpe_get_boxspacing());

	cur = gtk_label_new(_("Messages"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,cur);

	cur = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
	tmp = g_strdup_printf("<b>%s</b>",_("Messages Log"));
	gtk_label_set_markup(GTK_LABEL(cur),tmp);
	free(tmp);
	gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	

	cur = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cur),
  		GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cur),GTK_SHADOW_IN);
	gtk_tooltips_set_tip (tooltips, cur, 
  		_("This window shows all messages from the packet manager."), NULL);
  
	txLog = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txLog),GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(cur),txLog);
	gtk_box_pack_start(GTK_BOX(vbox),cur,TRUE,TRUE,0);	

	cur = gtk_label_new(_("Feeds"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_feed_edit(toolbar), cur);
	
	g_signal_connect (G_OBJECT (notebook), "switch-page", 
	                  G_CALLBACK (on_select_page), NULL);

	g_signal_connect(G_OBJECT (fMain), "destroy", gtk_main_quit, NULL);
 
    /* remaining toolbar contents */
   	sep = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep), FALSE);
	gtk_tool_item_set_expand(GTK_TOOL_ITEM(sep), TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(sep), -1);
	
	bExit = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(bExit), -1);
	g_signal_connect(G_OBJECT(bExit), "clicked", 
	                 G_CALLBACK(do_safe_exit), NULL);


	gtk_widget_show_all(fMain);
    feed_edit_set_active (FALSE);
}
