/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include <string.h>
#include <sys/stat.h>

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/windows.h>

#include "displays.h"

extern void crypt_init (void);

#define _(x) gettext(x)

Atom migrate_atom;
Atom string_atom;
Atom challenge_atom;
GdkWindow *grab_window;

gboolean grabbed;

struct client_window
{
  Display *dpy;
  Window w;
  gchar *name;
  GdkPixbuf *icon;
};

struct client_window *selected_client, *active_client;

extern gchar *sign_challenge (gchar *text, int length, gchar *target);

#define DISPLAY_CHANGE_SUCCESS			0
#define DISPLAY_CHANGE_UNABLE_TO_CONNECT	1
#define DISPLAY_CHANGE_NO_SUCH_SCREEN		2
#define DISPLAY_CHANGE_AUTHENTICATION_BAD	3
#define DISPLAY_CHANGE_INDETERMINATE_ERROR	4

struct gpe_icon my_icons[] = 
  {
    { "icon", PREFIX "/share/pixmaps/teleport.png" },
    { NULL }
  };

static gboolean
send_message (Display *dpy, Window w, char *host, gchar *method, gchar *data)
{
  gchar *buf = g_strdup_printf ("%s %s %s", host, method, data);
  gboolean rc = TRUE;

  gdk_error_trap_push ();
  
  XChangeProperty (dpy, w, migrate_atom, string_atom, 8, PropModeReplace, buf, strlen (buf));
  XFlush (dpy);

  if (gdk_error_trap_pop ())
    rc = FALSE;

  g_free (buf);

  return rc;
}

static gboolean
migrate_to (Display *dpy, Window w, char *host, int display, int screen)
{
  gchar *auth = "NONE";
  gchar *data = NULL;
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  gchar *target;
  gboolean rc = TRUE;

  target = g_strdup_printf ("%s:%d.%d", host, display, screen);

  if (XGetWindowProperty (dpy, w, challenge_atom, 0, 8192, False, string_atom,
			  &type, &format, &nitems, &bytes_after, &prop) == Success
      && type == string_atom && nitems != 0)
    {
      auth = "RSA-SIG";
      data = sign_challenge (prop, nitems, target);
      if (data == NULL)
	{
	  g_free (target);
	  if (prop)
	    XFree (prop);
	  return FALSE;
	}
    }

  if (prop)
    XFree (prop);

  if (send_message (dpy, w, target, auth, data ? data : "") == FALSE)
    rc = FALSE;

  g_free (target);

  if (data)
    g_free (data);

  return rc;
}

static gboolean
can_migrate (Display *dpy, Window w)
{
  Atom actual_type = None;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  gboolean rc = FALSE;

  gdk_error_trap_push ();

  rc = XGetWindowProperty (dpy, w, migrate_atom,
			   0, 0, False, AnyPropertyType, 
			   &actual_type, &actual_format,
			   &nitems, &bytes_after, &prop);

  if (gdk_error_trap_pop ())
    return FALSE;

  if (rc != Success)
    return FALSE;

  if (actual_type != None)
    rc = TRUE;

  if (prop)
    XFree (prop);

  return rc;
}

GSList *
get_clients (Display *dpy)
{
  Window *windows;
  guint nwin, i;
  GSList *clients = NULL;

  gpe_get_client_window_list (dpy, &windows, &nwin);
  for (i = 0; i < nwin; i++)
    {
      Window w = windows[i];
      gchar *name;
      struct client_window *cw;
      GdkPixbuf *icon;
 
      if (! can_migrate (dpy, w))
	continue;

      name = gpe_get_window_name (dpy, w);

      icon = gpe_get_window_icon (dpy, w);

      cw = g_malloc (sizeof (*cw));
      cw->dpy = dpy;
      cw->w = w;
      cw->name = name;
      if (icon)
	{
	  cw->icon = gdk_pixbuf_scale_simple (icon, 16, 16, GDK_INTERP_BILINEAR);
	  gdk_pixbuf_unref (icon);
	}
      else
	cw->icon = NULL;

      clients = g_slist_append (clients, cw);
    }

  return clients;
}

void
go_callback (GtkWidget *button, GtkWidget *the_combo)
{
  GtkWidget *entry = GTK_COMBO (the_combo)->entry;
  gchar *text = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  gchar *colon;
  int display_nr = -1, screen_nr = 0;
  gchar *host = NULL;
  gboolean new_display = TRUE;
  GSList *i;

  colon = strrchr (text, ':');
  if (colon)
    {
      gchar *dot;
      *(colon++) = 0;
      dot = strchr (colon, '.');
      if (dot)
	{
	  *(dot++) = 0;
	  screen_nr = atoi (dot);
	}
      host = g_strdup (text);
      display_nr = atoi (colon);
    }

  g_free (text);

  if (host == NULL || display_nr == -1)
    {
      gpe_error_box (_("Invalid display name"));
      return;
    }

  if (selected_client == NULL)
    {
      gpe_error_box (_("No window is selected"));
      return;
    }

  for (i = displays; i; i = i->next)
    {
      struct display *d = i->data;
      if (strcmp (d->host, host) == 0
	  && d->dpy == display_nr
	  && d->screen == screen_nr)
	new_display = FALSE;
    }

  if (new_display)
    add_display (host, display_nr, screen_nr);

  active_client = selected_client;

  if (migrate_to (selected_client->dpy, selected_client->w,
		  host, display_nr, screen_nr) == FALSE)
    {
      gpe_error_box (_("Unable to send migration request"));
      active_client = NULL;
    }
}

void
note_client_window (GtkWidget *w)
{
  struct client_window *cw = g_object_get_data (G_OBJECT (w), "client_window");
  
  selected_client = cw;
}

void
open_window (GSList *clients)
{
  GtkWidget *window;
  GtkWidget *quit_button;
  GtkWidget *go_button;
  GtkWidget *hbox1, *hbox2;
  GtkWidget *option_menu;
  GtkWidget *display_label;
  GtkWidget *display_combo;
  GtkWidget *client_label;
  GList *strings = NULL;
  GSList *i;
  GtkWidget *menu;

  for (i = displays; i; i = i->next)
    {
      struct display *d = i->data;
      strings = g_list_prepend (strings, d->str);
    }

  window = gtk_dialog_new ();

  gtk_container_set_border_width (GTK_CONTAINER (window), gpe_get_border ());

  hbox1 = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
  display_label = gtk_label_new (_("Display"));
  display_combo = gtk_combo_new ();
  gtk_combo_set_popdown_strings (GTK_COMBO (display_combo), strings);
  gtk_box_pack_start (GTK_BOX (hbox1), display_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1), display_combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox1, FALSE, FALSE, gpe_get_boxspacing ());

  hbox2 = gtk_hbox_new (FALSE, gpe_get_boxspacing ());
  client_label = gtk_label_new (_("Window"));
  option_menu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_box_pack_start (GTK_BOX (hbox2), client_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), option_menu, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox2, FALSE, FALSE, gpe_get_boxspacing ());

  for (i = clients; i; i = i->next)
    {
      struct client_window *cw = i->data;
      GtkWidget *item = gtk_image_menu_item_new_with_label (cw->name);
      if (cw->icon)
	{
	  GtkWidget *image = gtk_image_new_from_pixbuf (cw->icon);
	  gtk_widget_show (image);
	  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	}

      g_object_set_data (G_OBJECT (item), "client_window", cw);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (note_client_window), NULL);
      gtk_widget_show (item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    }

  if (clients)
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), 0);
      selected_client = clients->data;
    }
  else
    {
      gpe_error_box (_("There are no windows on the current display which support migration."));
      exit (0);	
    }

  quit_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  go_button = gtk_button_new_from_stock (GTK_STOCK_EXECUTE);

  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (window)->action_area), quit_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (window)->action_area), go_button, FALSE, FALSE, 0);

  gtk_window_set_title (GTK_WINDOW (window), _("Teleport"));
  gpe_set_window_icon (GTK_WIDGET (window), "icon");

  g_signal_connect (G_OBJECT (quit_button), "clicked", G_CALLBACK (g_main_loop_quit), NULL);
  g_signal_connect (G_OBJECT (go_button), "clicked", G_CALLBACK (go_callback), display_combo);

  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (g_main_loop_quit), NULL);

  gtk_widget_show_all (window);
}

static GdkFilterReturn 
filter_func (GdkXEvent *xev, GdkEvent *ev, gpointer p)
{
  XClientMessageEvent *xc = (XClientMessageEvent *)xev;

  if (active_client
      && xc->data.l[0] == active_client->w
      && xc->display == active_client->dpy)
    {
      int rc = xc->data.l[1];
      
      switch (rc)
	{
	case DISPLAY_CHANGE_SUCCESS:
	  fprintf (stderr, "Display change successful.\n");
	  break;
	case DISPLAY_CHANGE_UNABLE_TO_CONNECT:
	  gpe_error_box (_("Unable to connect to target display"));
	  break;
	case DISPLAY_CHANGE_NO_SUCH_SCREEN:
	  gpe_error_box (_("Specified screen doesn't exist on target display"));
	  break;
	case DISPLAY_CHANGE_AUTHENTICATION_BAD:
	  gpe_error_box (_("Not authorised to migrate this application"));
	  break;
	case DISPLAY_CHANGE_INDETERMINATE_ERROR:
	  gpe_error_box (_("Migration failed"));
	  break;
	}

      active_client = NULL;
    }

  return GDK_FILTER_CONTINUE;
}

int
main (int argc, char *argv[])
{
  Display *dpy;
  const gchar *home_dir;
  gchar *d;
  GSList *clients;
  GdkAtom migrate_gdkatom;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  dpy = GDK_DISPLAY ();
  string_atom = XInternAtom (dpy, "STRING", False);
  migrate_atom = XInternAtom (dpy, "_GPE_DISPLAY_CHANGE", False);
  challenge_atom = XInternAtom (dpy, "_GPE_DISPLAY_CHANGE_RSA_CHALLENGE", False);
  migrate_gdkatom = gdk_atom_intern ("_GPE_DISPLAY_CHANGE", FALSE);

  home_dir = g_get_home_dir ();

  d = g_strdup_printf ("%s/.gpe", home_dir);
  mkdir (d, 0700);
  g_free (d);
  d = g_strdup_printf ("%s/.gpe/migrate", home_dir);
  mkdir (d, 0700);
  g_free (d);

  crypt_init ();

  displays_init ();

  clients = get_clients (dpy);

  XSelectInput (dpy, RootWindow (dpy, DefaultScreen (dpy)), SubstructureNotifyMask);

  gdk_add_client_message_filter (migrate_gdkatom, filter_func, NULL);

  open_window (clients);

  gtk_main ();

  exit (0);
}
