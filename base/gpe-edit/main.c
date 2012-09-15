/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 * Copyright (C) 2004, 2006 Florian Boor <florian.boor@kernelconcepts.de>
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
#include <gdk/gdkkeysyms.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/gpedialog.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gpe/spacing.h>
#include <gpe/gpehelp.h>

#define WINDOW_NAME "Editor"
#define _(_x) gettext (_x)

gchar *filename = NULL;
gint file_modified = 0;
gint search_replace_open = 0;
gint last_found = 0;
gboolean utf8_mode;
const gchar *appname = "gpe-edit";

GtkWidget *main_window;
GtkWidget *text_area;
GtkWidget *file_selector;
GtkWidget *search_replace_vbox;

/* some forwards */
static void save_file (void);


struct gpe_icon my_icons[] = {
  { "save_as", "save_as" },
  { "cut", "cut" },
  { "copy", "copy" },
  { "paste", "paste" },
  { "search", "search" },
  { "search_and_replace", "search_and_replace" },
  { "right", "right" },
  { "dir-closed", "dir-closed" },
  { "dir-up", "dir-up" },
  { "stop", "stop" },
  { "icon", PREFIX "/share/pixmaps/gpe-edit.png" },
  { "help", "help" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

static void
show_help (void)
{
        gboolean test;
        gchar *topic = NULL;

        test = gpe_show_help(appname, topic);
        if (test == TRUE)
		gpe_error_box (_("Help not (or incorrectly) installed. Or no helpviewer application registered."));
}


static void
clear_text_area (void)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
  gtk_text_buffer_set_text(buf,"",0);
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
	displayname = g_strdup_printf("%s %s",_("Untitled"),file_modified ? "*" : "");
  }
  else
  {
	displayname = g_strdup_printf("%s %s",basename(filename),file_modified ? "*" : "");
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
new_file (GtkWidget *w, gpointer p)
{
  if (file_modified)
  {
    if (gpe_question_ask (_("Current file is modified, save?"), _("Question"), 
		                  "question", "!gtk-no", NULL, "!gtk-yes", NULL, NULL))
      save_file ();
	else
	  file_modified = 0;
  }
  
  if (!file_modified)
  {
    clear_text_area ();
    file_modified = 0;
    if (filename) g_free(filename);
    filename = NULL;
    update_window_title ();
  }
}

static void
open_file (const char *new_filename)
{
  struct stat file_stat;
  gchar *old_filename;
  FILE *fp;
  
  old_filename = filename;
  filename = g_strdup (new_filename);

  if ( (fp = fopen(filename, "r")) == NULL)
  {
    gpe_perror_box (filename);
    g_free (filename);
    filename = old_filename;
  }
  else
  {
    GtkTextIter start, end;
    GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
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
    if (old_filename) g_free(old_filename);
  }
}

static void
open_file_from_filesel (GtkFileSelection *selector, gpointer user_data)
{
  char *fname = NULL;
  
  fname = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_selector)));
  gtk_widget_destroy (file_selector);
  open_file (fname);
  if (fname) free(fname);
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
    GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
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
select_open_file (void)
{
  file_selector = gtk_file_selection_new (_("Open File ..."));
  
  if(filename != NULL)
  {
    gchar *dir, *dir_with_sep;
    gint path_len;

    dir = g_path_get_dirname(filename);
    path_len = strlen(dir);
    dir_with_sep = (gchar *) g_malloc(path_len+2);
    
    strncpy(dir_with_sep, dir, path_len);
    dir_with_sep[path_len] = G_DIR_SEPARATOR;
    dir_with_sep[path_len+1] = '\0';
    
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector), dir_with_sep);
    
    g_free(dir_with_sep);
    g_free(dir);    
  }

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (open_file_from_filesel), NULL);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)->cancel_button),
		             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
		             (gpointer) file_selector);
  gtk_window_set_transient_for(GTK_WINDOW(file_selector), GTK_WINDOW(main_window));
  gtk_window_set_modal(GTK_WINDOW(file_selector), TRUE);
	
  gtk_widget_show (file_selector);
}

static void
select_save_file_as (void)
{
  gchar *suggested_filename;
  guint text_length;
  GtkTextIter start, end;
  gint offset;
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));

  gtk_text_buffer_get_bounds (buf, &start, &end);
  offset = gtk_text_iter_get_offset (&start);
  text_length = gtk_text_iter_get_offset (&end) - offset;
  if (text_length > 10)
    text_length = 10;
  gtk_text_iter_set_offset (&end, offset + text_length);

  suggested_filename = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

  file_selector = gtk_file_selection_new (_("Save as..."));
  gtk_window_set_transient_for(GTK_WINDOW(file_selector), GTK_WINDOW(main_window));
  gtk_window_set_modal(GTK_WINDOW(file_selector), TRUE);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_selector), suggested_filename);

  gtk_widget_show (file_selector);
  
  if (gtk_dialog_run(GTK_DIALOG(file_selector)) == GTK_RESPONSE_OK)
    {
	  if (filename) 
		  g_free(filename);
	  filename = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_selector)));
	  
	  do_save_file (filename);
	
	  if (access(filename,W_OK))
		{
		  g_free(filename);
		  filename = NULL;
		}
    }
  gtk_widget_destroy (file_selector);
  g_free (suggested_filename);
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
    switch (gpe_question_ask (_("Save current file before exiting?"), _("Question"), "!gtk-question",
    _("Don't save"), "stop", "!gtk-save", NULL, NULL))
    {
    case 1: /* Save */
      save_file ();
	  if (!file_modified) 
		  gtk_exit(0);
	break;
    case 0: /* Don't Save */
      gtk_exit (0);
	break;
    }
  }
  else
  {
    gtk_exit (0);
  }
}

#ifdef UNUSED
static void
cut_selection (void)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_cut_clipboard (GTK_TEXT_BUFFER (buf), clipboard, TRUE);
}
#endif /* UNUSED */

static void
copy_selection (void)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_copy_clipboard (GTK_TEXT_BUFFER (buf), clipboard);
}

static void
paste_clipboard (void)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_paste_clipboard (GTK_TEXT_BUFFER (buf), clipboard, NULL, TRUE);
}

static void
do_find_string (GtkWidget *widget)
{
  gchar *found, *find;
  gint found_start;
  GtkWidget *entry;
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
  GtkTextIter start, end;
  gchar *buffer;
    
  entry = gtk_object_get_data (GTK_OBJECT (widget), "entry");
  find = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  gtk_text_buffer_get_bounds (buf, &start, &end);
  buffer = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

  found = strstr (last_found + buffer + 1, find);

  if (!found)
    {
      gpe_info_dialog (_("Text not found"));
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
do_replace_string (GtkWidget *widget, gpointer p)
{
  gchar *replace_with;
  GtkWidget *entry;
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
  GtkTextIter sel_start, sel_end, at;

  entry = gtk_object_get_data (GTK_OBJECT (widget), "entry");
  replace_with = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

  if (gtk_text_buffer_get_selection_bounds (buf, &sel_start, &sel_end))
  {
      gtk_text_buffer_get_iter_at_offset (buf, &at, last_found);
      gtk_text_buffer_insert (buf, &at, replace_with, strlen (replace_with));
      gtk_text_buffer_delete_selection (buf, FALSE, TRUE);
	  gtk_text_buffer_get_selection_bounds (buf, &sel_start, &sel_end);
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
  GtkWidget *hbox1, *entry, *find, *label;

  if (search_replace_open == 0)
  {
    search_replace_vbox = gtk_vbox_new (FALSE, 0);
    hbox1 = gtk_hbox_new (FALSE, 0);
    label = gtk_label_new (_("Search for: "));
    entry = gtk_entry_new ();

    find = gpe_picture_button (hbox1->style, _("Find"), "search");

    gtk_box_pack_end (GTK_BOX (parent_vbox), search_replace_vbox, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (search_replace_vbox), hbox1, TRUE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox1), entry, TRUE, TRUE, 0);

    gtk_box_pack_end (GTK_BOX (hbox1), find, FALSE, FALSE, 0);

    gtk_object_set_data (GTK_OBJECT (find), "entry", (gpointer) entry);

    gtk_signal_connect (GTK_OBJECT (find), "clicked",
		        GTK_SIGNAL_FUNC (do_find_string), NULL);

    gtk_widget_show (search_replace_vbox);
    gtk_widget_show (hbox1);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_widget_show (find);

	GTK_WIDGET_SET_FLAGS(find, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(find);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
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
  GtkWidget *entry1, *entry2, *replace, *find, *label1, *label2;

  if (search_replace_open == 0)
    {
	search_replace_vbox = gtk_table_new(2,3,FALSE);

    gtk_container_set_border_width (GTK_CONTAINER (search_replace_vbox), gpe_get_border() / 3);
    gtk_table_set_row_spacings (GTK_TABLE (search_replace_vbox), 0);
    gtk_table_set_col_spacings (GTK_TABLE (search_replace_vbox), gpe_get_boxspacing());
		
    label1 = gtk_label_new (_("Search for:"));
	gtk_misc_set_alignment(GTK_MISC(label1),0.0,0.5);
    label2 = gtk_label_new (_("Replace with:"));
	gtk_misc_set_alignment(GTK_MISC(label2),0.0,0.5);
    entry1 = gtk_entry_new ();
    entry2 = gtk_entry_new ();

    replace = gpe_picture_button (search_replace_vbox->style, _("Replace"), "search_and_replace");
    find = gpe_picture_button (search_replace_vbox->style, _("Find"), "search");

    gtk_box_pack_end (GTK_BOX (parent_vbox), search_replace_vbox, FALSE, FALSE, 0);
		
 	gtk_table_attach(GTK_TABLE(search_replace_vbox),label1,0,1,0,1,GTK_FILL,0,0,0);
 	gtk_table_attach(GTK_TABLE(search_replace_vbox),label2,0,1,1,2,GTK_FILL,0,0,0);
 	gtk_table_attach(GTK_TABLE(search_replace_vbox),entry1,1,2,0,1,GTK_FILL|GTK_EXPAND,0,0,0);
 	gtk_table_attach(GTK_TABLE(search_replace_vbox),entry2,1,2,1,2,GTK_FILL|GTK_EXPAND,0,0,0);
 	gtk_table_attach(GTK_TABLE(search_replace_vbox),find,2,3,0,1,GTK_FILL,0,0,0);
 	gtk_table_attach(GTK_TABLE(search_replace_vbox),replace,2,3,1,2,GTK_FILL,0,0,0);
	
    gtk_object_set_data (GTK_OBJECT (replace), "entry", (gpointer) entry2);
    gtk_object_set_data (GTK_OBJECT (find), "entry", (gpointer) entry1);

    gtk_signal_connect (GTK_OBJECT (find), "clicked",
		        GTK_SIGNAL_FUNC (do_find_string), NULL);

    g_signal_connect (G_OBJECT (replace), "clicked",
    		        G_CALLBACK (do_replace_string), NULL);

    gtk_widget_show (search_replace_vbox);
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

gboolean
main_window_key_pressed(GtkWidget *widget, 
                         GdkEventKey *event, 
                         GtkWidget *main_vbox)
{
  if (event->state & GDK_CONTROL_MASK)
    switch (event->keyval)
      {
        case GDK_n:
          new_file(widget, NULL);
        break;	
        case GDK_o:
          select_open_file();
        break;	
        case GDK_s:
          save_file();
        break;	
        case GDK_f:
			search_string(widget, main_vbox);
        break;	
        case GDK_r:
			replace_string(widget, main_vbox);
        break;	
        case GDK_q:
          ask_save_before_exit();
        break;	
      }
  return FALSE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *toolbar, *scroll;
  GtkTooltips *tooltips;
  GtkToolItem *item;
  GtkTextBuffer *buf;
  gint window_x, window_y;

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
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  window_x = gdk_screen_width() / 2;
  window_y = gdk_screen_height() / 2;  
  if (window_x < 240) window_x = 240;
  if (window_y < 310) window_y = 310;
    

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (main_window), window_x, window_y);
	
  update_window_title ();
  gtk_widget_set_usize (GTK_WIDGET (main_window), window_x, window_y);
  g_signal_connect (GTK_OBJECT (main_window), "delete-event",
		      GTK_SIGNAL_FUNC (ask_save_before_exit), NULL);

  gtk_widget_realize (main_window);

  tooltips = gtk_tooltips_new();
  gtk_object_set_data(GTK_OBJECT(main_window),"tooltips",tooltips);
	
  vbox = gtk_vbox_new (FALSE, 0);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  text_area = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_area), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_area), TRUE);
  gtk_tooltips_set_tip(tooltips,text_area,_("File editing field, additional functions are available by tap&hold or right click."),NULL);

  buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_area));
  g_signal_connect (G_OBJECT (buf), "changed",
		      GTK_SIGNAL_FUNC (text_changed), NULL);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), _("New document"), NULL);
  g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (new_file), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_OPEN);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), _("Open file"), NULL);
  g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (select_open_file), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), _("Save current file"), NULL);
  g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (save_file), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_SAVE_AS);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), _("Save current file as"), NULL);
  g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (select_save_file_as), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  if (window_x > 260)
    {
      item = gtk_separator_tool_item_new ();
      gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
    }
  item = gtk_tool_button_new_from_stock (GTK_STOCK_FIND);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), _("Search text for a string"), NULL);
  g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (search_string), vbox);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);


  item = gtk_tool_button_new_from_stock (GTK_STOCK_FIND_AND_REPLACE);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), _("Replace a string"), NULL);
  g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (replace_string), vbox);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);


  if(gpe_check_for_help(appname) == NULL )
    {
      item = gtk_separator_tool_item_new ();
      gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
      gtk_tool_item_set_expand (item, TRUE);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
    }

  item = gtk_tool_button_new_from_stock (GTK_STOCK_COPY);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), 
                        _("Copy selected, paste is possible by tap&hold"), NULL);
  g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (copy_selection), vbox);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
    
    
  /* add another icon if we screen is large */
  if (window_x > 260)
    {
      item = gtk_tool_button_new_from_stock (GTK_STOCK_PASTE);
      gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), 
                            _("Paste from clipboard"), NULL);
      g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (paste_clipboard), vbox);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	}			   

  if(gpe_check_for_help(appname) != NULL)
	{
      item = gtk_tool_button_new_from_stock (GTK_STOCK_HELP);
      gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), _("Get help"), NULL);
      g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (show_help), NULL);
      gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	}
  

  item = gtk_tool_button_new_from_stock (GTK_STOCK_QUIT);
  gtk_tooltips_set_tip (tooltips, GTK_WIDGET (item), 
                        _("Exit gpe-edit"), NULL);
  g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (ask_save_before_exit), vbox);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (text_area));
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

  g_signal_connect_after(G_OBJECT(main_window), "key-press-event", G_CALLBACK(main_window_key_pressed), vbox);
  gtk_widget_add_events(main_window, GDK_KEY_PRESS_MASK);

  gpe_set_window_icon (main_window, "icon");

  gtk_widget_show_all (main_window);

  if (argc > 1)
  {
    open_file (argv[1]);
  }

  gtk_widget_grab_focus (text_area);

  gtk_main();
  return 0;
}
