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

/* everything else */
#include "main.h"

#include "cfg.h"
#include "package.h"
#include "plugin.h"
#include "popupmenu.h"
#include "launch.h"
#include <dlfcn.h>
#include "tray.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(x) {fprintf x ;}
#define TRACE(x) {fprintf(stderr,"TRACE: " x "\n");}
#else
#define TRACE(x) ;
#define DBG(x) ;
#endif

GList *forced_groups=NULL;

GtkWidget *current_button=NULL;
int current_button_is_down=0;

GtkWidget *recent_tab=NULL;

time_t last_update=0;

GSList *translate_list;

GdkPixbuf *default_pixbuf;

struct gpe_icon my_icons[] = {
  { "icon", "/usr/share/pixmaps/gpe-appmgr.png" },
  {NULL, NULL}
};

char *translate_group_name (char *name);

gboolean flag_desktop;
gboolean flag_rows;
GtkWidget *child;

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
get_icon_fn (struct package *p, int iconsize)
{
  char *fn, *full_fn;
  
  fn = package_get_data (p, "icon");
  if (!fn)
    return fn;

  full_fn = find_icon (fn);

  if (full_fn == NULL)
    printf("couldn't find %s\n", fn);
  
  return full_fn;
}

void 
run_package (struct package *p) 
{
  printf ("Running: %s\n", package_get_data (p, "title"));

  /* Actually run the program */
  run_program (package_get_data (p, "command"), package_get_data (p, "title"));
}

gboolean 
btn_clicked (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  run_package ((struct package *)user_data);
  return TRUE;
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
	package_free ((struct package *)l->data);
    }
  g_list_free (items);

  for (l = groups; l; l = l->next)
    {
      struct package_group *g = l->data;
      g_list_free (g->items);
      g_free (g->name);
      g_free (g);
    }
  g_list_free (groups);
  
  groups = items = NULL;
}

static struct package_group *
find_group_by_name (const char *name)
{
  GList *i;
  struct package_group *g;

  for (i = groups; i; i = i->next)
    {
      g = i->data;
      if (!strcmp (g->name, name))
	return g;
    }

  g = g_malloc0 (sizeof (*g));
  g->name = g_strdup (name);
  groups = g_list_append (groups, g);
  return g;
}

static void 
cb_package_add (struct package *p)
{
  const char *group_name;
  struct package_group *group;

  group_name = package_get_data (p, "section");
  group = find_group_by_name (group_name);

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
	  struct package *p;
	  
	  if (entry->d_name[0] == '.')
	    continue;
	  
	  temp = g_strdup_printf ("%s/%s", PREFIX "/share/applications", entry->d_name);
	  p = package_from_dotdesktop (temp, NULL);
	  if (p)
	    cb_package_add (p);

	  g_free (temp);
	}

      closedir (dir);
    }
}

/* Reloads the menu files into memory */
int 
refresh_list (void)
{
  /* Remove the tap-hold popup menu timeout if it's there */
  popup_menu_cancel ();

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

/* Create the UI for the main window and
 * stuff
 */
void 
create_main_window (void)
{
  int w = 640, h = 480;

  TRACE ("create_main_window");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW( window), "Programs");
  gtk_widget_realize (window);

  if (flag_desktop)
    {
      gdk_window_set_type_hint (window->window, GDK_WINDOW_TYPE_HINT_DESKTOP);
      w = gdk_screen_width ();
      h = gdk_screen_height ();
    }

  gtk_window_set_default_size (GTK_WINDOW (window), w, h);
    
  g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (on_window_delete), NULL);

  child = flag_rows ? create_row_view () : create_tab_view ();
  gtk_container_add (GTK_CONTAINER (window), child);
  
  gpe_set_window_icon (window, "icon");

  gtk_widget_show_all (window);

  refresh_tabs ();
}

/* main():
 * init the libs, setup the signal handler,
 * run gtk bits
 */
int 
main (int argc, char *argv[]) 
{
  struct sigaction sa_old, sa_new;
  TRACE ("main");

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
	{0, 0, 0, 0}
      };
      
      c = getopt_long (argc, argv, "dr",
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
	}
    }
  
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
  
  gpe_appmgr_start_xsettings ();
  
  /* start the event loop */
  gtk_main();
  
  clean_up();
  
  exit (0);
}
