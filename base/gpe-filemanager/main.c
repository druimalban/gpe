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
#include <gpe/gpe-iconlist.h>
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

GtkWidget *window;
GtkWidget *combo;
GtkWidget *view_scrolld;
GtkWidget *view_widget;
GtkWidget *bluetooth_menu_item;
GtkWidget *copy_menu_item;
GtkWidget *paste_menu_item;

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

gchar *current_directory = "";
gchar *current_view = "icons";
gint current_zoom = 28;

static gchar *file_clipboard = NULL;

GHashTable *loaded_icons;

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
  { "preferences", "preferences" },
  { "zoom_in", "filemanager/zoom_in" },
  { "zoom_out", "filemanager/zoom_out" },
  { "icon", PREFIX "/share/pixmaps/gpe-filemanager.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

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
  { "/sep2",	         NULL, NULL,	             0, "<Separator>" },
  { "/_Properties",      NULL, show_file_properties, 0, "<StockItem>", GTK_STOCK_PROPERTIES },
};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


static gint 
transfer_callback(GnomeVFSXferProgressInfo *info, gpointer data)
{
    return 1;
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
  if (result != GNOME_VFS_OK)
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
  file_clipboard = gnome_vfs_get_uri_from_local_path(current_popup_file->filename);
}


static void 
paste_file_clip (void)
{
  gchar *target_file, *tmp;
  
  tmp = gnome_vfs_get_local_path_from_uri(file_clipboard);
  target_file = g_strdup_printf("%s/%s",current_directory,g_path_get_basename(tmp));
  g_free(tmp);
  copy_file(file_clipboard,target_file);
  refresh_current_directory(); 
}

static void
hide_menu (void)
{
  gpe_iconlist_popup_removed (GPE_ICONLIST (view_widget));
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

GtkWidget *create_icon_pixmap (GtkStyle *style, char *fn, int size)
{
  GdkPixbuf *pixbuf, *spixbuf;
  GtkWidget *w;
  pixbuf = gdk_pixbuf_new_from_file (fn, NULL);
  if (pixbuf == NULL)
    return NULL;

  spixbuf = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_BILINEAR);
  gdk_pixbuf_unref (pixbuf);
  w = gpe_render_icon (style, spixbuf);
  gdk_pixbuf_unref (spixbuf);

  return w;
}

static gchar *
get_file_extension (gchar *filename)
{
  int i;
  gchar *extension;

  for (i = strlen (filename); i > 0; i--)
  {
    if (filename[i] == '.')
      break;
  }

  if (i == strlen (filename))
  {
    return NULL;
  }
  else
  {
    extension = g_malloc (strlen (filename) - i);
    extension = g_strdup (filename + i + 1);
    return extension;
  }
}

static void
run_program (gchar *exec, gchar *mime_name)
{
  gchar *command, *search_mime, *program_command = NULL;
  GSList *iter;
  pid_t p_help;

  if (mime_programs)
  {
    for (iter = mime_programs; iter; iter = iter->next)
    {
      struct mime_program *program = iter->data;

      if (program->mime)
      {
        if (program->mime[strlen (program->mime) - 1] == '*')
        {
	  search_mime = g_strdup (program->mime);
	  search_mime[strlen (search_mime) - 1] = 0;

          if (strstr (mime_name, search_mime))
	    program_command = g_strdup (program->command);
        }
        else if (strcmp (mime_name, program->mime) == 0)
        {
	  program_command = g_strdup (program->command);
        }
      }
    }
  }

  if (program_command)
  {
	p_help = fork();
	switch (p_help)
	{
		case -1: 
			return; /* failed */
		break;
		case  0: 
			execlp(program_command,program_command,exec,NULL);
		break;
		default: 
			g_free(program_command);
		break;
	} 
  }
}

static void
open_with (GtkButton *button, gpointer data)
{
  GnomeVFSMimeApplication *application;
  FileInformation *file_info;
  char *command;

  file_info = gtk_object_get_data (GTK_OBJECT (button), "FileInformation");
  application = gtk_object_get_data (GTK_OBJECT (data), "GnomeVFSMimeApplication");

  if (application->requires_terminal)
    command = g_strdup_printf (DEFAULT_TERMINAL " %s %s &", application->command, file_info->filename);
  else
    command = g_strdup_printf ("%s %s &", application->command, file_info->filename);

  system (command);
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
  printf ("Rename dest: %s\n", dest);
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
  printf ("Show file properties\n");
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

  //gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog_window)->vbox), hbox);
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
  current_popup_file = file_info;

  if (bluetooth_available ())
    gtk_widget_set_sensitive (bluetooth_menu_item, TRUE);
  else
    gtk_widget_set_sensitive (bluetooth_menu_item, FALSE);

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
printf("Type: %s\n",file_info->vfs->mime_type);
printf("Act: %i\n",gnome_vfs_mime_get_default_action_type(file_info->vfs->mime_type));
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
  const gchar *mime_icon;
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
    return mime_path;

  mime_path = g_strdup_printf (PREFIX DEFAULT_ICON_PATH "/%s.png", mime_icon);
  if (stat (mime_path, &s) == 0)
    return mime_path;

  return g_strdup (PREFIX FILEMANAGER_ICON_PATH "/regular.png");
}

GdkPixbuf *
get_pixbuf (const gchar *filename)
{
  GdkPixbuf *pixbuf;

  pixbuf = g_hash_table_lookup (loaded_icons, (gconstpointer) filename);

  if ((GdkPixbuf *) pixbuf)
  {
    return (GdkPixbuf *) pixbuf;
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
  GdkPixbuf *pixbuf;
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
  gpe_iconlist_add_item_pixbuf (GPE_ICONLIST (view_widget), file_info->vfs->name, pixbuf, (gpointer) file_info);
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
  GnomeVFSDirectoryHandle *handle;
  GnomeVFSFileInfo *vfs_file_info;
  GnomeVFSResult result, open_dir_result;
  loading_directory = 1;
  GList *list = NULL, *iter;
  gchar *error = NULL;

  loaded_icons = g_hash_table_new (g_str_hash, g_str_equal);
  gpe_iconlist_clear (GPE_ICONLIST (view_widget));
  gtk_widget_draw (view_widget, NULL); // why?

  open_dir_result = gnome_vfs_directory_open (&handle, current_directory, GNOME_VFS_FILE_INFO_DEFAULT);

  while (open_dir_result == GNOME_VFS_OK)
  {
    vfs_file_info = gnome_vfs_file_info_new ();
    result = gnome_vfs_directory_read_next (handle, vfs_file_info);

    if (loading_directory == 0)
      break;

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
combo_button_pressed ()
{
  gtk_signal_disconnect (GTK_OBJECT (GTK_COMBO (combo)->list), combo_signal_id);
}

static void
combo_button_released ()
{
  combo_signal_id = gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->list), "selection-changed", GTK_SIGNAL_FUNC (goto_directory), NULL);
}
/*
static void
browse_uri (gchar *uri)
{
  current_directory = g_strdup (directory);
  add_history (directory);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), directory);
  make_view ();
}
*/
static void
browse_directory (gchar *directory)
{
  struct stat s;

 // if (stat (directory, &s) == 0)
  {
//    if (S_ISDIR (s.st_mode))
    {
 #warning free old!       
      current_directory = g_strdup (directory);
      add_history (directory);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), directory);
      make_view ();
    }
//    else
    {
      //gpe_error_box ("This file isn't a directory.");
    }
  }
//  else
  {
    //gpe_error_box ("No such file or directory.");
  }
}

static void
refresh_current_directory (void)
{
  browse_directory (current_directory);
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

void
zoom_in ()
{
  if (current_zoom < 48)
  {
    current_zoom = current_zoom + ZOOM_INCREMENT;
    printf ("ZOOMING IN!\n");
    gpe_iconlist_set_icon_size (GPE_ICONLIST (view_widget), current_zoom);
  }
}

void
zoom_out ()
{
  if (current_zoom > 16)
  {
    current_zoom = current_zoom - ZOOM_INCREMENT;
    printf ("ZOOMING OUT!\n");
    gpe_iconlist_set_icon_size (GPE_ICONLIST (view_widget), current_zoom);
  }
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *toolbar, *toolbar2;
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkAccelGroup *accel_group;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  bluetooth_init ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Filemanager");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "delete-event",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);

  combo = gtk_combo_new ();
  combo_signal_id = gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry),
    "activate", GTK_SIGNAL_FUNC (goto_directory), NULL);

  view_widget = gpe_iconlist_new ();
  gtk_signal_connect (GTK_OBJECT (view_widget), "clicked",
		      GTK_SIGNAL_FUNC (button_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (view_widget), "show-popup",
		      GTK_SIGNAL_FUNC (show_popup), NULL);
  gpe_iconlist_set_icon_size (GPE_ICONLIST (view_widget), current_zoom);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  toolbar2 = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);

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

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_ZOOM_IN,
			    _("Zoom in"), _("Zoom in."),
			    G_CALLBACK (zoom_in), NULL, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_ZOOM_OUT,
			    _("Zoom out"), _("Zoom out."),
			    G_CALLBACK (zoom_out), NULL, -1);

  p = gpe_find_icon ("dir-up");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Goto Location"), 
			   _("Goto Location"), _("Goto Location"), pw, 
			   G_CALLBACK (goto_directory), NULL);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), view_widget, TRUE, TRUE, 0);
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

  g_signal_connect (G_OBJECT (gtk_item_factory_get_widget (item_factory, "<main>")), "hide",
		    GTK_SIGNAL_FUNC (hide_menu), NULL);

  gtk_widget_show (window);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (toolbar);
  gtk_widget_show (toolbar2);
  gtk_widget_show (combo);
  gtk_widget_show (view_widget);

  gnome_vfs_init ();
  gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
						  (GnomeVFSModuleCallback) auth_callback,
						  NULL,
						  NULL);
 
  set_directory_home (NULL);

  gtk_main();

  return 0;
}
