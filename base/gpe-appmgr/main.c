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

/* directory listing */
#include<sys/types.h>
#include <dirent.h>
#include <string.h>

/* catch SIGHUP */
#include <signal.h>

/* I/O */
#include <stdio.h>

/* malloc / free */
#include <stdlib.h>

/* time() */
#include <time.h>

#include <X11/Xatom.h>

#include "render.h"

/* everything else */
#include "main.h"

#include "cfg.h"
#include "package.h"
#include "plugin.h"
#include "popupmenu.h"
#include "xsi.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(x) {fprintf x ;}
#define TRACE(x) {fprintf(stderr,"TRACE: " x "\n");}
#else
#define TRACE(x) ;
#define DBG(x) ;
#endif

GList *items=NULL;
GList *recent_items=NULL;
GList *groups=NULL;
GList *forced_groups=NULL;

GtkWidget *current_button=NULL;
int current_button_is_down=0;

GtkWidget *recent_tab=NULL;

time_t last_update=0;

/* For not starting an app twice after a double click */
int ignore_press = 0;

GdkPixbuf *default_pixbuf;

GtkWidget *create_tab (GList *all_items, char *current_group, tab_view_style style, GtkWidget *curr_sw);
void create_recent_list ();

void nb_switch (GtkNotebook *nb, GtkNotebookPage *page, guint pagenum)
{
	GtkWidget *hb;
	int i=0;

	if (!cfg_options.auto_hide_group_labels)
		return;

	while (1)
	{
		GtkWidget *page_contents;
		GList *children;

		page_contents = gtk_notebook_get_nth_page (GTK_NOTEBOOK(nb), i++);
		if (!page_contents)
			break;

		hb = gtk_notebook_get_tab_label (GTK_NOTEBOOK(nb), page_contents);
		if (!hb)
			continue;



		children = gtk_container_children (GTK_CONTAINER(hb));
		while (children)
		{
			if (GTK_IS_LABEL(children->data))
			{
				if (page_contents == page->child)
					gtk_widget_show (GTK_WIDGET(children->data));
				else
					gtk_widget_hide (GTK_WIDGET(children->data));
			}
			children = children->next;
		}
	}
}

gint unignore_press (gpointer data)
{
	ignore_press = 0;
	return FALSE;
}

/* Callback for selecting a program to run */
gint btn_released (GtkObject *btn, gpointer data)
{
	struct package *p;
	GList *l;

	popup_menu_cancel ();

	gtk_widget_set_state (GTK_WIDGET(btn), GTK_STATE_NORMAL);

	/* Clear the variables stating what button is down etc. 
	 * and close if we're not on the original button */
	if (GTK_WIDGET(btn) != current_button || !current_button_is_down)
	{
		current_button = NULL;
		current_button_is_down = 0;
		return TRUE;
	}

	if (!GTK_IS_EVENT_BOX(btn)) /* Could be the notebook! */
		return TRUE;

	if (ignore_press)
		return TRUE;

	/* So we ignore a second press within 1000msec */
	ignore_press = 1;
	gtk_timeout_add (1000, unignore_press, NULL);

	p = (struct package *) gtk_object_get_data (GTK_OBJECT(btn), "program");
	run_program (package_get_data (p, "command"), package_get_data (p, "windowtitle"));

	/* Add it to the recently-run list
	   Remove it first if it's already there
	   (so it moves to the front) */
	if ((l = g_list_find (recent_items, p)))
		recent_items = g_list_remove_link(recent_items, l);

	if (cfg_options.show_recent_apps)
	{
		recent_items = g_list_prepend (recent_items, p);

		while (g_list_length (recent_items) > cfg_options.recent_apps_number)
		{
			recent_items = g_list_remove_link(recent_items,
							  g_list_last (recent_items));
		}

		create_recent_list ();
	} else {
		g_list_free (recent_items);
		recent_items = NULL;
	}

	return TRUE;	
}

gint btn_pressed (GtkWidget *btn, GdkEventButton *ev, gpointer data)
{
	struct package *p;

	popup_menu_cancel ();

	/* We only want left mouse button events */
	if (ev && (!(ev->button == 1)))
	    return TRUE;

	if (ignore_press) /* Ignore double clicks! */ {
		return TRUE;
	}

	current_button = btn;
	current_button_is_down = 1;

	p = (struct package *) gtk_object_get_data (GTK_OBJECT(btn), "program");
	popup_menu_later (500, p);

	gtk_widget_set_state (btn, GTK_STATE_SELECTED);

	return TRUE;	
}

gint btn_enter (GtkWidget *btn, GdkEventCrossing *event)
{
	/* We only want left mouse button events */
	if (!(event->state & 256))
	{
		current_button = NULL;
		return TRUE;
	}

	/* If we're moving onto the button that was last selected,
	   do the same as if we've just started pressing on it again */
	if (btn == current_button)
		btn_pressed (btn, NULL, NULL);

	return TRUE;
}

gint btn_leave (GtkWidget *btn, GdkEventCrossing *event)
{
	popup_menu_cancel ();
	gtk_widget_set_state (btn, GTK_STATE_NORMAL);
	current_button_is_down = 0;
	return TRUE;
}

/* Remove the appmgr (not plugin) tabs from the notebook */
void clear_appmgr_tabs ()
{
	int i=0;
	while (1) {
		GtkWidget *page;
		page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i);
		if (page == NULL)
			return;
		if (!GTK_IS_SOCKET(page))
			gtk_notebook_remove_page (GTK_NOTEBOOK(notebook), i);
		else
			i++;
	}
}

char *find_icon (char *base)
{
	char *temp;
	FILE *inp;
	int i;

	char *directories[]=
	{
		"/usr/share/pixmaps",
		"/usr/X11R6/include/X11/pixmaps",
		"/mnt/ramfs/share/pixmaps",
		"/mnt/hda/usr/share/pixmaps",
		NULL
	};

	if (!base)
		return NULL;

	inp = fopen (base, "r");
	if (inp)
	{
		fclose (inp);
		return (char *) strdup (base);
	}

	for (i=0;directories[i];i++)
	{
		temp = g_strdup_printf ("%s/%s", directories[i], base);
		inp = fopen (temp, "r");
		if (inp)
		{
			fclose (inp);
			return temp;
		}
		g_free (temp);
	}
	return NULL;
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

GtkWidget *create_icon_pixmap (char *fn, int size)
{
	GdkPixbuf *pixbuf, *spixbuf;
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GtkWidget *w;
	pixbuf = gdk_pixbuf_new_from_file (fn);
	if (pixbuf == NULL)
		return NULL;
	spixbuf = gdk_pixbuf_scale_simple (pixbuf, size, size, 
					   GDK_INTERP_BILINEAR);
	gdk_pixbuf_unref (pixbuf);
	gdk_pixbuf_render_pixmap_and_mask (spixbuf, &pixmap, &bitmap, 127);
	gdk_pixbuf_unref (spixbuf);
	w = gtk_pixmap_new (pixmap, bitmap);
	return w;
}

GtkWidget *pixmap_from_file (char *fn)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (fn);
	GdkPixmap *pixmap;
	GdkBitmap *bitmap;
	GtkWidget *w;
	if (pixbuf == NULL)
		return NULL;
	gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, &bitmap, 127);
	w = gtk_pixmap_new (pixmap, bitmap);
	gdk_pixbuf_unref (pixbuf);
	return w;
}

char *get_closest_icon (struct package *p, int iconsize)
{
	char tmp[1024];
	char *fn;

	snprintf (tmp, 1000, "icon%d", iconsize);
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

	return fn;
}

char *get_icon_fn (struct package *p, int iconsize)
{
	char *fn, *full_fn;

	fn = get_closest_icon(p,iconsize);
	if (!fn)
		return fn;
	full_fn = find_icon(fn);

	return full_fn;
}

int has_icon (struct package *p)
{
	char *tmp;
	if ((tmp = get_icon_fn(p,0)))
	{
		free (tmp);
		return 1;
	} else
		return 0;
}

void create_recent_list ()
{
	GtkWidget *hb;
	GList *this_item;
	GList *cl;
	GtkWidget *w;

	TRACE("create_recent_list");

	/* Remove the previous list */
	cl = gtk_container_children(GTK_CONTAINER(recent_tab));
	if (cl)
	{
		w = cl->data;
		gtk_widget_destroy (w);
	}

	hb = gtk_hbox_new (0,0);

	gtk_container_add(GTK_CONTAINER(recent_tab),
			  hb);

	this_item = recent_items;
	while (this_item)
	{
		struct package *p;
		GtkWidget *btn;
		GtkWidget *img=NULL;
		char *icon;

		p = (struct package *) this_item->data;
		DBG((stderr, "recent info: %s\n", package_get_data (p, "title")));
		/* Button picture */
		icon = find_icon (get_closest_icon (p, 16));
		if (icon)
		{
			img = create_icon_pixmap (icon, 16);
			free (icon);
		}
		if (!img)
			img = pixmap_from_file("/usr/share/pixmaps/menu_unknown_program16.png");

		/* The button itself */
		btn = gtk_event_box_new ();

		gtk_container_add (GTK_CONTAINER(btn),img);
		gtk_object_set_data (GTK_OBJECT(btn), "program", (gpointer)p);
		gtk_signal_connect( GTK_OBJECT(btn), "button_release_event",
				    GTK_SIGNAL_FUNC(btn_released), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "button_press_event",
				    GTK_SIGNAL_FUNC(btn_pressed), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "enter_notify_event",
				    GTK_SIGNAL_FUNC(btn_enter), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "leave_notify_event",
				    GTK_SIGNAL_FUNC(btn_leave), NULL);

		gtk_box_pack_start (GTK_BOX(hb), btn, 0, 0, 0);

		this_item = this_item->next;
	}

	gtk_widget_show_all (recent_tab);
}

/* Make the contents for a notebook tab.
 * Generally we only want one group, but NULL means
 * ignore group setting (ie. "All").
 */
GtkWidget *create_tab (GList *all_items, char *current_group, tab_view_style style, GtkWidget *curr_sw)
{
	GtkWidget *tbl;
	GtkWidget *sw;
	GList *this_item;
	int i=0;
	int cols;

	if (style == TAB_VIEW_ICONS)
	{
		/* Get the number of columns assuming 70px per item, 20px scrollbar */
		/* This should really find out how much space it *can* use */
		if (window->allocation.width - 20 > 70) 
			cols = (window->allocation.width - 20) / 70;
		else
			cols = 1;
	}
	else
		cols = 1;

	tbl = gtk_table_new (3,cols,TRUE);

	/* Make the scrolled window */
	if (!curr_sw) /* We might already have one made for us */
		sw = gtk_scrolled_window_new (NULL, NULL);
	else
		sw = curr_sw;

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(sw), tbl);

	this_item = all_items;
	while (this_item)
	{
		struct package *p;
		GtkWidget *btn, *vb;

		p = (struct package *) this_item->data;

		/* Is the program going to appear in this group? */
		if (current_group && strcmp (current_group, package_get_data (p, "section")))
		{
			this_item = g_list_next (this_item);
			continue;
		}

		if (style == TAB_VIEW_ICONS)
		{
			GdkFont *font; /* Font in the label */
			GtkWidget *lbl;
			char *icon;
			GdkPixbuf *pixbuf = NULL;
			GtkWidget *pixmap;

			/* Button picture */
			icon = find_icon (get_closest_icon (p, 48));
			if (icon)
			{
				pixbuf = gdk_pixbuf_new_from_file (icon);
				free (icon);
			}
			if (!pixbuf) {
				if (!default_pixbuf)
					default_pixbuf = gdk_pixbuf_new_from_file ("/usr/share/pixmaps/menu_unknown_program48.png");
				pixbuf = default_pixbuf;
			}

			/* Button label */
			lbl = gtk_label_new("");

			/* Get the font used by the label */
			if (gtk_rc_get_style (lbl))
				font = gtk_rc_get_style (lbl)->font;
			else
				font = gtk_widget_get_style (lbl)->font;

			make_nice_title (lbl, font, package_get_data (p, "title"));

			/* Button VBox */
			vb = gtk_vbox_new(0,0);
			gtk_box_pack_end_defaults (GTK_BOX(vb),lbl);
			if (pixbuf) {
				GdkPixbuf *spixbuf = gdk_pixbuf_scale_simple (pixbuf, 48, 48, GDK_INTERP_BILINEAR);
				gdk_pixbuf_unref (pixbuf);
				pixmap = gpe_render_icon (notebook->style, 
							  spixbuf);
				gdk_pixbuf_unref (spixbuf);
				gtk_box_pack_end_defaults (GTK_BOX(vb), 
							   pixmap);
			}

			/* The 'button' itself */
			btn = gtk_event_box_new ();
			/* The "Ay" is arbitrary; I figure'd it'd give a decent approximation at the
			   max. height of the font */
			gtk_widget_set_usize (btn, 70, 55 + 2*gdk_text_height(font, "Ay", 2));

		} else /* Thus style == TAB_VIEW_LIST */
		{
			GtkWidget *img=NULL,*lbl;
			char *icon;

			/* Button picture */
			icon = find_icon (get_closest_icon (p, 16));
			if (icon)
			{
				img = create_icon_pixmap (icon, 16);
				free (icon);
			}
			if (!img)
				img = pixmap_from_file ("/usr/share/pixmaps/menu_unknown_program16.png");

			/* Button label */
			lbl = gtk_label_new(package_get_data (p, "title"));
			gtk_label_set_justify (GTK_LABEL(lbl), GTK_JUSTIFY_LEFT);
			gtk_widget_set_usize (lbl, 0, 16);

			/* Button HBox */
			vb = gtk_hbox_new (0,0);

			/* Pad it up */
			gtk_box_pack_end_defaults (GTK_BOX(vb),gtk_label_new(""));

			/* Put the label and pixmap in the box */
			gtk_box_pack_end (GTK_BOX(vb),lbl, 0, 0, 0);
			if (img)
				gtk_box_pack_end (GTK_BOX(vb),img, 0, 0, 0);


			/* The button itself */
			btn = gtk_event_box_new ();
		}

		gtk_container_add (GTK_CONTAINER(btn),vb);
		gtk_object_set_data (GTK_OBJECT(btn), "program", (gpointer)p);
		gtk_signal_connect( GTK_OBJECT(btn), "button_release_event",
				    GTK_SIGNAL_FUNC(btn_released), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "button_press_event",
				    GTK_SIGNAL_FUNC(btn_pressed), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "enter_notify_event",
				    GTK_SIGNAL_FUNC(btn_enter), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "leave_notify_event",
				    GTK_SIGNAL_FUNC(btn_leave), NULL);

		/* Put the button in the table */
		gtk_table_attach_defaults (GTK_TABLE(tbl), btn,
					   i%cols,i%cols+1,i/cols,i/cols+1);

		i++;

		this_item = g_list_next (this_item);
	}

	gtk_widget_show_all(sw);
	return sw;
}

/* Creates the image/label combo for a tab.
 */
GtkWidget *create_tab_label (char *name, char *icon_file)
{
	GtkWidget *img=NULL,*lbl,*hb;

	img = pixmap_from_file(icon_file);

	if (!img)
		img = pixmap_from_file("/usr/share/pixmaps/menu_unknown_group16.png");

	lbl = gtk_label_new (name);

	hb = gtk_hbox_new (FALSE, 0);
	if (img)
		gtk_box_pack_start_defaults (GTK_BOX(hb), img);
	gtk_box_pack_start_defaults (GTK_BOX(hb), lbl);

	gtk_widget_show_all (hb);
	if (cfg_options.auto_hide_group_labels)
		gtk_widget_hide (lbl);

	return hb;
}

/* Creates the image/label combo for the tab
 * of a specified group.
 */
GtkWidget *create_group_tab_label (char *group)
{
	GtkWidget *hb;
	char *icon_file;

	icon_file = g_strdup_printf ("/usr/share/pixmaps/group_%s.png", group);
	hb = create_tab_label (group, icon_file);
	g_free (icon_file);

	return hb;
}

int group_has_items (char *group)
{
	GList *l;
	l = items;

	while (l)
	{
		if (!strcasecmp (package_get_data(l->data,"section"), group))
			return TRUE;
		l = l->next;
	}

	return FALSE;
}

/* Wipe the old tabs / icons and replace them with whats
 * currently supposed to be there */
void refresh_tabs ()
{
	GList *l;
	int old_tab;

	TRACE ("refresh_tabs");

	old_tab = gtk_notebook_get_current_page (GTK_NOTEBOOK(notebook));

	clear_appmgr_tabs ();

	/* Create the 'All' tab if wanted */
	if (cfg_options.show_all_group)
		gtk_notebook_append_page (GTK_NOTEBOOK(notebook),
					  create_tab (items, NULL, cfg_options.tab_view, NULL),
					  create_group_tab_label("All"));

	/* Create the normal tabs if wanted */
	l = groups;
	while (l)
	{
		if (group_has_items (l->data))
			gtk_notebook_append_page (
				GTK_NOTEBOOK(notebook),
				create_tab (items, l->data, cfg_options.tab_view, NULL),
				create_group_tab_label(l->data));

		l = l->next;
	}

	gtk_widget_show_all (notebook);

	if (old_tab != -1)
		gtk_notebook_set_page (GTK_NOTEBOOK(notebook), old_tab);
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
	GList *l, *old_items, *old_groups;
	GList *ignored_items;
	DIR *dir;
	struct dirent *entry;
	int i;
	char *user_menu=NULL, *home_dir;

	char *directories[]=
	{
		"/usr/local/lib/menu",
		"/usr/lib/menu",
		"/home/mibus/programming/c/gpe/gpe-appmgr/dist/usr/lib/menu", /* test dir */
		"/mnt/ramfs/lib/menu",
		"/mnt/hda/usr/lib/menu",
		NULL, /* Placeholder for ~/.gpe/menu */
		NULL
	};

	TRACE ("refresh_list");

	if ((home_dir = (char*) getenv ("HOME")))
	{
		for (i=0;directories[i];i++)
			;
		directories[i] = user_menu = g_strdup_printf ("%s/.gpe/menu", home_dir);
	}

	/* Wipe out 'recent' list */
	recent_items = NULL;
	create_recent_list ();

	/* Remove the tap-hold popup menu timeout if it's there */
	popup_menu_cancel ();

	old_items = items;
	old_groups = groups;

	items = groups = NULL;

	forced_groups = load_forced_groups ();
	ignored_items = load_ignored_items ();

	for (i=0;directories[i];i++)
	{
		dir = opendir (directories[i]);
		if (!dir)
			continue;
		while ((entry = readdir (dir)))
		{
			char *temp;

			if (entry->d_name[0] == '.')
				continue;

			/* read the file if we don't want to ignore it */
			temp = g_strdup_printf ("%s/%s", directories[i], entry->d_name);
			if (g_list_find_custom (ignored_items, temp, (GCompareFunc)strcmp))
			{
				g_free (temp);
				continue;
			}

			package_read_to (temp, cb_package_add);
			g_free (temp);
		}
		closedir (dir);
	}

	groups = g_list_concat (forced_groups, groups);

	/* free the old data. note that we do it now so that
	   any stuff that references it can't ever
	   reference invalid data */
	l = old_items;
	while (l)
	{
		if (l->data)
			package_free ((struct package *)l->data);
		l = l->next;
	}
	g_list_free (old_items);

	l = old_groups;
	while (l)
	{
		if (l->data)
			free (l->data);
		l = l->next;
	}
	g_list_free (old_groups);

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

int draw_timeout_id=0;
int cb_win_draw_real (GtkWidget *widget,
		       gpointer user_data)
{
	refresh_tabs();
	draw_timeout_id = 0;
	return FALSE;
}

void cb_win_draw (GtkWidget *widget,
		  gpointer user_data)
{
	static int last_width = 0;
	TRACE ("cb_win_draw");
	DBG((stderr, "window: %p\n", window));
	DBG((stderr, "last_width: %d\n", last_width));
	DBG((stderr, "window->allocation.width: %d\n",
	     window->allocation.width));

	if (window && last_width != window->allocation.width)
	{
		if (draw_timeout_id != 0)
			gtk_timeout_remove (draw_timeout_id);
		draw_timeout_id = gtk_timeout_add (last_width == 0 ? : 200, (GtkFunction)cb_win_draw_real, NULL);
		last_width = window->allocation.width;
	}
	TRACE("cb_win_draw: end");
}

/* run on SIGHUP
 * just refreshes the program list, eg. if a new
 * program was just added */
static void catch_signal (int signo)
{
	TRACE ("catch_signal");
	/* FIXME: transfer this if needed */
	recent_items = NULL;

	cfg_load_if_newer (last_update);
	refresh_list ();
	refresh_tabs();
	last_update = time (NULL);
}

/* Create the 'recent' list as a dock app or normal widget */
void create_recent_box(GtkBox *cont)
{
	TRACE ("create_recent_box");
        recent_tab = gtk_vbox_new(0,0);

	if (cont)
	{
		gtk_box_pack_start (GTK_BOX(cont), recent_tab,0,0,0);
		gtk_widget_show (recent_tab);
	} else {
		GdkAtom window_type;
		GdkAtom window_type_dock;
		GtkWidget *w;

		w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_container_set_border_width(GTK_CONTAINER (w), 0);
		gtk_container_add(GTK_CONTAINER(w), recent_tab);

		window_type = gdk_atom_intern("_NET_WM_WINDOW_TYPE", FALSE);
		window_type_dock = gdk_atom_intern("_NET_WM_WINDOW_TYPE_DOCK", FALSE);
		gtk_widget_realize(w);
		gdk_property_change(w->window, window_type, XA_ATOM, 32, GDK_PROP_MODE_REPLACE, (guchar *) &window_type_dock, 1);
		gtk_widget_show_all (w);
	}
	TRACE ("create_recent_box: end");
}

void icons_page_up_down (int down)
{
	GtkWidget *sw;
	GtkAdjustment *adj;
	int page;
	gfloat newval;

	page = gtk_notebook_get_current_page (GTK_NOTEBOOK(notebook));
	sw = gtk_notebook_get_nth_page (GTK_NOTEBOOK(notebook), page);

	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(sw));

	if (down)
		newval = adj->value + adj->page_increment*1.5f;
	else
		newval = adj->value - adj->page_increment*1.5f;

	if (newval < adj->lower)
		newval = adj->lower;
	else if (newval > adj->upper-adj->page_size)
		newval = adj->upper-adj->page_size;

	gtk_adjustment_set_value(adj, newval);
}

gint keysnoop (GtkWidget *grab_widget, GdkEventKey *event, gpointer func_data)
{
	switch (event->keyval) {
	case GDK_Up:
		icons_page_up_down (0);
		break;
	case GDK_Down:
		icons_page_up_down (1);
		break;
	case GDK_Left:
		gtk_notebook_prev_page (GTK_NOTEBOOK(notebook));
		break;
	case GDK_Right:
		gtk_notebook_next_page (GTK_NOTEBOOK(notebook));
		break;
	default:
		DBG ((stderr, "Unhandled key: %d\n", event->keyval));
		DBG ((stderr, "     (%s)\n", gdk_keyval_name(event->keyval)));
		return 0;
	}
	return 1;
}

/* Create the UI for the main window and
 * stuff
 */
void create_main_window()
{
	GtkWidget *vbox;
	TRACE ("create_main_window");
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW(window), "Programs");
	gtk_widget_set_usize (window, 220,280);

#ifndef DEBUG
	gtk_signal_connect (GTK_OBJECT(window), "delete_event",
			    (GtkSignalFunc)gtk_true, NULL);
#else
	gtk_signal_connect (GTK_OBJECT(window), "delete_event",
			    (GtkSignalFunc)gtk_main_quit, NULL);
#endif
	
	gtk_signal_connect (GTK_OBJECT(window), "draw",
			    (GtkSignalFunc)cb_win_draw, NULL);

	notebook = gtk_notebook_new ();
	gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK(notebook), TRUE);
	gtk_notebook_set_tab_border (GTK_NOTEBOOK(notebook), 0);
	gtk_signal_connect (GTK_OBJECT(notebook), "switch_page",
			    (GtkSignalFunc)nb_switch, NULL);

	/* If a person releases the stylus when over the table, we want
	   current_button etc. to get cleared properly, so: */
	gtk_signal_connect( GTK_OBJECT(notebook), "button_release_event",
			    GTK_SIGNAL_FUNC(btn_released), NULL);

	vbox = gtk_vbox_new(0,0);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_box_pack_start (GTK_BOX(vbox), notebook, 1, 1, 0);

	/* The two styles of recent lists, appmgr or standalone */
	//create_recent_box (GTK_BOX(vbox));
	create_recent_box (NULL);

	gtk_widget_show_all (window);

	/* Example plugin usage:
	 plugin_load ("plug"); */

	/* Send all key events to the one place */
	gtk_key_snooper_install ((GtkKeySnoopFunc)keysnoop,NULL);

	//draw_timeout_id = gtk_timeout_add (0, (GtkFunction)cb_win_draw_real, NULL);
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
}
/* main():
 * init the libs, setup the signal handler,
 * run gtk bits
 */
int main(int argc, char *argv[]) {
        struct sigaction sa_old, sa_new;
	TRACE ("main");

	/* Init gtk & friends */
	gtk_init (&argc, &argv);

	/* load our configuration */
	cfg_load ();

	create_main_window();

	/* Register SIGHUP */
        sa_new.sa_handler = catch_signal;
        sigemptyset (&sa_new.sa_mask);
        sa_new.sa_flags = 0;
        sigaction (SIGHUP,&sa_new,&sa_old);

	/* So we don't keep 'defunct' processes in the process table */
	signal (SIGCHLD, SIG_IGN);

	/* update the menu data */
	refresh_list ();

	last_update = time (NULL);

	/* start the event loop */
	gtk_main();

	clean_up();

	gtk_exit (0);

	return 0;
}
