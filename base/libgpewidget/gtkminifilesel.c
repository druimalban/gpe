/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
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

#include "render.h"
#include "pixmaps.h"
#include "gtkminifilesel.h"
#include "picturebutton.h"
#include "errorbox.h"
#include "question.h"

#define _(x) dgettext(PACKAGE, x)

#define COMPLETED 0
#define LAST_SIGNAL 1

static GtkWindowClass *parent_class;
static guint signals[LAST_SIGNAL] = { 0 };

static int menu_timeout_id;

struct file_info
{
  gchar *filename;
  gchar *directory;
  guint clist_row;
  GtkWidget *clist;
  GtkMiniFileSelection *fs;
};

static void
kill_window (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(data);
}

static void set_directory (GtkMiniFileSelection *fs, char *directory);

static void
create_new_directory (GtkWidget *widget, gpointer data)
{
  struct file_info *fi = (struct file_info *)data;
  gchar *dir_name;
  gchar *full_dir_name;

  dir_name = gtk_entry_get_text (GTK_ENTRY (gtk_object_get_data (GTK_OBJECT (widget),"entry")));
  full_dir_name = g_strdup_printf ("%s/%s", fi->directory, dir_name);
  if (mkdir (full_dir_name, 0777) != 0) 
    gpe_perror_box (dir_name);
  g_free (full_dir_name);
  gtk_widget_destroy (GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (widget), "window")));
  set_directory (GTK_MINI_FILE_SELECTION (fi->fs), fi->directory);
}

static void
rename_file (GtkWidget *widget, gpointer data)
{
  struct file_info *fi = (struct file_info *)data;
  gchar *old_name;
  gchar *new_name;
  gchar *new_filename;
  gchar *error;

  new_filename = gtk_entry_get_text (GTK_ENTRY (gtk_object_get_data (GTK_OBJECT (widget),"entry")));
  old_name = g_strdup_printf ("%s/%s", fi->directory, fi->filename);
  new_name = g_strdup_printf ("%s/%s", fi->directory, new_filename);

  if (rename (old_name, new_name) != 0)
    {
      error = g_strdup_printf ("Couldn't rename %s to %s: ", fi->filename, new_filename);
      gpe_perror_box (error);
      g_free (error);
    }

  gtk_widget_destroy (GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (widget), "window")));
  set_directory (GTK_MINI_FILE_SELECTION (fi->fs), fi->directory);

  g_free (old_name);
  g_free (new_name);
  g_free (new_filename);
}

static void
delete_file (GtkWidget *widget, gpointer data)
{
  struct file_info *fi;
  gchar *rm_command;
  fi = (struct file_info *)data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_object_get_data(GTK_OBJECT(widget),"checkbutton"))) == 1)
  {
    if (gpe_question_ask_yn ("Delete directory recursively?") == 1)
    {
      rm_command = g_strdup_printf ("rm -rf %s/%s", fi->directory, fi->filename);
      system (rm_command);
      g_free (rm_command);
    }
  }
  else
  {
    rm_command = g_strdup_printf ("rm -f %s/%s", fi->directory, fi->filename);
    system (rm_command);
    g_free (rm_command);
  }

  gtk_widget_destroy (GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(widget), "window")));
  set_directory (GTK_MINI_FILE_SELECTION(fi->fs), fi->directory);
}

static void
ask_rename_file (GtkWidget *widget, gpointer data)
{
  GtkWidget *window, *label, *entry;
  GtkWidget *buttoncancel, *buttonrename;

  window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(window), _("Rename"));
  gtk_widget_realize (window);
 
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_false), NULL);

  label = gtk_label_new (_("New name:"));
  entry = gtk_entry_new ();

  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  gtk_widget_realize (window);

  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                      GTK_SIGNAL_FUNC (kill_window), window);

  buttonrename = gpe_picture_button (window->style, _("Rename"), "ok");
  gtk_signal_connect (GTK_OBJECT (buttonrename), "clicked",
                      GTK_SIGNAL_FUNC (rename_file), data);
  gtk_object_set_data (GTK_OBJECT(buttonrename), "entry", entry);
  gtk_object_set_data (GTK_OBJECT(buttonrename), "window", window);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                      buttoncancel);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                     buttonrename);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), label);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), entry);

  gtk_widget_show_all (window);
  gtk_widget_grab_focus (entry);
}

static void
ask_delete_file (GtkWidget *widget, gpointer data)
{
  struct file_info *fi;
  gchar *label_text;
  GtkWidget *window, *label, *checkbutton;
  GtkWidget *buttoncancel, *buttondelete;
  fi = (struct file_info *)data;

  window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (window), _("Delete"));
  gtk_widget_realize (window);
 
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_false), NULL);

  label_text = g_strdup_printf (_("Do you really want to delete %s"), fi->filename);
  label = gtk_label_new (label_text);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  checkbutton = gtk_check_button_new_with_label ("Delete file recursively?");

  gtk_widget_realize (window);

  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                      GTK_SIGNAL_FUNC (kill_window), window);

  buttondelete = gpe_picture_button (window->style, _("Delete"), "ok");
  gtk_signal_connect (GTK_OBJECT (buttondelete), "clicked",
                      GTK_SIGNAL_FUNC (delete_file), data);
  gtk_object_set_data (GTK_OBJECT (buttondelete), "window", window);
  gtk_object_set_data (GTK_OBJECT (buttondelete), "checkbutton", checkbutton);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                      buttoncancel);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                     buttondelete);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), label);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), checkbutton);

  gtk_widget_show_all (window);
}

static void
ask_new_directory (GtkWidget *widget, gpointer data)
{
  GtkWidget *window, *label, *entry;
  GtkWidget *buttoncancel, *buttoncreate;

  window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(window), _("Create directory"));
  gtk_widget_realize (window);
 
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_false), NULL);

  label = gtk_label_new ("Directory name:");
  entry = gtk_entry_new ();

  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  gtk_widget_realize (window);

  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                      GTK_SIGNAL_FUNC (kill_window), window);

  buttoncreate = gpe_picture_button (window->style, _("Create"), "ok");
  gtk_signal_connect (GTK_OBJECT (buttoncreate), "clicked",
                      GTK_SIGNAL_FUNC (create_new_directory), data);
  gtk_object_set_data (GTK_OBJECT(buttoncreate), "entry", entry);
  gtk_object_set_data (GTK_OBJECT(buttoncreate), "window", window);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                      buttoncancel);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->action_area),
                     buttoncreate);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), label);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), entry);

  gtk_widget_show_all (window);
}

static int
popup_menu_activate (gpointer data)
{
  GtkWidget *menu, *mi;

  menu = gtk_menu_new ();

  mi = gtk_menu_item_new_with_label (_("New directory"));
  gtk_menu_append (GTK_MENU (menu), mi);
  gtk_signal_connect (GTK_OBJECT (mi), "activate",
		      GTK_SIGNAL_FUNC (ask_new_directory), data);

  mi = gtk_menu_item_new_with_label (_("Rename"));
  gtk_menu_append (GTK_MENU (menu), mi);
  gtk_signal_connect (GTK_OBJECT (mi), "activate",
		      GTK_SIGNAL_FUNC (ask_rename_file), data);

  mi = gtk_menu_item_new_with_label (_("Delete"));
  gtk_menu_append (GTK_MENU (menu), mi);
  gtk_signal_connect (GTK_OBJECT (mi), "activate",
		      GTK_SIGNAL_FUNC (ask_delete_file), data);

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, 0);

  gtk_widget_show_all (menu);

  return FALSE;
}

static void
popup_menu_cancel (void)
{
  if (menu_timeout_id != 0)
    {
      gtk_timeout_remove (menu_timeout_id);
      menu_timeout_id = 0;
    }
}

static void
popup_menu_later (int delay, gpointer data)
{
  menu_timeout_id = gtk_timeout_add (delay, popup_menu_activate, data);
}

static int
button_pressed (GtkButton *button, GdkEventButton *event, gpointer data)
{
  GtkMiniFileSelection *fs = GTK_MINI_FILE_SELECTION (data);

  // Get some infomation about the file
  struct file_info *fi;
  fi = (struct file_info *) malloc (sizeof(struct file_info));
  gtk_clist_get_text (GTK_CLIST (fs->clist), fs->selected_row, 1, &(fi->filename));
  fi->directory = g_strdup (fs->directory);
  fi->clist = fs->clist;
  fi->clist_row = fs->selected_row;
  fi->fs = fs;

  popup_menu_cancel ();
  popup_menu_later (700, (gpointer) fi);

  return 0;
}

static int
button_released (GtkButton *button, GdkEventButton *event, gpointer data)
{
  popup_menu_cancel ();
  return 0;
}

static void
gtk_mini_file_selection_size_allocate (GtkWidget     *widget,
				       GtkAllocation *allocation)
{
  GtkMiniFileSelection *fs;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MINI_FILE_SELECTION (widget));
  g_return_if_fail (allocation != NULL);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
  
  fs = GTK_MINI_FILE_SELECTION (widget);
}

static void
gtk_mini_file_selection_size_request (GtkWidget     *widget,
				      GtkRequisition *requisition)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MINI_FILE_SELECTION (widget));
  g_return_if_fail (requisition != NULL);

  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);

  requisition->height = 240;  
}

static void
gtk_mini_file_selection_show (GtkWidget *widget)
{
  GtkMiniFileSelection *fs;
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MINI_FILE_SELECTION (widget));

  GTK_WIDGET_CLASS (parent_class)->show (widget);

  fs = GTK_MINI_FILE_SELECTION (widget);
  
  gtk_widget_grab_focus (fs->entry);
}

static void
menu_change_directory (GtkWidget *w, gpointer p)
{
  char *s = strdup (gtk_object_get_data (GTK_OBJECT (w), "text"));
  
  set_directory (p, s);
}

static void
free_string (GtkObject *o, gpointer p)
{
  g_free (p);
}

static void
add_directory (GtkWidget *menu, char *name, GtkMiniFileSelection *fs)
{
  GtkWidget *item = gtk_menu_item_new_with_label (name);
  gchar *p;
  gtk_widget_show (item);
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
		      menu_change_directory, fs);
  p = g_strdup (name);
  gtk_object_set_data (GTK_OBJECT (item), "text", p);
  gtk_signal_connect (GTK_OBJECT (item), "destroy",
		      free_string, p);
}

static void
set_directory_menu (GtkWidget *option, char *dir, GtkMiniFileSelection *fs)
{
  GtkWidget *menu = gtk_menu_new ();
  char *buf = alloca (strlen (dir) + 1);
  char *p, *n;
  guint nr = 1;

  strcpy (buf, dir);

  add_directory (menu, "/", fs);

  n = buf;
  if (*n == '/')
    n++;
  for (; p = strchr (n, '/'), p; n = p + 1)
    {
      *p = 0;
      add_directory (menu, buf, fs);
      *p = '/';
      nr++;
    }
 
  add_directory (menu, buf, fs);

  gtk_widget_show (menu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option), nr);
}

static void
set_members (GtkMiniFileSelection *fs)
{
  struct dirent *d;
  DIR *dir;
  GSList *files = NULL, *subdirs = NULL, *iter;
  gchar *buf = g_malloc (strlen (fs->directory) + 256 + 2);
  gchar *fp;
  gchar *text[2];
  GdkPixmap *dirup_pix;
  GdkBitmap *dirup_mask;
  GdkPixmap *folder_pix;
  GdkBitmap *folder_mask;

  gpe_find_icon_pixmap ("dir-up", &dirup_pix, &dirup_mask);
  gpe_find_icon_pixmap ("dir-closed", &folder_pix, &folder_mask);
 
  gtk_clist_clear (GTK_CLIST (fs->clist));

  sprintf (buf, "%s/", fs->directory);
  fp = buf + strlen (fs->directory) + 1;

  dir = opendir (fs->directory);
  if (dir)
    {
      while (d = readdir (dir), d != NULL)
	{
	  if (d->d_name[0] != '.')
	    {
	      struct stat s;
	      strcpy (fp, d->d_name);
	      if (stat (buf, &s) == 0)
		{
		  if (S_ISDIR (s.st_mode))
		    {
		      gchar *fn = g_malloc (strlen (d->d_name) + 3);
		      if (!folder_pix)
			{
		          sprintf (fn, "[%s]", d->d_name);
			}
		      else
			{
		          sprintf (fn, "%s", d->d_name);
			}
		      subdirs = g_slist_append (subdirs, fn);
		    }
		  else
		    {
		      gchar *fn = g_strdup (d->d_name);
		      files = g_slist_append (files, fn);
		    }
		}
	    }
	}
      closedir (dir);
    }

  subdirs = g_slist_sort (subdirs, (GCompareFunc)strcoll);
  files = g_slist_sort (files, (GCompareFunc)strcoll);

  if (strcmp (fs->directory, "/") != 0)
    {
      guint row;
      text[0] = NULL;
      text[1] = dirup_pix ? NULL : "../";
      row = gtk_clist_append (GTK_CLIST (fs->clist), text);
      if (dirup_pix)
      {
        gtk_clist_set_pixmap (GTK_CLIST (fs->clist), row, 0, dirup_pix, dirup_mask);
      }
      gtk_clist_set_row_data (GTK_CLIST (fs->clist), row, (gpointer)2);
    }

  for (iter = subdirs; iter; iter = iter->next)
    {
      guint row;
      text[1] = iter->data;
      text[0] = NULL;
      row = gtk_clist_append (GTK_CLIST (fs->clist), text);
      gtk_clist_set_pixmap (GTK_CLIST (fs->clist), row, 0, folder_pix, folder_mask);
      gtk_clist_set_row_data (GTK_CLIST (fs->clist), row, (gpointer)1);
      g_free (iter->data);
    }
  for (iter = files; iter; iter = iter->next)
    {
      guint row;
      text[1] = iter->data;
      text[0] = NULL;
      row = gtk_clist_append (GTK_CLIST (fs->clist), text);
      gtk_clist_set_row_data (GTK_CLIST (fs->clist), row, (gpointer)0);
      g_free (iter->data);
    }

  g_free (buf);
}

static void
set_directory (GtkMiniFileSelection *fs, char *directory)
{
  if (fs->directory)
    free (fs->directory);
  fs->directory = directory;

  set_directory_menu (fs->option, fs->directory, fs);
  set_members (fs);
}

static void 
selection_made(GtkWidget      *clist,
	       gint            row,
	       gint            column,
	       GdkEventButton *event,
	       GtkWidget      *widget)
{
  guint type = (guint)gtk_clist_get_row_data (GTK_CLIST (clist), row);
  gchar *text;
  GtkMiniFileSelection *fs = GTK_MINI_FILE_SELECTION (widget);
  fs->selected_row = row;
  gtk_clist_get_text (GTK_CLIST (clist), row, 1, &text);
 
  if (event->type == GDK_2BUTTON_PRESS)
    {
      switch (type)
	{
	case 1:
	  {
	    size_t l = strlen (text);
	    size_t dl = strlen (fs->directory);
	    gchar *s = malloc (l + dl + 2);
	    gchar *p = s;
	    if (strcmp (fs->directory, "/"))
	      {
		dl = strlen (fs->directory);
		p = stpcpy (s, fs->directory);
	      }
	    *p++ = '/';
	    strcpy (p, text);
	    set_directory (fs, s);
	    gtk_widget_grab_focus (fs->entry);
	    break;
	  }
	case 2:
	  {
	    char *p = alloca (strlen (fs->directory) + 1);
	    char *d;
	    strcpy (p, fs->directory);
	    d = dirname (p);
	    set_directory (fs, strdup (d));
	    gtk_widget_grab_focus (fs->entry);
	    break;
	  }
	case 0:
	  {
	    gtk_entry_set_text (GTK_ENTRY (fs->entry), text);
	    gtk_signal_emit (GTK_OBJECT (fs), signals[COMPLETED]);
	    break;
	  }
	}
    }
  else
    {
      if (type == 0)
	gtk_entry_set_text (GTK_ENTRY (fs->entry), text);

      gtk_widget_grab_focus (fs->entry);
    }
}

static void
emit_completed (GtkWidget *w, gpointer p)
{
  gtk_signal_emit (GTK_OBJECT (p), signals[COMPLETED]);
}

static void
enter_struck (GtkWidget *w, gpointer p)
{
  gchar *path = gtk_mini_file_selection_get_filename (GTK_MINI_FILE_SELECTION (p));
  struct stat s;
  
  if (stat (path, &s) == 0)
    {
      if (S_ISDIR (s.st_mode))
	{
	  char *dp = strdup (path);
	  set_directory (p, dp);
	  g_free (path);
	  return;
	}
    }

  gtk_signal_emit (GTK_OBJECT (p), signals[COMPLETED]);  
}

static void
gtk_mini_file_selection_init (GtkMiniFileSelection *fs)
{
  GtkWidget *vbox, *hbox, *scrolled;
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (fs), vbox);

  fs->option = gtk_option_menu_new ();
  gtk_widget_show (fs->option);
  gtk_box_pack_start (GTK_BOX (vbox), fs->option, FALSE, FALSE, 0);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  fs->clist = gtk_clist_new (2);
  gtk_clist_set_column_width (GTK_CLIST (fs->clist), 0, 12);
  gtk_widget_show (fs->clist);
  gtk_container_add (GTK_CONTAINER (scrolled), fs->clist);

  gtk_signal_connect (GTK_OBJECT (fs->clist), "select_row",
		      GTK_SIGNAL_FUNC (selection_made), fs);
  gtk_signal_connect( GTK_OBJECT(fs->clist), "button_press_event",
		      GTK_SIGNAL_FUNC (button_pressed), (gpointer) fs);
  gtk_signal_connect( GTK_OBJECT(fs->clist), "button_release_event",
		      GTK_SIGNAL_FUNC (button_released), (gpointer) fs);

  fs->entry = gtk_entry_new ();
  gtk_widget_show (fs->entry);
  gtk_box_pack_start (GTK_BOX (vbox), fs->entry, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  gtk_widget_realize (GTK_WIDGET (fs));
  fs->cancel_button = gpe_picture_button (GTK_WIDGET (fs)->style, _("Cancel"), "cancel");
  gtk_widget_show (fs->cancel_button);
  gtk_box_pack_start (GTK_BOX (hbox), fs->cancel_button, TRUE, TRUE, 0);

  fs->ok_button = gpe_picture_button (GTK_WIDGET (fs)->style, _("OK"), "ok");
  gtk_widget_show (fs->ok_button);
  gtk_box_pack_start (GTK_BOX (hbox), fs->ok_button, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (fs->ok_button), "clicked", 
		      emit_completed, fs);
  gtk_signal_connect (GTK_OBJECT (fs->entry), "activate", enter_struck, fs);

  fs->directory = NULL;
  set_directory (fs, get_current_dir_name ());
}

static void
gtk_mini_file_selection_destroy (GtkObject *fs)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (fs);
  popup_menu_cancel ();
}

static void
gtk_mini_file_selection_class_init (GtkMiniFileSelectionClass * klass)
{
  GtkObjectClass *oclass;
  GtkWidgetClass *widget_class;

  parent_class = gtk_type_class (gtk_window_get_type ());
  oclass = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  oclass->destroy = gtk_mini_file_selection_destroy;
  
  widget_class->size_request = gtk_mini_file_selection_size_request;
  widget_class->size_allocate = gtk_mini_file_selection_size_allocate;
  widget_class->show = gtk_mini_file_selection_show;

  klass->completed = NULL;

  signals[0] = gtk_signal_new ("completed",
			       GTK_RUN_FIRST | GTK_RUN_ACTION,
#if GTK_MAJOR_VERSION < 2
			       oclass->type,
#else
			       GTK_CLASS_TYPE (oclass),				  
#endif
			       GTK_SIGNAL_OFFSET (GtkMiniFileSelectionClass, completed),
			       gtk_marshal_NONE__NONE,
			       GTK_TYPE_NONE, 0);

#if GTK_MAJOR_VERSION < 2
  gtk_object_class_add_signals (oclass, signals, LAST_SIGNAL);
#endif
}

GtkType
gtk_mini_file_selection_get_type (void)
{
  static guint mini_file_selection_type;

  if (! mini_file_selection_type)
    {
      static const GtkTypeInfo mini_file_selection_info =
      {
	"GtkMiniFileSelection",
	sizeof (GtkMiniFileSelection),
	sizeof (GtkMiniFileSelectionClass),
	(GtkClassInitFunc) gtk_mini_file_selection_class_init,
	(GtkObjectInitFunc) gtk_mini_file_selection_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      mini_file_selection_type = gtk_type_unique (gtk_window_get_type (), 
						  &mini_file_selection_info);
    }
  return mini_file_selection_type;
}

GtkWidget *
gtk_mini_file_selection_new (const gchar *title)
{
  GtkWidget *w = GTK_WIDGET (gtk_type_new (gtk_mini_file_selection_get_type ()));
  gtk_window_set_title (GTK_WINDOW (w), title);
  return w;
}

gchar *
gtk_mini_file_selection_get_filename (GtkMiniFileSelection *fs)
{
  gchar *chars = gtk_editable_get_chars (GTK_EDITABLE (fs->entry), 0, -1);

  if (chars[0] != '/')
    {
      size_t dl = strlen (fs->directory);
      gchar *buf = g_malloc (strlen (chars) + dl + 2);
      if (strcmp (fs->directory, "/"))
	sprintf (buf, "%s/%s", fs->directory, chars);
      else
	sprintf (buf, "/%s", chars);
      g_free (chars);
      return buf;
    }

  return chars;
}

void
gtk_mini_file_selection_set_directory (GtkMiniFileSelection *fs, char *directory)
{
  set_directory (fs,strdup(directory));
}
