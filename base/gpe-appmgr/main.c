/* gpe-appmgr - a program launcher

   Copyright 2002 Robert Mibus;

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

/* Gtk etc. */
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

/* directory listing */
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

/* catch SIGHUP */
#include <signal.h>

/* I/O */
#include <stdio.h>

/* Option parsing */
#include <getopt.h>

/* malloc / free */
#include <stdlib.h>

/* time() */
#include <time.h>

#include <X11/Xatom.h>

/* i18n */
#include <libintl.h>
#include <locale.h>

/* GPE */
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/spacing.h>
#include <gpe/gpe-iconlist.h>
#include <gpe/tray.h>
#include <gpe/launch.h>

/* everything else */
#include "main.h"
#include "cfg.h"
#include "tray.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(x) {fprintf x ;}
#define TRACE(x) {fprintf(stderr,"TRACE: " x "\n");}
#else
#define TRACE(x) ;
#define DBG(x) ;
#endif

#define _(x) gettext(x)

time_t last_update=0;

GdkPixbuf *default_pixbuf;

struct package_group *other_group;

struct gpe_icon my_icons[] = {
  { "icon", "/usr/share/pixmaps/gpe-appmgr.png" },
  {NULL, NULL}
};

char *translate_group_name (char *name);

gboolean flag_desktop;
gboolean flag_rows;
GtkWidget *child;
Display *dpy;
gchar *only_group;
gchar *title_from_dotdesktop;

extern gboolean gpe_appmgr_start_xsettings (void);

char *
find_icon (char *base)
{
  char *temp;
  int i;
  
  char *directories[]=
    {
      "/usr/share/pixmaps",
      NULL
    };
  
  if (!base)
    return NULL;

  if (base[0] == '/')
    return (char *) strdup (base);
  
  for (i=0;directories[i];i++)
    {
      temp = g_strdup_printf ("%s/%s", directories[i], base);
      if (!access (temp, R_OK))
	return temp;

      g_free (temp);
    }

  return NULL;
}

GtkWidget *
create_icon_pixmap (GtkStyle *style, char *fn, int size)
{
  GdkPixbuf *pixbuf, *spixbuf;
  GtkWidget *w;

  pixbuf = gdk_pixbuf_new_from_file (fn, NULL);
  
  if (pixbuf == NULL)
    return NULL;
  
  spixbuf = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_BILINEAR);
  gdk_pixbuf_unref (pixbuf);
  
  w = gtk_image_new_from_pixbuf (spixbuf);
  return w;
}

char *
get_icon_fn (GnomeDesktopFile *p, int iconsize)
{
  char *fn, *full_fn;
  
  gnome_desktop_file_get_string (p, NULL, "Icon", &fn);
  if (!fn)
    return fn;

  full_fn = find_icon (fn);

  if (full_fn == NULL)
    printf("couldn't find icon \"%s\"\n", fn);
  
  g_free (fn);

  return full_fn;
}

void 
run_package (GnomeDesktopFile *p)
{
  gchar *cmd;
  gchar *title;

  gnome_desktop_file_get_string (p, NULL, "Exec", &cmd);
  gnome_desktop_file_get_string (p, NULL, "Comment", &title);

  /* Actually run the program */
  gpe_launch_program (dpy, cmd, title);
}

/* clean_up():
 * free all data etc.
 */
static void 
clean_up (void)
{
  GList *l;

  for (l = items; l; l = l->next)
    {
      if (l->data)
	gnome_desktop_file_free ((GnomeDesktopFile *)l->data);
    }
  g_list_free (items);
  items = NULL;

#if 0
  for (l = groups; l; l = l->next)
    {
      struct package_group *g = l->data;
      g_list_free (g->items);
      g_free (g->name);
      g_free (g);
    }
  g_list_free (groups);
  
  groups = NULL;
#endif
}

static struct package_group *
find_group_for_package (GnomeDesktopFile *package)
{
  GList *i;
  gchar **secs;
  int nsecs;

  if (gnome_desktop_file_get_strings (package, NULL, "Categories", NULL, &secs, &nsecs))
    {
      for (i = groups; i; i = i->next)
	{
	  GList *c;
	  struct package_group *g;
	  g = i->data;
	  for (c = g->categories; c; c = c->next)
	    {
	      int i;
	      for (i = 0; i < nsecs; i++)
		if (!strcmp (secs[i], c->data))
		  {
		    g_strfreev (secs);
		    return g;
		  }
	    }
	}
    }

  g_strfreev (secs);

  return other_group;
}

static gint
package_compare (GnomeDesktopFile *a, GnomeDesktopFile *b)
{
  gchar *na, *nb;
  gint r;

  gnome_desktop_file_get_string (a, NULL, "Name", &na);
  gnome_desktop_file_get_string (b, NULL, "Name", &nb);
  
  r = strcmp (na, nb);

  g_free (na);
  g_free (nb);

  return r;
}

static void 
add_one_package (GnomeDesktopFile *p)
{
  struct package_group *group;

  group = find_group_for_package (p);

  items = g_list_insert_sorted (items, p, (GCompareFunc)package_compare);
  group->items = g_list_insert_sorted (group->items, p, (GCompareFunc)package_compare);
}

static void
load_from (const char *path)
{
  DIR *dir;
  struct dirent *entry;

  dir = opendir (path);
  if (dir)
    {
      while ((entry = readdir (dir)))
	{
	  char *temp;
	  GnomeDesktopFile *p;
	  GError *err = NULL;
	  
	  if (entry->d_name[0] == '.')
	    continue;
	  
	  temp = g_strdup_printf ("%s/%s", PREFIX "/share/applications", entry->d_name);
	  p = gnome_desktop_file_load (temp, &err);
	  if (p)
	    {
	      gchar *type;
	      gnome_desktop_file_get_string (p, NULL, "Type", &type);

	      if (type == NULL || strcmp (type, "Application"))
		{
		  fprintf (stderr, "ignoring .desktop with type=\"%s\"\n", type ? type : "<null>");
		  gnome_desktop_file_free (p);
		}
	      else
		add_one_package (p);

	      if (type)
		g_free (type);
	    }
	  else
	    fprintf (stderr, "Couldn't load \"%s\": %s\n", temp, err->message);

	  if (err)
	    g_error_free (err);

	  g_free (temp);
	}

      closedir (dir);
    }
}

/* Reloads the menu files into memory */
int 
refresh_list (void)
{
  clean_up ();
  
  load_from (PREFIX "/share/applications");

  TRACE ("refresh_list: end");
  return FALSE;
}

void
refresh_tabs (void)
{
  void (*func)(void);
  func = g_object_get_data (G_OBJECT (child), "refresh_func");
  (*func) ();
}

/* run on SIGHUP
 * just refreshes the program list, eg. if a new
 * program was just added */
static void catch_signal (int signo)
{
  TRACE ("catch_signal");
  /* FIXME: transfer this if needed */

  refresh_list ();
  refresh_tabs();
  last_update = time (NULL);
}

void
on_window_delete (GObject *object, gpointer data)
{
  gtk_main_quit ();
}

extern GtkWidget * create_tab_view (void);
extern GtkWidget * create_row_view (void);
extern GtkWidget * create_single_view (void);

/* Create the UI for the main window and
 * stuff
 */
void 
create_main_window (void)
{
  int w = 640, h = 480;
  GnomeDesktopFile *d = NULL;
  gchar *title = NULL;
  GdkPixbuf *icon = NULL;

  if (title_from_dotdesktop)
    {
      GError *err = NULL;
      d = gnome_desktop_file_load (title_from_dotdesktop, &err);
      if (!d)
	{
	  fprintf (stderr, "couldn't load desktop file \"%s\": %s\n", title_from_dotdesktop, err->message);
	}
      if (err)
	g_error_free (err);
    }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  if (d)
    {
      gchar *icon_fn = NULL;
      gnome_desktop_file_get_string (d, NULL, "Name", &title);
      gnome_desktop_file_get_string (d, NULL, "Icon", &icon_fn);
      if (icon_fn)
	icon = gdk_pixbuf_new_from_file (icon_fn, NULL);
    }

  if (!icon)
    icon = gpe_find_icon ("icon");
  if (!title)
    title = _("Programs");

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_window_set_icon (GTK_WINDOW (window), icon);
  gtk_widget_realize (window);

  if (flag_desktop)
    {
      gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_DESKTOP);
      w = gdk_screen_width ();
      h = gdk_screen_height ();
    }

  gtk_window_set_default_size (GTK_WINDOW (window), w, h);
    
  g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (on_window_delete), NULL);

  child = flag_rows ? create_row_view () : only_group ? create_single_view () : create_tab_view ();
  gtk_container_add (GTK_CONTAINER (window), child);
  
  gtk_widget_show_all (window);

  if (d)
    gnome_desktop_file_free (d);

  refresh_tabs ();
}

struct package_group *
make_group (gchar *name)
{
  struct package_group *g;
  g = g_malloc0 (sizeof (struct package_group));
  g->name = name;
  groups = g_list_append (groups, g);  
  return g;
}

/* main():
 * init the libs, setup the signal handler,
 * run gtk bits
 */
int 
main (int argc, char *argv[]) 
{
  struct package_group *g;
  struct sigaction sa_old, sa_new;

  /* Hmm */
  items = groups = NULL;

  /* Init gtk & friends */
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);
  
  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  while (1)
    {
      int option_index = 0;
      int c;

      static struct option long_options[] = {
	{"desktop", 0, 0, 'd'},
	{"rows", 0, 0, 'r'},
	{"group", 0, 0, 'g'},
	{0, 0, 0, 0}
      };
      
      c = getopt_long (argc, argv, "drg:D:",
		       long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 'd':
	  flag_desktop = TRUE;
	  break;
	case 'r':
	  flag_rows = TRUE;
	  break;
	case 'g':
	  only_group = optarg;
	  break;
	case 'D':
	  title_from_dotdesktop = optarg;
	  break;
	}
    }

  g = make_group ("Office");
  g->categories = g_list_append (g->categories, "Office");
  g = make_group ("Internet");
  g->categories = g_list_append (g->categories, "Internet");
  g = make_group ("PIM");
  g->categories = g_list_append (g->categories, "PIM");
  g = make_group ("Games");
  g->categories = g_list_append (g->categories, "Games");
  g = make_group ("Settings");
  g->categories = g_list_append (g->categories, "SystemSettings");
  g->categories = g_list_append (g->categories, "Settings");
  g->hide = TRUE;

  other_group = make_group ("Other");

  /* update the menu data */
  refresh_list ();

  /* make the UI */
  create_main_window();
  
  /* Register SIGHUP */
  sa_new.sa_handler = catch_signal;
  sigemptyset (&sa_new.sa_mask);
  sa_new.sa_flags = 0;
  sigaction (SIGHUP,&sa_new,&sa_old);

  /* So we don't keep 'defunct' processes in the process table */
  signal (SIGCHLD, SIG_IGN);
  
  last_update = time (NULL);
 
#if 0 
  gpe_appmgr_start_xsettings ();
#endif
  
  dpy = GDK_WINDOW_XDISPLAY (window->window);
  
  /* start the event loop */
  gtk_main ();
  
  clean_up ();
  
  exit (0);
}
