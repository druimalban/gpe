 /*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *               2004, 2005, 2007 Florian Boor <florian@linuxtogo.org>
 *               2007, 2008, 2010 Graham R. Cobb <g+gpe@cobb.uk.net>
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
#include <gdk/gdkkeysyms.h>
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
#include <gpe/question.h>
#include <gpe/gpeiconlistview.h>
#include <gpe/spacing.h>
#include <gpe/gpehelp.h>
#include <gpe/infoprint.h>

#ifdef USE_HILDON
#if HILDON_VER > 0
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>
#include <hildon/hildon-defines.h>
#include <libosso.h>
#include <hildon-mime.h>
#include <hildon/hildon-banner.h>
#else
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-defines.h>
#include <libosso.h>
#include <osso-mime.h>
#include <hildon-widgets/hildon-banner.h>
#endif /* HILDON_VER */
#define gpe_popup_infoprint(x, y) \
	hildon_banner_show_information(window, NULL, y)
#endif

#include "main.h"
#include "fileops.h"
#include "guitools.h"
#include "bluetooth.h"

#define _(x) dgettext(PACKAGE, x)

#define COMPLETED 0
#define LAST_SIGNAL 1

#define DEFAULT_TERMINAL "rxvt -e"
#define FILEMANAGER_ICON_PATH "/share/pixmaps/gpe/default/filemanager/document-icons"
#define ZOOM_INCREMENT 8

#ifdef USE_HILDON
#define DEFAULT_ICON_PATH "pixmaps/gpe/default"
#else
#define DEFAULT_ICON_PATH "/pixmaps"

struct gpe_icon my_icons[] = {
  { "icon", PREFIX "/share/pixmaps/gpe-filemanager.png" },
  {NULL, NULL}
};

#endif

#define MAX_SYM_DEPTH 256

#define N_(x) (x)

#define HELPMESSAGE "GPE-Filemanager\nVersion " VERSION \
		"\nGPE File Manager\n\ndctanner@magenet.com"\
		"\nflorian@linuxtogo.org"

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

GtkWidget *window;
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

static GHashTable *loaded_icons;
static gboolean initialized = FALSE;

FileInformation *current_popup_file;

#ifdef USE_HILDON
static gboolean is_fullscreen = FALSE;
#endif

/* some forward declarations */

void browse_directory (gchar *directory);
static void popup_ask_open_with (void);
static void popup_ask_move_file (void);
static void popup_ask_rename_file (void);
static void popup_ask_delete_file (void);
static void popup_copy_file_clip (void);
static void show_file_properties (void);
static void send_with_bluetooth (void);
static void send_with_irda (void);
static void create_directory_interactive(void);
static GtkWidget* create_view_widget_icons(void);
static GtkWidget* create_view_widget_list(void);
void on_about_clicked (GtkWidget * w);
void on_help_clicked (GtkWidget * w);
void on_dirbrowser_setting_changed(GtkCheckMenuItem *menuitem, gpointer user_data);
void on_myfiles_setting_changed(GtkCheckMenuItem *menuitem, gpointer user_data);
void do_select_all(GtkWidget *w, gpointer d);
static void view_icons (GtkWidget *widget);
static void view_list (GtkWidget *widget);
#ifdef USE_HILDON
static GtkWidget *menubar_to_menu (GtkWidget *widget);
static gchar *get_icon_name (const gchar *mime_type);
static gboolean window_key_press(GtkWindow *window, GdkEventKey *event,
		gpointer data);
#endif


/* items of the context menu */
static GtkItemFactoryEntry menu_items[] =
{
  { "/Open Wit_h",	 NULL, popup_ask_open_with,  0, "<StockItem>", GTK_STOCK_OPEN },
  { "/sep1",	         NULL, NULL,	             0, "<Separator>" },
  { "/Send via _Bluetooth", NULL, send_with_bluetooth, 0, "<Item>" },
  { "/Send via _Infrared", NULL, send_with_irda, 0, "<Item>" },
  { "/_Copy",          NULL, popup_copy_file_clip,         0, "<StockItem>", GTK_STOCK_COPY },
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
  { N_("/_File/_Copy"),          NULL, popup_copy_file_clip,         0, "<StockItem>", GTK_STOCK_COPY },
  { N_("/_File/_Paste"),          NULL, paste_file_clip,         0, "<StockItem>", GTK_STOCK_PASTE },
  { N_("/_File/_Move"),            NULL, popup_ask_move_file,            0, "<Item>" },
  { N_("/_File/_Rename"),          NULL, popup_ask_rename_file,          0, "<Item>" },
  { N_("/_File/_Delete"),   "Delete", popup_ask_delete_file,         0, "<StockItem>", GTK_STOCK_DELETE },
  { N_("/_File/_Create Directory"),NULL, create_directory_interactive, 0, "<Item>"},
  { N_("/_File/s1"), NULL , NULL,    0, "<Separator>"},
  { N_("/_File/Select _All"),"Control+A", do_select_all, 0, "<Item>"},
  { N_("/_File/s2"), NULL , NULL,    0, "<Separator>"},
  { N_("/_File/_Close"),  NULL, gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT },
  { N_("/_Settings"),         NULL,         NULL, 0, "<Branch>" },
  { N_("/_Settings/Directory Browser"), NULL, on_dirbrowser_setting_changed, 0, "<CheckItem>"},
  { N_("/_Settings/My Files only"), NULL, on_myfiles_setting_changed, 0, "<CheckItem>"},
  { N_("/_Settings/s3"), NULL , NULL,    0, "<Separator>"},
  { N_("/_Settings/View Icons"), NULL, view_icons, 0, "<RadioItem>"},
  { N_("/_Settings/View List"), NULL, view_list, 0, "<RadioItem>"},
  { N_("/_Go To"),         NULL,         NULL,           0, "<Branch>" },
  { N_("/_Help"),         NULL,         NULL,           0, "<Branch>" },
  { N_("/_Help/Index"),   NULL,         on_help_clicked,    0, "<StockItem>",GTK_STOCK_HELP },
  { N_("/_Help/About"),   NULL,         on_about_clicked,    0, "<Item>" },
};

int mMain_items_count = sizeof(mMain_items) / sizeof(GtkItemFactoryEntry);


static void 
popup_ask_delete_file (void)
{
  gint col;
  
  if (active_view == view_widget)
    col = COL_DATA;
  else
    col = COL_DIRDATA;
  
  ask_delete_file(active_view, col);
}

static void 
popup_ask_move_file (void)
{
  gint col;
  
  if (active_view == view_widget)
    col = COL_DATA;
  else
    col = COL_DIRDATA;
  
  ask_move_file(active_view, col);
}

static void 
popup_copy_file_clip (void)
{
  gint col;
  
  if (active_view == view_widget)
    col = COL_DATA;
  else
    col = COL_DIRDATA;
  copy_file_clip(active_view, col);
}

/* create menu from description */
GtkWidget *
create_mMain(GtkWidget  *window)
{
  GtkItemFactory *itemfactory;
  GtkAccelGroup *accelgroup;
  GtkWidget *item_icons, *item_list;
  
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
  item_icons = 
    gtk_item_factory_get_item (itemfactory, N_("/Settings/View Icons"));
  item_list = 
    gtk_item_factory_get_item (itemfactory, N_("/Settings/View List"));
  goto_menu_item = 
    gtk_item_factory_get_item (itemfactory, N_("/Go To"));
  
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(set_dirbrowser_menu_item),
                                 directory_browser); 
  gtk_radio_menu_item_set_group(GTK_RADIO_MENU_ITEM(item_list), 
                                gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item_icons)));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM (item_list), TRUE); 
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM (item_icons), FALSE); 
 
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
					 "%s", message);
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
    label_text = g_strdup_printf ("<b>%s</b>\n%s",_("Login failed, please try again"), q_in->uri);
  else
    label_text = g_strdup_printf ("<b>%s</b>\n%s", _("Enter credentials to access"), q_in->uri);

  dialog_window = gtk_dialog_new_with_buttons (_("Restricted Resource"), 
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
  label = gtk_label_new(_("Username"));
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
  gtk_table_attach(GTK_TABLE(table),label,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
  label = gtk_label_new(_("Password"));
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

void
do_select_all(GtkWidget *w, gpointer d)
{
  GtkTreeSelection *sel;
  
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(active_view));
  gtk_tree_selection_select_all(sel);
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
  g_object_unref (pixbuf);
  w = gtk_image_new_from_pixbuf(spixbuf);
  g_object_unref (spixbuf);

  return w;
}


static void
open_with (GnomeVFSMimeApplication *application, 
           FileInformation *file_info)
{
#ifdef USE_HILDON
  DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
#if HILDON_VER > 0
  hildon_mime_open_file(conn, file_info->filename);
#else
  osso_mime_open_file(conn, file_info->filename);
#endif
  dbus_connection_unref(conn);
#else
    
  if (application)
    {
      gchar *msg = g_strdup_printf("%s %s",_("Starting"), application->name);
      gchar *commandline = g_strdup_printf ("%s '%s'", application->command, file_info->filename);

      gpe_popup_infoprint(GDK_DISPLAY(), msg);
      g_free(msg);
      g_spawn_command_line_async (commandline, NULL);
      g_free (commandline);
    }
#endif
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
            error = g_strdup (_("File is read only."));
            break;
          case GNOME_VFS_ERROR_ACCESS_DENIED:
            error = g_strdup (_("Access denied."));
            break;
          case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
            error = g_strdup (_("Read only file system."));
            break;
          case GNOME_VFS_ERROR_FILE_EXISTS:
            error = g_strdup (_("Destination file already exists."));
            break;
          default:
            error = g_strdup_printf ("%s: %s", _("Error"),
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
rename_one_file (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
                 gpointer data)
{
  GtkWidget *dialog_window;
  GtkWidget *vbox, *label, *entry, *btnok;
  gchar *label_text;
  gint col = (gint)data;

  gtk_tree_model_get(model, iter, col, &current_popup_file, -1);

  g_return_if_fail(current_popup_file->vfs);

  label_text = g_strdup_printf ("%s %s %s", _("Rename file"), 
                                current_popup_file->vfs->name, _("to:"));

  dialog_window = gtk_dialog_new_with_buttons (_("Rename file"), 
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
  gtk_entry_set_text (GTK_ENTRY(entry), current_popup_file->vfs->name);
  gtk_editable_select_region (GTK_EDITABLE(entry), 0, -1);
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
  g_object_set_data (G_OBJECT (dialog_window), "entry", entry);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog_window)->vbox), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  gtk_widget_show_all (dialog_window);
}

static void
popup_ask_rename_file ()
{

  /* FIXME: different functions use different ways to find the file(s) 
     to be operated on.

     popup_ask_move_file and others which use the functions in fileops.c
     always act on the active selection.

     popup_ask_open_with and others rely on current_popup_file being set
     (this is set if we are invoked from a right click and is set from the
     x,y co-ords of the mouse click, not the selection).  These functions
     crash or operate on the wrong file if called from the main menu.

     These two mechanisms result in different files being selected depending
     on the operation.  One day they should be fully combined.

     For now, in order to make rename work from the main menu, we set up
     current_popup_file here (from the selection), over-riding the value
     set up by show_popup, if any. */

  gint col;
  GtkTreeSelection *sel = NULL;
  
  if (active_view == view_widget)
    col = COL_DATA;
  else
    col = COL_DIRDATA;

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(active_view));

  if (gtk_tree_selection_count_selected_rows(sel) == 0) return;

  gtk_tree_selection_selected_foreach(sel, 
				      (GtkTreeSelectionForeachFunc)rename_one_file,
				      (gpointer)col);
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
	                                  "%s", current_popup_file->vfs->name);
  table = gtk_table_new(4,2,FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
  
  /* location */
  text = g_strdup_printf("<b>%s</b>", _("Location:"));
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
  text = g_strdup_printf("<b>%s</b>", _("Size:"));
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
  text = g_strdup_printf("<b>%s</b>", _("MIME-Type:"));
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label),1,0.5); /* align right, gnomish */
  gtk_label_set_markup(GTK_LABEL(label),text);
  g_free(text);
  gtk_table_attach(GTK_TABLE(table),label,0,1,2,3,GTK_FILL,GTK_FILL,0,0);
  label = gtk_label_new(current_popup_file->vfs->mime_type);
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5); 
  gtk_table_attach(GTK_TABLE(table),label,1,2,2,3,GTK_FILL,GTK_FILL,0,0);
  
  /* change time */
  text = g_strdup_printf("<b>%s</b>", _("Changed:"));
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
  GtkWidget *dialog_window, *combo, *label; // *entry;
  GtkWidget *open_button;
  GList *applications = NULL;
  GList *appnames = NULL;
  gchar *s;

  dialog_window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(dialog_window), _("Open With..."));
  gtk_window_set_transient_for (GTK_WINDOW(dialog_window), GTK_WINDOW(window));
  gtk_window_set_modal (GTK_WINDOW (dialog_window), TRUE);
  
  label = gtk_label_new (NULL);
  s = g_strdup_printf("<b>%s</b>", _("Open with program"));
  gtk_label_set_markup(GTK_LABEL(label), s);
  g_free (s);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  //entry = gtk_entry_new ();

  combo = gtk_combo_new();
  gtk_combo_set_value_in_list(GTK_COMBO(combo), TRUE, FALSE);
    
  gtk_dialog_add_button(GTK_DIALOG(dialog_window), GTK_STOCK_CANCEL, 
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
    gchar *s;
    gtk_widget_set_sensitive (combo, FALSE);
    gtk_widget_set_sensitive (open_button, FALSE);

    s = g_strdup_printf("<b>%s</b>", _("No available applications"));
    gtk_label_set_markup (GTK_LABEL (label), s);
    g_free (s);

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
  gchar *mime_icon;
  gchar *mime_path, *p;

  mime_icon = g_strdup(gnome_vfs_mime_get_icon (mime_type));
  if (mime_icon)
  {
    if (mime_icon[0] == '/')
    {
      if (access (mime_icon, R_OK) == 0)
        return mime_icon;
    }

    mime_path = g_strdup_printf (PREFIX FILEMANAGER_ICON_PATH "/%s", mime_icon);
    if (access (mime_path, R_OK) == 0)
      return mime_path;

    mime_path = g_strdup_printf (PREFIX DEFAULT_ICON_PATH "/%s", mime_icon);
    if (access (mime_path, R_OK) == 0)
      return mime_path;
  }

  mime_icon = g_strdup (mime_type);
  while ((p = strchr(mime_icon, '/')) != NULL)
    *p = '-';

  mime_path = g_strdup_printf (PREFIX FILEMANAGER_ICON_PATH "/%s.png", mime_icon);
  if (access (mime_path, R_OK) == 0)
  {
	g_free(mime_icon);
    return mime_path;
  }
  mime_path = g_strdup_printf (PREFIX DEFAULT_ICON_PATH "/%s.png", mime_icon);
  if (access (mime_path, R_OK) == 0)
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
#ifdef USE_HILDON
    GtkIconTheme *theme = gtk_icon_theme_get_default();
    pixbuf = gtk_icon_theme_load_icon(theme, filename, 24, 0, NULL);
#else
    pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
#endif
    g_hash_table_insert (loaded_icons, (gpointer) g_strdup(filename), (gpointer) pixbuf);
    return pixbuf;
  }
}

void
add_icon (FileInformation *file_info)
{
  GdkPixbuf *pixbuf = NULL;
  gchar *mime_icon = NULL;
  
  if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
#ifdef USE_HILDON
    mime_icon = g_strdup("qgn_list_filesys_common_fldr");
#else
    mime_icon = g_strdup (PREFIX FILEMANAGER_ICON_PATH "/directory.png");
#endif
  else if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_REGULAR || file_info->vfs->type == GNOME_VFS_FILE_TYPE_UNKNOWN)
  {
   file_info->vfs->mime_type = gnome_vfs_get_mime_type (file_info->filename);
    if (file_info->vfs->mime_type)
#ifdef USE_HILDON
      mime_icon = get_icon_name(file_info->vfs->mime_type);
#else
      mime_icon = find_icon_path (file_info->vfs->mime_type);
#endif
    else
#ifdef USE_HILDON
      mime_icon = g_strdup("qgn_list_gene_unknown_file");
#else
      mime_icon = g_strdup (PREFIX FILEMANAGER_ICON_PATH "/regular.png");
#endif
  }
#ifndef USE_HILDON
  else if (file_info->vfs->type == GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK)
    mime_icon = g_strdup (PREFIX FILEMANAGER_ICON_PATH "/symlink.png");
  else
    mime_icon = g_strdup (PREFIX FILEMANAGER_ICON_PATH "/regular.png");
#else
  if (mime_icon == NULL)
    mime_icon = g_strdup("qgn_list_gene_unknown_file");
#endif

  pixbuf = get_pixbuf (mime_icon);
  g_free(mime_icon);
  
  /* now be careful, we sort the items into the view widgets */
  if ((directory_browser) && (file_info->vfs->type == GNOME_VFS_FILE_TYPE_DIRECTORY))
    { /* add to directory browser */
		GtkTreeIter iter;
#ifndef USE_HILDON
		GdkPixbuf *pb;
#endif
		  
		gtk_tree_store_append (dirstore, &iter, NULL);
#ifndef USE_HILDON
		pb = gdk_pixbuf_scale_simple(pixbuf,24,24,GDK_INTERP_BILINEAR);
#endif
		
		gtk_tree_store_set (dirstore, 
		                    &iter,
		                    COL_DIRICON,
#ifndef USE_HILDON
				    pb,
#else
				    pixbuf,
#endif
			                COL_DIRNAME, file_info->vfs->name,
			                COL_DIRDATA, (gpointer) file_info,
			                -1);
#ifndef USE_HILDON
		g_object_unref(pb);
#endif
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
#ifndef USE_HILDON
		GdkPixbuf *pb;
#endif
		gchar *size;
		gchar *time;
		  
#ifndef USE_HILDON
		pb = gdk_pixbuf_scale_simple(pixbuf,24,24,GDK_INTERP_BILINEAR);
#endif
		size = gnome_vfs_format_file_size_for_display(file_info->vfs->size); 
		time = g_strdup(ctime(&file_info->vfs->ctime));
		time[strlen(time)-1] = 0; /* strip newline */
		  
		gtk_tree_store_append (store, &iter, NULL);
		
		gtk_tree_store_set (store, &iter,
			COL_NAME, file_info->vfs->name,
			COL_SIZE, size,
			COL_CHANGED, time,
#ifndef USE_HILDON
			COL_ICON, pb,
#else
			COL_ICON, pixbuf,
#endif
			COL_DATA, (gpointer) file_info,
			-1);
		g_free(size);
		g_free(time);
#ifndef USE_HILDON
		g_object_unref(pb);
#endif
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
  {
    gpe_icon_list_view_clear (GPE_ICON_LIST_VIEW (view_widget));
	if (directory_browser)
        gtk_tree_store_clear(GTK_TREE_STORE(dirstore));
  }
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
			g_object_unref(buf);
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
      error = g_strdup (_("Destination is read only."));
      break;
    case GNOME_VFS_ERROR_ACCESS_DENIED:
      error = g_strdup (_("Access denied."));
      break;
    case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
      error = g_strdup (_("Read only file system."));
      break;
	case GNOME_VFS_ERROR_NOT_FOUND:
	case GNOME_VFS_ERROR_GENERIC:
	break;
    default:
      error = g_strdup_printf ("%s %s", _("Error:"), 
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
       {
         msg = g_strdup_printf("%s %s",_("Opening"), strrchr(directory, '/') + 1);
#ifdef USE_HILDON
         gtk_window_set_title(GTK_WINDOW(window), strrchr(directory, '/') + 1);
#endif
       }
       else /* should only happen for "/" */
       {
         msg = g_strdup_printf("%s %s",_("Opening"), strrchr(directory, '/'));
#ifdef USE_HILDON
         gtk_window_set_title(GTK_WINDOW(window), strrchr(directory, '/'));
#endif
       }
       gpe_popup_infoprint(GDK_DISPLAY(), msg);
       g_free(msg);
    } 
	else 
	{
#ifdef USE_HILDON
      gtk_window_set_title(GTK_WINDOW(window), _("My Documents"));
#endif
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

void
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
  if (!initialized) return;
  gtk_tree_store_clear(GTK_TREE_STORE(store));
  view_widget = create_view_widget_icons();
  view_is_icons = TRUE;
  refresh_current_directory ();
}


static void
view_list (GtkWidget *widget)
{
  if (!initialized) return;
  view_widget = create_view_widget_list();
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

  return FALSE;
}

static gboolean
tree_key_down (GtkWidget *tree, GdkEventKey *k, gpointer user_data)
{
  GtkTreePath *path;
  
  if (k->keyval == GDK_Return)
    {
      GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
      
      gtk_tree_view_get_cursor(GTK_TREE_VIEW(tree), &path, NULL);
      
      if (path)
        {
          GtkTreeIter iter;
          FileInformation *i;
    
          gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
    
          gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 
                              tree == view_widget ? COL_DATA : COL_DIRDATA, 
                              &i, -1);
            
          gtk_tree_path_free (path);
          
          button_clicked(NULL, i);
          return TRUE;
        }
    }
  return FALSE;
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
  	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), TRUE);
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
	g_signal_connect (G_OBJECT(treeview), "key-press-event", 
		G_CALLBACK(tree_key_down), NULL);
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
  	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);
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
	g_signal_connect (G_OBJECT(treeview), "key-press-event", 
		G_CALLBACK(tree_key_down), NULL);
	g_signal_connect (G_OBJECT(treeview), "focus-in-event", 
		G_CALLBACK(tree_focus_in), NULL);
    gtk_widget_show_all(dir_view_window);
	return treeview;
}


int
main (int argc, char *argv[])
{
  GtkWidget *hbox, *toolbar, *toolbar2, *mMain;
  GtkTooltips *tooltips;
  GtkWidget *pw;
  GtkToolItem *item;
  GtkAccelGroup *accel_group;
  GtkWidget *storage_menu;
  gint size_x, size_y, arg;
#ifdef USE_HILDON
  HildonProgram *program;
  osso_context_t *osso;
#endif

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

#ifdef USE_HILDON
  if ((osso = osso_initialize("org.handhelds.gpe_filemanager",
	  VERSION,
	  FALSE,
	  NULL)) == NULL)
    exit(1);
#else
  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);
#endif

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
  
  bluetooth_init ();

  gnome_vfs_init ();

  gtk_icon_theme_get_default();
  
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
  
#ifdef USE_HILDON
  g_set_application_name (_("GPE File manager"));
  window = hildon_window_new ();
  program = hildon_program_get_instance();
  hildon_program_add_window(program, HILDON_WINDOW(window));

  g_signal_connect(window, "key-release-event", G_CALLBACK(window_key_press),
		  NULL);
#else
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpe_set_window_icon(window, "icon");
  gtk_window_set_title (GTK_WINDOW (window),_("Filemanager"));
#endif

  g_set_prgname(_("Filemanager"));
  gtk_window_set_default_size (GTK_WINDOW (window), size_x, size_y);
  gtk_signal_connect (GTK_OBJECT (window), "delete-event",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);

  main_paned = gtk_hpaned_new ();
  
  vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);

  /* main menu */ 
  mMain = create_mMain(window);
#ifdef USE_HILDON
  mMain = menubar_to_menu(mMain);
  hildon_window_set_menu(HILDON_WINDOW(window), GTK_MENU(mMain));
#else
  gtk_box_pack_start(GTK_BOX(vbox), mMain, FALSE, TRUE, 0);
#endif
  
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

  /* toolbars */
  tooltips = gtk_tooltips_new();
  
  toolbar = gtk_toolbar_new ();
  GTK_WIDGET_UNSET_FLAGS(toolbar, GTK_CAN_FOCUS);
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  toolbar2 = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2), GTK_ORIENTATION_HORIZONTAL);

  item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
  gtk_tool_item_set_tooltip(item, tooltips, _("Go back in history."), NULL);
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(history_back), NULL);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
  item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
  gtk_tool_item_set_tooltip(item, tooltips, _("Go forward in history."), NULL);
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(history_forward), NULL);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
  item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP);
  gtk_tool_item_set_tooltip(item, tooltips, _("Go up one level."), NULL);
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(up_one_level), NULL);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  btnGoUp = GTK_WIDGET(item);
  
  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
  if (!directory_browser)
    {
      item = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
      gtk_tool_item_set_tooltip(item, tooltips, _("Goto your home directory."), NULL);
      g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(set_directory_home), NULL);
      GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);
      gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    }
  
  item = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
  gtk_tool_item_set_tooltip(item, tooltips, _("Refresh current directory."), NULL);
  g_signal_connect(G_OBJECT(item), "clicked", 
                   G_CALLBACK(refresh_current_directory), NULL);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  item = gtk_tool_button_new_from_stock(GTK_STOCK_STOP);
  gtk_tool_item_set_tooltip(item, tooltips, _("Stop the current process."), NULL);
  g_signal_connect(G_OBJECT(item), "clicked", 
                   G_CALLBACK(safety_check), NULL);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  item = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
  gtk_tool_item_set_expand(item, TRUE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    
  item = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
  gtk_tool_item_set_tooltip(item, tooltips, _("Exit from this program."), NULL);
  g_signal_connect(G_OBJECT(item), "clicked", 
                   G_CALLBACK(gtk_main_quit), NULL);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(item), GTK_CAN_FOCUS);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
  
  pw = gtk_image_new_from_stock(GTK_STOCK_JUMP_TO, 
                                gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar2)));
  pw = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Go!"), 
                                _("Goto Location"), NULL, pw, 
                                G_CALLBACK (goto_directory), NULL);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
#ifndef USE_HILDON
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
#else
  hildon_window_add_toolbar(HILDON_WINDOW(window), GTK_TOOLBAR(toolbar));
#endif
  if (directory_browser)
    gtk_box_pack_start (GTK_BOX (vbox), storage_menu, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), main_paned, TRUE, TRUE, 0);


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
  
  gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
						  (GnomeVFSModuleCallback) auth_callback,
						  NULL,
						  NULL);
						  
  gnome_vfs_mime_info_reload();
  gnome_vfs_application_registry_reload();
  
  /* check command line args */
  while ((arg = getopt(argc, argv, "p:dt:")) >= 0)
  {
    if (arg == 'p')
      {
        GnomeVFSResult res;
        GnomeVFSDirectoryHandle *handle;
        res = gnome_vfs_directory_open(&handle, optarg, GNOME_VFS_FILE_INFO_DEFAULT);
        if (res == GNOME_VFS_OK)
          {
            gnome_vfs_directory_close(handle);
            browse_directory (optarg);
          }
        else
          set_directory_home (NULL);
      }
    if (arg == 'd')
      {
        gtk_window_set_type_hint(GTK_WINDOW(window), 
                                 GDK_WINDOW_TYPE_HINT_DIALOG);
      }
    if (arg == 't')
      {
        gtk_window_set_title(GTK_WINDOW(window), optarg);
      }
  }
  
  if (argc < 2)
    set_directory_home (NULL);
  
  gtk_widget_show_all (window);
  
  if (directory_browser)
    {
      gtk_widget_hide(goto_menu_item);
      gtk_widget_show_all(dir_view_window);
    }
  else
	gtk_widget_hide(dir_view_window);
  gtk_widget_grab_focus(view_widget);
  
  setup_tabchain();
  do_scheduled_update();
  initialized = TRUE;  
  gtk_main();

  return 0;
}

#ifdef USE_HILDON
static GtkWidget *menubar_to_menu (GtkWidget *widget)
{
  GtkWidget *retval = gtk_menu_new();

  gtk_container_foreach(GTK_CONTAINER(widget),
      (GtkCallback)gtk_widget_reparent, retval);

#if HILDON_VER > 0
  g_object_ref_sink(GTK_OBJECT(widget));
#else
  gtk_object_sink(GTK_OBJECT(widget));
#endif /* HILDON_VER */

  return retval;
}

static gchar *get_icon_name (const gchar *mime_type)
{
#if HILDON_VER > 0
  gchar **names = hildon_mime_get_icon_names(
      mime_type,
      NULL);
#else
  gchar **names = osso_mime_get_icon_names(
      mime_type,
      NULL);
#endif /* HILDON_VER */
  gchar *retval = NULL;
  guint i = 0;
  GtkIconTheme *theme = gtk_icon_theme_get_default();

  for (i = 0; names[i]; i++) {
    if (gtk_icon_theme_has_icon(theme, names[i])) {
      retval = g_strdup(names[i]);
      break;
    }
  }
  g_strfreev(names);

  return retval;
}

static gboolean window_key_press(GtkWindow *window, GdkEventKey *event,
		gpointer data)
{
  if (event->type == GDK_KEY_RELEASE) {
    switch (event->keyval) {
      case HILDON_HARDKEY_FULLSCREEN:
	if (is_fullscreen)
	  gtk_window_unfullscreen(window);
	else
	  gtk_window_fullscreen(window);
	is_fullscreen = !is_fullscreen;
	return TRUE;
	break;

      case HILDON_HARDKEY_INCREASE:
	view_icons(NULL);
	return TRUE;
	break;
      case HILDON_HARDKEY_DECREASE:
	view_list(NULL);
	return TRUE;
	break;
      default:
	break;
    }
  }
  return FALSE;
}
#endif
