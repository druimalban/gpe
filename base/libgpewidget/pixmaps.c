/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "pixmaps.h"
#include "errorbox.h"
#include "link-warning.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(x) dgettext(PACKAGE, x)
#else
#define _(x) (x)
#endif

#define GPE_THEME_LOCATION      PREFIX "/share/gpe/pixmaps/"
#define DEFAULT_ICON_LOCATION   PREFIX "/share/pixmaps/"
#define SYSTEM_ICON_LOCATION    "/usr/share/pixmaps/"

static GData *pbdata = NULL;

static const gchar *theme_dir_tail = "/.gpe/pixmaps";
#ifdef IS_HILDON
static const gchar *default_theme_dir = "/usr/share/icons/hicolor/26x26";
#else
static const gchar *default_theme_dir = GPE_THEME_LOCATION "default";
#endif

static gchar *theme_dir = NULL;

void 
gpe_icon_free_dynamic (struct gpe_icon *p)
{
  if (p->pixbuf)
    {
      g_object_unref (p->pixbuf);
      p->pixbuf = NULL;
    }
}

static GdkPixbuf *
gpe_load_one_icon (const gchar *filename, gchar **error)
{
  const gchar *pathname;
  gchar buf[1024];
  GdkPixbuf *pb;
  GError *g_error = NULL;
  
  if (filename[0] == '/')
    {
      pathname = filename;
    }
  else
    {
      gboolean found = FALSE;
      if (theme_dir)
        {
          snprintf (buf, sizeof (buf) - 1, "%s/%s.png", theme_dir, filename);
          buf[sizeof (buf) - 1] = 0;
          if (access (buf, R_OK) == 0)
            found = TRUE;
        }
      if (found == FALSE && theme_dir != default_theme_dir)
        {
          snprintf (buf, sizeof (buf) - 1, "%s/%s.png", default_theme_dir, 
                    filename);
          buf[sizeof (buf) - 1] = 0;
          if (access (buf, R_OK) == 0)
            found = TRUE;
        }
      if (found == FALSE)
        {
          snprintf (buf, sizeof (buf) - 1, "%s/%s.png", SYSTEM_ICON_LOCATION, 
                    filename);
          buf[sizeof (buf) - 1] = 0;
          if (access (buf, R_OK) == 0)
            found = TRUE;
        }
      if (found == FALSE)
        {
          snprintf (buf, sizeof (buf) - 1, "%s/%s.png", DEFAULT_ICON_LOCATION, 
                    filename);
          buf[sizeof (buf) - 1] = 0;
        }
      pathname = buf;
    }
  
  pb = gdk_pixbuf_new_from_file (pathname, &g_error);

  if (pb == NULL && error)
    {
      *error = g_strdup (g_error->message);
      g_error_free (g_error);
    }

  return pb;
}

/**
 * gpe_set_theme:
 * @theme_name: Name of the theme to be used for GPE.
 *
 * Set the name of the theme used by GPE functions. This will cause GPE icon 
 * functions to load icons from the given theme instead of the default location
 * (if the icon is present in the selected theme). The name of the theme points
 * GPE to the directory in $PREFIX/share/gpe/pixmaps where to search for the 
 * themed icons. How the actual theme is set and changed needs to be handled
 * by the application.
 * Passing NULL to @theme_name resets the theme to the default/user theme.
 */
void
gpe_set_theme (const gchar *theme_name)
{
  if (theme_dir != NULL)
      g_free (theme_dir);
  
  if (theme_name == NULL)
    {
      theme_dir = NULL;
      return;
    }
  theme_dir = g_strdup_printf ("%s%s", GPE_THEME_LOCATION, theme_name);
}

gboolean 
gpe_load_icons (struct gpe_icon *p)
{
  const gchar *home = g_get_home_dir();
  gchar *buf;
  size_t s;
  gboolean ok = TRUE;

  if (home == NULL)
    home = "/";

  if (theme_dir == NULL)
    {
      s = strlen (home) + strlen (theme_dir_tail) + 1;
      buf = alloca (s);
      strcpy (buf, home);
      strcat (buf, theme_dir_tail);
  
      if (access (buf, F_OK) == 0)
        theme_dir = g_strdup (buf);
      else
        theme_dir = g_strdup(default_theme_dir);
    }
    
  if (pbdata == NULL) 
    g_datalist_init (&pbdata);
  else
    g_datalist_clear (&pbdata);
  
  while (p->shortname)
    {
      gchar *error;
      p->pixbuf = gpe_load_one_icon (p->filename ? p->filename : p->shortname,
				     &error);

      if (p->pixbuf == NULL)
        {
          gpe_error_box (error);
          g_free (error);
          ok = FALSE;
        }

      g_datalist_set_data_full (&pbdata, p->shortname, p, gpe_icon_free_dynamic);
      p++;
    }

  return ok;
}

GdkPixbuf *
gpe_try_find_icon (const gchar *name, gchar **error)
{
  struct gpe_icon *p = g_datalist_get_data (&pbdata, name);

  if (p == NULL)
    {
      GdkPixbuf *buf = gpe_load_one_icon (name, error);
      if (buf)
       {
         p = g_malloc (sizeof (struct gpe_icon));
         p->shortname = g_strdup (name);
         p->pixbuf = buf;
         g_datalist_set_data_full (&pbdata, p->shortname, p, gpe_icon_free_dynamic);
       }
    }
  
  return p ? p->pixbuf : NULL;
}

GdkPixbuf *
gpe_find_icon (const gchar *name)
{
  struct gpe_icon *p = g_datalist_get_data (&pbdata, name);

  if (p == NULL)
    {
      gchar *error = g_strdup_printf (_("Icon \"%s\" not loaded."), name);
      gpe_error_box (error);
      g_free (error);
      exit (1);
    }

  return p->pixbuf;
}

GdkPixbuf *
gpe_find_icon_scaled (const gchar *name, GtkIconSize size)
{
  GdkPixbuf *p = gpe_find_icon (name);
  gint width, height;

  if (gtk_icon_size_lookup (size, &width, &height))
    p = gdk_pixbuf_scale_simple (p, width, height, GDK_INTERP_BILINEAR);

  return p;
}

/** 
 * gpe_find_icon_scaled_free:
 * @name: name of the icon to find
 * @width: designated icon width
 * @height: designated icon height
 * 
 * Find an icon by name and scale it to a new size defined by the user.
 * 
 * Returns: New allocated and scaled pixbuf.
 */
GdkPixbuf *
gpe_find_icon_scaled_free (const gchar *name, gint width, gint height)
{
  GdkPixbuf *p = gpe_find_icon (name);

  p = gdk_pixbuf_scale_simple (p, width, height, GDK_INTERP_BILINEAR);

  return p;
}

gboolean
gpe_find_icon_pixmap (const gchar *name, GdkPixmap **pixmap, GdkBitmap **bitmap)
{
  GdkPixbuf *pixbuf = gpe_find_icon (name);
  gdk_pixbuf_render_pixmap_and_mask (pixbuf,
				     pixmap,
				     bitmap,
				     127);
  return TRUE;
}
link_warning(gpe_find_icon_pixmap, "warning: gpe_find_icon_pixmap is obsolescent.");

void
gpe_set_window_icon (GtkWidget *window, gchar *icon)
{
  GdkPixbuf *p =  gpe_find_icon (icon);
  gtk_window_set_icon (GTK_WINDOW (window), p);
}
