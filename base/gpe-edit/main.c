/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <libintl.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include "init.h"
#include "errorbox.h"
#include "render.h"
#include "pixmaps.h"
#include "gtkminifilesel.h"
#include "what.h"

#define WINDOW_NAME "GPE Edit"
#define _(_x) gettext (_x)

gchar *buffer;
gchar *filename = "";

GtkWidget *main_window;
GtkWidget *text_area;
GtkWidget *file_selector;

struct gpe_icon my_icons[] = {
  { "new", "new" },
  { "open", "open" },
  { "save", "save" },
  { "save_as", "save_as" },
  { "cut", "cut" },
  { "copy", "copy" },
  { "paste", "paste" },
  { "exit", "exit" },
  { "dir-closed", "dir-closed" },
  { "dir-up", "dir-up" },
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "icon", PREFIX "/share/pixmaps/gpe-edit.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

void
clear_text_area (void)
{
  gtk_text_freeze (GTK_TEXT (text_area));
  gtk_editable_delete_text ( GTK_EDITABLE (text_area), 0, gtk_text_get_length (GTK_TEXT (text_area)));
  gtk_text_thaw (GTK_TEXT (text_area));
}

void
update_window_title (void)
{
  gchar *window_title;
  gchar *displayname;

  if (filename == "")
  {
    displayname = g_malloc (strlen (_("Untitled")));
    strcpy (displayname, _("Untitled"));
  }
  else
  {
    displayname = g_malloc (strlen (basename (filename)));
    strcpy (displayname, basename (filename));
  }

  window_title = g_malloc (strlen (WINDOW_NAME " - ") + strlen (displayname) + 1);
  strcpy (stpcpy (window_title, WINDOW_NAME " - "), displayname);

  gtk_window_set_title (GTK_WINDOW (main_window), window_title);
}

void
new_file (void)
{
  clear_text_area ();
  filename = "";
  update_window_title ();
}

void
open_file (GtkFileSelection *selector, gpointer user_data)
{
  struct stat file_stat;
  FILE *fp;
  int pos = 0;

  filename = gtk_mini_file_selection_get_filename (GTK_MINI_FILE_SELECTION (file_selector));

  if ( (fp = fopen(filename, "r")) == NULL)
  {
    gpe_perror_box (filename);
  }
  else
  {
    stat (filename, &file_stat);
    buffer = g_malloc (file_stat.st_size);
    fread (buffer, file_stat.st_size, 1, fp);
    fclose (fp);

    clear_text_area ();

    gtk_editable_insert_text (GTK_EDITABLE (text_area), buffer, file_stat.st_size, &pos);

    update_window_title ();
  }

  gtk_widget_destroy (file_selector);
}

void
save_file_as (GtkFileSelection *selector, gpointer user_data)
{
  guint text_length;
  FILE *fp;

  filename = gtk_mini_file_selection_get_filename (GTK_MINI_FILE_SELECTION (file_selector));

  if ( (fp = fopen(filename, "w")) == NULL)
  {
    gpe_perror_box (filename);
  }
  else
  {
    text_length = gtk_text_get_length (GTK_TEXT (text_area));
    buffer = g_malloc (text_length);
    buffer = gtk_editable_get_chars (GTK_EDITABLE (text_area), 0, text_length);

    fwrite (buffer, 1, text_length, fp);
    fclose (fp);
    g_free (buffer);

    update_window_title ();
  }

  gtk_widget_destroy (file_selector);
}

void
select_open_file (void)
{
  file_selector = gtk_mini_file_selection_new ("Open File ...");

  gtk_signal_connect (GTK_OBJECT (file_selector),
		      "completed", GTK_SIGNAL_FUNC (open_file), NULL);

  gtk_signal_connect_object (GTK_OBJECT (GTK_MINI_FILE_SELECTION(file_selector)->cancel_button),
		             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
		             (gpointer) file_selector);
  gtk_widget_show (file_selector);
}

void
select_save_file_as (void)
{
  file_selector = gtk_mini_file_selection_new ("Save As ..");

  gtk_signal_connect (GTK_OBJECT (file_selector),
		      "completed", GTK_SIGNAL_FUNC (save_file_as), NULL);

  gtk_signal_connect_object (GTK_OBJECT (GTK_MINI_FILE_SELECTION(file_selector)->cancel_button),
		             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
		             (gpointer) file_selector);
  gtk_widget_show (file_selector);
}

void
save_file (void)
{
  guint text_length;
  FILE *fp;

  if ( filename == "" )
  {
    select_save_file_as ();
  }
  else
  {
    if ( (fp = fopen(filename, "w")) == NULL)
    {
      gpe_perror_box (filename);
    }
    else
    {
      text_length = gtk_text_get_length (GTK_TEXT (text_area));
      buffer = g_malloc (text_length);
      buffer = gtk_editable_get_chars (GTK_EDITABLE (text_area), 0, text_length);

      fwrite (buffer, 1, text_length, fp);
      fclose (fp);
      g_free (buffer);
    }
  }
}

void
cut_selection (void)
{
  gtk_editable_cut_clipboard (GTK_EDITABLE (text_area));
}

void
copy_selection (void)
{
  gtk_editable_copy_clipboard (GTK_EDITABLE (text_area));
}

void
paste_clipboard (void)
{
  gtk_editable_paste_clipboard (GTK_EDITABLE (text_area));
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *toolbar, *scroll;
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
  
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  update_window_title ();
  gtk_widget_set_usize (GTK_WIDGET (main_window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (main_window);

  vbox = gtk_vbox_new (FALSE, 0);

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  text_area = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text_area), TRUE);

  p = gpe_find_icon ("new");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New document"), 
			   _("New document"), _("New document"), pw, new_file, NULL);

  p = gpe_find_icon ("open");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open file"), 
			   _("Open file"), _("Open file"), pw, select_open_file, NULL);

  p = gpe_find_icon ("save");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Save current file"), 
			   _("Save current file"), _("Save current file"), pw, save_file, NULL);

  p = gpe_find_icon ("save_as");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Save current file as"), 
			   _("Save current file as"), _("Save current file as"), pw, select_save_file_as, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("cut");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Cut the selection"), 
			   _("Cut the selection"), _("Cut the selection"), pw, cut_selection, NULL);

  p = gpe_find_icon ("copy");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Copy the selection"), 
			   _("Copy the selection"), _("Copy the selection"), pw, copy_selection, NULL);

  p = gpe_find_icon ("paste");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Paste the clipboard"), 
			   _("Paste the clipboard"), _("Paste the clipboard"), pw, paste_clipboard, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("exit");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"), 
			   _("Exit"), _("Exit"), pw, gtk_exit, NULL);

  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (text_area));
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (main_window->window, NULL, pmap, bmap);

  gtk_widget_show (main_window);
  gtk_widget_show (vbox);
  gtk_widget_show (toolbar);
  gtk_widget_show (scroll);
  gtk_widget_show (text_area);

  what_init ();

  gtk_main();
  return 0;
}

