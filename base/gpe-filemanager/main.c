 /*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
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
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-module-callback.h>
#include <libgnomevfs/gnome-vfs-standard-callbacks.h>
#include <libgnomevfs/gnome-vfs-xfer.h>
#include <libgnomevfs/gnome-vfs-types.h>


#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/gpeiconlistview.h>
#include <gpe/dirbrowser.h>
#include <gpe/spacing.h>

#include "mime-sql.h"
#include "mime-programs-sql.h"

#include "bluetooth.h"

#define _(x) dgettext(PACKAGE, x)

#define COMPLETED 0
#define LAST_SIGNAL 1

#define DEFAULT_TERMINAL "rxvt -e"
#define FILEMANAGER_ICON_PATH "/share/gpe/pixmaps/default/filemanager/document-icons"
#define DEFAULT_ICON_PATH "/pixmaps"
#define ZOOM_INCREMENT 8

#define MAX_SYM_DEPTH 256

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

static GtkWidget *window;
static GtkWidget *combo;
static GtkWidget *view_widget;
static GtkWidget *view_window;
static GtkWidget *vbox;

static GtkWidget *bluetooth_menu_item;
static GtkWidget *copy_menu_item;
static GtkWidget *paste_menu_item;
static GtkWidget *open_menu_item;
static GtkWidget *move_menu_item;
static GtkWidget *rename_menu_item;
static GtkWidget *properties_menu_item;
static GtkWidget *delete_menu_item;

static GtkWidget *btnListView;
static GtkWidget *btnIconView;

GdkPixbuf *default_pixbuf;

guint screen_w, screen_h;

GtkWidget *current_button=NULL;
int current_button_is_down=0;

GtkItemFactory *item_factory;

/* For not starting an app twice after a double click */
int ignore_press = 0;

int loading_directory = 0;
int combo_signal_id;

int history_place = 0;
GList *history = NULL;

gchar *current_directory = NULL;
gchar *current_view = "icons";
gint current_zoom = 36;
static gboolean view_is_icons = FALSE;

static gchar *file_clipboard = NULL;

static GHashTable *loaded_icons;
static GtkWidget *progress_dialog = NULL;
static gboolean abort_transfer = FALSE;

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


static void browse_directory (gchar *directory);
static void popup_ask_open_with (void);
static void popup_ask_move_file (void);
static void popup_ask_rename_file (void);
static void popup_ask_delete_file (void);
static void show_file_properties (void);
static void refresh_current_directory (void);
static void send_with_bluetooth (void);
static void copy_file_clip (void);
static void paste_file_clip (void);
static void create_directory_interactive(void);
static GtkWidget* create_view_widget_icons(void);
static GtkWidget* create_view_widget_list(void);

static GtkItemFactoryEntry menu_items[] =
{
  { "/Open Wit_h",	 NULL, popup_ask_open_with,  0, "<StockItem>", GTK_STOCK_OPEN },
  { "/sep1",	         NULL, NULL,	             0, "<Separator>" },
  { "/Send via _Bluetooth", NULL, send_with_bluetooth, 0, "<Item>" },
  { "/_Copy",          NULL, copy_file_clip,         0, "<StockItem>", GTK_STOCK_COPY },
  { "/_Paste",          NULL, paste_file_clip,         0, "<StockItem>", GTK_STOCK_PASTE },
  { "/_Move",            NULL, popup_ask_move_file,            0, "<Item>" },
  { "/_Rename",          NULL, popup_ask_rename_file,          0, "<Item>" },
  { "/_Delete",          NULL, popup_ask_delete_file,         0, "<StockItem>", GTK_STOCK_DELETE },
  { "/_Create Directory",NULL, create_directory_interactive, 0, "<Item>"},
  { "/sep2",	         NULL, NULL,	             0, "<Separator>" },
  { "/_Properties",      NULL, show_file_properties, 0, "<StockItem>", GTK_STOCK_PROPERTIES },
};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


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
         g_signal_connect(G_OBJECT(dialog),"response",G_CALLBACK(progress_response),NULL);
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
  GtkWidget *entry;
  int response;
  gchar *directory, *diruri;

  dialog = gtk_message_dialog_new (GTK_WINDOW (window), 
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
    GTK_MESSAGE_QUESTION,
    GTK_BUTTONS_OK_CANCEL, "Create directory");
  entry = gtk_entry_new ();

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry, FALSE, TRUE, 0);

  gtk_widget_show_all (dialog);
  gtk_widget_grab_focus(GTK_WIDGET(entry));
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
    refresh_current_directory();
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
  gnome_vfs_uri_unref(uri_src);
  gnome_vfs_uri_unref(uri_dest);
}

static void 
copy_file_clip (void)
{
  if (file_clipboard) 
    g_free(file_clipboard);
  if (GNOME_VFS_FILE_INFO_LOCAL(current_popup_file->vfs))
    file_clipboard = 
      gnome_vfs_get_uri_from_local_path(current_popup_file->filename);
  else
    file_clipboard = g_strdup(current_popup_file->filename);
}


static void 
paste_file_clip (void)
{
  gchar *target_file;
  
  if (file_clipboard == NULL) 
    return;
  
  target_file = 
    g_strdup_printf("%s/%s",
                    current_directory,
                    g_path_get_basename(file_clipboard));
  if (strcmp(file_clipboard,target_file)) /* check if not the same */
    copy_file(file_clipboard,target_file);
  g_free(target_file);
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
open_with (GtkButton *button, gpointer data)
{
  GnomeVFSMimeApplication *application;
  FileInformation *file_info;
  pid_t pid;

  file_info = gtk_object_get_data (GTK_OBJECT (button), "FileInformation");
  application = gtk_object_get_data (GTK_OBJECT (data), "GnomeVFSMimeApplication");

  if (application)
  {
	pid = fork();
	switch (pid)
	{
		case -1: 
			return; /* failed */
		break;
		case  0: 
          if (application->requires_terminal)
               execlp(DEFAULT_TERMINAL,DEFAULT_TERMINAL,application->command,file_info->filename);
          else
               execlp(application->command,application->command,file_info->filename,NULL);
		break;
		default: 
		break;
	} 
  }
}

static void
open_with_row_selected (GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer entry)
{
  GnomeVFSMimeApplication *application;

  application = gtk_clist_get_row_data (GTK_CLIST (clist), row);
  gtk_object_set_data (GTK_OBJECT (entry), "GnomeVFSMimeApplication", (gpointer) application);
  gtk_entry_set_text (GTK_ENTRY (entry), application->name);
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
      error = g_strdup_printf ("Error: %s", gnome_vfs_result_to_string(result));
      break;
    }

    gpe_error_box (error);
    g_free (error);
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
    refresh_current_directory();

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
  GtkWidget *vbox, *label, *entry;
  gchar *label_text;

  label_text = g_strdup_printf ("Rename file %s to:", current_popup_file->vfs->name);

  dialog_window = gtk_dialog_new_with_buttons ("Rename file", GTK_WINDOW (window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
  g_signal_connect (G_OBJECT (dialog_window), "delete_event", GTK_SIGNAL_FUNC (kill_widget), dialog_window);
  g_signal_connect (G_OBJECT (dialog_window), "response", GTK_SIGNAL_FUNC (rename_file), NULL);

  vbox = gtk_vbox_new (FALSE, 0);

  label = gtk_label_new (label_text);
  g_free (label_text);

  entry = gtk_entry_new ();
  g_object_set_data (G_OBJECT (dialog_window), "entry", entry);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog_window)->vbox), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  gtk_widget_show_all (dialog_window);
}

static void
popup_ask_delete_file ()
{
  gchar *label_text;

  label_text = g_strdup_printf (_("Delete \"%s\"?"), current_popup_file->vfs->name);

  if (gpe_question_ask (label_text, _("Confirm"), "!gtk-dialog-question", 
			"!gtk-cancel", NULL, "!gtk-delete", NULL, NULL, NULL) == 1)
    {
      GnomeVFSResult r;

      r = gnome_vfs_unlink_from_uri (gnome_vfs_uri_new (current_popup_file->filename));

      if (r != GNOME_VFS_OK)
        gpe_error_box (gnome_vfs_result_to_string (r));
      
      refresh_current_directory();
    }

  g_free (label_text);
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
  GtkWidget *dialog_window, *fakeparentwindow, *clist, *entry, *label;
  GtkWidget *open_button, *cancel_button;
  GList *applications;
  GnomeVFSMimeApplication *application;
  int row_num = 0;
  gchar *row_text[1];

  fakeparentwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakeparentwindow);

  dialog_window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(dialog_window), "Open With...");
  gtk_window_set_transient_for (GTK_WINDOW(dialog_window), GTK_WINDOW(fakeparentwindow));
  gtk_widget_realize (dialog_window);
 
  gtk_window_set_modal (GTK_WINDOW (dialog_window), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog_window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (dialog_window), "destroy",
                      GTK_SIGNAL_FUNC (kill_widget),
                      dialog_window);

  label = gtk_label_new ("Open with program");
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  entry = gtk_entry_new ();

  clist = gtk_clist_new (1);
  gtk_signal_connect (GTK_OBJECT (clist), "select-row",
                      GTK_SIGNAL_FUNC (open_with_row_selected),
                      entry);

  open_button = gpe_picture_button (dialog_window->style, "Open", "open");
  gtk_object_set_data (GTK_OBJECT (open_button), "FileInformation", (gpointer) file_info);
  gtk_signal_connect (GTK_OBJECT (open_button), "clicked",
                      GTK_SIGNAL_FUNC (open_with),
                      entry);
  gtk_signal_connect (GTK_OBJECT (open_button), "clicked",
                      GTK_SIGNAL_FUNC (kill_widget),
                      dialog_window);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog_window)->action_area),
                      open_button);

  cancel_button = gpe_picture_button (dialog_window->style, "Cancel", "cancel");
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
                      GTK_SIGNAL_FUNC (kill_widget),
                      dialog_window);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog_window)->action_area),
                      cancel_button);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), label, TRUE, TRUE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), clist, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), entry, TRUE, TRUE, 4);

  if (file_info->vfs->mime_type)
    applications = gnome_vfs_mime_get_short_list_applications (file_info->vfs->mime_type);
  else
    applications = gnome_vfs_mime_get_all_applications (file_info->vfs->mime_type);

  while (applications)
  {
    application = (GnomeVFSMimeApplication *)(((GList *)applications)->data);

    row_text[0] = application->name;
    gtk_clist_append (GTK_CLIST (clist), row_text);
    gtk_clist_set_row_data (GTK_CLIST (clist), row_num, (gpointer) application);

    printf ("Got application %s\n", ((GnomeVFSMimeApplication *)(((GList *)applications)->data))->command);
    applications = applications->next;
    row_num ++;
  }

/*
  if (mime_programs)
  {
    for (iter = mime_programs; iter; iter = iter->next)
    {
      struct mime_program *program = iter->data;

      row_text[0] = "";
      row_text[1] = program->name;
      gtk_clist_append (GTK_CLIST (clist), row_text);
      gtk_clist_set_row_data (GTK_CLIST (clist), row_num, (gpointer) program->command);

      pixmap_file = g_strdup_printf ("%s/share/pixmaps/%s.png", PREFIX, program->command);
      pixbuf = gdk_pixbuf_new_from_file (pixmap_file, NULL);
      if (pixbuf != NULL)
      {
        spixbuf = gdk_pixbuf_scale_simple (pixbuf, 12, 12, GDK_INTERP_BILINEAR);
        gpe_render_pixmap (NULL, spixbuf, &pixmap, &bitmap);
        gtk_clist_set_pixmap (GTK_CLIST (clist), row_num, 0, pixmap, bitmap);
      }
      row_num++;
    }
  }
*/

  if (row_num == 0)
  {
    gtk_widget_destroy (entry);
    gtk_widget_destroy (clist);
    gtk_widget_destroy (open_button);
    gtk_label_set_text (GTK_LABEL (label), _("No available applications"));
  }

  gtk_widget_show_all (dialog_window);	
}

static void
send_with_bluetooth (void)
{
  bluetooth_send_file (current_popup_file->filename, current_popup_file->vfs);
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
  gchar *command;
    
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
            printf ("Symbolic link %s leads to inaccessable file!\n", file_info->filename);
            return;
      }

      file_info->vfs = vfs;
    } while (file_info->vfs->type == GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK && (--cur_depth > 0));

    if (cur_depth <= 0) {
      printf ("max symbolic link depth exceeded for %s\n", file_info->filename);
      return;
    }
  }
  
    if ((file_info->vfs->mime_type) && (!strcmp(file_info->vfs->mime_type,"application/x-gnome-app-info")))
        browse_directory (file_info->filename);
    else    
      if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_REGULAR || file_info->vfs->type == GNOME_VFS_FILE_TYPE_UNKNOWN)
      {
        if (file_info->vfs->mime_type)
        {
          default_mime_application = gnome_vfs_mime_get_default_application (file_info->vfs->mime_type);
          if (default_mime_application != NULL)
          {
            if (default_mime_application->requires_terminal)
              command = g_strdup_printf (DEFAULT_TERMINAL " %s %s &", default_mime_application->command, file_info->filename);
            else
              command = g_strdup_printf ("%s %s &", default_mime_application->command, file_info->filename);
            system (command);
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


const gchar *
find_icon_path (gchar *mime_type)
{
  struct stat s;
  gchar *mime_icon;
  gchar *mime_path, *p;

  mime_icon = gnome_vfs_mime_get_icon (mime_type);
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
  const gchar *mime_icon;
  
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
  
  /* now be careful */
  if (view_is_icons)
  	gpe_icon_list_view_add_item_pixbuf (GPE_ICON_LIST_VIEW (view_widget), file_info->vfs->name, pixbuf, (gpointer) file_info);
  else
  {
	GtkTreeIter iter;
    GdkPixbuf *pb;
	gchar *size;
	  
	pb = gdk_pixbuf_scale_simple(pixbuf,24,24,GDK_INTERP_BILINEAR);
	size = gnome_vfs_format_file_size_for_display(file_info->vfs->size); 
	gtk_tree_store_append (store, &iter, NULL);
	  
    gtk_tree_store_set (store, &iter,
	    COL_NAME, file_info->vfs->name,
	    COL_SIZE, size,
	    COL_CHANGED, ctime(&file_info->vfs->ctime),
		COL_ICON, pb,
		COL_DATA, (gpointer) file_info,
    	-1);
	g_free(size);
	gdk_pixbuf_unref(pb);
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
make_view ()
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
  }
  
  gtk_widget_draw (view_widget, NULL); // why?

  open_dir_result = 
    gnome_vfs_directory_open (&handle, current_directory, 
                              GNOME_VFS_FILE_INFO_DEFAULT);

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
      break;
    }
  }
  
  gnome_vfs_directory_close (handle);

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
    default:
      error = g_strdup_printf ("Error: %s", gnome_vfs_result_to_string(result));
      break;
    }
    gpe_error_box (error);
    g_free (error);
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

static void
browse_directory (gchar *directory)
{
  if (current_directory) 
    g_free(current_directory);
  current_directory = g_strdup (directory);
  add_history (directory);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), directory);
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

  new_directory = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (combo)->entry), 0, -1);

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

  if (history_place < g_list_length (history) - 1)
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
tree_button_press (GtkWidget *tree,GdkEventButton *b, gpointer user_data)
{
  if (b->button == 3)
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
	    GtkTreeIter iter;
	    FileInformation *i;

	    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);

	    gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, COL_DATA, &i, -1);
		
	    gtk_tree_path_free (path);
        show_popup (NULL, i);
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
	  GtkTreeIter iter;
	  FileInformation *i;

	  gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);

	  gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, COL_DATA, &i, -1);
		
	  gtk_tree_path_free (path);
		
      button_clicked (NULL, i);

	}
  }

  return TRUE;
}


static GtkWidget*
create_view_widget_list(void)
{
    GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
    if (GTK_IS_WIDGET(view_widget)) gtk_widget_destroy(view_widget);
    if (GTK_IS_WIDGET(view_window)) gtk_widget_destroy(view_window);
	
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview),TRUE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(treeview),TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview),TRUE);
  
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
    gtk_box_pack_end (GTK_BOX (vbox), view_window, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT(treeview), "button_press_event", 
		G_CALLBACK(tree_button_press), NULL);
	g_signal_connect (G_OBJECT(treeview), "button_release_event", 
		G_CALLBACK(tree_button_release), NULL);
	
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
  gtk_box_pack_end (GTK_BOX (vbox), view_window, TRUE, TRUE, 0);
  
  gtk_widget_show_all(view_window);  
  return view_icons;
}


int
main (int argc, char *argv[])
{
  GtkWidget *hbox, *toolbar, *toolbar2;
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkAccelGroup *accel_group;
  int size_x, size_y;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);
  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  bluetooth_init ();

	/* init tree storage stuff */
	store = gtk_tree_store_new (N_COLUMNS,
  		G_TYPE_OBJECT,
		G_TYPE_STRING,
	 	G_TYPE_STRING,
		G_TYPE_INT,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_POINTER);
  
  /* main window */
  size_x = gdk_screen_width() / 2;
  size_y = gdk_screen_height() * 2 / 3;  
  if (size_x < 240) size_x = 240;
  if (size_y < 320) size_y = 320;
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Filemanager");
  gtk_window_set_default_size (GTK_WINDOW (window), size_x, size_y);
  gtk_signal_connect (GTK_OBJECT (window), "delete-event",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);
  gpe_set_window_icon(window,"icon");

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);

  combo = gtk_combo_new ();
  combo_signal_id = gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry),
    "activate", GTK_SIGNAL_FUNC (goto_directory), NULL);
  
  view_widget = create_view_widget_list();
  
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  toolbar2 = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2), GTK_ORIENTATION_HORIZONTAL);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_BACK,
			    _("Back"), _("Go back in history."),
			    G_CALLBACK (history_back), NULL, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_FORWARD,
			    _("Forward"), _("Go forward in history."),
			    G_CALLBACK (history_forward), NULL, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_UP,
			    _("Up one level"), _("Go up one level."),
			    G_CALLBACK (up_one_level), NULL, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_STOP,
			    _("Stop"), _("Stop the current process."),
			    G_CALLBACK (safety_check), NULL, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_HOME,
			    _("Home"), _("Goto your home directory."),
			    G_CALLBACK (set_directory_home), NULL, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("icon-view");
  pw = gtk_image_new_from_pixbuf(p);
  btnIconView = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Icon view"), 
			   _("Icon view"), _("View files as icons"), pw, 
			   G_CALLBACK (view_icons), NULL);
  
  p = gpe_find_icon ("list-view");
  pw = gtk_image_new_from_pixbuf(p);
  btnListView = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("List view"), 
			   _("List view"), _("View files in a list"), pw, 
			   G_CALLBACK (view_list), NULL);
  gtk_widget_set_sensitive(btnListView,FALSE);
               
  p = gpe_find_icon ("dir-up");
  pw = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Goto Location"), 
			   _("Goto Location"), _("Goto Location"), pw, 
			   G_CALLBACK (goto_directory), NULL);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar2, FALSE, FALSE, 0);

  gpe_set_window_icon (window, "icon");

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", accel_group);
  g_object_set_data_full (G_OBJECT (window), "<main>", item_factory, (GDestroyNotify) g_object_unref);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

  bluetooth_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Send via Bluetooth");
  copy_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Copy");
  paste_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Paste");
  open_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Open With");
  move_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Move");
  rename_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Rename");
  properties_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Properties");
  delete_menu_item = gtk_item_factory_get_widget (item_factory, "<main>/Delete");

  g_signal_connect (G_OBJECT (gtk_item_factory_get_widget (item_factory, "<main>")), "hide",
		    GTK_SIGNAL_FUNC (hide_menu), NULL);
  gtk_widget_show_all (window);

  gnome_vfs_init ();
  gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
						  (GnomeVFSModuleCallback) auth_callback,
						  NULL,
						  NULL);
                    
  set_directory_home (NULL);

  gtk_main();

  return 0;
}
