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

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/gtkminifilesel.h>

#define WINDOW_NAME "GPE Edit"
#define _(_x) gettext (_x)

gchar *buffer;
gchar *filename = NULL;
int file_modified = 0;
int search_replace_open = 0;
int last_found = 0;

GtkWidget *main_window;
GtkWidget *text_area;
GtkWidget *file_selector;
GtkWidget *search_replace_vbox;

struct gpe_icon my_icons[] = {
  { "new", "new" },
  { "open", "open" },
  { "save", "save" },
  { "save_as", "save_as" },
  { "cut", "cut" },
  { "copy", "copy" },
  { "paste", "paste" },
  { "search", "search" },
  { "search_and_replace", "search_and_replace" },
  { "right", "right" },
  { "dir-closed", "dir-closed" },
  { "dir-up", "dir-up" },
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "stop", "stop" },
  { "question", "question" },
  { "error", "error" },
  { "icon", PREFIX "/share/pixmaps/gpe-edit.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

static void
destroy_widget (GtkWidget *parent_widget, GtkWidget *widget)
{
  gtk_widget_destroy (widget);
}

static void
clear_text_area (void)
{
  gtk_text_freeze (GTK_TEXT (text_area));
  gtk_editable_delete_text ( GTK_EDITABLE (text_area), 0, gtk_text_get_length (GTK_TEXT (text_area)));
  gtk_text_thaw (GTK_TEXT (text_area));
}

static void
update_window_title (void)
{
  gchar *window_title;
  gchar *displayname;

  if (filename == NULL)
  {
    displayname = g_malloc (strlen (_("Untitled")) + 2);
    strcpy (displayname, _("Untitled"));
  }
  else
  {
    displayname = g_malloc (strlen (basename (filename)) + 2);
    strcpy (displayname, basename (filename));
  }

  if (file_modified == 1)
  {
    strcat (displayname, " *");
  }

  window_title = g_malloc (strlen (WINDOW_NAME " - ") + strlen (displayname) + 1);
  strcpy (stpcpy (window_title, WINDOW_NAME " - "), displayname);

  gtk_window_set_title (GTK_WINDOW (main_window), window_title);
}

static void
text_changed (void)
{
  if (file_modified == 0)
  {
    file_modified = 1;
    update_window_title ();
  }
}

static void
new_file (void)
{
  clear_text_area ();
  filename = "";
  update_window_title ();
}

static void
open_file (char *filename)
{
  struct stat file_stat;
  FILE *fp;
  int pos = 0;

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

    file_modified = 1;
    gtk_editable_insert_text (GTK_EDITABLE (text_area), buffer, file_stat.st_size, &pos);
    file_modified = 0;

    update_window_title ();
  }
}

static void
open_file_from_filesel (GtkFileSelection *selector, gpointer user_data)
{

  filename = gtk_mini_file_selection_get_filename (GTK_MINI_FILE_SELECTION (file_selector));
  open_file (filename);

  gtk_widget_destroy (file_selector);
}

static void
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

    file_modified = 0;
    update_window_title ();
  }

  gtk_widget_destroy (file_selector);
}

static void
select_open_file (void)
{
  file_selector = gtk_mini_file_selection_new ("Open File ...");

  gtk_signal_connect (GTK_OBJECT (file_selector),
		      "completed", GTK_SIGNAL_FUNC (open_file_from_filesel), NULL);

  gtk_signal_connect_object (GTK_OBJECT (GTK_MINI_FILE_SELECTION(file_selector)->cancel_button),
		             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
		             (gpointer) file_selector);
  gtk_widget_show (file_selector);
}

static void
select_save_file_as (void)
{
  gchar *suggested_filename;
  guint text_length;

  text_length = gtk_text_get_length (GTK_TEXT (text_area));
  if (text_length > 10)
    text_length = 10;

  suggested_filename = g_malloc (text_length);
  suggested_filename = gtk_editable_get_chars (GTK_EDITABLE (text_area), 0, text_length);

  file_selector = gtk_mini_file_selection_new ("Save As ..");

  gtk_signal_connect (GTK_OBJECT (file_selector),
		      "completed", GTK_SIGNAL_FUNC (save_file_as), NULL);

  gtk_signal_connect_object (GTK_OBJECT (GTK_MINI_FILE_SELECTION(file_selector)->cancel_button),
		             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
		             (gpointer) file_selector);

  gtk_entry_set_text (GTK_ENTRY (GTK_MINI_FILE_SELECTION (file_selector)->entry), suggested_filename);

  gtk_widget_show (file_selector);

  g_free (suggested_filename);
  while (GTK_IS_WIDGET(file_selector))
    while (gtk_events_pending())
      gtk_main_iteration_do(FALSE);
}

static void
save_file (void)
{
  guint text_length;
  FILE *fp;

  if ( filename == NULL )
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

    file_modified = 0;
    update_window_title ();
    }
  }
}

static void
ask_save_before_exit (void)
{

  if (file_modified == 1)
  {
    switch (gpe_question_ask ("Save current file before exiting?", _("Question"), "question",
    _("Don't save"), "stop", _("Save"), "save"))
    {
    case 1: /* Save */
      save_file ();
    case 0: /* Don't Save */
      gtk_exit (0);
      default:
    }
  }
  else
  {
    gtk_exit (0);
  }
}

static void
cut_selection (void)
{
  gtk_editable_cut_clipboard (GTK_EDITABLE (text_area));
}

static void
copy_selection (void)
{
  gtk_editable_copy_clipboard (GTK_EDITABLE (text_area));
}

static void
paste_clipboard (void)
{
  gtk_editable_paste_clipboard (GTK_EDITABLE (text_area));
}

static void
do_find_string (GtkWidget *widget)
{
  gchar *found, *find;
  gint found_start;
  GtkWidget *entry;

  entry = gtk_object_get_data (GTK_OBJECT (widget), "entry");
  find = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  found = strstr (last_found + buffer + 1, find);

  if (!found)
    {
      gpe_error_box ("Text not found");
      last_found = 0;
    }
  else
    {
  found_start = found - buffer;

  gtk_editable_select_region (GTK_EDITABLE (text_area), found_start, found_start + strlen (find));

  last_found = found_start;
    }

  g_free (find);
}

static void
do_replace_string (GtkWidget *widget)
{
  gchar *replace_with;
  gint sel_start, sel_end, pos, text_length;
  GtkWidget *entry;

  entry = gtk_object_get_data (GTK_OBJECT (widget), "entry");
  replace_with = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  if (gtk_editable_get_selection_bounds (GTK_EDITABLE (text_area), &sel_start, &sel_end))
  {
    pos = last_found;
    gtk_editable_delete_text (GTK_EDITABLE (text_area), sel_start, sel_end);
    gtk_editable_insert_text (GTK_EDITABLE (text_area), replace_with, strlen (replace_with), &pos);
    last_found = sel_end;

    text_length = gtk_text_get_length (GTK_TEXT (text_area));
    buffer = g_malloc (text_length);
    buffer = gtk_editable_get_chars (GTK_EDITABLE (text_area), 0, text_length);
  }
  else
  {
    gpe_error_box ("Unable to replace text");
  }
}

static void
search_string (GtkWidget *widget, GtkWidget *parent_vbox)
{
  GtkWidget *hbox1, *hbox2, *entry, *find, *label;

  if (search_replace_open == 0)
  {
    search_replace_vbox = gtk_vbox_new (FALSE, 0);
    hbox1 = gtk_hbox_new (FALSE, 0);
    hbox2 = gtk_hbox_new (FALSE, 0);
    label = gtk_label_new ("Search for: ");
    entry = gtk_entry_new ();

    find = gpe_picture_button (hbox2->style, _("Find"), "search");

    gtk_box_pack_end (GTK_BOX (parent_vbox), search_replace_vbox, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (search_replace_vbox), hbox1, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (search_replace_vbox), hbox2, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox1), entry, TRUE, TRUE, 0);

    gtk_box_pack_end (GTK_BOX (hbox2), find, FALSE, FALSE, 0);

    gtk_object_set_data (GTK_OBJECT (find), "entry", (gpointer) entry);

    gtk_signal_connect (GTK_OBJECT (find), "clicked",
		        GTK_SIGNAL_FUNC (do_find_string), NULL);

    gtk_widget_show (search_replace_vbox);
    gtk_widget_show (hbox1);
    gtk_widget_show (hbox2);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_widget_show (find);

    search_replace_open = 1;
  }
  else
    {
  search_replace_open = 0;
  gtk_widget_destroy (search_replace_vbox);
  }
}

static void
replace_string (GtkWidget *widget, GtkWidget *parent_vbox)
{
  GtkWidget *hbox1, *hbox2, *hbox3, *entry1, *entry2, *replace, *find, *label1, *label2;

  if (search_replace_open == 0)
    {
    search_replace_vbox = gtk_vbox_new (FALSE, 0);
    hbox1 = gtk_hbox_new (FALSE, 0);
    hbox2 = gtk_hbox_new (FALSE, 0);
    hbox3 = gtk_hbox_new (FALSE, 0);
    label1 = gtk_label_new ("Search for: ");
    label2 = gtk_label_new ("Replace with: ");
    entry1 = gtk_entry_new ();
    entry2 = gtk_entry_new ();

    replace = gpe_picture_button (hbox2->style, _("Replace"), "search_and_replace");
    find = gpe_picture_button (hbox2->style, _("Find"), "search");

    gtk_box_pack_end (GTK_BOX (parent_vbox), search_replace_vbox, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (search_replace_vbox), hbox1, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (search_replace_vbox), hbox2, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (search_replace_vbox), hbox3, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox1), entry1, TRUE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (hbox2), label2, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox2), entry2, TRUE, TRUE, 0);

    gtk_box_pack_end (GTK_BOX (hbox3), find, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (hbox3), replace, FALSE, FALSE, 0);

    gtk_object_set_data (GTK_OBJECT (replace), "entry", (gpointer) entry2);
    gtk_object_set_data (GTK_OBJECT (find), "entry", (gpointer) entry1);

    gtk_signal_connect (GTK_OBJECT (find), "clicked",
		        GTK_SIGNAL_FUNC (do_find_string), NULL);

    gtk_signal_connect (GTK_OBJECT (replace), "clicked",
    		        GTK_SIGNAL_FUNC (do_replace_string), NULL);

    gtk_widget_show (search_replace_vbox);
    gtk_widget_show (hbox1);
    gtk_widget_show (hbox2);
    gtk_widget_show (hbox3);
    gtk_widget_show (label1);
    gtk_widget_show (label2);
    gtk_widget_show (entry1);
    gtk_widget_show (entry2);
    gtk_widget_show (replace);
    gtk_widget_show (find);

    search_replace_open = 1;
  }
  else
    {
  search_replace_open = 0;
  gtk_widget_destroy (search_replace_vbox);
    }
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

#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  text_area = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text_area), TRUE);
  gtk_signal_connect (GTK_OBJECT (text_area), "changed",
		      GTK_SIGNAL_FUNC (text_changed), NULL);

  p = gpe_find_icon ("new");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New"), 
			   _("New document"), _("New document"), pw, new_file, NULL);

  p = gpe_find_icon ("open");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open"), 
			   _("Open file"), _("Open file"), pw, select_open_file, NULL);

  p = gpe_find_icon ("save");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Save"), 
			   _("Save current file"), _("Save current file"), pw, save_file, NULL);

  p = gpe_find_icon ("save_as");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Save as"), 
			   _("Save current file as"), _("Save current file as"), pw, select_save_file_as, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("cut");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Cut"), 
			   _("Cut the selection"), _("Cut the selection"), pw, cut_selection, NULL);

  p = gpe_find_icon ("copy");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Copy"), 
			   _("Copy the selection"), _("Copy the selection"), pw, copy_selection, NULL);

  p = gpe_find_icon ("paste");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Paste"), 
			   _("Paste the clipboard"), _("Paste the clipboard"), pw, paste_clipboard, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("search");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Search"), 
			   _("Search for a string"), _("Search for a string"), pw, search_string, vbox);

  p = gpe_find_icon ("search_and_replace");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Replace"), 
			   _("Replace a string"), _("Replace a string"), pw, replace_string, vbox);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("exit");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"), 
			   _("Exit"), _("Exit"), pw, ask_save_before_exit, NULL);

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

  if (argc > 1)
  {
    open_file (argv[1]);
  }

  gtk_widget_grab_focus (text_area);

  gtk_main();
  return 0;
}

