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

#define _(x) dgettext(PACKAGE, x)

#define COMPLETED 0
#define LAST_SIGNAL 1

GtkWidget *window;
GtkWidget *view_scrolld;
GtkWidget *view_widget;

GdkPixbuf *default_pixbuf;

guint screen_w, screen_h;

gchar *current_directory = "/";
gchar *current_view = "icons";

struct gpe_icon my_icons[] = {
  { "left", "left" },
  { "right", "right" },
  { "up", "up" },
  { "refresh", "refresh" },
  { "stop", "stop" },
  { "home", "home" },
  { "dir-up", "dir-up" },
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "question", "question" },
  { "error", "error" },
  { "icon", PREFIX "/share/pixmaps/gpe-filemanager.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

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

/* Make the contents for the current directory. */
static void
set_icons_view (void)
{
  struct dirent *d;
  DIR *dir;
  int i = 0;
  int cols;
  gchar *fp, *buf;
  GdkFont *font; /* Font in the label */
  char *icon;
  GdkPixbuf *pixbuf = NULL, *spixbuf;
  GtkWidget *pixmap;
  GtkWidget *table, *button, *label, *vbox;

  gtk_widget_destroy (view_widget);

  /* Get the number of columns assuming 70px per item, 20px scrollbar */
  /* This should really find out how much space it *can* use */
  if (window->allocation.width - 20 > 70)
    cols = (window->allocation.width - 20) / 70;
  else
    cols = 1;

  table = gtk_table_new (3, cols, TRUE);
  view_widget = table;
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (view_scrolld), table);
  gtk_widget_show (table);

  printf ("%s", "Setting icons view.\n");

  dir = opendir (current_directory);
  if (dir)
  {
    while (d = readdir (dir), d != NULL)
    {
      printf ("d->d_name: %s\n", d->d_name);
      if (d->d_name[0] != '.')
      {
        struct stat s;
        fp = g_strdup_printf ("%s", d->d_name);
        buf = g_strdup_printf ("%s/%s", current_directory, fp);
        printf ("Adding icon: %s\n", buf);

        if (stat (buf, &s) == 0)
        {

          if (S_ISDIR (s.st_mode))
          {
            pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/directory.png");
            printf ("%s", "Added directory.\n");
          }
          else
          {
            pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/regular.png");
            printf ("%s", "Added file.\n");
          }
          /* Button label */
          label = gtk_label_new (fp);

          /* Get the font used by the label */
          //if (gtk_rc_get_style (label))
          //  font = gtk_rc_get_style (label)->font;
          //else
          //  font = gtk_widget_get_style (label)->font;

          //make_nice_title (label, font, fp);

          /* Button VBox */
          vbox = gtk_vbox_new (0,0);
          gtk_box_pack_end_defaults (GTK_BOX (vbox), label);

          spixbuf = gdk_pixbuf_scale_simple (pixbuf, 32, 32, GDK_INTERP_BILINEAR);

          pixmap = gpe_render_icon (view_scrolld->style, spixbuf);
          gdk_pixbuf_unref (spixbuf);
          gtk_box_pack_end_defaults (GTK_BOX (vbox), pixmap);

          /* The 'button' itself */
          button = gtk_event_box_new ();
          /* The "Ay" is arbitrary; I figure'd it'd give a decent approximation at the
             max. height of the font */
          //gtk_widget_set_usize (button, 70, 55 + 2*gdk_text_height(font, "Ay", 2));

          gtk_container_add (GTK_CONTAINER (button), vbox);
          gtk_table_attach_defaults (GTK_TABLE (table), button, i%cols,i%cols+1,i/cols,i/cols+1);

          gtk_widget_show (vbox);
          gtk_widget_show (label);
          gtk_widget_show (pixmap);
          gtk_widget_show (button);

          i++;
        }
      }
    }
    closedir (dir);
  }
}

static void
set_list_view (void)
{
  
}

static void
set_view (GtkWidget *button, gpointer data)
{
  if (strcmp (current_view, (gchar *) data) == 0)
  {
    printf ("%s", "Different view");
    //gtk_widget_destroy (view_widget);

    if (strcmp ((gchar *) data, "icons") == 0)
    {
      current_view = g_strdup_printf ("icons");
      set_icons_view ();
    }
    else if (strcmp ((gchar *) data, "list") == 1)
    {
      current_view = g_strdup_printf ("list");
      set_list_view ();
    }
  }
}

static void
refresh_view (void)
{
  if (strcmp (current_view, "icons") == 0)
  {
    current_view = g_strdup_printf ("icons");
    set_icons_view ();
  }
  else if (strcmp (current_view, "list") == 1)
  {
    current_view = g_strdup_printf ("list");
    set_list_view ();
  }
}

static void
set_directory (GtkWidget *widget, GtkWidget *entry)
{
  struct stat s;
  gchar *new_directory;

  new_directory = gtk_entry_get_text (GTK_ENTRY (entry));

  if (stat (new_directory, &s) == 0)
  {
    if (S_ISDIR (s.st_mode))
    {
      current_directory = g_strdup_printf ("%s", new_directory);
      refresh_view ();
    }
  }
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *hbox2, *toolbar, *toolbar2, *entry, *view_option_menu, *view_menu, *view_menu_item;
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
  gtk_window_set_title (GTK_WINDOW (window), "GPE Filemanager");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  hbox2 = gtk_hbox_new (FALSE, 0);

  entry = gtk_entry_new ();

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
			   _("Back"), _("Back"), pw, NULL, NULL);

  p = gpe_find_icon ("right");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Forward"), 
			   _("Forward"), _("Forward"), pw, NULL, NULL);

  p = gpe_find_icon ("up");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Up one level"), 
			   _("Up one level"), _("Up one level"), pw, NULL, NULL);

  p = gpe_find_icon ("stop");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Stop"), 
			   _("Stop"), _("Stop"), pw, NULL, NULL);

  p = gpe_find_icon ("refresh");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Refresh"), 
			   _("Refresh"), _("Refresh"), pw, NULL, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("home");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Home"), 
			   _("Home"), _("Home"), pw, NULL, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("dir-up");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Goto Location"), 
			   _("Goto Location"), _("Goto Location"), pw, set_directory, entry);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), view_option_menu, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
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
  gtk_widget_show (entry);
  gtk_widget_show (view_option_menu);
  gtk_widget_show (view_menu);
  gtk_widget_show (view_scrolld);

  gtk_main();

  return 0;
}