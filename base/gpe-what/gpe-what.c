/*
 * Copyright (C) 2002 - 2006 Philip Blundell <philb@gnu.org>
 *               2005, 2006 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/spacing.h>
#include <gpe/tray.h>
#include <gpe/popup.h>


#define _(x) gettext(x)

struct gpe_icon my_icons[] = {
  {"help", PREFIX "/share/pixmaps/gpe-what.png"},
  {"help-active", PREFIX "/share/pixmaps/gpe-what-active.png"},
  {NULL}
};

typedef enum
{
  HS_INACTIVE,
  HS_SELECT,
  HS_FINALIZE
} t_status;

typedef enum
{
  PU_INFO,
  PU_HELP
} t_popup;

static GtkWidget *icon;
static GtkWidget *popup = NULL;
static GtkWidget *window;
static GdkWindow *dock_window;
static t_status help_status = HS_INACTIVE;
static GdkDisplay *display;
static GdkAtom help_atom, infoprint_atom;

static Display *dpy;
static int screen;
static guint timeout_id = 0;
static int last_x = 0, last_y = 0;
static int arc_size = 16;

#define HELP_TIMEOUT  4000
#define INFO_TIMEOUT  3000


static gboolean
close_popup (GtkWidget *mypopup)
{
  if (GTK_IS_WIDGET (mypopup))
    {
      gtk_widget_destroy (mypopup);
      popup = NULL;
      timeout_id = 0;
    }
  return FALSE;
}

static gboolean
handle_size_allocate (GtkWidget *w, GtkAllocation *a)
{
  GdkBitmap *bitmap;
  GdkGC *zero_gc, *one_gc;
  GdkColor zero, one;
  int x0, y0, x1, y1;
  int width, height;

  width = a->width;
  height = a->height;

  x0 = y0 = 0;
  x1 = width - 1;
  y1 = height - 1;

  bitmap = gdk_pixmap_new (NULL, width, height, 1);
  
  zero_gc = gdk_gc_new (bitmap);
  one_gc = gdk_gc_new (bitmap);

  zero.pixel = 0;
  one.pixel = 1;

  gdk_gc_set_foreground (zero_gc, &zero);
  gdk_gc_set_foreground (one_gc, &one);

  gdk_draw_rectangle (bitmap, zero_gc, TRUE, 0, 0, width, height);

  /* North-west corner */
  gdk_draw_arc (bitmap, one_gc, TRUE, x0, y0, arc_size * 2, arc_size * 2, 90*64, 90*64);
  gdk_draw_arc (bitmap, one_gc, FALSE, x0, y0, arc_size * 2, arc_size * 2, 90*64, 90*64);
  /* North-east corner */
  gdk_draw_arc (bitmap, one_gc, TRUE, x1 - arc_size * 2, y0, arc_size * 2, arc_size * 2, 0*64, 90*64);
  gdk_draw_arc (bitmap, one_gc, FALSE, x1 - arc_size * 2, y0, arc_size * 2, arc_size * 2, 0*64, 90*64);
  /* South-west corner */
  gdk_draw_arc (bitmap, one_gc, TRUE, x0, y1 - arc_size * 2, arc_size * 2, arc_size * 2, 180*64, 90*64);
  gdk_draw_arc (bitmap, one_gc, FALSE, x0, y1 - arc_size * 2, arc_size * 2, arc_size * 2, 180*64, 90*64);
  /* South-east corner */
  gdk_draw_arc (bitmap, one_gc, TRUE, x1 - arc_size * 2, y1 - arc_size * 2, arc_size * 2, arc_size * 2, 270*64, 90*64);	
  gdk_draw_arc (bitmap, one_gc, FALSE, x1 - arc_size * 2, y1 - arc_size * 2, arc_size * 2, arc_size * 2, 270*64, 90*64);

  /* Middle */
  gdk_draw_rectangle (bitmap, one_gc, TRUE, x0 + arc_size, y0 + arc_size, width - arc_size * 2, height - arc_size * 2);

  /* Top */
  gdk_draw_rectangle (bitmap, one_gc, TRUE, x0 + arc_size, y0, width - arc_size * 2, arc_size);
  /* Bottom */
  gdk_draw_rectangle (bitmap, one_gc, TRUE, x0 + arc_size, y1 - arc_size, width - arc_size * 2, arc_size + 1);
  /* Left */
  gdk_draw_rectangle (bitmap, one_gc, TRUE, x0, y0 + arc_size, arc_size, height - arc_size * 2);
  /* Right */
  gdk_draw_rectangle (bitmap, one_gc, TRUE, x1 - arc_size, y0 + arc_size, arc_size + 1, height - arc_size * 2);

  g_object_unref (one_gc);
  g_object_unref (zero_gc);

  gtk_widget_shape_combine_mask (w, bitmap, 0, 0);

  g_object_unref (bitmap);
}

static gboolean
handle_expose (GtkWidget *w, GdkEventExpose *ev, GtkWidget *child)
{
  GdkGC *black_gc;
  GdkDrawable *dr;
  int x0, y0, x1, y1;

  black_gc = w->style->black_gc;
  dr = w->window;
  x0 = y0 = 0;
  x1 = w->allocation.width - 1;
  y1 = w->allocation.height - 1;

  /* North-west corner */
  gdk_draw_arc (dr, black_gc, FALSE, x0, y0, arc_size * 2, arc_size * 2, 90*64, 90*64);
  /* North-east corner */
  gdk_draw_arc (dr, black_gc, FALSE, x1 - arc_size * 2, y0, arc_size * 2, arc_size * 2, 0*64, 90*64);
  /* South-west corner */
  gdk_draw_arc (dr, black_gc, FALSE, x0, y1 - arc_size * 2, arc_size * 2, arc_size * 2, 180*64, 90*64);
 /* South-east corner */
  gdk_draw_arc (dr, black_gc, FALSE, x1 - arc_size * 2, y1 - arc_size * 2, arc_size * 2, arc_size * 2, 270*64, 90*64);	

  /* Join them up */
  gdk_draw_line (dr, black_gc, x0 + arc_size, y0, x1 - arc_size, y0);
  gdk_draw_line (dr, black_gc, x0 + arc_size, y1, x1 - arc_size, y1);
  gdk_draw_line (dr, black_gc, x0, y0 + arc_size, x0, y1 - arc_size);
  gdk_draw_line (dr, black_gc, x1, y0 + arc_size, x1, y1 - arc_size);

  /* Draw the contents */
  gtk_container_propagate_expose (GTK_CONTAINER (w), child, ev);

  return TRUE;
}

static void
popup_box (const gchar *text, gint length, gint x, gint y, gint type)
{
  GtkWidget *label, *box, *icon;
  GtkRequisition req;
  GdkGeometry geometry;
  gint spacing = gpe_get_border ();
  gint width = 18;
  gint height = 32;
  gint timeout, xsize, lwidth, iwidth, ysize, lheight, iheight, xpos, ypos;
  PangoLayout *layout;
  GdkColor color;

  if ((text == NULL) || !strlen(text))
    return;      

  /* If there is still an open box, destroy old one. */
  if (popup)
    {
      g_source_remove (timeout_id);
      gtk_widget_destroy(popup);
      popup = NULL;
    }
  
  /* Set icon and timeout according to popup type.*/
  if (type == PU_HELP)
    {      
      icon = gtk_image_new_from_stock(GTK_STOCK_HELP, 
				      GTK_ICON_SIZE_SMALL_TOOLBAR);
      timeout = HELP_TIMEOUT;
    }
  else if (type == PU_INFO)
    {      
      icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, 
				      GTK_ICON_SIZE_SMALL_TOOLBAR);
      timeout = INFO_TIMEOUT;
    }
  else /* unknown type */
    return;

  /* create popup widgets */
  popup = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  label = gtk_label_new (text);
  box = gtk_hbox_new (FALSE, gpe_get_boxspacing());

  gtk_widget_add_events (popup, GDK_BUTTON_PRESS_MASK);
  
  g_signal_connect (G_OBJECT (popup), "expose-event", G_CALLBACK (handle_expose), box);
  g_signal_connect (G_OBJECT (popup), "size-allocate", G_CALLBACK (handle_size_allocate), NULL);
  g_signal_connect (G_OBJECT (popup), "button-press-event", 
                    G_CALLBACK (close_popup), popup);

  /* set popup clour and frame apperance */
  gdk_color_parse ("#F8E784", &color);
  gtk_widget_modify_bg (popup, GTK_STATE_NORMAL, &color);
  
  gtk_window_set_default_size (GTK_WINDOW (popup), width, height);
  gtk_window_set_decorated (GTK_WINDOW (popup), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (popup), GDK_WINDOW_TYPE_HINT_MENU);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (box), spacing);
    
  gtk_box_pack_start_defaults(GTK_BOX(box), icon);
  gtk_box_pack_start_defaults(GTK_BOX(box), label);
  gtk_container_add (GTK_CONTAINER (popup), box);
  gtk_widget_realize (label);
 
  /* Calculate size used by the popup.*/ 
  gtk_label_get_layout_offsets(GTK_LABEL(label), &xsize, &ysize);
  layout = gtk_label_get_layout(GTK_LABEL(label));
  pango_layout_get_pixel_size(layout, &lwidth, &lheight); 
  
  gtk_icon_size_lookup(GTK_ICON_SIZE_SMALL_TOOLBAR, &iwidth, &iheight);
  xsize = xsize + lwidth + 6 + iwidth + gpe_get_boxspacing() + (spacing * 2);
  ysize = ysize + lheight + 6 + iheight;

  /* Place popup on window according to given coordinates. Default to the top 
     right corner if no coordinates given. */
	 
  if (x >= 0)
    {
      if ((gdk_screen_width() < (xsize + x)) && (x > (gdk_screen_width()/2)))
        {
          xpos = x - xsize;
          geometry.max_width = gdk_screen_width () - spacing - (x - xsize);
        }
      else
        {
          xpos = x;
          geometry.max_width = gdk_screen_width () - spacing - x;
        }
      if ((gdk_screen_height() < (ysize + y)) && (y > (gdk_screen_height()/2)))
        {
          ypos = y - ysize;
          geometry.max_height = gdk_screen_height () / 2;
        }
      else
        {
          ypos = y;
          geometry.max_height = gdk_screen_height () / 2;
        }
      gtk_widget_set_uposition (GTK_WIDGET (popup), xpos, ypos);
    }
  else
    {
      gtk_widget_set_uposition (GTK_WIDGET (popup),
				gdk_screen_width () - xsize - spacing, spacing);
      geometry.max_width = gdk_screen_width () - spacing;
      geometry.max_height = gdk_screen_height () / 2;
    }

  gtk_window_set_geometry_hints (GTK_WINDOW(popup), box, &geometry, 
                                 GDK_HINT_MAX_SIZE);
  gtk_widget_show_all (popup);
    
  timeout_id = g_timeout_add (timeout, (GSourceFunc) close_popup, popup);
}

static void
set_image (int sx, int sy)
{
  GdkBitmap *bitmap;
  GdkPixbuf *sbuf, *dbuf;
  int size;

  if (!sx)
    {
      sy = gdk_pixbuf_get_height (gtk_image_get_pixbuf (GTK_IMAGE (icon)));
      sx = gdk_pixbuf_get_width (gtk_image_get_pixbuf (GTK_IMAGE (icon)));
    }

  size = (sx > sy) ? sy : sx;
  sbuf = gpe_find_icon (help_status ? "help-active" : "help");

  dbuf = gdk_pixbuf_scale_simple (sbuf, size, size, GDK_INTERP_HYPER);
  gdk_pixbuf_render_pixmap_and_mask (dbuf, NULL, &bitmap, 60);
  gtk_widget_shape_combine_mask (GTK_WIDGET (window), NULL, 0, 0);
  gtk_widget_shape_combine_mask (GTK_WIDGET (window), bitmap, 0, 0);
  gdk_bitmap_unref (bitmap);
  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), dbuf);
}

static gboolean
handle_click (Window w, Window orig_w, int x, int y)
{
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  XClientMessageEvent event;

  if (XGetWindowProperty(dpy, w, gdk_x11_atom_to_xatom (help_atom), 0, 0, False, 
                         None, &type, &format, &nitems, &bytes_after, &prop) 
                         != Success || type == None)
    {
      Window root, parent, *children;
      unsigned int nchildren;

      XQueryTree (dpy, w, &root, &parent, &children, &nchildren);
      if (children)
        XFree (children);

      if (root != w && root != parent)
        {
          unsigned int px, py, b, d, root_w, root_h;
          Window r;
          XGetGeometry (dpy, w, &r, &px, &py, &root_w, &root_h, &b, &d);

          return handle_click (parent, orig_w, x + px, y + py);
        }

      return FALSE;
    }

  if (prop)
    XFree (prop);

  event.type = ClientMessage;
  event.window = w;
  event.message_type = gdk_x11_atom_to_xatom (help_atom);
  event.format = 32;
  memset (&event.data.b[0], 0, sizeof (event.data.b));
  event.data.l[0] = gdk_x11_drawable_get_xid (dock_window);
  event.data.l[1] = x;
  event.data.l[2] = y;
  event.data.l[3] = gdk_x11_atom_to_xatom (help_atom);

  XSendEvent (dpy, w, False, 0, (XEvent *) & event);

  return TRUE;
}

static Window
find_deepest_window (Display * dpy, Window grandfather, Window parent,
		     int x, int y, int *rx, int *ry)
{
  int dest_x, dest_y;
  Window child;

  XTranslateCoordinates (dpy, grandfather, parent, x, y,
			 &dest_x, &dest_y, &child);

  if (child == None)
    {
      *rx = dest_x;
      *ry = dest_y;

      return parent;
    }

  return find_deepest_window (dpy, parent, child, dest_x, dest_y, rx, ry);
}

gboolean
configure_event (GtkWidget * window, GdkEventConfigure * event,
		 GdkBitmap * bitmap_)
{
  if (event->type == GDK_CONFIGURE)
    set_image (event->width, event->height);

  return FALSE;
}

void
released (GtkWidget * w, GdkEventButton *b, gpointer userdata)
{
  if (help_status == HS_FINALIZE)
    {
      XUngrabPointer (dpy, b->time);
      help_status = HS_INACTIVE;
      set_image (0, 0);
    }
}

void
clicked (GtkWidget * w, GdkEventButton *b, gpointer userdata)
{
  if (help_status == HS_SELECT)
    {
      Window w;
      int x, y;

      w = find_deepest_window (dpy, DefaultRootWindow (dpy), DefaultRootWindow (dpy),
			       b->x_root, b->y_root, &x, &y);
      
      last_x = b->x_root;
      last_y = b->y_root;
      
      if (handle_click (w, w, x, y) == FALSE)
        popup_box (_("No help available here."), -1, b->x_root, b->y_root, PU_HELP);

      help_status = HS_FINALIZE;
    }
  else
    {
      XGrabPointer (dpy, GDK_WINDOW_XWINDOW (w->window), False, 
            ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
		    None, None, b->time);
      help_status = HS_SELECT;
    }

  set_image (0, 0);
}

void
setup_xdisplay (void)
{
  dpy = GDK_DISPLAY_XDISPLAY (display);
  screen = DefaultScreen (dpy);

  XSetSelectionOwner (dpy, gdk_x11_atom_to_xatom (infoprint_atom),
		      GDK_DRAWABLE_XID (window->window), CurrentTime);
}

GdkFilterReturn
do_help_message (GdkXEvent * xevent, GdkEvent * event, gpointer data)
{
  unsigned char *prop;
  unsigned long nitems, bytes_after;
  Atom type;
  int format;

  if (XGetWindowProperty (dpy, gdk_x11_drawable_get_xid (dock_window),
       gdk_x11_atom_to_xatom (help_atom), 0, 65536, False, XA_STRING, &type,
       &format, &nitems, &bytes_after, &prop) == Success)
    {
      popup_box (prop, nitems, last_x, last_y, PU_HELP);
      XFree (prop);
    }

  return GDK_FILTER_REMOVE;
}

GdkFilterReturn
do_infoprint_message (GdkXEvent * xevent, GdkEvent * event, gpointer data)
{
  unsigned char *prop;
  unsigned long nitems, bytes_after;
  Atom type;
  int format;

  if (XGetWindowProperty (dpy, gdk_x11_drawable_get_xid (dock_window),
			  gdk_x11_atom_to_xatom (infoprint_atom), 0, 65536,
			  False, XA_STRING, &type, &format, &nitems,
			  &bytes_after, &prop) == Success)
    {
      popup_box (prop, nitems, -1, -1, PU_INFO);
      XFree (prop);
    }

  return GDK_FILTER_REMOVE;
}

gboolean
on_property_change (GtkWidget * widget, GdkEventProperty * event,
		    gpointer user_data)
{
  if (event->atom == help_atom)
    do_help_message (NULL, (GdkEvent *) event, NULL);
  else if (event->atom == infoprint_atom)
    do_infoprint_message (NULL, (GdkEvent *) event, NULL);
  
  return FALSE;
}

GdkFilterReturn
on_event (XEvent *xevent, GdkEvent * event,
		    gpointer user_data)
{
	if (xevent->type == PropertyNotify)
	{
		if (xevent->xproperty.atom == gdk_x11_atom_to_xatom(infoprint_atom))
			do_infoprint_message(NULL, NULL, NULL);
		if ((xevent->xproperty.atom == gdk_x11_atom_to_xatom(help_atom)) 
             && (xevent->xproperty.state == PropertyNewValue) 
             && (xevent->xproperty.window == gdk_x11_drawable_get_xid (dock_window)))
			do_help_message(NULL, NULL, NULL);
        return GDK_FILTER_REMOVE;
	}
	
  return GDK_FILTER_CONTINUE;
}

int
main (int argc, char *argv[])
{
  GtkTooltips *tooltips;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);


  window = gtk_plug_new (0);
  gtk_widget_realize (window);

  gtk_window_set_title (GTK_WINDOW (window), _("Interactive help"));

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon ("help"));
  gtk_widget_show (icon);

  gpe_set_window_icon (window, "help");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window,
			_("This is the interactive help button. "
			  " Tap here and then on another icon to get help."),
			NULL);

  g_signal_connect (G_OBJECT (window), "configure-event",
		    G_CALLBACK (configure_event), NULL);
  g_signal_connect (G_OBJECT (window), "button-press-event",
		    G_CALLBACK (clicked), NULL);
  g_signal_connect (G_OBJECT (window), "button-release-event",
		    G_CALLBACK (released), NULL);
  gtk_widget_add_events (window,
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_PROPERTY_CHANGE_MASK |
			 GDK_STRUCTURE_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  gtk_widget_show (window);

  dock_window = window->window;
  gdk_window_set_events (dock_window, gdk_window_get_events(dock_window) | 
			 GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
			 GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK |
			 GDK_BUTTON_RELEASE_MASK);
  gpe_system_tray_dock (dock_window);


  display = gdk_display_get_default ();
  help_atom = gdk_atom_intern ("_GPE_INTERACTIVE_HELP", FALSE);
  infoprint_atom = gdk_atom_intern ("_GPE_INFOPRINT", FALSE);
  
  gdk_property_change (dock_window, help_atom,
		       gdk_x11_xatom_to_atom_for_display (display, XA_STRING),
		       8, GDK_PROP_MODE_REPLACE, NULL, 0);

  g_signal_connect (G_OBJECT (window), "property-notify-event",
		    G_CALLBACK (on_property_change), NULL);
            
//  gdk_window_add_filter (dock_window, on_event, NULL);
  gdk_window_add_filter (NULL, (GdkFilterFunc)on_event, NULL);
  setup_xdisplay ();

  gtk_widget_add_events (window,
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_PROPERTY_CHANGE_MASK |
			 GDK_STRUCTURE_MASK);
  gtk_main ();

  exit (0);
}
