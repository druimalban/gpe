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
      if (access (temp, R_OK))
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

char *get_closest_icon (struct package *p, int iconsize)
{
  char *tmp;
  char *fn;

  tmp = g_strdup_printf ("icon%d", iconsize);
  fn = package_get_data (p, tmp);
  if (!fn)
    fn = package_get_data (p, "icon");
  if (!fn)
    fn = package_get_data (p, "icon48");
  if (!fn)
    {
      /* TODO: run 48->16
       * maybe get actual closest, preferring larger?
       */
    }
  
  g_free (tmp);
  
  return fn;
}

char *
get_icon_fn (struct package *p, int iconsize)
{
  char *fn, *full_fn;
  
  fn = get_closest_icon(p,iconsize);
  if (!fn)
    return fn;

  full_fn = find_icon(fn);
  
  return full_fn;
}

int 
has_icon (struct package *p)
{
  char *tmp;
  if ((tmp = get_icon_fn(p,0)))
    {
      free (tmp);
      return 1;
    }

  return 0;
}

void 
run_package (struct package *p) 
{
  GList *l;

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

GList *load_simple_list (char *fn)
{
	GList *l=NULL;
	char line[1024], *t;
	FILE *inp;

	inp = fopen (fn, "r");
	if (!inp)
		return NULL;

	t = line;
	do
	{
		*t = getc (inp);
		if (*t == '\n' || t - line > 1000)
		{
			*t = 0;
			l = g_list_append (l, strdup (line));
			t = line;
			continue;
		}
		t++;
	} while (!feof (inp));

	fclose (inp);

	return l;
}

GList *load_forced_groups ()
{
	char *fn;
	GList *l;

	fn = g_strdup_printf ("%s/.gpe/gpe-appmgr_group-order", g_get_home_dir ());

	l = load_simple_list (fn);

	g_free (fn);

	return l;
}

GList *load_ignored_items ()
{
	char *fn;
	GList *l;

	fn = g_strdup_printf ("%s/.gpe/gpe-appmgr_ignored-items", g_get_home_dir ());

	l = load_simple_list (fn);

	g_free (fn);

	return l;
}

GList *load_translations_from (char *fmt, char *a1, char *a2)
{
	GList *rl;
	char *p = g_strdup_printf (fmt, a1, a2);
	rl = load_simple_list (p);
	free (p);
	return rl;
}

struct translation_entry
{
  gchar *a, *b;
};


GSList *load_group_translations ()
{
	GList *l = NULL, *i;
	GSList *tmap = NULL;

	char *locale = setlocale (LC_MESSAGES, NULL);
	if (locale) {
		l = load_translations_from ("%s/.gpe/gpe-appmgr_group-map.%s", (char*)g_get_home_dir (), locale);
		if (!l)
			l = load_translations_from ("/usr/share/gpe/group-map.%s", locale, NULL);
		if (!l) {
			gchar *ln = g_strdup (locale);
			char *p = strchr (ln, '_');
			if (p) {
				*p = 0;
				l = load_translations_from ("%s/.gpe/gpe-appmgr_group-map.%s",  (char*)g_get_home_dir (), ln);
				if (!l)
					l = load_translations_from ("/usr/share/gpe/group-map.%s", ln, NULL);
			}
			g_free (ln);
		}
	}
	if (!l)
		l = load_translations_from ("%s/.gpe/gpe-appmgr_group-map",  (char*)g_get_home_dir (), NULL);
	if (!l)
		l = load_translations_from ("/usr/share/gpe/group-map", NULL, NULL);

	for (i = l; i; i = i->next) {
		char *s = i->data;
		char *p = strchr (s, '=');
		if (p) {
			struct translation_entry *t = g_malloc (sizeof (*t));
			*p++ = 0;
			t->a = g_strdup (s);
			t->b = g_strdup (p);
			tmap = g_slist_append (tmap, t);
		}
		free (i->data);
	}
	g_list_free (i);

	return tmap;
}

char *translate_group_name (char *name)
{
	GSList *i;
	
	for (i = translate_list; i; i = i->next) {
		struct translation_entry *t = i->data;
		if (!strcmp (t->a, name)) {
			return t->b;
		}
	}

	return name;
}

/* clean_up():
 * free all data etc.
 */
void clean_up ()
{
	GList *l;

	TRACE("clean_up");

	l = items;
	while (l)
	{
		if (l->data)
			package_free ((struct package *)l->data);
		l = l->next;
	}
	g_list_free (items);

	l = groups;
	while (l)
	{
		if (l->data)
			free (l->data);
		l = l->next;
	}
	g_list_free (groups);

	groups = items = NULL;
}

void cb_package_add (struct package *p)
{
	TRACE ("cb_package_add");

	/* add the item to the list */
	if (p)
	{
		/* The package has to provide an icon and a section */
		if (has_icon (p) && package_get_data (p, "section"))
		{
			/* Don't allow X/Y section names (eg. "Games/Strategy") */
			if (strchr (package_get_data (p, "section"), '/'))
				*(strchr(package_get_data (p, "section"), '/'))=0;
			items = g_list_insert_sorted (items, p, (GCompareFunc)package_compare);
			/* Update the groups if necessary */
			if ((g_list_find_custom (groups, package_get_data (p, "section"),
						 (GCompareFunc) strcmp) == NULL) &&
			    (g_list_find_custom (forced_groups, package_get_data (p, "section"),
						 (GCompareFunc) strcmp) == NULL))
				groups = g_list_insert_sorted (groups, strdup(package_get_data(p,"section")), (GCompareFunc)strcasecmp);
		}
		else
			package_free (p);

	}
}

/* Reloads the menu files into memory */
int refresh_list ()
{
	GList *ignored_items;
	DIR *dir;
	struct dirent *entry;
	char *user_menu = NULL, *home_dir;
	GSList *j;

	TRACE ("refresh_list");

	/* Remove the tap-hold popup menu timeout if it's there */
	popup_menu_cancel ();

	clean_up ();

	/* flush old translations */
	for (j = translate_list; j; j = j->next) {
		struct translation_entry *t = j->data;
		g_free (t->a);
		g_free (t->b);
		g_free (t);
	}
	g_slist_free (translate_list);

	forced_groups = load_forced_groups ();
	ignored_items = load_ignored_items ();
	translate_list = load_group_translations ();
	
	dir = opendir (PREFIX "/share/applications");
	if (dir)
	{
		while ((entry = readdir (dir)))
		{
			char *temp;

			if (entry->d_name[0] == '.')
				continue;

			/* read the file if we don't want to ignore it */
			temp = g_strdup_printf ("%s/%s", PREFIX "/share/applications", entry->d_name);
			if (g_list_find_custom (ignored_items, temp, (GCompareFunc)strcmp))
			{
				g_free (temp);
				continue;
			}

			cb_package_add (package_from_dotdesktop (temp, NULL));
			g_free (temp);
		}
		closedir (dir);
	}

	groups = g_list_concat (forced_groups, groups);

	/* We allocated a string for the user's menu dir; free it */
	if (user_menu)
		g_free (user_menu);

	TRACE ("refresh_list: end");
	return FALSE;
}

/* refresh_tabs() is called through GTK so that
   the window already exists; thus we can get it's
   size and determine the number of columns available.
   It's also called when XRANDR does its job.

   It's done this way (a timeout via a draw callback)
   because calling refresh_tabs() directly from the
   draw callback was causing crashes (apparently
   within Gtk, not sure though)
*/

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
