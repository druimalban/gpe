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

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "mime-sql.h"

#define _(x) dgettext(PACKAGE, x)

#define COMPLETED 0
#define LAST_SIGNAL 1
#define SPINNER_TIMEOUT 3

GtkWidget *window;
GtkWidget *combo;
GtkWidget *spinner;
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

int current_spinner_num = 0;
int num_spinner_files = 30;
gchar *spinner_file[] = {"001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "015", "016", "017", "018", "019", "020", "021", "022", "023", "024", "025", "026", "027", "028", "029", "030"};
GdkPixbuf *spinner_pixbuf[30], *spinner_rest_pixbuf;
GtkStyle *spinner_style;

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

guint window_x = 240, window_y = 310;

static void browse_directory (gchar *directory);
static void update_spinner (void);
static void set_spinner_rest (void);

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
    sleep (0.2);
  }
}

/* Limit the title to two lines at <68px wide
   Eeeevil and Uuuugly but it works just fine AFAIK */
void make_nice_title (GtkWidget *label, GdkFont *font, char *full_title)
{
	int i;
	char *title;
	char *last_break=NULL;

	g_assert (label != NULL);
	g_assert (font != NULL);
	g_assert (full_title != NULL);

	title = g_strdup (full_title);
	for (i=0;i<strlen(title);i++)
	{
		if (gdk_text_width (font, title, i) >= 68)
		{
			if (last_break)
			{
				char *t;
				int k;
				int done=0;
				*last_break = '\n';
				t = last_break+1;
				for (k=0;k<strlen(t) && !done;k++)
				{
					if (gdk_text_width (font, t, k) >= 68)
					{
						/* Cut the title short & make it end in "..." */
						int j;
						if (k > 0)
							*(t + --k) = 0;
						for (j=0;j<3;j++)
							*(t + --k) = '.';
						done = 1;
					}
				}
				break;
			} else
			{
				/* Cut the title short & make it end in "..." */
				int j;
				if (i > 0)
					*(title + --i) = 0;
				for (j=0;j<3;j++)
					*(title + --i) = '.';
				break;
			}
		}

		/* Note if this character can be changed to a newline safely.
		 * This is done now not at the top because bad wrapping can
		 * occur if it is the "breakable character" pushes the string
		 * past the 70 pixel width. */
		if (title[i] == ' ')
			last_break = title + i;
	}

	gtk_label_set_text (GTK_LABEL (label), title);
	g_free (title);
}

GtkWidget *create_icon_pixmap (GtkStyle *style, char *fn, int size)
{
  GdkPixbuf *pixbuf, *spixbuf;
  GtkWidget *w;
  pixbuf = gdk_pixbuf_new_from_file (fn);
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
run_program (char *exec, char *program)
{
  char *command;
  //char *cmd[] = {"/bin/sh", "-c", ""};

  command = g_strdup_printf ("%s %s &", program, exec);
  //printf ("%s %s\n", program, exec);
  //cmd[3] = g_strdup_printf ("%s", command);
  //printf ("%s", cmd[3]);
  //gnome_execute_async (NULL, 3, cmd);
  system (command);
}

void static
open_with (GtkButton *button, gpointer data)
{
  char *command, *program, *exec;

  program = gtk_entry_get_text (GTK_ENTRY (data));
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

  int i = 5;
  int row_num = 0;
  gchar *programs[] = {"GPE Edit", "gpe-edit", "Dillo", "dillo", "Quick Image Viewer", "qiv"};
  gchar *row_text[2];
  gchar *pixmap_file;

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

  open_button = gpe_picture_button (window->style, "Open with", "open");
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

  for ( ; i > 0; i = i - 2)
  {
    row_text[0] = 0;
    row_text[1] = programs[i-1];
    gtk_clist_append (GTK_CLIST (clist), row_text);
    gtk_clist_set_row_data (GTK_CLIST (clist), row_num, (gpointer) programs[i]);

    pixmap_file = g_strdup_printf ("%s/share/pixmaps/%s.png", PREFIX, programs[i]);
    pixbuf = gdk_pixbuf_new_from_file (pixmap_file);
    if (pixbuf != NULL)
    {
      spixbuf = gdk_pixbuf_scale_simple (pixbuf, 12, 12, GDK_INTERP_BILINEAR);
      gpe_render_pixmap (NULL, spixbuf, &pixmap, &bitmap);
      gtk_clist_set_pixmap (GTK_CLIST (clist), row_num, 0, pixmap, bitmap);
    }

    row_num++;
  }

  gtk_widget_show_all (window);	
}

static gint
unignore_press (gpointer data)
{
  ignore_press = 0;
  return FALSE;
}

/* Callback for selecting a program to run */
static gint
button_released (GtkObject *button, gpointer data)
{
  struct mime_type *mime;
  gchar *path, *type;

  gtk_widget_set_state (GTK_WIDGET(button), GTK_STATE_NORMAL);

  /* Clear the variables stating what button is down etc. 
   * and close if we're not on the original button */
  if (GTK_WIDGET (button) != current_button || !current_button_is_down)
  {
    current_button = NULL;
    current_button_is_down = 0;
    return TRUE;
  }

  if (!GTK_IS_EVENT_BOX (button)) /* Could be the notebook! */
    return TRUE;

  if (ignore_press)
    return TRUE;

  /* So we ignore a second press within 1000msec */
  ignore_press = 1;
  gtk_timeout_add (500, unignore_press, NULL);

  mime = (struct mime_type *) gtk_object_get_data (GTK_OBJECT (button), "mime");
  path = (gchar *) gtk_object_get_data (GTK_OBJECT (button), "path");
  type = (gchar *) gtk_object_get_data (GTK_OBJECT (button), "type");

  if (strcmp (type, "file") == 0)
  {
    if (mime)
    {
      run_program (path, (gpointer) mime->program);
    }
    else
    {
      ask_open_with (path);
    }
  }
  else if (strcmp (type, "directory") == 0)
    browse_directory (path);

  return TRUE;	
}

static gint
button_pressed (GtkWidget *button, GdkEventButton *event, gpointer data)
{
  /* We only want left mouse button events */
  if (event && (!(event->button == 1)))
    return TRUE;

  if (ignore_press) /* Ignore double clicks! */ 
  {
    return TRUE;
  }

  current_button = button;
  current_button_is_down = 1;

  gtk_widget_set_state (button, GTK_STATE_SELECTED);

  return TRUE;	
}

static gint
button_enter (GtkWidget *button, GdkEventCrossing *event)
{
  /* We only want left mouse button events */
  if (!(event->state & 256))
  {
    current_button = NULL;
    return TRUE;
  }

  /* If we're moving onto the button that was last selected,
     do the same as if we've just started pressing on it again */
  if (button == current_button)
    button_pressed (button, NULL, NULL);

  return TRUE;
}

static gint
button_leave (GtkWidget *button, GdkEventCrossing *event)
{
  gtk_widget_set_state (button, GTK_STATE_NORMAL);
  current_button_is_down = 0;
  return TRUE;
}

/* Make the contents for the current directory. */
static void
make_view (gchar *view)
{
  struct dirent *d;
  struct mime_type *file_mime = NULL;
  DIR *dir;
  int i = 0;
  int cols;
  int loop_num = 0;
  gchar *fp, *buf, *extension, *file_type;
  gchar *previous_extension = "";
  GdkFont *font; /* Font in the label */
  GdkPixbuf *pixbuf = NULL, *spixbuf;
  GtkWidget *pixmap;
  GtkWidget *table, *button, *label, *box;

  loading_directory = 1;

  if (view_widget)
    gtk_widget_destroy (view_widget);

  /* Get the number of columns assuming 70px per item, 20px scrollbar */
  /* This should really find out how much space it *can* use */
  if (window->allocation.width - 20 > 70 && strcmp (view, "icons") == 0)
    cols = (window->allocation.width - 20) / 70;
  else
    cols = 1;

  table = gtk_table_new (5, cols, TRUE);
  view_widget = table;
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (view_scrolld), view_widget);
  gtk_widget_show (view_widget);

  dir = opendir (current_directory);
  if (dir)
  {
    while (d = readdir (dir), d != NULL)
    {
      if (loading_directory == 0)
        break;

      if (d->d_name[0] != '.')
      {
        struct stat s;
        fp = g_strdup (d->d_name);

          printf ("Filename: %s\n", fp);

        if (strcmp (current_directory, "/") == 0)
          buf = g_strdup_printf ("/%s", fp);
        else
          buf = g_strdup_printf ("%s/%s", current_directory, fp);

        if (stat (buf, &s) == 0)
        {
          if (loop_num > SPINNER_TIMEOUT)
          {
            loop_num = 0;
            update_spinner ();

            while (gtk_events_pending ())
              gtk_main_iteration ();
          }
          loop_num++;

          if (S_ISDIR (s.st_mode))
          {
            file_type = g_strdup ("directory");
            pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/directory.png");
            previous_extension = "";
          }
          else if (__S_ISTYPE ((s.st_mode), __S_IEXEC))
          {
            file_type = g_strdup ("executable");
            pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/executable.png");
            previous_extension = "";
          }
          else if (strcmp (fp, "COPYING") == 0)
          {
            pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/gnome-text-x-copying.png");
            file_type = g_strdup ("copying");
          }
          else if (strcmp (fp, "CREDITS") == 0)
          {
            pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/gnome-text-x-credits.png");
            file_type = g_strdup ("credits");
          }
          else
          {
            file_type = g_strdup ("file");
            extension = get_file_extension (fp);

            if (mime_types)
            {
              GSList *iter;

              for (iter = mime_types; iter; iter = iter->next)
              {
                struct mime_type *c = iter->data;

                if (loading_directory == 0)
                  break;

                if (strcmp (c->extension, extension) == 0)
                {
                  file_mime = c;
                  break;
                }
                else
                {
                  file_mime = NULL;
                }
              }
              if (file_mime == NULL)
              {
                pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/regular.png");
                previous_extension = "";
              }
              else if (strcmp (previous_extension, file_mime->extension))
              {
                  pixbuf = gdk_pixbuf_new_from_file (g_strdup_printf ("%s/share/gpe/pixmaps/default/gpe-filemanager/document-icons/gnome-%s.png", PREFIX, file_mime->icon));
                  previous_extension = g_strdup (file_mime->extension);
              }
            }
            else
            {
              pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/regular.png");
              previous_extension = "";
            }
          }

          if (loading_directory == 0)
            break;

          /* Button label */
          label = gtk_label_new ("");

          /* Get the font used by the label */
          if (gtk_rc_get_style (label))
            font = gtk_rc_get_style (label)->font;
          else
            font = gtk_widget_get_style (label)->font;

          if (strcmp (view, "icons") == 0)
          {
            make_nice_title (label, font, fp);
          }
          else
          {
            gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
            gtk_label_set_text (GTK_LABEL (label), fp);
            gtk_widget_set_usize (label, 0, 16);
          }

          /* Button VBox */
          if (strcmp (view, "icons") == 0)
          {
            box = gtk_vbox_new (0,0);
          }
          else
          {
            box = gtk_hbox_new (0,0);
            gtk_box_pack_end_defaults (GTK_BOX (box), gtk_label_new (""));
          }


          if (strcmp (view, "icons") == 0)
            spixbuf = gdk_pixbuf_scale_simple (pixbuf, 32, 32, GDK_INTERP_BILINEAR);
          else
            spixbuf = gdk_pixbuf_scale_simple (pixbuf, 16, 16, GDK_INTERP_BILINEAR);

          pixmap = gpe_render_icon (view_scrolld->style, spixbuf);
          gdk_pixbuf_unref (spixbuf);

          gtk_box_pack_start (GTK_BOX (box), pixmap, FALSE, FALSE, 0);
          gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

          /* The 'button' itself */
          button = gtk_event_box_new ();
          /* The "Ay" is arbitrary; I figure'd it'd give a decent approximation at the
             max. height of the font */
          //gtk_widget_set_usize (button, 70, 55 + 2*gdk_text_height(font, "Ay", 2));

          gtk_object_set_data (GTK_OBJECT (button), "mime", (gpointer) file_mime);
          gtk_object_set_data (GTK_OBJECT (button), "path", (gpointer) buf);
          gtk_object_set_data (GTK_OBJECT (button), "type", (gpointer) file_type);

          gtk_signal_connect( GTK_OBJECT (button), "button_release_event", GTK_SIGNAL_FUNC (button_released), NULL);
          gtk_signal_connect( GTK_OBJECT (button), "button_press_event", GTK_SIGNAL_FUNC (button_pressed), NULL);
          gtk_signal_connect( GTK_OBJECT (button), "enter_notify_event", GTK_SIGNAL_FUNC (button_enter), NULL);
          gtk_signal_connect( GTK_OBJECT (button), "leave_notify_event", GTK_SIGNAL_FUNC (button_leave), NULL);

          gtk_container_add (GTK_CONTAINER (button), box);

          gtk_table_attach_defaults (GTK_TABLE (table), button, i%cols,i%cols+1,i/cols,i/cols+1);

          gtk_widget_show (box);
          gtk_widget_show (label);
          gtk_widget_show (pixmap);
          gtk_widget_show (button);

          i++;
        }
      }
    }
    closedir (dir);
    set_spinner_rest ();
  }
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

  new_directory = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (combo)->entry));
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
  struct stat s;

  if (stat (directory, &s) == 0)
  {
    if (S_ISDIR (s.st_mode))
    {
      current_directory = g_strdup (directory);
      printf ("Current directory: %s\n", current_directory);
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

  new_directory = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (combo)->entry));

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

static void
update_spinner (void)
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;

  if (current_spinner_num == num_spinner_files)
    current_spinner_num = 0;

  if (spinner_pixbuf[current_spinner_num] != NULL)
  {
    gpe_render_pixmap (spinner_style ? &spinner_style->bg[GTK_STATE_NORMAL] : NULL,  spinner_pixbuf[current_spinner_num], &pixmap, &bitmap);
  
    gtk_pixmap_set (GTK_PIXMAP (spinner), pixmap, bitmap);
  }
  current_spinner_num++;
}

static void
set_spinner_rest (void)
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;

  gpe_render_pixmap (spinner_style ? &spinner_style->bg[GTK_STATE_NORMAL] : NULL,  spinner_rest_pixbuf, &pixmap, &bitmap);
  gtk_pixmap_set (GTK_PIXMAP (spinner), pixmap, bitmap);

  current_spinner_num = 0;
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *hbox2, *toolbar, *toolbar2, *view_option_menu, *view_menu, *view_menu_item;
  GdkPixbuf *p;
  GtkWidget *pw;
  GdkPixmap *pmap;
  GdkBitmap *bmap;
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *rest_pixbuf = NULL;
  GdkPixmap *rest_pixmap;
  GdkBitmap *rest_bitmap;
  int i = 0;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GPE Filemanager");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  hbox2 = gtk_hbox_new (FALSE, 0);

  combo = gtk_combo_new ();
  //combo_signal_id = gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed", GTK_SIGNAL_FUNC (goto_directory), combo);

  view_scrolld = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view_scrolld), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

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

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  toolbar2 = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_SPACE_LINE);

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

  spinner_style = hbox2->style;

  rest_pixbuf = gdk_pixbuf_new_from_file (g_strdup_printf ("%s/share/gpe/pixmaps/default/gpe-filemanager/spinner/rest.png", PREFIX));
  spinner_rest_pixbuf =  gdk_pixbuf_scale_simple (rest_pixbuf, 16, 16, GDK_INTERP_BILINEAR);
  gpe_render_pixmap (spinner_style ? &spinner_style->bg[GTK_STATE_NORMAL] : NULL,  spinner_rest_pixbuf, &rest_pixmap, &rest_bitmap);
  spinner = gtk_pixmap_new (rest_pixmap, rest_bitmap);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), view_option_menu, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), toolbar2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), view_scrolld, TRUE, TRUE, 0);

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
  gtk_widget_show (view_scrolld);
  gtk_widget_show (spinner);

  if (sql_start ())
    exit (1);

  gtk_option_menu_set_history (GTK_OPTION_MENU (view_option_menu), 0);

  /*
  if (mime_types)
  {
    GSList *iter;

    printf ("|\textension\t\t|\tmime_name\t|\tprogram\t|\n\n");

    for (iter = mime_types; iter; iter = iter->next)
    {
      struct mime_type *c = iter->data;

      printf ("|\t%s\t\t|\t%s\t|\t%s\t|\n", c->mime_name, c->extension, c->program);
    }
  */

  while (i <= num_spinner_files)
  {
    pixbuf = gdk_pixbuf_new_from_file (g_strdup_printf ("%s/share/gpe/pixmaps/default/gpe-filemanager/spinner/%s.png", PREFIX, spinner_file[i]));
    spinner_pixbuf[i] =  gdk_pixbuf_scale_simple (pixbuf, 16, 16, GDK_INTERP_BILINEAR);

    i++;
  }

  set_directory_home (NULL);
  //history_place--;

  gtk_main();

  return 0;
}