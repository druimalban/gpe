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
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/gpe-iconlist.h>

#include "mime-sql.h"
#include "mime-programs-sql.h"

#define _(x) dgettext(PACKAGE, x)

#define COMPLETED 0
#define LAST_SIGNAL 1

GtkWidget *window, *vbox2;
GtkWidget *combo;
GtkWidget *view_scrolld;
GtkWidget *view_widget;

GdkPixbuf *default_pixbuf;

guint screen_w, screen_h;

GtkWidget *current_button=NULL;
int current_button_is_down=0;

/* For not starting an app twice after a double click */
int ignore_press = 0;

int loading_directory = 0;
int combo_signal_id;

int history_place = 0;
GList *history = NULL;

gchar *current_directory = "";
gchar *current_view = "icons";

struct file_infomation
{
  struct mime_type *mime;
  gchar *filename;
  gchar *type;
};

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
  { "icon", PREFIX "/share/pixmaps/gpe-filemanager.png" },
  {NULL, NULL}
};

#if GTK_MAJOR_VERSION < 2
#define gdk1_pixbuf_new_from_file(x)  gdk_pixbuf_new_from_file (x)
#else
#define gdk1_pixbuf_new_from_file(x)  gdk_pixbuf_new_from_file (x, NULL)
#endif

guint window_x = 240, window_y = 310;

static void browse_directory (gchar *directory);

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
  pixbuf = gdk1_pixbuf_new_from_file (fn);
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
  //char *cmd[] = {"/bin/sh", "-c", ""};

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
    command = g_strdup_printf ("%s %s &", program_command, exec);
    //printf ("%s %s\n", program, exec);
    //cmd[3] = g_strdup_printf ("%s", command);
    //printf ("%s", cmd[3]);
    //gnome_execute_async (NULL, 3, cmd);
    system (command);
  }
}

void static
open_with (GtkButton *button, gpointer data)
{
  char *command, *program, *exec;

  program = gtk_editable_get_chars (GTK_EDITABLE (data), 0, -1);
  exec = gtk_object_get_data (GTK_OBJECT (button), "exec");

  command = g_strdup_printf ("%s %s &", program, exec);
  system (command);
}

void static
open_with_row_selected (GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
  gchar *program;

  program = gtk_clist_get_row_data (GTK_CLIST (clist), row);
  gtk_entry_set_text (GTK_ENTRY (data), program);
}

void static
ask_open_with (char *exec)
{
  GtkWidget *window, *fakeparentwindow, *clist, *entry, *label;
  GtkWidget *open_button, *cancel_button;
  GdkPixbuf *pixbuf = NULL, *spixbuf;
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  GSList *iter;
  int row_num = 0;
  gchar *pixmap_file, *row_text[2];

  fakeparentwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakeparentwindow);

  window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(window), "Open With...");
  gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(fakeparentwindow));
  gtk_widget_realize (window);
 
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (kill_widget),
                      window);

  label = gtk_label_new ("Open with program");
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "gpe-edit");

  clist = gtk_clist_new (2);
  gtk_signal_connect (GTK_OBJECT (clist), "select-row",
                      GTK_SIGNAL_FUNC (open_with_row_selected),
                      entry);
  gtk_clist_set_column_width (GTK_CLIST (clist), 0, 12);

  open_button = gpe_picture_button (window->style, "Open", "open");
  gtk_object_set_data (GTK_OBJECT (open_button), "exec", (gpointer) exec);
  gtk_signal_connect (GTK_OBJECT (open_button), "clicked",
                      GTK_SIGNAL_FUNC (open_with),
                      entry);
  gtk_signal_connect (GTK_OBJECT (open_button), "clicked",
                      GTK_SIGNAL_FUNC (kill_widget),
                      window);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                      open_button);

  cancel_button = gpe_picture_button (window->style, "Cancel", "cancel");
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
                      GTK_SIGNAL_FUNC (kill_widget),
                      window);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                      cancel_button);

  //gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), hbox);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), label, TRUE, TRUE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), clist, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), entry, TRUE, TRUE, 4);

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
      pixbuf = gdk1_pixbuf_new_from_file (pixmap_file);
      if (pixbuf != NULL)
      {
        spixbuf = gdk_pixbuf_scale_simple (pixbuf, 12, 12, GDK_INTERP_BILINEAR);
        gpe_render_pixmap (NULL, spixbuf, &pixmap, &bitmap);
        gtk_clist_set_pixmap (GTK_CLIST (clist), row_num, 0, pixmap, bitmap);
      }
      row_num++;
    }
  }

  if (row_num == 0)
  {
    gtk_widget_destroy (entry);
    gtk_widget_destroy (clist);
    gtk_widget_destroy (open_button);
    gtk_label_set_text (GTK_LABEL (label), "No avalible applications.");
  }

  gtk_widget_show_all (window);	
}

void
button_clicked (GtkWidget *widget, gpointer udata)
{
  //printf ("My type is %s\n", ((struct file_infomation *) udata)->type);

  if (strcmp (((struct file_infomation *) udata)->type, "file") == 0)
  {
    run_program (((struct file_infomation *) udata)->filename,((struct file_infomation *) udata)->mime ->mime_name);
  }
  else if (strcmp (((struct file_infomation *) udata)->type, "regular") == 0)
  {
    ask_open_with (((struct file_infomation *) udata)->filename);
  }
  else
  {
    browse_directory (((struct file_infomation *) udata)->filename);
  }
}

void
add_icon (gchar *base_filename, gchar *filename)
{
  struct stat file_stats;
  struct mime_type *file_mime = NULL;
  struct file_infomation *file_info = g_malloc (sizeof (struct file_infomation));
  GSList *iter;
  gchar *image_filename = PREFIX "/share/gpe/pixmaps/default/filemanager/document-icons/";
  gchar *extension;

  printf ("add_icon varibles:\n\tbase_filename = %s\n\tfilename = %s\n\n", base_filename, filename);

  if (stat (filename, &file_stats) == 0)
  {
    file_info->filename = filename;

    if (S_ISDIR (file_stats.st_mode))
    {
      file_info->type = "directory";
      image_filename = g_strdup_printf ("%s%s", image_filename, "directory");
    }
    else if (__S_ISTYPE ((file_stats.st_mode), __S_IEXEC))
    {
      file_info->type = "executable";
      image_filename = g_strdup_printf ("%s%s", image_filename, "executable");
    }
     else
    {
      extension = get_file_extension (filename);

      if (mime_types)
      {

        for (iter = mime_types; iter; iter = iter->next)
         {
          struct mime_type *type = iter->data;

          if (loading_directory == 0)
            break;

          if (strcmp (type->extension, extension) == 0)
          {
            file_mime = type;
            break;
          }
          else
          {
            file_mime = NULL;
           }
        }

        if (file_mime == NULL)
        {
          file_info->type = "regular";
          file_info->mime = NULL;
          image_filename = g_strdup_printf ("%s%s", image_filename, "regular");
        }
         else
        {
          file_info->type = "file";
          file_info->mime = file_mime;
          image_filename = g_strdup_printf ("%s%s", image_filename, "gnome-");
          image_filename = g_strdup_printf ("%s%s", image_filename, file_mime->icon);
        }
      }
    }
    image_filename = g_strdup_printf ("%s%s", image_filename, ".png");
  }
  //printf ("Found image %s\n", image_filename);
  gpe_iconlist_add_item (GPE_ICONLIST (view_widget), base_filename, image_filename, (gpointer) file_info);
}

gint
sort_filenames (gconstpointer *a, gconstpointer *b)
{
  return (strcoll ((char *) a, (char *) b));
}

/* Render the contents for the current directory. */
static void
make_view (gchar *view)
{
  struct dirent *d;
  DIR *dir;
  gchar *filename;
  GList *filenames = NULL;
  GnomeVFSDirectoryHandle *handle;
  GnomeVFSFileInfo *file_info;
  GnomeVFSURI *uri;
  GnomeVFSResult result, open_dir_result;
  loading_directory = 1;

  if (view_widget)
    gtk_widget_destroy (view_widget);

  view_widget = gpe_iconlist_new ();
  gtk_signal_connect (GTK_OBJECT (view_widget), "clicked",
		      GTK_SIGNAL_FUNC (button_clicked), NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), view_widget, TRUE, TRUE, 0);
  gtk_widget_show (view_widget);


  uri = gnome_vfs_uri_new (current_directory);
  file_info = gnome_vfs_file_info_new ();

  open_dir_result = gnome_vfs_directory_open_from_uri (&handle, uri, GNOME_VFS_FILE_INFO_DEFAULT);

  while (open_dir_result == GNOME_VFS_OK)
  {
    result = gnome_vfs_directory_read_next (handle, file_info);
    printf ("Gnome VFS FileInfo->Name: %s\n", file_info->name);

    if (loading_directory == 0)
      break;

    if (file_info->name != NULL)
    {
      if (strcmp (current_directory, "/"))
        filename = g_strdup_printf ("%s/%s", current_directory, file_info->name);
      else
        filename = g_strdup_printf ("/%s", file_info->name);

      if (basename (filename)[0] != '.')
        filenames = g_list_append (filenames, filename);
    }

    if (result != GNOME_VFS_OK)
      break;
  }

  if (open_dir_result == GNOME_VFS_OK)
  {
    gnome_vfs_file_info_unref (file_info);
    gnome_vfs_directory_close (handle);

    filenames = g_list_sort (filenames, sort_filenames);
    filenames = g_list_first (filenames);

    while (filenames)
    {
      add_icon (basename ((gchar *) filenames->data), (gchar *) filenames->data);
      filenames = filenames->next;
    }
  }

  /*
  dir = opendir (current_directory);
  if (dir)
  {
    while (d = readdir (dir), d != NULL)
    {
      if (loading_directory == 0)
        break;

      if (strcmp (current_directory, "/"))
        filename = g_strdup_printf ("%s/%s", current_directory, d->d_name);
     else
        filename = g_strdup_printf ("/%s", d->d_name);

      if (basename (filename)[0] != '.')
      {

	//printf ("Found file %s\n", filename);
	filenames = g_list_append (filenames, filename);
	//add_icon (basename (filename), filename);
      }
    }
    closedir (dir);

    filenames = g_list_sort (filenames, sort_filenames);
    filenames = g_list_first (filenames);

    while (filenames)
    {
      add_icon (basename ((gchar *) filenames->data), (gchar *) filenames->data);
      filenames = filenames->next;
    }
  }
  */
  loading_directory = 0;
}

static void
set_view (GtkWidget *button, gpointer data)
{
  current_view = (gchar *) data;
  make_view ((gchar *) data);
}

static void
refresh_view (void)
{
  set_view (NULL, current_view);
}

static void
goto_directory (GtkWidget *widget)
{
  gchar *new_directory;

  safety_check ();

  new_directory = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (combo)->entry), 0, -1);
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

static void
browse_directory (gchar *directory)
{
  struct stat s;

  if (stat (directory, &s) == 0)
  {
    if (S_ISDIR (s.st_mode))
    {
      current_directory = g_strdup (directory);
      //printf ("Current directory: %s\n", current_directory);
      add_history (directory);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), directory);
      refresh_view ();
    }
    else
    {
      //gpe_error_box ("This file isn't a directory.");
    }
  }
  else
  {
    //gpe_error_box ("No such file or directory.");
  }
}
static void
set_directory_home (GtkWidget *widget)
{
  browse_directory (g_get_home_dir ());
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
    refresh_view ();
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
    refresh_view ();
  }
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *hbox2, *toolbar, *toolbar2, *view_option_menu, *view_menu, *view_menu_item;
  GdkPixbuf *p;
  GtkWidget *pw;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Filemanager");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  hbox2 = gtk_hbox_new (FALSE, 0);

  combo = gtk_combo_new ();
  combo_signal_id = gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "activate", GTK_SIGNAL_FUNC (goto_directory), NULL);

  view_option_menu = gtk_option_menu_new ();
  view_menu = gtk_menu_new ();
  gtk_option_menu_set_menu(GTK_OPTION_MENU (view_option_menu) ,view_menu);

  view_menu_item = gtk_menu_item_new_with_label ("Icon view");
  gtk_widget_show (view_menu_item);
  gtk_menu_append (GTK_MENU (view_menu), view_menu_item);
  gtk_signal_connect (GTK_OBJECT (view_menu_item), "activate", 
     (GtkSignalFunc) set_view, "icons");

  view_menu_item = gtk_menu_item_new_with_label ("List view");
  gtk_widget_show (view_menu_item);
  gtk_menu_append (GTK_MENU (view_menu), view_menu_item);
  gtk_signal_connect (GTK_OBJECT (view_menu_item), "activate", 
     (GtkSignalFunc) set_view, "list");


#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  toolbar2 = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_SPACE_LINE);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  toolbar2 = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);
#endif

  p = gpe_find_icon ("left");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Back"), 
			   _("Back"), _("Back"), pw, history_back, NULL);

  p = gpe_find_icon ("right");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Forward"), 
			   _("Forward"), _("Forward"), pw, history_forward, NULL);

  p = gpe_find_icon ("up");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Up one level"), 
			   _("Up one level"), _("Up one level"), pw, up_one_level, NULL);

  p = gpe_find_icon ("stop");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Stop"), 
			   _("Stop"), _("Stop"), pw, safety_check, NULL);

  p = gpe_find_icon ("refresh");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Refresh"), 
			   _("Refresh"), _("Refresh"), pw, refresh_view, NULL);

  p = gpe_find_icon ("home");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Home"), 
			   _("Home"), _("Home"), pw, set_directory_home, NULL);

  p = gpe_find_icon ("dir-up");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Goto Location"), 
			   _("Goto Location"), _("Goto Location"), pw, goto_directory, NULL);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), view_option_menu, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), toolbar2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show (window);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (hbox2);
  gtk_widget_show (toolbar);
  gtk_widget_show (toolbar2);
  gtk_widget_show (combo);
  gtk_widget_show (view_option_menu);
  gtk_widget_show (view_menu);
  gtk_widget_show (vbox2);

  if (sql_start ())
    exit (1);

  if (programs_sql_start ())
    exit (1);

  gtk_option_menu_set_history (GTK_OPTION_MENU (view_option_menu), 0);

  
  if (mime_programs)
  {
    GSList *iter;

    //printf ("|\tname\t\t|\tmime\t|\tcommand\t|\n\n");

    for (iter = mime_programs; iter; iter = iter->next)
    {
      struct mime_program *c = iter->data;

      //printf ("|\t%s\t\t|\t%s\t|\t%s\t|\n", c->name, c->mime, c->command);
    }
  }

  set_directory_home (NULL);
  //history_place--;

  gnome_vfs_init ();

  gtk_main();

  return 0;
}
