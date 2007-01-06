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
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/popup_menu.h>

#include <displaymigration.h>

#define WINDOW_NAME "Word"
#define _(_x) gettext (_x)

gchar *filename = NULL;
int file_modified = 0;
int search_replace_open = 0;
int last_found = 0;
gboolean utf8_mode;

GtkWidget *main_window;
GtkWidget *text_view;
GtkWidget *file_selector;
GtkWidget *search_replace_vbox;

GHashTable *tag_widgets;

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
  { "icon", PREFIX "/share/pixmaps/gpe-word.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

static void
gtk_ifactory_cb (gpointer   callback_data,
		 guint      callback_action,
		 GtkWidget *widget)
{
  g_message ("ItemFactory: activated \"%s\"",
	     gtk_item_factory_path_from_widget (widget));
}

static GtkItemFactoryEntry menu_items[] =
{
  { "/_File",		 NULL, 0,		      0, "<Branch>" },
  { "/File/_New",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_NEW },
  { "/File/_Open",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_OPEN },
  { "/File/_Save",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_SAVE },
  { "/File/Save _As...", NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_SAVE_AS  },
  { "/File/sep1",	 NULL, gtk_ifactory_cb,	      0, "<Separator>" },
  { "/File/_Quit",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_QUIT },

  { "/_Edit",		 NULL, 0,		      0, "<Branch>" },
  { "/Edit/_Undo",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_UNDO },
  { "/Edit/_Redo",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_REDO },
  { "/Edit/sep1",	 NULL, gtk_ifactory_cb,	      0, "<Separator>" },
  { "/Edit/Cu_t",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_CUT },
  { "/Edit/_Copy",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_COPY },
  { "/Edit/_Paste",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_PASTE },
  { "/Edit/sep2",	 NULL, gtk_ifactory_cb,	      0, "<Separator>" },
  { "/Edit/Cle_ar",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_CLEAR },
  { "/Edit/Select A_ll", NULL, gtk_ifactory_cb,	      0, },
  { "/Edit/sep3",	 NULL, gtk_ifactory_cb,	      0, "<Separator>" },
  { "/Edit/_Find...",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_FIND },
  { "/Edit/R_eplace...", NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_FIND_AND_REPLACE },

  { "/_Format", 	 NULL, 0,	              0, "<Branch>" },
  { "/Format/_Bold",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_BOLD },
  { "/Format/_Italic",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_ITALIC },
  { "/Format/_Underline",NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_UNDERLINE },
  { "/Format/Stri_ke",   NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_STRIKETHROUGH },
  { "/Format/sep1",	 NULL, gtk_ifactory_cb,	      0, "<Separator>" },
  { "/Format/_Left",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_JUSTIFY_LEFT },
  { "/Format/_Center",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_JUSTIFY_CENTER },
  { "/Format/_Right",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_JUSTIFY_RIGHT },
  { "/Format/_Fill",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_JUSTIFY_FILL },
  { "/Format/sep2",	 NULL, gtk_ifactory_cb,	      0, "<Separator>" },
  { "/Format/_Color",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_SELECT_COLOR },
  { "/Format/F_ont",	 NULL, gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_SELECT_FONT },

  { "/_Help",		 NULL,	       0,		      0, "<LastBranch>" },
  { "/Help/_About",	 NULL,	       gtk_ifactory_cb,	      0, "<StockItem>", GTK_STOCK_DIALOG_INFO },
};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static void
clear_text_area (void)
{
  GtkTextIter start, end;
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  gtk_text_buffer_get_bounds (buf, &start, &end);
  gtk_text_buffer_delete (buf, &start, &end);
}

static void
update_window_title (void)
{
  gchar *window_title;
  GError *error = NULL;
  gchar *displayname, *udisplayname;
  gchar *wname = _(WINDOW_NAME);

  if (filename == NULL || strlen (filename) == 0)
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

  udisplayname = g_locale_to_utf8 (displayname, -1, NULL, NULL, &error);
  if (error)
    {
      gpe_error_box (error->message);
      g_error_free (error);
    }
  
  if (udisplayname)
    {
      window_title = g_strdup_printf ("%s - %s", wname, udisplayname);

      gtk_window_set_title (GTK_WINDOW (main_window), window_title);
      g_free (udisplayname);
      g_free (window_title);
    }

  g_free (displayname);
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
  file_modified = 0;
  update_window_title ();
}

static void
open_file (char *new_filename)
{
  struct stat file_stat;
  FILE *fp;

  filename = g_strdup (new_filename);

  if ( (fp = fopen(filename, "r")) == NULL)
  {
    gpe_perror_box (filename);
  }
  else
  {
    GtkTextIter start, end;
    GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
    gchar *text, *buffer;
    gsize size;

    stat (filename, &file_stat);
    buffer = g_malloc (file_stat.st_size);
    fread (buffer, file_stat.st_size, 1, fp);
    fclose (fp);

    clear_text_area ();

    if (utf8_mode)
      {
	text = buffer;
	size = file_stat.st_size;
      }
    else
      {
	GError *error = NULL;
	text = g_locale_to_utf8 (buffer, file_stat.st_size, NULL, &size, &error);
	g_free (buffer);
	if (error)
	  {
	    gpe_error_box (error->message);
	    g_error_free (error);
	  }
      }

    if (text)
      {
	gtk_text_buffer_get_bounds (buf, &start, &end);
	gtk_text_buffer_insert (buf, &start, text, size);
	g_free (text);
      }
 
    file_modified = 0;

    update_window_title ();
  }
}

static void
open_file_from_filesel (GtkFileSelection *selector, gpointer user_data)
{
  filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_selector)));

  gtk_widget_destroy (file_selector);

  open_file (filename);
}

static void
do_save_file (gchar *filename)
{
  guint text_length;
  FILE *fp;

  if ( (fp = fopen(filename, "w")) == NULL)
  {
    gpe_perror_box (filename);
  }
  else
  {
    GtkTextIter start, end;
    GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
    gchar *text;

    gtk_text_buffer_get_bounds (buf, &start, &end);
    text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

    if (!utf8_mode)
      {
	gchar *p = text;
	text = g_locale_from_utf8 (text, -1, NULL, NULL, NULL);
	g_free (p);
      }

    if (text)
      {
	text_length = strlen (text);
	fwrite (text, 1, text_length, fp);
	fclose (fp);
	g_free (text);
      }

    file_modified = 0;
    update_window_title ();
  }
}

static void
save_file_as (GtkFileSelection *selector, gpointer user_data)
{
  filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_selector)));
  do_save_file (filename);

  gtk_widget_destroy (file_selector);
}

static void
select_open_file (void)
{
  file_selector = gtk_file_selection_new (_("Open File ..."));

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (open_file_from_filesel), NULL);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button),
		             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
		             (gpointer) file_selector);
  gtk_widget_show (file_selector);
}

static void
select_save_file_as (void)
{
  gchar *suggested_filename;
  guint text_length;
  GtkTextIter start, end;
  gint offset;
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  offset = gtk_text_iter_get_offset (&start);
  text_length = gtk_text_iter_get_offset (&end) - offset;
  if (text_length > 10)
    text_length = 10;
  gtk_text_iter_set_offset (&end, offset + text_length);

  suggested_filename = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

  file_selector = gtk_file_selection_new (_("Save as .."));

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (save_file_as), NULL);
  
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button),
		             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
		             (gpointer) file_selector);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_selector), suggested_filename);

  gtk_widget_show (file_selector);

  g_free (suggested_filename);
  while (GTK_IS_WIDGET(file_selector))
    while (gtk_events_pending())
      gtk_main_iteration_do(FALSE);
}

static void
save_file (void)
{
  if ( filename == NULL )
  {
    select_save_file_as ();
  }
  else
  {
    do_save_file (filename);
  }
}

static void
ask_save_before_exit (void)
{

  if (file_modified == 1)
  {
    switch (gpe_question_ask (_("Save current file before exiting?"), _("Question"), "question",
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
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_cut_clipboard (GTK_TEXT_BUFFER (buf), clipboard, TRUE);
}

static void
copy_selection (void)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_copy_clipboard (GTK_TEXT_BUFFER (buf), clipboard);
}

static void
paste_clipboard (void)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_paste_clipboard (GTK_TEXT_BUFFER (buf), clipboard, NULL, TRUE);
}

static void
do_find_string (GtkWidget *widget)
{
  gchar *found, *find;
  gint found_start;
  GtkWidget *entry;
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  GtkTextIter start, end;
  gchar *buffer;
    
  entry = gtk_object_get_data (GTK_OBJECT (widget), "entry");
  find = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  gtk_text_buffer_get_bounds (buf, &start, &end);
  buffer = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

  found = strstr (last_found + buffer + 1, find);

  if (!found)
    {
      gpe_error_box (_("Text not found"));
      last_found = 0;
    }
  else
    {
      found_start = found - buffer;
      gtk_text_iter_set_offset (&start, found_start);
      gtk_text_iter_set_offset (&end, found_start + strlen (find));
      
      gtk_text_buffer_move_mark_by_name (buf, "insert", &start);
      gtk_text_buffer_move_mark_by_name (buf, "selection_bound", &end);
      
      last_found = found_start;
    }

  g_free (find);
  g_free (buffer);
}

static void
do_replace_string (GtkWidget *widget)
{
  gchar *replace_with;
  GtkWidget *entry;
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  GtkTextIter sel_start, sel_end, at;

  entry = gtk_object_get_data (GTK_OBJECT (widget), "entry");
  replace_with = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  if (gtk_text_buffer_get_selection_bounds (buf, &sel_start, &sel_end))
  {
    gtk_text_iter_set_offset (&at, last_found);
    gtk_text_buffer_delete_selection (buf, FALSE, TRUE);
    gtk_text_buffer_insert (buf, &at, replace_with, strlen (replace_with));
    last_found = gtk_text_iter_get_offset (&sel_end);
  }
  else
  {
    gpe_error_box (_("Unable to replace text"));
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
    label = gtk_label_new (_("Search for:"));
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

    gtk_widget_grab_focus (entry);

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
    label1 = gtk_label_new (_("Search for:"));
    label2 = gtk_label_new (_("Replace with:"));
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

static void
toggle_tag (GtkWidget *parent_button, const gchar *tag_name)
{
  GtkTextBuffer *text_buffer;
  GtkTextIter iter_start, iter_end;

  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  if (gtk_text_buffer_get_selection_bounds (text_buffer, &iter_start, &iter_end))
  {
    if (GTK_IS_TOGGLE_BUTTON (parent_button))
    {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (parent_button)))
      gtk_text_buffer_apply_tag_by_name (text_buffer, tag_name, &iter_start, &iter_end);
    else
      gtk_text_buffer_remove_tag_by_name (text_buffer, tag_name, &iter_start, &iter_end);
    }
    else
      gtk_text_buffer_apply_tag_by_name (text_buffer, tag_name, &iter_start, &iter_end);
  }
}

static void
cursor_position_changed (GtkTextView *text_view, GdkEventButton *event)
{
  GtkWidget *tag_widget;
  GtkTextBuffer *text_buffer;
  GtkTextIter iter_start, iter_end;
  GSList *applied_tags;
  gboolean bold, italic, underline, strikethrough = FALSE;

  printf ("Cursor moved.\n");
  /*
  text_buffer = gtk_text_view_get_buffer (text_view);
    if (gtk_text_buffer_get_selection_bounds (text_buffer, &iter_start, &iter_end))
    {
      applied_tags = gtk_text_iter_get_tags (&iter_end);

      while (applied_tags != NULL)
      {
	g_object_get (G_OBJECT (applied_tags->data), "weight", &bold, NULL);
	bold = TRUE;

        applied_tags = g_slist_next (applied_tags);
      }

      tag_widget = g_hash_table_lookup (tag_widgets, (gconstpointer) "bold");
      if (bold == TRUE)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tag_widget), TRUE);
      else
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tag_widget), FALSE);
    }
  */
}

static GtkWidget *
construct_justification_popup_add_button (const gchar *stock_id)
{
  GtkWidget *button, *hbox, *image, *label, *alignment;
  GtkStockItem item;

  gtk_stock_lookup (stock_id, &item);
  alignment = gtk_alignment_new (0, 0, 0, 0);
  hbox = gtk_hbox_new (FALSE, 2);
  button = gtk_button_new ();
  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_SMALL_TOOLBAR);
  label = gtk_label_new_with_mnemonic (item.label);

  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (alignment), hbox);
  gtk_container_add (GTK_CONTAINER (button), alignment);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  return button;
}

static GtkWidget *
construct_justification_popup (GtkWidget *parent_button)
{
  GtkWidget *vbox, *button;

  vbox = gtk_vbox_new (FALSE, 0);

  button = construct_justification_popup_add_button (GTK_STOCK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = construct_justification_popup_add_button (GTK_STOCK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = construct_justification_popup_add_button (GTK_STOCK_JUSTIFY_RIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = construct_justification_popup_add_button (GTK_STOCK_JUSTIFY_FILL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  return vbox;
}

static void
add_default_buffer_tags (GtkTextBuffer *buffer)
{
  PangoFontFamily **families;
  gint n_families, i;

  gtk_text_buffer_create_tag (buffer, "blue",
			      "foreground", "blue", NULL);
  gtk_text_buffer_create_tag (buffer, "green",
			      "foreground", "green", NULL);
  gtk_text_buffer_create_tag (buffer, "red",
			      "foreground", "red", NULL);

  gtk_text_buffer_create_tag (buffer, "bold",
			      "weight", PANGO_WEIGHT_BOLD, NULL);
  gtk_text_buffer_create_tag (buffer, "italic",
			      "style", PANGO_STYLE_ITALIC, NULL);
  gtk_text_buffer_create_tag (buffer, "underline",
			      "underline", PANGO_UNDERLINE_SINGLE, NULL);
  gtk_text_buffer_create_tag (buffer, "strikethrough",
			      "strikethrough", TRUE, NULL);

    pango_context_list_families (gtk_widget_get_pango_context (GTK_WIDGET (text_view)), &families, &n_families);
    for (i=0; i<n_families; i++)
    {
      const gchar *font_name = pango_font_family_get_name (families[i]);
      gtk_text_buffer_create_tag (buffer, font_name,
				  "font", font_name, NULL);
    }
}

static void
font_selected (GtkWidget *parent, PangoFontFamily *font)
{
  printf ("Selected font %s\n", pango_font_family_get_name (font));
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *toolbar, *toolbar2, *scroll, *toolbar_icon;
  GtkWidget *toolbar_button_bold, *toolbar_button_italic, *toolbar_button_underline, *toolbar_button_strikethrough;
  GtkWidget *justify_button, *justify_button_icon, *color_button, *font_button;
  GdkPixmap *pmap;
  GdkBitmap *bmap;
  GtkTextBuffer *buf;
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  if (argc >= 2 && !strcmp (argv[1], "-u"))
    {
      argv++;
      argc--;
      utf8_mode = 1;
    }
  
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  tag_widgets = g_hash_table_new (g_str_hash, g_str_equal);
  
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  update_window_title ();
  gtk_widget_set_usize (GTK_WIDGET (main_window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  displaymigration_init ();

  displaymigration_mark_window (main_window);

  gtk_widget_realize (main_window);

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
  g_object_set_data_full (G_OBJECT (main_window), "<main>", item_factory, (GDestroyNotify) g_object_unref);
  gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gtk_item_factory_get_item (item_factory, "/Preferences/Shape/Oval")), TRUE);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  toolbar2 = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  text_view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), TRUE);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), 3);
  buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  g_signal_connect (G_OBJECT (buf), "changed",
		      GTK_SIGNAL_FUNC (text_changed), NULL);
  //g_signal_connect (G_OBJECT (text_view), "button-press-event",
  //		    GTK_SIGNAL_FUNC (cursor_position_changed), NULL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New"), 
			   _("New document"), _("New document"), toolbar_icon, new_file, NULL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Open"), 
			   _("Open file"), _("Open file"), toolbar_icon, select_open_file, NULL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Save"), 
			   _("Save current file"), _("Save current file"), toolbar_icon, save_file, NULL);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Save as"), 
			   _("Save current file as"), _("Save current file as"), toolbar_icon, select_save_file_as, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Search"), 
			   _("Search for a string"), _("Search for a string"), toolbar_icon, search_string, vbox);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Replace"), 
			   _("Replace a string"), _("Replace a string"), toolbar_icon, replace_string, vbox);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_BOLD, GTK_ICON_SIZE_SMALL_TOOLBAR);
  toolbar_button_bold = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, _("Bold"), _("Bold"), _("Make the selected text bold."), toolbar_icon, toggle_tag, "bold");
  g_hash_table_insert (tag_widgets, (gpointer) "bold", (gpointer) toolbar_button_bold);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_ITALIC, GTK_ICON_SIZE_SMALL_TOOLBAR);
  toolbar_button_italic = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, _("Italic"), _("Italic"), _("Make the selected text italic."), toolbar_icon, toggle_tag, "italic");
  g_hash_table_insert (tag_widgets, (gpointer) "italic", (gpointer) toolbar_button_italic);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_UNDERLINE, GTK_ICON_SIZE_SMALL_TOOLBAR);
  toolbar_button_italic = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, _("Underline"), _("Underline"), _("Make the selected text underlined."), toolbar_icon, toggle_tag, "underline");
  g_hash_table_insert (tag_widgets, (gpointer) "underline", (gpointer) toolbar_button_underline);

  toolbar_icon = gtk_image_new_from_stock (GTK_STOCK_STRIKETHROUGH, GTK_ICON_SIZE_SMALL_TOOLBAR);
  toolbar_button_strikethrough = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, _("Strikethrough"), _("Strikethrough"), _("Make the selected text have a strike through it."), toolbar_icon, toggle_tag, "strikethrough");
  g_hash_table_insert (tag_widgets, (gpointer) "strikethrough", (gpointer) toolbar_button_strikethrough);

  justify_button = popup_menu_button_new_from_stock (GTK_STOCK_JUSTIFY_LEFT, construct_justification_popup, NULL);
  color_button = popup_menu_button_new_type_color (NULL);
  font_button = popup_menu_button_new_type_font (font_selected);

  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), gtk_item_factory_get_widget (item_factory, "<main>"), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar2, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), font_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), color_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), justify_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (text_view));
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (main_window->window, NULL, pmap, bmap);

  gtk_widget_show (main_window);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (gtk_item_factory_get_widget (item_factory, "<main>"));
  gtk_widget_show (toolbar);
  gtk_widget_show (toolbar2);
  gtk_widget_show (scroll);
  gtk_widget_show (text_view);

  if (argc > 1)
  {
    filename = argv[1];
    open_file (argv[1]);
  }

  add_default_buffer_tags (buf);

  gtk_widget_grab_focus (text_view);

  gtk_main();
  return 0;
}

