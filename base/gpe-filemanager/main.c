 /*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *               2004 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <gdk/gdkx.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-mime-info.h>
#include <libgnomevfs/gnome-vfs-module-callback.h>
#include <libgnomevfs/gnome-vfs-standard-callbacks.h>
#include <libgnomevfs/gnome-vfs-xfer.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-application-registry.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/gpeiconlistview.h>
#include <gpe/dirbrowser.h>
#include <gpe/spacing.h>
#include <gpe/gpehelp.h>
#include <X11/Xlib.h>
#include <gpe/infoprint.h>

#include "guitools.h"
#include "bluetooth.h"

#define _(x) dgettext(PACKAGE, x)

#define COMPLETED 0
#define LAST_SIGNAL 1

#define DEFAULT_TERMINAL "rxvt -e"
#define FILEMANAGER_ICON_PATH "/share/gpe/pixmaps/default/filemanager/document-icons"
#define DEFAULT_ICON_PATH "/pixmaps"
#define ZOOM_INCREMENT 8

#define MAX_SYM_DEPTH 256

#define N_(x) (x)

#define HELPMESSAGE "GPE-Filemanager\nVersion " VERSION \
		"\nGPE File Manager\n\ndctanner@magenet.com"\
		"\nflorian@handhelds.org"

#define NOHELPMESSAGE N_("Help for this application is not installed.")


/* IDs for data storage fields */
enum
{
	COL_ICON,
	COL_NAME,
	COL_SIZE,
	COL_PERM,
    COL_CHANGED,
    COL_TYPE,
	COL_DATA,
	N_COLUMNS
};
static GtkTreeStore *store = NULL;

enum
{
	COL_DIRICON,
	COL_DIRNAME,
	COL_DIRDATA,
	N_DIRCOLUMNS
};

static GtkTreeStore *dirstore = NULL;

static GtkWidget *window;
static GtkWidget *combo;
static GtkWidget *view_widget;
static GtkWidget *dir_view_widget;
static GtkWidget *view_window;
static GtkWidget *dir_view_window;
static GtkWidget *active_view;
static GtkWidget *vbox, *main_paned;

static GtkWidget *bluetooth_menu_item;
static GtkWidget *irda_menu_item;
static GtkWidget *copy_menu_item;
static GtkWidget *paste_menu_item;
static GtkWidget *open_menu_item;
static GtkWidget *move_menu_item;
static GtkWidget *rename_menu_item;
static GtkWidget *properties_menu_item;
static GtkWidget *delete_menu_item;
static GtkWidget *set_dirbrowser_menu_item;
static GtkWidget *set_myfiles_menu_item;
static GtkWidget *goto_menu_item;

static GtkWidget *btnListView;
static GtkWidget *btnIconView;
static GtkWidget *btnGoUp = NULL;

GdkPixbuf *default_pixbuf;

GtkWidget *current_button = NULL;
int current_button_is_down = 0;

GtkItemFactory *item_factory;

/* For not starting an app twice after a double click */
int ignore_press = 0;

int loading_directory = 0;
int combo_signal_id;

int history_place = 0;
GList *history = NULL;

gchar *current_directory = NULL;
static GnomeVFSMonitorHandle *dir_monitor = NULL;
static gboolean refresh_scheduled = FALSE;

gint current_zoom = 36;
static gboolean view_is_icons = FALSE;
static gboolean directory_browser = TRUE;
static gboolean limited_view = FALSE;

static GList *file_clipboard = NULL;

static GHashTable *loaded_icons;
static GtkWidget *progress_dialog = NULL;
static gboolean abort_transfer = FALSE;
static GList *dirlist = NULL;
static gboolean initialized = FALSE;


typedef struct
{
  gchar *filename;
  GnomeVFSFileInfo *vfs;
} FileInformation;

FileInformation *current_popup_file;

struct gpe_icon my_icons[] = {
  { "left", "left" },
  { "right", "right" },
  { "up", "up" },
  { "refresh", "refresh" },
  { "stop", "stop" },
  { "home", "home" },
  { "open", "open" },
  { "dir-up", "dir-up" },
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "question", "question" },
  { "error", "error" },
  { "list-view" },
  { "icon-view" },
  { "icon", PREFIX "/share/pixmaps/gpe-filemanager.png" },
  {NULL, NULL}
};


/* some forward declarations */

void browse_directory (gchar *directory);
static void popup_ask_open_with (void);
static void popup_ask_move_file (void);
static void popup_ask_rename_file (void);
static void popup_ask_delete_file (void);
static void show_file_properties (void);
static void refresh_current_directory (void);
static void send_with_bluetooth (void);
static void send_with_irda (void);
static void copy_file_clip (void);
static void paste_file_clip (void);
static void create_directory_interactive(void);
static GtkWidget* create_view_widget_icons(void);
static GtkWidget* create_view_widget_list(void);
void on_about_clicked (GtkWidget * w);
void on_help_clicked (GtkWidget * w);
void on_dirbrowser_setting_changed(GtkCheckMenuItem *menuitem, gpointer user_data);
void on_myfiles_setting_changed(GtkCheckMenuItem *menuitem, gpointer user_data);


/* items of the context menu */
static GtkItemFactoryEntry menu_items[] =
{
  { "/Open Wit_h",	 NULL, popup_ask_open_with,  0, "<StockItem>", GTK_STOCK_OPEN },
  { "/sep1",	         NULL, NULL,	             0, "<Separator>" },
  { "/Send via _Bluetooth", NULL, send_with_bluetooth, 0, "<Item>" },
  { "/Send via _Infrared", NULL, send_with_irda, 0, "<Item>" },
  { "/_Copy",          NULL, copy_file_clip,         0, "<StockItem>", GTK_STOCK_COPY },
  { "/_Paste",          NULL, paste_file_clip,         0, "<StockItem>", GTK_STOCK_PASTE },
  { "/_Move",            NULL, popup_ask_move_file,            0, "<Item>" },
  { "/_Rename",          NULL, popup_ask_rename_file,          0, "<Item>" },
  { "/_Delete",   "Delete", popup_ask_delete_file,         0, "<StockItem>", GTK_STOCK_DELETE },
  { "/_Create Directory",NULL, create_directory_interactive, 0, "<Item>"},
  { "/sep2",	         NULL, NULL,	             0, "<Separator>" },
  { "/_Properties",      NULL, show_file_properties, 0, "<StockItem>", GTK_STOCK_PROPERTIES },
};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

/* items of the main menu */
static GtkItemFactoryEntry mMain_items[] = {
  { N_("/_File"),         NULL,         NULL, 0, "<Branch>" },
  { N_("/_File/_Create Directory"),NULL, create_directory_interactive, 0, "<Item>"},
  { N_("/_File/s1"), NULL , NULL,    0, "<Separator>"},
  { N_("/_File/_Close"),  NULL, gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT },
  { N_("/_Settings"),         NULL,         NULL, 0, "<Branch>" },
  { N_("/_Settings/Directory Browser"), NULL, on_dirbrowser_setting_changed, 0, "<CheckItem>"},
  { N_("/_Settings/My Files only"), NULL, on_myfiles_setting_changed, 0, "<CheckItem>"},
  { N_("/_Go To"),         NULL,         NULL,           0, "<Branch>" },
  { N_("/_Help"),         NULL,         NULL,           0, "<Branch>" },
  { N_("/_Help/Index"),   NULL,         on_help_clicked,    0, "<StockItem>",GTK_STOCK_HELP },
  { N_("/_Help/About"),   NULL,         on_about_clicked,    0, "<Item>" },
};

int mMain_items_count = sizeof(mMain_items) / sizeof(GtkItemFactoryEntry);

/* create menu from description */
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
  
  set_dirbrowser_menu_item = 
    gtk_item_factory_get_item (itemfactory, N_("/Settings/Directory Browser"));
  set_myfiles_menu_item = 
    gtk_item_factory_get_item (itemfactory, N_("/Settings/My Files only"));
  goto_menu_item = 
    gtk_item_factory_get_item (itemfactory, N_("/Go To"));
  
 gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(set_dirbrowser_menu_item),
                                 directory_browser); 
  
  return (gtk_item_factory_get_widget (itemfactory, "<main>"));
}


void
show_message(GtkMessageType type, char* message)
{
	GtkWidget* dialog;
	
	dialog = gtk_message_dialog_new (GTK_WINDOW(window),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 type,
					 GTK_BUTTONS_OK,
					 message);
	gtk_dialog_run (GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void
on_dirbrowser_setting_changed(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  if (!initialized) 
    return;
  directory_browser = 
    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(set_dirbrowser_menu_item));
  if (directory_browser)
    gtk_widget_show_all(dir_view_window);
  else
	gtk_widget_hide(dir_view_window);
  refresh_current_directory();
}

void
on_myfiles_setting_changed(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  gboolean enabled;
  if (!initialized) 
    return;
  limited_view = 
    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(set_myfiles_menu_item));
  enabled = (!limited_view || strcmp(current_directory,g_get_home_dir()));
  gtk_widget_set_sensitive(btnGoUp, enabled);
}

void
on_about_clicked (GtkWidget * w)
{
	show_message(GTK_MESSAGE_INFO, HELPMESSAGE);
}


void
on_help_clicked (GtkWidget * w)
{
	if (gpe_show_help(PACKAGE,NULL))
		show_message(GTK_MESSAGE_INFO, NOHELPMESSAGE);
}


static void
progress_response(GtkDialog *dialog, gint arg1, gpointer user_data)
{
  /* check and break */
  abort_transfer = TRUE;  
}

static GtkWidget*
progress_dialog_create(gchar *title)
{
  GtkWidget *dialog;
  GtkWidget *progress;
  dialog = gtk_dialog_new_with_buttons (title, GTK_WINDOW (window), 
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
         progress = gtk_progress_bar_new();
         gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                            progress,FALSE,TRUE,0);
         g_object_set_data(G_OBJECT(dialog),"bar",progress);
         g_signal_connect(G_OBJECT(dialog),"response",
                          G_CALLBACK(progress_response),NULL);
         gtk_widget_show_all(dialog);
  return dialog;
}

static gint 
transfer_callback(GnomeVFSXferProgressInfo *info, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *label;
  gchar *text;
  int response;
  gboolean applytoall;

  if (abort_transfer)
  {
    gtk_widget_destroy(progress_dialog);
    progress_dialog = NULL;
    abort_transfer = FALSE;
    return 0;
  }
  switch (info->status)
  {
	  case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
          if (progress_dialog == NULL)
              progress_dialog = progress_dialog_create("Transfer progress");
          else if (info->bytes_total)
          {
            GtkWidget *bar = g_object_get_data(G_OBJECT(progress_dialog),"bar");
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar),
                                          (gdouble)info->total_bytes_copied
                                          /(gdouble)(info->bytes_total));
          }
          if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED)
          {
            gtk_widget_destroy(progress_dialog);
            progress_dialog = NULL;
          }
          gtk_main_iteration_do(FALSE);
          return 1;
	  break;
	  case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
		  /* error query dialog */
          text = g_strdup_printf ("Error: %s", 
            gnome_vfs_result_to_string(info->vfs_status));
          dialog = gtk_dialog_new_with_buttons ("Error", GTK_WINDOW (window), 
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
             "_Skip", GTK_RESPONSE_HELP,
             "_Retry", GTK_RESPONSE_APPLY, 
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
         label = gtk_label_new(text);
         g_free(text);
         gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
         gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,FALSE,TRUE,0);
         gtk_widget_show_all(dialog);
         response = gtk_dialog_run(GTK_DIALOG(dialog));
         gtk_widget_destroy(dialog);
         switch (response)
         {
             case GTK_RESPONSE_CANCEL: 
                 gtk_widget_destroy(progress_dialog);
                 progress_dialog = NULL;
                 return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
             break;
             case GTK_RESPONSE_HELP: 
                 return GNOME_VFS_XFER_ERROR_ACTION_SKIP;
             break;
             case GTK_RESPONSE_APPLY: 
                 return GNOME_VFS_XFER_ERROR_ACTION_RETRY;
             break;
             default: 
                 gtk_widget_destroy(progress_dialog);
                 progress_dialog = NULL;
                 return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
             break;
         }
	  break;
	  case GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
		 /* overwrite query dialog */
          text = g_strdup_printf ("Overwrite file:\n%s", info->target_name);
          dialog = gtk_dialog_new_with_buttons ("File exists", GTK_WINDOW (window), 
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
             GTK_STOCK_YES, GTK_RESPONSE_YES,
             GTK_STOCK_NO, GTK_RESPONSE_NO, NULL);
         label = gtk_label_new(text);
         g_free(text);
         gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,FALSE,TRUE,0);
         label = gtk_check_button_new_with_label("Apply to all");
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),FALSE);
         gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,FALSE,TRUE,0);
         gtk_widget_show_all(dialog);
         response = gtk_dialog_run(GTK_DIALOG(dialog));
         applytoall = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(label));
         gtk_widget_destroy(dialog);
         switch (response)
         {
           case GTK_RESPONSE_CANCEL:
             gtk_widget_destroy(progress_dialog);
             progress_dialog = NULL;
             return GNOME_VFS_XFER_OVERWRITE_ACTION_ABORT;
           break;
           case GTK_RESPONSE_YES:
             if (applytoall)                 
                 return GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE_ALL;
             else
                 return GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE;
           break;
           case GTK_RESPONSE_NO: 
               if (applytoall)
                 return GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP_ALL;
               else
                 return GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP;
           break;
           default:
             gtk_widget_destroy(progress_dialog);
             progress_dialog = NULL;
             return GNOME_VFS_XFER_OVERWRITE_ACTION_ABORT;
           break;
         }
	  break;
     default:
        return 1; /* we should never get there */
     break;	  
  }
}


static void 
auth_callback (gconstpointer in,
               gsize         in_size,
               gpointer      out,
               gsize         out_size,
               gpointer      callback_data)
{
  const GnomeVFSModuleCallbackAuthenticationIn *q_in = in;
  GnomeVFSModuleCallbackAuthenticationOut *q_out = out;
  GtkWidget *dialog_window;
  GtkWidget *table, *label, *entry_user, *entry_passwd;
  gchar *label_text;
  struct passwd *pwd = getpwuid(getuid());

  q_out->username = NULL;
  q_out->password = NULL;

  if (q_in->previous_attempt_failed)
    label_text = g_strdup_printf ("<b>Login failed, please try again</b>\n%s", q_in->uri);
  else
    label_text = g_strdup_printf ("<b>Enter credentials to access</b>\n%s", q_in->uri);

  dialog_window = gtk_dialog_new_with_buttons ("Restricted Resource", 
    GTK_WINDOW (window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
    GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, 
    GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
  
  table = gtk_table_new(3,2,FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
  
  label = gtk_label_new (NULL);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
  gtk_label_set_markup(GTK_LABEL(label),label_text);
  g_free (label_text);

  entry_user = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(entry_user),pwd->pw_name);
  entry_passwd = gtk_entry_new ();
  gtk_entry_set_visibility(GTK_ENTRY(entry_passwd),FALSE);

  gtk_table_attach(GTK_TABLE(table),entry_user,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),entry_passwd,1,2,2,3,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),label,0,2,0,1,GTK_FILL,GTK_FILL,0,0);
  label = gtk_label_new("Username");
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
  gtk_table_attach(GTK_TABLE(table),label,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
  label = gtk_label_new("Password");
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
  gtk_table_attach(GTK_TABLE(table),label,0,1,2,3,GTK_FILL,GTK_FILL,0,0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog_window)->vbox),table);

  gtk_widget_show_all (dialog_window);
  if (gtk_dialog_run(GTK_DIALOG(dialog_window)) == GTK_RESPONSE_ACCEPT)
  {
	q_out->username = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_user)));
	q_out->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_passwd)));
  }
  gtk_widget_destroy(dialog_window);
}

static void
create_directory_interactive(void)
{
  GtkWidget *dialog;
  GtkWidget *entry, *btnOK;
  int response;
  gchar *directory, *diruri;

  dialog = gtk_message_dialog_new (GTK_WINDOW (window), 
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
    GTK_MESSAGE_QUESTION,
    GTK_BUTTONS_CANCEL, "Create directory");
  entry = gtk_entry_new ();

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry, FALSE, TRUE, 0);
  btnOK = gtk_dialog_add_button(GTK_DIALOG(dialog),GTK_STOCK_OK,GTK_RESPONSE_OK);

  gtk_widget_show_all (dialog);
  gtk_widget_grab_focus(GTK_WIDGET(entry));
  GTK_WIDGET_SET_FLAGS(btnOK,GTK_CAN_DEFAULT);
  gtk_entry_set_activates_default(GTK_ENTRY(entry),TRUE);
  gtk_widget_grab_default(btnOK);
  
  response = gtk_dialog_run(GTK_DIALOG(dialog));
  if (response == GTK_RESPONSE_OK)
  {
    directory = 
      g_strdup_printf("%s/%s",current_directory,
        gtk_entry_get_text(GTK_ENTRY(entry)));
    diruri = gnome_vfs_get_uri_from_local_path(directory);
    response = gnome_vfs_make_directory(diruri,S_IRWXU);
    if (response != GNOME_VFS_OK)
      gpe_error_box (gnome_vfs_result_to_string(response));
    else
      {
        gpe_popup_infoprint(GDK_DISPLAY(), _("Directory created."));
        refresh_current_directory();
      }
  }
  gtk_widget_destroy(dialog);
}

static void
copy_file (const gchar *src_uri_txt, const gchar *dest_uri_txt)
{
  GnomeVFSURI *uri_src = gnome_vfs_uri_new(src_uri_txt);
  GnomeVFSURI *uri_dest = gnome_vfs_uri_new(dest_uri_txt);
  GnomeVFSResult result;
  gchar *error;
  
      
  result = gnome_vfs_xfer_uri (uri_src,
                               uri_dest,
                               GNOME_VFS_XFER_FOLLOW_LINKS 
                                 | GNOME_VFS_XFER_RECURSIVE 
                                 | GNOME_VFS_XFER_EMPTY_DIRECTORIES,
                               GNOME_VFS_XFER_ERROR_MODE_QUERY,
                               GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,
                               (GnomeVFSXferProgressCallback) transfer_callback,
                               NULL);
  if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_INTERRUPTED))
    {
      error = g_strdup_printf ("Error: %s", gnome_vfs_result_to_string(result));
      gpe_error_box (error);
      g_free (error);
    }
  else
    {
      gpe_popup_infoprint(GDK_DISPLAY(), _("File copied."));
    }
  gnome_vfs_uri_unref(uri_src);
  gnome_vfs_uri_unref(uri_dest);
}

static void 
clear_clipboard (void)
{
  GList *iter = file_clipboard;
  
  if (!file_clipboard) 
    return;
  
  while (iter)
  {
    g_free(iter->data);
    iter = iter->next;
  }
  g_list_free(file_clipboard);
  file_clipboard = NULL;
}

static void
clip_one_file (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
               gpointer data)
{
  FileInformation *cfi = NULL;
  gint col = (gint)data;

  gtk_tree_model_get(model, iter, col, &cfi, -1);
  if (cfi)
  {
    if (GNOME_VFS_FILE_INFO_LOCAL(cfi->vfs))
      file_clipboard = g_list_append(file_clipboard, 
                                     gnome_vfs_get_uri_from_local_path(cfi->filename));
    else
      file_clipboard = g_list_append(file_clipboard, g_strdup(cfi->filename));
  }
}

static void 
copy_file_clip (void)
{
  clear_clipboard();
  GtkTreeSelection *sel = NULL;
  gint col;

  if (active_view)
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(active_view));
  if (active_view == view_widget)
    col = COL_DATA;
  else
    col = COL_DIRDATA;
  
  if (!sel) 
    return;
  
  gtk_tree_selection_selected_foreach(sel, 
                                     (GtkTreeSelectionForeachFunc)clip_one_file,
                                     (gpointer)col);
  
  gpe_popup_infoprint(GDK_DISPLAY(), _("File(s) copied to clipboard."));
}


static void
copy_one_file (gpointer file, gpointer userdata)
{
  gchar *target_file;
  target_file = 
    g_strdup_printf("%s/%s",
                    current_directory,
                    g_path_get_basename(file));
  if (strcmp(file, target_file)) /* check if not the same */
    copy_file(file, target_file);
  g_free(target_file);
}

static void 
paste_file_clip (void)
{
  if (file_clipboard == NULL) 
    return;
  
  g_list_foreach(file_clipboard, copy_one_file, NULL);
  
  refresh_current_directory(); 
}

static void
hide_menu (void)
{
  if (view_is_icons)
    gpe_icon_list_view_popup_removed (GPE_ICON_LIST_VIEW (view_widget));
}

static void
kill_widget (GtkObject *object, GtkWidget *widget)
{
  gtk_widget_destroy (widget);
}

static void
safety_check (void)
{
  if (loading_directory == 1)
  {
    loading_directory = 0;
    sleep (0.1);
  }
}

GtkWidget 
*create_icon_pixmap (GtkStyle *style, char *fn, int size)
{
  GdkPixbuf *pixbuf, *spixbuf;
  GtkWidget *w;
  pixbuf = gdk_pixbuf_new_from_file (fn, NULL);
  if (pixbuf == NULL)
    return NULL;

  spixbuf = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_BILINEAR);
  gdk_pixbuf_unref (pixbuf);
  w = gtk_image_new_from_pixbuf(spixbuf);
  gdk_pixbuf_unref (spixbuf);

  return w;
}


static void
open_with (GnomeVFSMimeApplication *application, 
           FileInformation *file_info)
{
  pid_t pid;
    
  if (application)
  {
    gchar *msg = g_strdup_printf("%s %s",_("Starting"),application->name);
    Display *dpy = GDK_DISPLAY();
      
    gpe_popup_infoprint(dpy, msg);
    g_free(msg);
      
	pid = fork();
	switch (pid)
	{
		case -1: 
			return; /* failed */
		break;
		case  0: 
          if (application->requires_terminal)
               execlp(DEFAULT_TERMINAL,DEFAULT_TERMINAL,
                      application->command,
                      file_info->filename);
          else
               execlp(application->command,
                      application->command,
                      file_info->filename,
                      NULL);
          exit(-1); /* die in case of failing to exec */
		break;
		default: 
		break;
	} 
  }
}


static void
rename_file (GtkWidget *dialog_window, gint response_id)
{
  gchar *dest, *error;
  GnomeVFSResult result;

  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      gchar *tmp = g_strdup (current_popup_file->filename);
      dest = g_strdup_printf ("%s/%s", dirname (tmp), gtk_entry_get_text (GTK_ENTRY (g_object_get_data (G_OBJECT (dialog_window), "entry"))));
      g_free (tmp);
      result = gnome_vfs_move_uri (gnome_vfs_uri_new (current_popup_file->filename), gnome_vfs_uri_new (dest), TRUE);
    
      if (result != GNOME_VFS_OK)
      {
        switch (result)
          {
          case GNOME_VFS_ERROR_READ_ONLY:
            error = g_strdup ("File is read only.");
            break;
          case GNOME_VFS_ERROR_ACCESS_DENIED:
            error = g_strdup ("Access denied.");
            break;
          case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
            error = g_strdup ("Read only file system.");
            break;
          case GNOME_VFS_ERROR_FILE_EXISTS:
            error = g_strdup ("Destination file already exists.");
            break;
          default:
            error = g_strdup_printf ("Error: %s", 
                                     gnome_vfs_result_to_string(result));
            break;
          }
          gpe_error_box (error);
          g_free (error);
        }
      else
        {
          gpe_popup_infoprint(GDK_DISPLAY(), _("File renamed."));
        }
      g_free (dest);
    }

  gtk_widget_destroy (dialog_window);
  refresh_current_directory();
}

static void
move_file (gchar *directory)
{
  gchar *dest, *error;
  GnomeVFSResult result;

  dest = g_strdup_printf ("%s/%s", directory, current_popup_file->vfs->name);

  result = gnome_vfs_move_uri (gnome_vfs_uri_new (current_popup_file->filename), gnome_vfs_uri_new (dest), TRUE);

  if (result != GNOME_VFS_OK)
    {
      switch (result)
      {
      case GNOME_VFS_ERROR_NO_SPACE:
        error = g_strdup ("No space left on device.");
        break;
      case GNOME_VFS_ERROR_READ_ONLY:
        error = g_strdup ("Destination is read only.");
        break;
      case GNOME_VFS_ERROR_ACCESS_DENIED:
        error = g_strdup ("Access denied.");
        break;
      case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
        error = g_strdup ("Read only file system.");
        break;
      default:
        error = g_strdup_printf ("Error: %s", gnome_vfs_result_to_string(result));
        break;
      }
      gpe_error_box (error);
      g_free (error);
    }
  else
    {
      gpe_popup_infoprint(GDK_DISPLAY(), _("File moved."));
      refresh_current_directory();
    }
  g_free (dest);
}

static void
popup_ask_move_file ()
{
  GtkWidget *dirbrowser_window;

  dirbrowser_window = gpe_create_dir_browser (_("Move to directory..."), 
    (gchar *) g_get_home_dir (), GTK_SELECTION_SINGLE, move_file);
  gtk_window_set_transient_for (GTK_WINDOW (dirbrowser_window), GTK_WINDOW (window));

  gtk_widget_show_all (dirbrowser_window);
}

static void
popup_ask_rename_file ()
{
  GtkWidget *dialog_window;
  GtkWidget *vbox, *label, *entry, *btnok;
  gchar *label_text;

  label_text = g_strdup_printf ("Rename file %s to:", current_popup_file->vfs->name);

  dialog_window = gtk_dialog_new_with_buttons ("Rename file", 
                                               GTK_WINDOW (window), 
                                               GTK_DIALOG_MODAL 
                                                 | GTK_DIALOG_DESTROY_WITH_PARENT, 
                                               GTK_STOCK_CANCEL, 
                                               GTK_RESPONSE_REJECT, NULL);
  btnok = gtk_dialog_add_button(GTK_DIALOG(dialog_window), 
                                GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
  GTK_WIDGET_SET_FLAGS(btnok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(btnok);
  g_signal_connect (G_OBJECT (dialog_window), "delete_event", 
                    G_CALLBACK (kill_widget), dialog_window);
  g_signal_connect (G_OBJECT (dialog_window), "response", 
                    G_CALLBACK (rename_file), NULL);
  
  vbox = gtk_vbox_new (FALSE, 0);

  label = gtk_label_new (label_text);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  g_free (label_text);

  entry = gtk_entry_new ();
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
  g_object_set_data (G_OBJECT (dialog_window), "entry", entry);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog_window)->vbox), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  gtk_widget_show_all (dialog_window);
}

gboolean 
delete_cb_recursive(const gchar *rel_path,
                   GnomeVFSFileInfo *info,
                   gboolean recursing_will_loop,
                   gpointer data,
                   gboolean *recurse)
{
   gchar *path = data;
   gchar *fn = g_strdup_printf("%s/%s", path, rel_path);
  
   if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
   {
     *recurse = TRUE;
     GnomeVFSResult r = 0;
     if (!recursing_will_loop)
     r = gnome_vfs_directory_visit (fn,
                                GNOME_VFS_FILE_INFO_DEFAULT,
                                GNOME_VFS_DIRECTORY_VISIT_DEFAULT,
                                (GnomeVFSDirectoryVisitFunc) delete_cb_recursive,
                                strdup(fn)); 
     /* HACK: We collect the empty directories in a list to delete after 
              visiting the complete tree to prevent gnome-vfs from stopping
              visiting directories 
     */
     if (r == GNOME_VFS_OK)
     {
       dirlist = g_list_append(dirlist, fn);
     }
     else
       fprintf(stderr, "Err %s\n", gnome_vfs_result_to_string(r));

   }
   else
   {
     gnome_vfs_unlink_from_uri (gnome_vfs_uri_new (fn));
     g_free(fn);
   }
   
   return TRUE;
}


static GnomeVFSResult
delete_directory_recursive(FileInformation *dir)
{
  gchar *path = g_strdup(dir->filename);
  GList *iter;
  gnome_vfs_directory_visit    (dir->filename,
                                GNOME_VFS_FILE_INFO_DEFAULT,
                                GNOME_VFS_DIRECTORY_VISIT_DEFAULT,
                                (GnomeVFSDirectoryVisitFunc) delete_cb_recursive,
                                path); 
  g_free(path);
  iter = dirlist;
  while (iter)
    {
      gnome_vfs_remove_directory(iter->data);
      g_free(iter->data);
      iter=iter->next;
    }
  g_list_free(dirlist);
  dirlist = NULL;    
  return gnome_vfs_remove_directory_from_uri(gnome_vfs_uri_new (dir->filename));
}


static void
delete_one_file (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
                 gpointer data)
{
  gboolean isdir;
  GnomeVFSURI *uri;
  GnomeVFSResult r;
  gint col = (gint)data;

  FileInformation *cfi = NULL;
  
  gtk_tree_model_get(model, iter, col, &cfi, -1);
  if (cfi)
  {
    isdir = (cfi->vfs->type == GNOME_VFS_FILE_TYPE_DIRECTORY);
      if (isdir)
        r = delete_directory_recursive(cfi);
      else
        {
          uri = gnome_vfs_uri_new (cfi->filename);
          r = gnome_vfs_unlink_from_uri (uri);
          gnome_vfs_uri_unref(uri);
        }

      if (r != GNOME_VFS_OK)
        gpe_error_box (gnome_vfs_result_to_string (r));
  }
}


static void
popup_ask_delete_file ()
{
  GtkTreeSelection *sel = NULL;
  gint col;
  
  if (active_view)
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(active_view));
  if (active_view == view_widget)
    col = COL_DATA;
  else
    col = COL_DIRDATA;
  
  if (!sel) 
    return;
  
  if (gtk_tree_selection_count_selected_rows(sel))
  {
    if (gpe_question_ask (_("Delete selected object(s)?"), 
                          _("Confirm"), "!gtk-dialog-question", 
	                     "!gtk-cancel", NULL, "!gtk-delete", NULL, 
                         NULL, NULL) == 1)
      {
        gtk_tree_selection_selected_foreach(sel, 
                                            (GtkTreeSelectionForeachFunc)delete_one_file,
                                            (gpointer)col);
        refresh_current_directory();
      }
  }
}

static void
show_file_properties ()
{
  GtkWidget *infodialog;
  GtkWidget *label; /* will be used to hold several labels */
  GtkWidget *table;
  gchar *text;

  infodialog = gtk_message_dialog_new(GTK_WINDOW(window),
	                                  GTK_DIALOG_DESTROY_WITH_PARENT 
	                                    | GTK_DIALOG_MODAL,
	                                  GTK_MESSAGE_INFO,
	                                  GTK_BUTTONS_CLOSE,
	                                  current_popup_file->vfs->name);
  table = gtk_table_new(4,2,FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
  
  /* location */
  text = g_strdup_printf("<b>%s</b>","Location:");
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),1,0.5); /* align right, gnomish */
  gtk_label_set_markup(GTK_LABEL(label),text);
  g_free(text);
  gtk_table_attach(GTK_TABLE(table),label,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
  text = g_strdup(current_popup_file->filename);
  if (strrchr(text,'/')) /* eliminate name */
	  strrchr(text,'/')[0] = 0;
  label = gtk_label_new(text);
  g_free(text);
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5); 
  gtk_table_attach(GTK_TABLE(table),label,1,2,0,1,GTK_FILL,GTK_FILL,0,0);
	
  /* size */
  text = g_strdup_printf("<b>%s</b>","Size:");
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),1,0.5); /* align right, gnomish */
  gtk_label_set_markup(GTK_LABEL(label),text);
  g_free(text);
  gtk_table_attach(GTK_TABLE(table),label,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
  text = gnome_vfs_format_file_size_for_display(current_popup_file->vfs->size);
  label = gtk_label_new(text);
  g_free(text);
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5); 
  gtk_table_attach(GTK_TABLE(table),label,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
  
  /* mime type */
  text = g_strdup_printf("<b>%s</b>","MIME-Type:");
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),1,0.5); /* align right, gnomish */
  gtk_label_set_markup(GTK_LABEL(label),text);
  g_free(text);
  gtk_table_attach(GTK_TABLE(table),label,0,1,2,3,GTK_FILL,GTK_FILL,0,0);
  label = gtk_label_new(current_popup_file->vfs->mime_type);
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5); 
  gtk_table_attach(GTK_TABLE(table),label,1,2,2,3,GTK_FILL,GTK_FILL,0,0);
  
  /* change time */
  text = g_strdup_printf("<b>%s</b>","Changed:");
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),1,0.5); /* align right, gnomish */
  gtk_label_set_markup(GTK_LABEL(label),text);
  g_free(text);
  gtk_table_attach(GTK_TABLE(table),label,0,1,3,4,GTK_FILL,GTK_FILL,0,0);
  text = g_strdup(ctime(&current_popup_file->vfs->ctime));
  if ((text) && text[strlen(text)-1] == '\n') 
	text[strlen(text)-1] = 0;
  label = gtk_label_new(text);
  g_free(text);
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5); 
  gtk_table_attach(GTK_TABLE(table),label,1,2,3,4,GTK_FILL,GTK_FILL,0,0);
  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(infodialog)->vbox),table,FALSE,TRUE,0);
  gtk_widget_show_all(infodialog);
  gtk_dialog_run(GTK_DIALOG(infodialog));
  gtk_widget_destroy(infodialog);
}

static void
ask_open_with (FileInformation *file_info)
{
  GtkWidget *dialog_window, *combo, *label, *entry;
  GtkWidget *open_button, *cancel_button;
  GList *applications = NULL;
  GList *appnames = NULL;

  dialog_window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(dialog_window), "Open With...");
  gtk_window_set_transient_for (GTK_WINDOW(dialog_window), GTK_WINDOW(window));
  gtk_window_set_modal (GTK_WINDOW (dialog_window), TRUE);
  
  label = gtk_label_new (NULL);
  gtk_label_set_markup(GTK_LABEL(label), _("<b>Open with program</b>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  entry = gtk_entry_new ();

  combo = gtk_combo_new();
  gtk_combo_set_value_in_list(GTK_COMBO(combo), TRUE, FALSE);
    
  cancel_button = gtk_dialog_add_button(GTK_DIALOG(dialog_window), 
                                        GTK_STOCK_CANCEL, 
                                        GTK_RESPONSE_CANCEL);

  open_button = gtk_dialog_add_button(GTK_DIALOG(dialog_window), GTK_STOCK_OK, 
                                      GTK_RESPONSE_OK);
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG (dialog_window)->vbox), 
                                 gpe_get_border());
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG (dialog_window)->vbox), 
                                 gpe_get_boxspacing());
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), label, 
                      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), combo, 
                      TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS(open_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(open_button);

  if (file_info->vfs->mime_type)
    {
      applications = 
        gnome_vfs_application_registry_get_applications(file_info->vfs->mime_type);
      
      if (applications) /* translate list, get app info */
        {
          GList *iter;
          for (iter = applications; iter; iter = iter->next)
            iter->data = 
              gnome_vfs_application_registry_get_mime_application(iter->data);
        }
      else  
        applications = 
          gnome_vfs_mime_get_short_list_applications (file_info->vfs->mime_type);
    }
  else
    applications = 
      gnome_vfs_mime_get_all_applications (file_info->vfs->mime_type);

  /* fill combo with application names */
  if (applications)
  {
    GnomeVFSMimeApplication *app; 
    GList *iter = applications;
    
    while (iter)
    {
      app = (GnomeVFSMimeApplication *)(iter->data);

      appnames = g_list_append(appnames, app->name);
      iter = iter->next;
    }
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), appnames);
  }
  else
  {
    gtk_widget_set_sensitive (combo, FALSE);
    gtk_widget_set_sensitive (open_button, FALSE);
    gtk_label_set_markup (GTK_LABEL (label), 
                          _("<b>No available applications</b>"));
  }

  gtk_widget_show_all (dialog_window);
  
  /* run dialog */
  if (gtk_dialog_run(GTK_DIALOG(dialog_window)) == GTK_RESPONSE_OK)
    {
      GnomeVFSMimeApplication *app; 
      GList *iter = applications;
      const gchar *selected_app = 
        gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
      
      while (iter)
      {
        app = (GnomeVFSMimeApplication *)(((GList *)iter)->data);
  
        if (!strcmp(selected_app, app->name))
          {
            open_with(app, file_info);
            break;
          }
        iter = iter->next;
      }
    }
    
  gtk_widget_destroy(dialog_window);
  g_list_free(appnames);
}

static void
send_with_bluetooth (void)
{
  bluetooth_send_file (current_popup_file->filename, current_popup_file->vfs);
}

static void
send_with_irda (void)
{
  irda_send_file (current_popup_file->filename, current_popup_file->vfs);
}

static void
popup_ask_open_with ()
{
  ask_open_with (current_popup_file);
}

void
show_popup (GtkWidget *widget, gpointer udata)
{
  FileInformation *file_info;

  file_info = (FileInformation *) udata;
	
  if (file_info == NULL)
  {
    gtk_widget_set_sensitive(open_menu_item,FALSE);
    gtk_widget_set_sensitive(copy_menu_item,FALSE);
    gtk_widget_set_sensitive(bluetooth_menu_item,FALSE);
    gtk_widget_set_sensitive(irda_menu_item,FALSE);
    gtk_widget_set_sensitive(move_menu_item,FALSE);
    gtk_widget_set_sensitive(rename_menu_item,FALSE);
    gtk_widget_set_sensitive(properties_menu_item,FALSE);
    gtk_widget_set_sensitive(delete_menu_item,FALSE);
  }
  else
  {	  
    current_popup_file = file_info;

    gtk_widget_set_sensitive(open_menu_item,TRUE);
    gtk_widget_set_sensitive(copy_menu_item,TRUE);
    gtk_widget_set_sensitive(move_menu_item,TRUE);
    gtk_widget_set_sensitive(rename_menu_item,TRUE);
    gtk_widget_set_sensitive(properties_menu_item,TRUE);
    gtk_widget_set_sensitive(delete_menu_item,TRUE);
	  
    if (bluetooth_available ())
      gtk_widget_set_sensitive (bluetooth_menu_item, TRUE);
    else
      gtk_widget_set_sensitive (bluetooth_menu_item, FALSE);
    if (irda_available ())
      gtk_widget_set_sensitive (irda_menu_item, TRUE);
    else
      gtk_widget_set_sensitive (irda_menu_item, FALSE);
  }
  if (file_clipboard)
    gtk_widget_set_sensitive (paste_menu_item, TRUE);
  else
    gtk_widget_set_sensitive (paste_menu_item, FALSE);
  gtk_menu_popup (GTK_MENU (gtk_item_factory_get_widget (item_factory, "<main>")), 
		  NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time ());
}

void
button_clicked (GtkWidget *widget, gpointer udata)
{
  GnomeVFSMimeApplication *default_mime_application;
  FileInformation *file_info;
    
  file_info = (FileInformation *) udata;

  if (file_info == NULL)
    return;
  
  /* Handle Symbolic Links -- CM */
  if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK) {
    GnomeVFSURI *uri = gnome_vfs_uri_new (file_info->vfs->symlink_name);
    GnomeVFSFileInfo *vfs = gnome_vfs_file_info_new ();
    int cur_depth = MAX_SYM_DEPTH;

    do {
      uri = gnome_vfs_uri_new (file_info->vfs->symlink_name);
      file_info->vfs = gnome_vfs_file_info_new ();

      if (gnome_vfs_get_file_info_uri (uri, vfs, GNOME_VFS_FILE_INFO_DEFAULT)
          != GNOME_VFS_OK) {
            gpe_error_box_fmt (_("Symbolic link %s leads to inaccessable file!"), file_info->filename);
            return;
      }

      file_info->vfs = vfs;
    } while (file_info->vfs->type == GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK && (--cur_depth > 0));

    if (cur_depth <= 0) {
      gpe_error_box_fmt (_("Maximum symbolic link depth exceeded for %s"), file_info->filename);
      return;
    }
  }
  
    if ((file_info->vfs->mime_type) && 
		((!strcmp(file_info->vfs->mime_type,"application/x-desktop") && !file_info->vfs->size) 
		 || strstr(file_info->vfs->mime_type,"x-directory/")))
        browse_directory (file_info->filename);
    else    
      if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_REGULAR || file_info->vfs->type == GNOME_VFS_FILE_TYPE_UNKNOWN)
      {
        if (file_info->vfs->mime_type)
        {
          default_mime_application = gnome_vfs_mime_get_default_application (file_info->vfs->mime_type);
          if (default_mime_application != NULL)
          {
            open_with(default_mime_application, file_info);
          }
          else
            ask_open_with (file_info);
        }
        else
          ask_open_with (file_info);
      }
      else if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
        browse_directory (file_info->filename);
}


gchar *
find_icon_path (gchar *mime_type)
{
  struct stat s;
  gchar *mime_icon;
  gchar *mime_path, *p;

  mime_icon = g_strdup(gnome_vfs_mime_get_icon (mime_type));
  if (mime_icon)
  {
    if (mime_icon[0] == '/')
    {
      if (stat (mime_icon, &s) == 0)
        return mime_icon;
    }

    mime_path = g_strdup_printf (PREFIX FILEMANAGER_ICON_PATH "/%s", mime_icon);
    if (stat (mime_path, &s) == 0)
      return mime_path;

    mime_path = g_strdup_printf (PREFIX DEFAULT_ICON_PATH "/%s", mime_icon);
    if (stat (mime_path, &s) == 0)
      return mime_path;
  }

  mime_icon = g_strdup (mime_type);
  while ((p = strchr(mime_icon, '/')) != NULL)
    *p = '-';

  mime_path = g_strdup_printf (PREFIX FILEMANAGER_ICON_PATH "/%s.png", mime_icon);
  if (stat (mime_path, &s) == 0)
  {
	g_free(mime_icon);
    return mime_path;
  }
  mime_path = g_strdup_printf (PREFIX DEFAULT_ICON_PATH "/%s.png", mime_icon);
  if (stat (mime_path, &s) == 0)
  {
	g_free(mime_icon);
    return mime_path;
  }
  
  g_free(mime_icon);
  return g_strdup (PREFIX FILEMANAGER_ICON_PATH "/regular.png");
}

GdkPixbuf *
get_pixbuf (const gchar *filename)
{
  GdkPixbuf *pixbuf;

  pixbuf = g_hash_table_lookup (loaded_icons, (gconstpointer) filename);

  if (pixbuf)
  {
    return pixbuf;
  }
  else
  {
    pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
    g_hash_table_insert (loaded_icons, (gpointer) filename, (gpointer) pixbuf);
    return pixbuf;
  }
}

void
add_icon (FileInformation *file_info)
{
  GdkPixbuf *pixbuf = NULL;
  gchar *mime_icon;
  
  if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
    mime_icon = g_strdup (PREFIX FILEMANAGER_ICON_PATH "/directory.png");
  else if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_REGULAR || file_info->vfs->type == GNOME_VFS_FILE_TYPE_UNKNOWN)
  {
   file_info->vfs->mime_type = gnome_vfs_get_mime_type (file_info->filename);
    if (file_info->vfs->mime_type)
      mime_icon = find_icon_path (file_info->vfs->mime_type);
    else
      mime_icon = g_strdup (PREFIX FILEMANAGER_ICON_PATH "/regular.png");
  }
  else if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK)
    mime_icon = g_strdup (PREFIX FILEMANAGER_ICON_PATH "/symlink.png");
  else
    mime_icon = g_strdup (PREFIX FILEMANAGER_ICON_PATH "/regular.png");

  pixbuf = get_pixbuf (mime_icon);
  g_free(mime_icon);
  
  /* now be careful, we sort the items into the view widgets */
  if ((directory_browser) && (file_info->vfs->type == GNOME_VFS_FILE_TYPE_DIRECTORY))
    { /* add to directory browser */
		GtkTreeIter iter;
		GdkPixbuf *pb;
		  
		gtk_tree_store_append (dirstore, &iter, NULL);
		pb = gdk_pixbuf_scale_simple(pixbuf,24,24,GDK_INTERP_BILINEAR);
		
		gtk_tree_store_set (dirstore, 
		                    &iter,
		                    COL_DIRICON, pb,
			                COL_DIRNAME, file_info->vfs->name,
			                COL_DIRDATA, (gpointer) file_info,
			                -1);
		gdk_pixbuf_unref(pb);
    }
  else
    { /* add to iconlist or normal browser */
	  if (view_is_icons)
		gpe_icon_list_view_add_item_pixbuf (GPE_ICON_LIST_VIEW (view_widget), 
                                            file_info->vfs->name, pixbuf, 
                                            (gpointer) file_info);
	  else
	  {
		GtkTreeIter iter;
		GdkPixbuf *pb;
		gchar *size;
		gchar *time;
		  
		pb = gdk_pixbuf_scale_simple(pixbuf,24,24,GDK_INTERP_BILINEAR);
		size = gnome_vfs_format_file_size_for_display(file_info->vfs->size); 
		time = g_strdup(ctime(&file_info->vfs->ctime));
		time[strlen(time)-1] = 0; /* strip newline */
		  
		gtk_tree_store_append (store, &iter, NULL);
		
		gtk_tree_store_set (store, &iter,
			COL_NAME, file_info->vfs->name,
			COL_SIZE, size,
			COL_CHANGED, time,
			COL_ICON, pb,
			COL_DATA, (gpointer) file_info,
			-1);
		g_free(size);
		g_free(time);
		gdk_pixbuf_unref(pb);
	  }
  }  
}

gint
sort_filenames (gconstpointer *a, gconstpointer *b)
{
  return (strcoll ((char *) a, (char *) b));
}

gint
compare_file_info (FileInformation *a, FileInformation *b)
{
  return strcoll (a->filename, b->filename);
}

/* Render the contents for the current directory. */
static void
make_view (void)
{
  GnomeVFSDirectoryHandle *handle = NULL;
  GnomeVFSFileInfo *vfs_file_info = NULL;
  GnomeVFSResult result = GNOME_VFS_OK, open_dir_result;
  GList *list = NULL, *iter;
  gchar *error = NULL;
  loading_directory = 1;

  loaded_icons = g_hash_table_new (g_str_hash, g_str_equal);
  if (view_is_icons)
    gpe_icon_list_view_clear (GPE_ICON_LIST_VIEW (view_widget));
  else
  {
	GtkTreeIter iter;
	FileInformation *i;
	GdkPixbuf *buf;
 
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),&iter))
	{
		do
		{
			gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,COL_DATA,&i,COL_ICON,&buf,-1);
			gnome_vfs_file_info_unref(i->vfs);
			gdk_pixbuf_unref(buf);
			g_free(i->filename);
		}while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store),&iter));
	}
    gtk_tree_store_clear(GTK_TREE_STORE(store));
	if (directory_browser)
        gtk_tree_store_clear(GTK_TREE_STORE(dirstore));
  }
  
  gtk_widget_draw (view_widget, NULL); // why?

  open_dir_result = 
    gnome_vfs_directory_open (&handle, current_directory, 
                              GNOME_VFS_FILE_INFO_DEFAULT 
                                | GNOME_VFS_FILE_INFO_FOLLOW_LINKS 
                                | GNOME_VFS_FILE_INFO_GET_MIME_TYPE );

  while (open_dir_result == GNOME_VFS_OK)
  {
    if (loading_directory == 0)
      break;
	
    vfs_file_info = gnome_vfs_file_info_new ();
    result = gnome_vfs_directory_read_next (handle, vfs_file_info);

    if (vfs_file_info->name != NULL && vfs_file_info->name[0] != '.')
    {
      FileInformation *file_info = g_malloc (sizeof (*file_info));
	
      if (strcmp (current_directory, "/"))
        file_info->filename = g_strdup_printf ("%s/%s", current_directory, vfs_file_info->name);
      else
        file_info->filename = g_strdup_printf ("/%s", vfs_file_info->name);

      file_info->vfs = vfs_file_info;

      list = g_list_insert_sorted (list, file_info, (GCompareFunc)compare_file_info);
    }

    if (result != GNOME_VFS_OK)
    {
      gnome_vfs_directory_close (handle);
      break;
    }
  }

  if (open_dir_result != GNOME_VFS_OK)
  {
    switch (open_dir_result)
    {
    case GNOME_VFS_ERROR_READ_ONLY:
      error = g_strdup ("Destination is read only.");
      break;
    case GNOME_VFS_ERROR_ACCESS_DENIED:
      error = g_strdup ("Access denied.");
      break;
    case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
      error = g_strdup ("Read only file system.");
      break;
	case GNOME_VFS_ERROR_NOT_FOUND:
	case GNOME_VFS_ERROR_GENERIC:
	break;
    default:
      error = g_strdup_printf ("Error: %s", 
	                           gnome_vfs_result_to_string(open_dir_result));
    break;
    }
	if (error)
	{
	  gpe_error_box (error);
      g_free (error);
	}
  }  
  for (iter = list; iter; iter = iter->next)
    add_icon (iter->data);

  g_list_free (list);
  
  gtk_widget_draw (view_widget, NULL);
  loading_directory = 0;
}

static void
goto_directory (GtkWidget *widget)
{
  gchar *new_directory;

  safety_check ();

  new_directory = 
    gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (combo)->entry), 0, -1);
  browse_directory (new_directory);
}

static void
add_history (gchar *directory)
{
  history_place++;
  history = g_list_insert (history, directory, history_place);
  gtk_combo_set_popdown_strings (GTK_COMBO (combo), history);
}

static gboolean
do_scheduled_refresh(void)
{
  refresh_current_directory();
  refresh_scheduled = FALSE;
  return FALSE;
}

void
directory_changed (GnomeVFSMonitorHandle *handle,
                   const gchar *monitor_uri,
                   const gchar *info_uri,
                   GnomeVFSMonitorEventType event_type,
                   gpointer user_data)
{
  /* schedule timeout if necessary */
  if (!refresh_scheduled)
    {
      g_timeout_add(2000, (GSourceFunc)do_scheduled_refresh, NULL);
      refresh_scheduled = TRUE;
    }
}

void
browse_directory (gchar *directory)
{
  gboolean enabled, ishome;
  gchar *msg;
  
  ishome = !strcmp(directory, g_get_home_dir());
  
  /* disable "up" button if we are in homedir and view is limited */
  enabled = (!limited_view || !ishome);
  if (btnGoUp)
    gtk_widget_set_sensitive(btnGoUp, enabled);
  set_active_item(directory);  

  /* some hacks to handle nasty smb urls */
  if (g_str_has_prefix(directory,"smb:"))
  {
	int i, j, result;
	GnomeVFSDirectoryHandle *handle;
	char *mark = strstr(directory,"///");
	if (mark)
	{
	   for (i=mark-directory+2;i<strlen(directory)-1;i++)
		   directory[i] = directory[i+1];
	   directory[strlen(directory)-1] = 0;
	}
	/* check if we can open it, if not strip workgroup */
	if (strlen(directory) > 6) 
	{
		result = 
			gnome_vfs_directory_open (&handle, directory, 
								  GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE);
		if (result != GNOME_VFS_OK)
		{
			if (result == GNOME_VFS_ERROR_NOT_A_DIRECTORY)
			{
				mark = strstr(directory+6,"/");
				j = mark - (directory+5);
				if (mark && (j>1))
				{
					for (i=5;i<strlen(directory)-j;i++)
				   		directory[i] = directory[i+j];
			   		directory[strlen(directory)-j] = 0;
				}
			}
		}
		else
			gnome_vfs_directory_close(handle);
	}
  }

  if (current_directory) 
    g_free(current_directory);
  current_directory = g_strdup (directory);
  
  if (!ishome)
    {
       if (strlen(strrchr(directory, '/')) > 1)
         msg = g_strdup_printf("%s %s",_("Opening"), strrchr(directory, '/') + 1);
       else /* should only happen for "/" */
         msg = g_strdup_printf("%s %s",_("Opening"), strrchr(directory, '/'));
       gpe_popup_infoprint(GDK_DISPLAY(), msg);
       g_free(msg);
    }
  
  add_history (g_strdup(directory));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), directory);
  
  /* monitor current directory */
  if (dir_monitor)
	  gnome_vfs_monitor_cancel(dir_monitor);
  if (gnome_vfs_monitor_add(&dir_monitor, 
	                        directory, 
                            GNOME_VFS_MONITOR_DIRECTORY, 
                            (GnomeVFSMonitorCallback)directory_changed, 
                            NULL) != GNOME_VFS_OK)
      dir_monitor = NULL;
  make_view ();
}

static void
refresh_current_directory (void)
{
  browse_directory (g_strdup(current_directory));
}


static void
set_directory_home (GtkWidget *widget)
{
  browse_directory ((gchar *) g_get_home_dir ());
}

static void
up_one_level (GtkWidget *widget)
{
  int i;
  int found_slash = 0;
  gchar *new_directory;

  safety_check ();

  new_directory = 
	gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (combo)->entry), 0, -1);

  for (i = strlen (new_directory); i > 0; i--)
  {
    if (new_directory[i] == '/')
    {
      if (found_slash ==1)
        break;

      found_slash = 1;
      new_directory[i] = 0;
    }

    if (found_slash == 0)
    {
      new_directory[i] = 0;
    }
  }
  browse_directory (new_directory);
  g_free(new_directory);
}

static void
history_back (GtkWidget *widget)
{
  gchar *new_directory;

  safety_check ();

  if (history_place > 0)
  {
    history_place--;

    new_directory = g_list_nth_data (history, history_place);
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), history);
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), new_directory);
    current_directory = g_strdup (new_directory);
    make_view ();
  }
}

static void
history_forward (GtkWidget *widget)
{
  gchar *new_directory;

  safety_check ();

  if (history_place < (g_list_length (history) - 1))
  {
    history_place++;

    new_directory = g_list_nth_data (history, history_place);
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), history);
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), new_directory);
    current_directory = g_strdup (new_directory);
    make_view ();
  }
}

static void
view_icons (GtkWidget *widget)
{
  gtk_tree_store_clear(GTK_TREE_STORE(store));
  view_widget = create_view_widget_icons();
  gtk_widget_set_sensitive(btnListView,TRUE);
  gtk_widget_set_sensitive(btnIconView,FALSE);
  view_is_icons = TRUE;
  refresh_current_directory ();
}


static void
view_list (GtkWidget *widget)
{
  view_widget = create_view_widget_list();
  gtk_widget_set_sensitive(btnListView,FALSE);
  gtk_widget_set_sensitive(btnIconView,TRUE);
  view_is_icons = FALSE;
  refresh_current_directory ();
}


static gboolean
tree_button_press (GtkWidget *tree, GdkEventButton *b, gpointer user_data)
{
  if (b->button == 1)
    gtk_widget_grab_focus(tree);
  
  if ((b->button == 3) || ((b->button == 1) && (b->type == GDK_2BUTTON_PRESS)))
    {
      gint x, y;
      GtkTreeViewColumn *col;
      GtkTreePath *path;
      GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
  
      x = b->x;
      y = b->y;
      
      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tree),
					 x, y,
					 &path, &col,
					 NULL, NULL))
      {
	    GtkTreeIter iter;
	    FileInformation *i;

	    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);

	    gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 
                            tree == view_widget ? COL_DATA : COL_DIRDATA, 
                            &i, -1);
		
	    gtk_tree_path_free (path);
        if (b->button == 3) 
          show_popup (NULL, i);
        else
          button_clicked (NULL, i);
	  }
	  else
        show_popup (NULL, NULL);
  }

  return TRUE;
}


static gboolean
tree_button_release (GtkWidget *tree, GdkEventButton *b)
{
  
  if (b->button == 1)
    {
      gint x, y;
      GtkTreeViewColumn *col;
      GtkTreePath *path;
  
      x = b->x;
      y = b->y;

      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tree),
					 x, y,
					 &path, &col,
					 NULL, NULL))
      {
	    /*GtkTreeIter iter;
        GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	    FileInformation *i;
	    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
        */
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path, NULL, FALSE);

	   // gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, tree == view_widget ? COL_DATA : COL_DIRDATA, &i, -1);
		
	    gtk_tree_path_free (path);
		
        //button_clicked (NULL, i);
      }
  }
  return TRUE;
}


static gboolean
tree_focus_in (GtkWidget *tree, GdkEventFocus *f, gpointer data)
{
  active_view = tree;
  return FALSE;
}


static void
setup_tabchain(void)
{
  GList *chain = NULL;
  
  chain = g_list_append(chain, main_paned);
  gtk_container_set_focus_chain(GTK_CONTAINER(vbox), chain);
  g_list_free(chain);
}

static GtkWidget*
create_view_widget_list(void)
{
    GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
	
    if (GTK_IS_WIDGET(view_widget)) gtk_widget_destroy(view_widget);
    if (GTK_IS_WIDGET(view_window)) gtk_widget_destroy(view_window);
	
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview),TRUE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(treeview),TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview),TRUE);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    
    
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set(renderer,"stock-size",GTK_ICON_SIZE_SMALL_TOOLBAR,NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Icon"),
							   renderer,
							   "pixbuf",
							   COL_ICON,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
							   renderer,
							   "text",
							   COL_NAME,
							   NULL);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column),TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Size"),
							   renderer,
							   "text",
							   COL_SIZE,
							   NULL);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column),TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Changed"),
							   renderer,
							   "text",
							   COL_CHANGED,
							   NULL);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column),TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	
    view_window = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(view_window),
  		GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(view_window),treeview);
    gtk_paned_pack2(GTK_PANED (main_paned), view_window, TRUE, TRUE);

	g_signal_connect (G_OBJECT(treeview), "button_press_event", 
		G_CALLBACK(tree_button_press), NULL);
	g_signal_connect (G_OBJECT(treeview), "button_release_event", 
		G_CALLBACK(tree_button_release), NULL);
	g_signal_connect (G_OBJECT(treeview), "focus-in-event", 
		G_CALLBACK(tree_focus_in), NULL);
	
    gtk_widget_show_all(view_window);
	return treeview;
}


static GtkWidget*
create_view_widget_icons(void)
{
  GtkWidget *view_icons;
  	
  if (GTK_IS_WIDGET(view_widget)) 
	  gtk_widget_destroy(view_widget);
  if (GTK_IS_WIDGET(view_window)) 
	  gtk_widget_destroy(view_window);
	
  view_icons = gpe_icon_list_view_new ();
  gtk_signal_connect (GTK_OBJECT (view_icons), "clicked",
		      GTK_SIGNAL_FUNC (button_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (view_icons), "show-popup",
		      GTK_SIGNAL_FUNC (show_popup), NULL);
                             
  gpe_icon_list_view_set_icon_size (GPE_ICON_LIST_VIEW (view_icons), current_zoom);
  gpe_icon_list_view_set_icon_xmargin (GPE_ICON_LIST_VIEW (view_icons), 30);
  gpe_icon_list_view_set_textpos (GPE_ICON_LIST_VIEW (view_icons), GPE_TEXT_BELOW);  
	
  view_window = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(view_window),
  	GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(view_window),view_icons);
  gtk_paned_pack2(GTK_PANED (main_paned), view_window, TRUE, TRUE);
  
  gtk_widget_show_all(view_window);  
  return view_icons;
}


static GtkWidget*
create_dir_view_widget(void)
{
    GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
	
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (dirstore));
  	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview),FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(treeview),TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview),TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview),TRUE);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
  
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set(renderer,"stock-size",GTK_ICON_SIZE_SMALL_TOOLBAR,NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Icon"),
							   renderer,
							   "pixbuf",
							   COL_ICON,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Directory"),
							   renderer,
							   "text",
							   COL_DIRNAME,
							   NULL);
    /* min width = 100 pixels */
    gtk_tree_view_column_set_min_width(column, 100);
    /* max width = 50% */
    gtk_tree_view_column_set_max_width(column, gdk_screen_width() / 2);
      
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column),TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    dir_view_window = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dir_view_window),
  		GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(dir_view_window),treeview);
    gtk_paned_pack1 (GTK_PANED(main_paned), dir_view_window, FALSE, TRUE);
    
	g_signal_connect (G_OBJECT(treeview), "button_press_event", 
		G_CALLBACK(tree_button_press), NULL);
	g_signal_connect (G_OBJECT(treeview), "button_release_event", 
		G_CALLBACK(tree_button_release), NULL);
	g_signal_connect (G_OBJECT(treeview), "focus-in-event", 
		G_CALLBACK(tree_focus_in), NULL);
    gtk_widget_show_all(dir_view_window);
	return treeview;
}


int
main (int argc, char *argv[])
{
  GtkWidget *hbox, *toolbar, *toolbar2, *mMain;
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkAccelGroup *accel_group;
  GtkWidget *storage_menu;
  int size_x, size_y;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);
  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  
  bluetooth_init ();

  gnome_vfs_init ();
  
  /* init tree storage stuff */
  store = gtk_tree_store_new (N_COLUMNS,
                              G_TYPE_OBJECT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_INT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_POINTER);
  
  dirstore = gtk_tree_store_new (N_DIRCOLUMNS,
                                 G_TYPE_OBJECT,
                                 G_TYPE_STRING,
                                 G_TYPE_POINTER);
  /* main window */
  size_x = gdk_screen_width();
  size_y = gdk_screen_height();
   /* if screen is large enough and landscape or if it is really large */
  directory_browser = (((size_x > 240) && (size_y > 320) && (size_x > size_y))
                      || (size_x > 600));
  size_x = size_x / 2;
  size_y = size_y * 2 / 3;
  if (size_x < 240) size_x = 240;
  if (size_y < 320) size_y = 320;
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Filemanager");
  gtk_window_set_default_size (GTK_WINDOW (window), size_x, size_y);
  gtk_signal_connect (GTK_OBJECT (window), "delete-event",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);
  gpe_set_window_icon(window,"icon");

  gtk_widget_realize (window);

  main_paned = gtk_hpaned_new ();
  
  vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);

  /* main menu */ 
  mMain = create_mMain(window);
  gtk_box_pack_start(GTK_BOX(vbox), mMain, FALSE, TRUE, 0);
  
  /* location stuff */
  combo = gtk_combo_new ();
  GTK_WIDGET_UNSET_FLAGS(combo, GTK_CAN_FOCUS);
  combo_signal_id = gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry),
                                        "activate", 
                                        GTK_SIGNAL_FUNC (goto_directory), NULL);
  
  view_widget = create_view_widget_list();
  
  dir_view_widget = create_dir_view_widget();
  
  storage_menu = build_storage_menu(directory_browser);
  GTK_WIDGET_UNSET_FLAGS(storage_menu, GTK_CAN_FOCUS);

  toolbar = gtk_toolbar_new ();
  GTK_WIDGET_UNSET_FLAGS(toolbar, GTK_CAN_FOCUS);
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  toolbar2 = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2), GTK_ORIENTATION_HORIZONTAL);

  pw = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_BACK,
		                         _("Go back in history."), NULL,
			                     G_CALLBACK (history_back), NULL, -1);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);

  pw = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_FORWARD,
			                     _("Go forward in history."), NULL,
			                     G_CALLBACK (history_forward), NULL, -1);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);

  btnGoUp = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_UP,
			                          _("Go up one level."), NULL,
			                          G_CALLBACK (up_one_level), NULL, -1);
  GTK_WIDGET_UNSET_FLAGS(btnGoUp, GTK_CAN_FOCUS);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  if (!directory_browser)
    {
      pw = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_HOME,
	                                 _("Goto your home directory."), NULL,
                                     G_CALLBACK (set_directory_home), NULL, -1);
      GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);
    }
                
  pw = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_REFRESH,
                                 _("Refresh current directory."), NULL,
                                 G_CALLBACK (refresh_current_directory), NULL, -1);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);    
                
  pw = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_STOP,
                                 _("Stop the current process."), NULL,
                                 G_CALLBACK (safety_check), NULL, -1);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("icon-view");
  pw = gtk_image_new_from_pixbuf(p);
  btnIconView = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Icon view"), 
			   _("View files as icons"), NULL, pw, 
			   G_CALLBACK (view_icons), NULL);
  GTK_WIDGET_UNSET_FLAGS(btnIconView, GTK_CAN_FOCUS);
  
  p = gpe_find_icon ("list-view");
  pw = gtk_image_new_from_pixbuf(p);
  btnListView = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("List view"), 
			   _("View files in a list"), NULL, pw, 
			   G_CALLBACK (view_list), NULL);
  gtk_widget_set_sensitive(btnListView,FALSE);
  GTK_WIDGET_UNSET_FLAGS(btnListView, GTK_CAN_FOCUS);
               
  pw = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, 
                                gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar2)));
  pw = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Go!"), 
                                _("Goto Location"), NULL, pw, 
                                G_CALLBACK (goto_directory), NULL);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  if (directory_browser)
    gtk_box_pack_start (GTK_BOX (vbox), storage_menu, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), main_paned, TRUE, TRUE, 0);

  gpe_set_window_icon (window, "icon");

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", accel_group);
  g_object_set_data_full (G_OBJECT (window), "<main>", item_factory, (GDestroyNotify) g_object_unref);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
  
  if (!directory_browser)
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(goto_menu_item), storage_menu);
  
  bluetooth_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Send via Bluetooth");
  irda_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Send via Infrared");
  copy_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Copy");
  paste_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Paste");
  open_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Open With");
  move_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Move");
  rename_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Rename");
  properties_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Properties");
  delete_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Delete");

  g_signal_connect (G_OBJECT (gtk_item_factory_get_widget (item_factory, "<main>")), "hide",
		    GTK_SIGNAL_FUNC (hide_menu), NULL);
			
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(set_dirbrowser_menu_item), directory_browser);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(set_myfiles_menu_item), limited_view);

  gtk_widget_show_all (window);
  
  gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
						  (GnomeVFSModuleCallback) auth_callback,
						  NULL,
						  NULL);
						  
  gnome_vfs_mime_info_reload();
  gnome_vfs_application_registry_reload();
  
  if (directory_browser)
    {
      gtk_widget_hide(goto_menu_item);
      gtk_widget_show_all(dir_view_window);
    }
  else
	gtk_widget_hide(dir_view_window);
  gtk_widget_grab_focus(view_widget);

  if (argc < 2)
    set_directory_home (NULL);
  else
    {
      GnomeVFSResult res;
      GnomeVFSDirectoryHandle *handle;
      res = gnome_vfs_directory_open(&handle, argv[1], GNOME_VFS_FILE_INFO_DEFAULT);
      if (res == GNOME_VFS_OK)
        {
          gnome_vfs_directory_close(handle);
          browse_directory (argv[1]);
        }
      else
        set_directory_home (NULL);
    }
  
  setup_tabchain();
  initialized = TRUE;  
  gtk_main();

  return 0;
}
