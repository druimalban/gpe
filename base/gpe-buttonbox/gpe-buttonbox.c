/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libintl.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/gpewindowlist.h>
#include <gpe/popup.h>

#include <gpe/launch.h>
#include <gpe/desktop_file.h>

#include <locale.h>

#include "config.h"
#include "globals.h"

#define _(_x) gettext (_x)

GtkWidget *window;
GtkWidget *box;
Display *dpy;

GList *windows;
GList *classes;

int xpos;

GList *other_classes;
GtkWidget *other_button;
GdkPixbuf *other_icon;

char **g_argv;

#define NR_SLOTS 7
#define SLOT_WIDTH 64

Atom atoms[12];

char *atom_names[] =
  {
    "_NET_WM_WINDOW_TYPE_DOCK",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_STATE_HIDDEN",
    "_NET_WM_SKIP_PAGER",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_PSION_DESKTOP_SWIZZLE",
    "_NET_ACTIVE_WINDOW",
    "_NET_SHOWING_DESKTOP",
    "_NET_SYSTEM_TRAY_OPCODE",
    "_NET_SYSTEM_TRAY_MESSAGE_DATA",
    "MANAGER",
    "_NET_SYSTEM_TRAY_S0"
};

#define ICON_SIZE	24

struct class_record *class_slot[NR_SLOTS];

struct window_record
{
  Window w;
  Window leader;

  gchar *class;
  gchar *icon_name;
  gchar *exec;
  GdkPixbuf *icon;
  struct class_record *c;
};

struct class_record
{
  gint slot;
  gchar *name;
  GList *windows;
  GtkWidget *widget;
  gchar *exec;
  gboolean fixed;
};

#define MY_PIXMAPS_DIR "gpe-buttonbox/"

struct gpe_icon my_icons[] = 
  {
    { "desktop", MY_PIXMAPS_DIR "show-desktop" },
    { "other", MY_PIXMAPS_DIR "other-app" },
    { "docs", MY_PIXMAPS_DIR "docs" },
    { "yellow-down", MY_PIXMAPS_DIR "yellow_button_down" },
    { "yellow-up", MY_PIXMAPS_DIR "yellow_button_up" },
    { "blue-down", MY_PIXMAPS_DIR "blue_button_down" },
    { "blue-up", MY_PIXMAPS_DIR "blue_button_up" },
    { "separator", MY_PIXMAPS_DIR "separator" },
    { NULL }
  };

static gboolean is_viewable (Display *dpy, Window w);

/* copied from libmatchbox */
static void
activate_window (Display *dpy, Window win)
{
  XEvent ev;

  memset (&ev, 0, sizeof ev);
  ev.xclient.type = ClientMessage;
  ev.xclient.window = win;
  ev.xclient.message_type = atoms[_NET_ACTIVE_WINDOW];
  ev.xclient.format = 32;

  XSendEvent (dpy, RootWindow (dpy, DefaultScreen (dpy)), False, SubstructureRedirectMask, &ev);
}

void
raise_window (struct window_record *r)
{
  activate_window (dpy, r->w);
}

void
popup_window_list (GtkWidget *widget, GdkEventButton *button, GList *windows)
{
  GtkWidget *menu;
  GList *l;
  gint x, y;

  menu = gtk_menu_new ();
  
  for (l = windows; l; l = l->next)
    {
      GtkWidget *item;
      struct window_record *r;
      gchar *name;
 
      r = l->data;
      g_assert (r != NULL);

      name = gpe_get_window_name (dpy, r->w);
      if (name == NULL && r->leader != r->w)
	name = gpe_get_window_name (dpy, r->leader);
      if (name == NULL)
	name = g_strdup ("?");

      item = gtk_image_menu_item_new_with_label (name);
      
      gtk_widget_show (item);
      
      if (r->icon)
	{
	  GtkWidget *icon;
	  GdkPixbuf *pix;
	  
	  pix = gdk_pixbuf_scale_simple (r->icon, 16, 16, GDK_INTERP_BILINEAR);
	  icon = gtk_image_new_from_pixbuf (pix);
	  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
	}
      
      g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (raise_window), r);
      
      gtk_menu_append (GTK_MENU (menu), item);
    }

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
		  gpe_popup_menu_position,
		  widget, button->button, button->time);
}

void
raise_one_window (GList *windows)
{
  GList *l;
  gboolean use_next = FALSE;

  for (l = windows; l; l = l->next)
    {
      struct window_record *r = l->data;

      if (use_next)
	{
	  raise_window (r);
	  return;
	}

      if (is_viewable (dpy, r->w))
	use_next = TRUE;
    }

  raise_window (windows->data);
}

void
update_icon (GObject *obj, int down)
{
  GObject *widget;
  widget = g_object_get_data (G_OBJECT (obj), "icon-widget");
  g_object_set_data (G_OBJECT (widget), "down", (gpointer)down);
  gtk_widget_queue_draw (GTK_WIDGET (widget));
}

struct tap
{
  GtkWidget *widget;
  GdkEventButton button;
  struct class_record *c;
  gboolean flag;
};

static struct tap *t;

#define TIMEOUT 350

gboolean
timeout (gpointer p)
{
  struct tap *tt = (struct tap *)p;

  if (tt->flag == TRUE)
    {
      update_icon (G_OBJECT (tt->widget), 0);
      popup_window_list (tt->widget, &tt->button, tt->c->windows);
    }

  t = NULL;

  g_free (tt);

  return FALSE;
}

void
button_press_event (GtkWidget *widget, GdkEventButton *button, struct class_record *c)
{
  if (c)
    {
      struct tap *tt;
      
      if (button->button != 1)
	return;
      
      tt = g_malloc (sizeof (struct tap));
      g_timeout_add (TIMEOUT, timeout, tt);
      memcpy (&tt->button, button, sizeof (*button));
      tt->widget = widget;
      tt->c = c;
      tt->flag = TRUE;
      t = tt;
    }

  update_icon (G_OBJECT (widget), 1);
}

void
button_release_event (GtkWidget *widget, GdkEventButton *button, struct class_record *c)
{
  if (button->button != 1)
    return;

  update_icon (G_OBJECT (widget), 0);

  if (t)
    t->flag = FALSE;

  if (c->windows)
    {
      if (t)
	raise_one_window (c->windows);
    }
  else if (c->exec)
    {
      Window w;
      w = gpe_launch_get_window_for_binary (dpy, c->exec);
      if (w != None)
	{
	  activate_window (dpy, w);
	  return;
	}

      if (! gpe_launch_startup_is_pending (dpy, c->exec))
	{
	  if (c->name)
	    {
	      gchar *text;

	      text = g_strdup_printf ("Starting %s", c->name);
	      gpe_popup_infoprint (dpy, text);
	      g_free (text);
	    }
	  
	  gpe_launch_program_with_callback (dpy, c->exec, c->exec, TRUE, NULL, NULL);
	}
    }
  else
    {
      fprintf (stderr, "no windows and nothing to exec!\n");
      return;
    }
}

void
other_button_release (GtkWidget *widget, GdkEventButton *button)
{
  GList *list = NULL, *class_iter;

  update_icon (G_OBJECT (widget), 0);

  for (class_iter = other_classes; class_iter; class_iter = class_iter->next)
    {
      GList *window_iter;
      struct class_record *c = class_iter->data;

      for (window_iter = c->windows; window_iter; window_iter = window_iter->next)
	list = g_list_append (list, window_iter->data);
    }

  popup_window_list (widget, button, list);

  g_list_free (list);
}

void
docs_button_release (GtkWidget *widget, GdkEventButton *button)
{
  Window w;
  const char *docs_str;
  
  docs_str = g_strdup_printf ("gpe-filemanager -p \"%s/My Documents\" -t Docs --class Gpe-filemanager-docs", g_get_home_dir ());

  update_icon (G_OBJECT (widget), 0);

  w = gpe_launch_get_window_for_binary (dpy, docs_str);
  if (w != None)
    activate_window (dpy, w);
  else
    {
      if (! gpe_launch_startup_is_pending (dpy, docs_str))
	gpe_launch_program_with_callback (dpy, docs_str, docs_str, TRUE, NULL, NULL);
    }

  g_free (docs_str);
}

void
desktop_button_release (GtkWidget *widget, GdkEventButton *button)
{
  XEvent ev;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data;
  int val = 1;

  update_icon (G_OBJECT (widget), 0);

  data = NULL;
  if (XGetWindowProperty (dpy, RootWindow (dpy, DefaultScreen (dpy)),
			  atoms[_NET_SHOWING_DESKTOP], 0, 65535LL, False,
			  XA_CARDINAL, &actual_type, &actual_format,
			  &nitems, &bytes_after, &data) == Success)
    {
      if (data)
	{
	  if (*((int *)data) != 0)
	    {
	      /* Desktop was already showing */
	      memset (&ev, 0, sizeof ev);
	      ev.xclient.type = ClientMessage;
	      ev.xclient.window = None;
	      ev.xclient.message_type = atoms[_PSION_DESKTOP_SWIZZLE];
	      ev.xclient.format = 32;
	      ev.xclient.data.l[0] = val;

	      XSendEvent (dpy, RootWindow (dpy, DefaultScreen (dpy)), False, 
			  PropertyChangeMask, &ev);
	    }

	  XFree (data);
	}
    }

  memset (&ev, 0, sizeof ev);
  ev.xclient.type = ClientMessage;
  ev.xclient.window = None;
  ev.xclient.message_type = atoms[_NET_SHOWING_DESKTOP];
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = val;

  XSendEvent (dpy, RootWindow (dpy, DefaultScreen (dpy)), False, SubstructureRedirectMask, &ev);
}

void
draw_icon (GtkWidget *widget, GdkEventExpose *ev)
{
  GdkPixbuf *base, *pix;
  GdkRectangle r, br;
  gchar *text;
  PangoLayout *layout;
  gboolean yellow, down;
  const char *name;
  int w, h;

  yellow = (gboolean)g_object_get_data (G_OBJECT (widget), "yellow");
  down = (gboolean)g_object_get_data (G_OBJECT (widget), "down");

  if (yellow)
    name = down ? "yellow-down" : "yellow-up";
  else
    name = down ? "blue-down" : "blue-up";

  base = gpe_find_icon (name);
  pix = g_object_get_data (G_OBJECT (widget), "icon-pixbuf");
  text = g_object_get_data (G_OBJECT (widget), "icon-text");

  gdk_draw_pixbuf (widget->window, 
		   widget->style->fg_gc[widget->state],
		   base,
		   ev->area.x, ev->area.y,
		   ev->area.x, ev->area.y,
		   ev->area.width, ev->area.height,
		   GDK_RGB_DITHER_NORMAL, 0, 0);

  // scale to desired height but retain aspect ratio
  w = gdk_pixbuf_get_width (pix);
  h = gdk_pixbuf_get_height (pix);
  w = (w * ICON_SIZE) / h;

  br.x = (64 - w) / 2;
  br.y = 4;
  br.width = w;
  br.height = ICON_SIZE;

  if (gdk_rectangle_intersect (&ev->area, &br, &r))
    {
      gdk_draw_pixbuf (widget->window, 
		       widget->style->fg_gc[widget->state],
		       pix,
		       r.x - br.x, r.y - br.y,
		       r.x, r.y,
		       r.width, r.height,
		       GDK_RGB_DITHER_NORMAL, 0, 0);
    }

  layout = gtk_widget_create_pango_layout (GTK_WIDGET (widget),
					   text);

  pango_layout_set_width (layout, SLOT_WIDTH * PANGO_SCALE);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

  gtk_paint_layout (widget->style, widget->window, widget->state,
		    FALSE, &ev->area, widget, "", 0, ICON_SIZE + 6, layout);  

  g_object_unref (layout);
}

GtkWidget *
do_build_icon (gchar *name, GdkPixbuf *icon_pix, gboolean yellow)
{
  GtkWidget *wbox;
  GtkWidget *icon;
  GtkWidget *text;
  GtkWidget *ebox;
  GdkPixbuf *composite, *scaled;
  int w, h;

  // scale to desired height but retain aspect ratio
  w = gdk_pixbuf_get_width (icon_pix);
  h = gdk_pixbuf_get_height (icon_pix);
  w = (w * ICON_SIZE) / h;

  scaled = gdk_pixbuf_scale_simple (icon_pix, w, ICON_SIZE, 
				    GDK_INTERP_BILINEAR);

  icon = gtk_drawing_area_new ();
  ebox = gtk_event_box_new ();

  gtk_widget_show (icon);

  gtk_widget_set_usize (GTK_WIDGET (ebox), SLOT_WIDTH, 48);
  
  gtk_container_add (GTK_CONTAINER (ebox), icon);

  g_object_set_data (G_OBJECT (ebox), "icon-widget", icon);
  g_object_set_data (G_OBJECT (icon), "icon-pixbuf", scaled);
  g_object_set_data (G_OBJECT (icon), "icon-text", name);
  g_object_set_data (G_OBJECT (icon), "yellow", (gpointer)yellow);
  g_object_set_data (G_OBJECT (icon), "down", (gpointer)FALSE);

  g_signal_connect (G_OBJECT (icon), "expose_event",
		    G_CALLBACK (draw_icon), NULL);

  return ebox;
}

GtkWidget *
build_icon_for_class (struct class_record *c)
{
  GtkWidget *ebox;
  struct window_record *r;

  r = c->windows->data;

  ebox = do_build_icon (r->icon_name, r->icon, FALSE);

  g_signal_connect (G_OBJECT (ebox), "button_press_event", G_CALLBACK (button_press_event), c);
  g_signal_connect (G_OBJECT (ebox), "button_release_event", G_CALLBACK (button_release_event), c);
  gtk_widget_add_events (GTK_WIDGET (ebox), GDK_BUTTON_RELEASE_MASK);

  return ebox;
}

struct class_record *
find_class_record (struct window_record *r)
{
  struct class_record *c;
  GList *l;

  /* If the window has a class, use it.  */
  if (r->class)
    {
      for (l = classes; l; l = l->next)
	{
	  c = l->data;
	  
	  if (c->name && !strcmp (c->name, r->class))
	    return c;
	}
    }

  /* If the window has an exec, and it matches a class we know about,
     use it.  */
  if (r->exec)
    {
      for (l = classes; l; l = l->next)
	{
	  c = l->data;
	  
	  if (c->exec && !strcmp (c->exec, r->exec))
	    return c;
	}
    }

  /* Look for another window with the same client leader.  This must
     be another instance of the same application, so inherit its
     class.  */
  if (r->leader != None)
    {
      for (l = windows; l; l = l->next)
	{
	  struct window_record *rr = l->data;
	  if (rr->leader == r->leader)
	    return rr->c;
	}
    }
  return NULL;
}

struct class_record *
make_class_record (struct window_record *r)
{
  struct class_record *c;
  c = g_malloc0 (sizeof (*c));
  if (r->class)
    c->name = g_strdup (r->class);
  if (r->exec)
    c->exec = g_strdup (r->exec);
  c->slot = -1;
  classes = g_list_append (classes, c);
  return c;
}

void
find_slot_for_class (struct class_record *c)
{
  if (c->slot == -1)
    {
      int i;

      for (i = 0; i < NR_SLOTS; i++)
	{
	  if (class_slot[i] == NULL)
	    {
	      class_slot[i] = c;
	      c->slot = i;
	      c->widget = build_icon_for_class (c);
	      gtk_fixed_put (GTK_FIXED (box), c->widget, (i + 2) * SLOT_WIDTH, 0);
	      gtk_widget_show (c->widget);
	      return;
	    }
	}
    }
}

static void
show_other_button (void)
{
  struct class_record *c;
  c = class_slot[NR_SLOTS - 1];

  other_classes = g_list_append (other_classes, c);
  gtk_container_remove (GTK_CONTAINER (box), c->widget);
  c->slot = -1;
  class_slot[NR_SLOTS - 1] = (void *)1;

  gtk_fixed_put (GTK_FIXED (box), other_button, (NR_SLOTS + 1) * SLOT_WIDTH, 0);
  gtk_widget_show (other_button);
}

static void
add_window_record (struct window_record *r)
{
  struct class_record *c;
  GList *l;
  gboolean new_class = FALSE;

  g_assert (r != NULL);

  if (r->icon)
    g_object_ref (r->icon);

  c = find_class_record (r);
  if (!c)
    c = make_class_record (r);
  r->c = c;

  if (c->windows == NULL)
    new_class = TRUE;

  c->windows = g_list_append (c->windows, r);

  find_slot_for_class (c);

  if (c->slot == -1 && new_class)
    {
      gboolean other_button_visible = (other_classes != NULL) ? TRUE : FALSE;
      other_classes = g_list_append (other_classes, c);
      if (!other_button_visible)
	show_other_button ();
    }
}

static void
hide_other_button (void)
{
  assert (class_slot[NR_SLOTS - 1] == (void *)1);
  assert (other_classes == NULL);

  gtk_container_remove (GTK_CONTAINER (box), other_button);
}

static void
delete_window_record (struct window_record *r)
{
  struct class_record *c;
  gboolean slot_is_free = FALSE;

  c = r->c;

  c->windows = g_list_remove (c->windows, r);
  if (c->windows == NULL && c->fixed == FALSE)
    {
      if (c->slot != -1)
	{
	  int slot = c->slot;

	  gtk_container_remove (GTK_CONTAINER (box), c->widget);
	  c->widget = NULL;
	  class_slot[c->slot] = NULL;
	  c->slot = -1;

	  /* Shuffle up all the icons to the right of this one.  */
	  for (; slot < (NR_SLOTS - 1); slot++)
	    {
	      if (class_slot[slot + 1] != NULL && class_slot[slot + 1] != (void *)1)
		{
		  struct class_record *cc = class_slot[slot + 1];
		  class_slot[slot] = cc;
		  class_slot[slot + 1] = NULL;
		  cc->slot = slot;
		  gtk_fixed_move (GTK_FIXED (box), cc->widget, (slot + 2) * SLOT_WIDTH, 0);
		}
	    }

	  slot_is_free = TRUE;
	}
      else
	other_classes = g_list_remove (other_classes, c);

      if (other_classes)
	{
	  if (slot_is_free)
	    {
	      /* A slot became empty.  Since the "other" button was showing, the empty
		 slot must be the last one before that button.  Put a new button in it.  */
	      struct class_record *cc = other_classes->data;

	      assert (class_slot[NR_SLOTS - 2] == NULL);

	      other_classes = g_list_remove (other_classes, cc);

	      cc->widget = build_icon_for_class (cc);
	      gtk_widget_show (cc->widget);
	      cc->slot = NR_SLOTS - 2;
	      gtk_fixed_put (GTK_FIXED (box), cc->widget, (cc->slot + 2) * SLOT_WIDTH, 0);
	      class_slot[cc->slot] = cc;
	    }

	  if (g_list_length (other_classes) == 1)
	    {
	      /* "Other" list is degenerate.  Replace it with a button for the remaining class.  */
	      struct class_record *cc = other_classes->data;
	      g_list_free (other_classes);
	      other_classes = NULL;
	      hide_other_button ();

	      cc->widget = build_icon_for_class (cc);
	      gtk_widget_show (cc->widget);
	      cc->slot = NR_SLOTS - 1;
	      gtk_fixed_put (GTK_FIXED (box), cc->widget, (cc->slot + 2) * SLOT_WIDTH, 0);
	      class_slot[cc->slot] = cc;
	    }
	}
    }

  if (r->icon_name)
    g_free (r->icon_name);
  if (r->exec)
    g_free (r->exec);
  if (r->class)
    g_free (r->class);
  if (r->icon)
    g_object_unref (r->icon);
  g_free (r);
}

static gchar *
gpe_get_wm_icon_name (Display *dpy, Window w)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  gchar *name = NULL;
  int rc;

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, XA_WM_ICON_NAME,
			  0, G_MAXLONG, False, XA_STRING, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop);

  if (gdk_error_trap_pop () || rc != Success)
    return NULL;

  if (nitems)
    {
      name = g_strdup (prop);
      XFree (prop);
    }

  return name;
}

static Window
get_transient_for (Display *dpy, Window w)
{
  Window result = None;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  int rc;

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, XA_WM_TRANSIENT_FOR,
			  0, 1, False, XA_WINDOW, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop);

  if (gdk_error_trap_pop () || rc != Success)
    return None;

  if (prop)
    {
      memcpy (&result, prop, sizeof (result));
      XFree (prop);
    }
  return result;
}

static gboolean
is_override_redirect (Display *dpy, Window w)
{
  XWindowAttributes attr;

  gdk_error_trap_push ();
  
  XGetWindowAttributes (dpy, w, &attr);
  
  if (gdk_error_trap_pop ())
    return FALSE;

  return attr.override_redirect ? TRUE : FALSE;
}

static gboolean
is_viewable (Display *dpy, Window w)
{
  Window active_w = None;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  int rc;

  rc = XGetWindowProperty (dpy, DefaultRootWindow (dpy), atoms[_NET_ACTIVE_WINDOW],
			  0, 1, False, XA_WINDOW, &actual_type, &actual_format,
			  &nitems, &bytes_after, &prop);

  if (rc != Success)
    return FALSE;

  if (prop)
    {
      memcpy (&active_w, prop, sizeof (active_w));
      XFree (prop);
    }

  return active_w == w ? TRUE : FALSE;
}

static void
window_added (GPEWindowList *list, Window w)
{
  struct window_record *r;
  GtkWidget *widget;
  Atom type;
  Window leader;
  gchar *class;

  type = gpe_get_window_property (dpy, w, atoms[_NET_WM_WINDOW_TYPE]);
  if (type == atoms[_NET_WM_WINDOW_TYPE_DOCK] || type == atoms[_NET_WM_WINDOW_TYPE_DESKTOP])
    return;

  if (gpe_get_window_property (dpy, w, atoms[_NET_WM_STATE_HIDDEN]) != None)
    return;

  if (gpe_get_window_property (dpy, w, atoms[_NET_WM_SKIP_PAGER]) != None)
    return;

  if (get_transient_for (dpy, w) != None)
    return;

  if (is_override_redirect (dpy, w))
    return;

  if (gpe_get_wm_class (dpy, w, NULL, &class) == FALSE)
    return;

  if (!strcmp (class, "Gpe-filemanager-docs"))
    {
      /* treat this one specially */
      g_free (class);
      return;
    }

  leader = gpe_get_wm_leader (dpy, w);
  if (leader == None)
    leader = w;

  r = g_malloc0 (sizeof (*r));
  r->w = w;
  r->leader = leader;
  r->icon_name = gpe_get_wm_icon_name (dpy, w);
  if (r->icon_name == NULL)
    r->icon_name = gpe_get_wm_icon_name (dpy, leader);
  if (r->icon_name == NULL)
    r->icon_name = gpe_get_window_name (dpy, leader);
  if (r->icon_name == NULL)
    r->icon_name = gpe_get_window_name (dpy, w);
  if (r->icon_name == NULL)
    r->icon_name = gpe_get_window_name (dpy, leader);
  if (r->icon_name == NULL)
    r->icon_name = g_strdup ("?");
  r->icon = gpe_get_window_icon (dpy, w);
  if (r->icon == NULL)
    r->icon = other_icon;
  r->exec = gpe_launch_get_binary_for_window (dpy, w);
  r->class = class;

  add_window_record (r);

  windows = g_list_append (windows, r);
}

struct timeout_record
{
  GPEWindowList *list;
  Window w;
};

static gboolean
timeout_func (void *p)
{
  struct timeout_record *d = (struct timeout_record *)p;
  window_added (d->list, d->w);
  g_free (d);
  return FALSE;
}

static void
window_added_callback (GPEWindowList *list, Window w)
{
  struct timeout_record *d;
  d = g_malloc (sizeof (*d));
  d->list = list;
  d->w = w;
  g_timeout_add (250, timeout_func, d);
}

static void
window_removed (GPEWindowList *list, Window w)
{
  GList *l;

  for (l = windows; l; l = l->next)
    {
      struct window_record *r = l->data;

      if (r->w == w)
	{
	  delete_window_record (r);
	  windows = g_list_remove_link (windows, l);
	  g_list_free (l);
	  break;
	}
    }
}

void
add_initial_windows (GPEWindowList *list)
{
  GList *l;

  l = gpe_window_list_get_clients (list);

  while (l)
    {
      window_added (list, (Window)l->data);
      l = l->next;
    }
}

gboolean
add_fixed_button (gchar *name, gchar *icon, gchar *exec, gchar *class)
{
  GdkPixbuf *pix;
  gchar *pix_fn;
  int i;
  int slot = -1;
  struct class_record *c;

  if (!name || !icon || !exec)
    return FALSE;

  for (i = 0; i < NR_SLOTS; i++)
    {
      if (class_slot[i] == NULL)
	{
	  slot = i;
	  break;
	}
    }

  if (slot == -1)
    {
      fprintf (stderr, "no free slots available\n");
      return FALSE;
    }

  pix_fn = g_strdup_printf (PREFIX "/share/gpe/overrides/%s", icon);
  if (access (pix_fn, R_OK) != 0)
    {
      g_free (pix_fn);
      pix_fn = g_strdup_printf (PREFIX "/share/pixmaps/%s", icon);
    }
  pix = gdk_pixbuf_new_from_file (pix_fn, NULL);
  if (pix == NULL)
    {
      fprintf (stderr, "couldn't load icon \"%s\"\n", pix_fn);
      g_free (pix_fn);
      return FALSE;
    }

  g_free (pix_fn);

  c = g_malloc0 (sizeof (*c));
  class_slot[slot] = c;
  c->slot = slot;
  c->widget = do_build_icon (name, pix, FALSE);
  c->exec = exec;
  c->name = class;
  c->fixed = TRUE;
  classes = g_list_append (classes, c);

  g_signal_connect (G_OBJECT (c->widget), "button_press_event", G_CALLBACK (button_press_event), c);
  g_signal_connect (G_OBJECT (c->widget), "button_release_event", G_CALLBACK (button_release_event), c);
  gtk_fixed_put (GTK_FIXED (box), c->widget, (slot + 2) * SLOT_WIDTH, 0);
  gtk_widget_show (c->widget);

  return TRUE;
}

void
load_one_preset (gchar *name)
{
  gchar *path;
  GnomeDesktopFile *d;
  GError *error = NULL;

  path = g_strdup_printf (PREFIX "/share/gpe/overrides/%s.desktop", name);
  if (access (path, R_OK) != 0)
    {
      g_free (path);
      path = g_strdup_printf (PREFIX "/share/applications/%s.desktop", name);
    }
  d = gnome_desktop_file_load (path, &error);
  if (d)
    {
      gchar *name = NULL;
      gchar *icon = NULL;
      gchar *exec = NULL;
      gchar *class = NULL;

      gnome_desktop_file_get_string (d, NULL, "Name", &name);
      gnome_desktop_file_get_string (d, NULL, "Icon", &icon);
      gnome_desktop_file_get_string (d, NULL, "Exec", &exec);
      gnome_desktop_file_get_string (d, NULL, "X-GPE-Class", &class);
      
      gnome_desktop_file_free (d);

      if (add_fixed_button (name, icon, exec, class) == FALSE)
	{
	  if (name)
	    g_free (name);
	  if (icon)
	    g_free (icon);
	  if (exec)
	    g_free (exec);
	  if (class)
	    g_free (class);
	}
    }
  else
    {
      fprintf (stderr, "couldn't read \"%s\": %s\n", path, error ? error->message : "");
    }
  if (error)
    g_error_free (error);
  g_free (path);
}

void
load_presets (void)
{
  FILE *fp;
  gchar *fn;

  fn = g_strdup_printf ("%s/.gpe/button-box.apps", g_get_home_dir ());
  fp = fopen (fn, "r");
  if (!fp)
    fp = fopen ("/etc/gpe/button-box.apps", "r");
  if (fp)
    {
      char buf[256];
      while (fgets (buf, 255, fp))
	{
	  if (buf[0] != 0 && buf[0] != '\n')
	    {
	      buf[strlen(buf)-1] = 0;
	      load_one_preset (buf);
	    }
	}
      fclose (fp);
    }
  g_free (fn);
}

void
re_exec (void)
{
  execvp (g_argv[0], g_argv);
}

gboolean signalled;

gboolean
source_prepare (GSource *source, gint *timeout)
{
  *timeout = -1;
  return signalled;
}

gboolean
source_check (GSource *source)
{
  return signalled;
}

gboolean
source_dispatch (GSource    *source,
		 GSourceFunc callback,
		 gpointer    user_data)
{
  signalled = FALSE;

  re_exec ();

  return TRUE;
}

GSourceFuncs
source_funcs = 
  {
    source_prepare,
    source_check,
    source_dispatch,
    NULL
  };

static void 
catch_signal (int signo)
{
  signalled = TRUE;
}

int
main (int argc, char *argv[])
{
  GObject *list;
  GtkWidget *desktop_button, *docs_button;
  GdkPixbuf *desk_icon, *docs_icon;
  gboolean flag_panel = FALSE, flag_tray = FALSE;
  gchar *geometry = NULL;
  GSource *source;
  
  g_argv = argv;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  while (1)
    {
      int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      int c;

      static struct option long_options[] = {
	{"panel", 0, 0, 'p'},
	{"geometry", 1, 0, 'g'},
	{0, 0, 0, 0}
      };
      
      c = getopt_long (argc, argv, "ptg:", long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 'p':
	  flag_panel = TRUE;
	  break;
	case 't':
	  flag_tray = TRUE;
	  break;
	case 'g':
	  geometry = optarg;
	  break;
	}
    }

  if (flag_panel && flag_tray)
    {
      fprintf (stderr, "Can't mix -p and -t\n");
      exit (1);
    }

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  if (flag_panel)
    window = gtk_plug_new (0);
  else
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_widget_set_name (window, "gpe-buttonbox");

  box = gtk_fixed_new ();

  list = gpe_window_list_new (gdk_screen_get_default ());
  dpy = gdk_x11_get_default_xdisplay ();

  XInternAtoms (dpy, atom_names, 12, False, atoms);

  g_signal_connect (G_OBJECT (list), "window-added", G_CALLBACK (window_added_callback), NULL);
  g_signal_connect (G_OBJECT (list), "window-removed", G_CALLBACK (window_removed), NULL);

  desk_icon = gpe_find_icon ("desktop");
  desktop_button = do_build_icon ("Desk", desk_icon, TRUE);
  gtk_widget_set_name (desktop_button, "Desk");
  gtk_fixed_put (GTK_FIXED (box), desktop_button, 0, 0);
  g_signal_connect (G_OBJECT (desktop_button), "button_press_event", G_CALLBACK (button_press_event), NULL);
  g_signal_connect (G_OBJECT (desktop_button), "button_release_event", G_CALLBACK (desktop_button_release), NULL);
  gtk_widget_show (desktop_button);

  docs_icon = gpe_find_icon ("docs");
  docs_button = do_build_icon ("Docs", docs_icon, TRUE);
  gtk_widget_set_name (docs_button, "Docs");
  gtk_fixed_put (GTK_FIXED (box), docs_button, SLOT_WIDTH, 0);
  g_signal_connect (G_OBJECT (docs_button), "button_press_event", G_CALLBACK (button_press_event), NULL);
  g_signal_connect (G_OBJECT (docs_button), "button_release_event", G_CALLBACK (docs_button_release), NULL);
  gtk_widget_show (docs_button);

  other_icon = gpe_find_icon ("other");
  other_button = do_build_icon ("Other", other_icon, FALSE);
  g_signal_connect (G_OBJECT (other_button), "button_press_event", G_CALLBACK (button_press_event), NULL);
  g_signal_connect (G_OBJECT (other_button), "button_release_event", G_CALLBACK (other_button_release), NULL);
  g_object_ref (other_button);

  load_presets ();

  gpe_launch_monitor_display (dpy);

  add_initial_windows (GPE_WINDOW_LIST (list));

  gtk_widget_set_usize (box, (NR_SLOTS + 2) * SLOT_WIDTH, -1);

  gtk_container_set_border_width (GTK_CONTAINER (window), 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show (box);
  gtk_widget_show (window);

  if (geometry)
    gtk_window_parse_geometry (GTK_WINDOW (window), geometry);

  if (flag_panel)
    {
      gtk_widget_realize (window);
      gtk_window_move (GTK_WINDOW (window), -1, 0);
      gpe_system_tray_dock (window->window);
    }

  if (flag_tray)
    {
      gtk_widget_realize (window);
      if (system_tray_init (dpy, GDK_WINDOW_XWINDOW (window->window)) == FALSE)
	flag_tray = FALSE;
    }
  
  /* So we don't keep 'defunct' processes in the process table */
  signal (SIGCHLD, SIG_IGN);

  signal (SIGHUP, catch_signal);
  source = g_source_new (&source_funcs, sizeof (GSource));
  g_source_attach (source, NULL);

  gpe_launch_install_filter ();
  XSelectInput (dpy, DefaultRootWindow (dpy), PropertyChangeMask | StructureNotifyMask);

  gtk_main ();

  return 0;
}
