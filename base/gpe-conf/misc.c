/*
 * Miscellaneous functions for gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#include <gtk/gtk.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <stdio.h>
#include <string.h>

/*
 *  Check if network is up. 
 */
int is_network_up()
{
	FILE *pipe;
	char buffer[256];
	int result = FALSE;
	
	pipe = popen ("/bin/netstat -rn", "r");

	if (pipe > 0)
    {
  		while ((feof(pipe) == 0))
    	{
      		fgets (buffer, 255, pipe);
			if (g_str_has_prefix(buffer,"0.0.0.0") || g_str_has_prefix(buffer,"default"))
			  result = TRUE;
		}
		pclose(pipe);		
	}
	return result;
}

GtkWidget*
gpe_create_pixmap                      (GtkWidget       *widget,
                                        const gchar     *filename,
					const guint      pxwidth,
					const guint      pxheight)
{
  GtkWidget *pixmap;
  GdkPixbuf *icon;
  gchar* err;
  GError *g_error = NULL;
	
  guint width, height;
  /* some "random" initial values: */
  gfloat scale, scale_width = 2.72, scale_height = 3.14;
  guint maxwidth = 32, maxheight = 32;
  
  icon = gpe_try_find_icon (filename, &err);
  if (!icon) { 
	  g_free(err);
      icon = gdk_pixbuf_new_from_file (filename, &g_error);

      if (icon == NULL)
      {
        g_error_free (g_error);
      }
  }	
  width  = gdk_pixbuf_get_width (icon);
  height = gdk_pixbuf_get_height (icon);

  maxwidth  = pxwidth;
  maxheight = pxheight;
 
  if (width > maxwidth)
    scale_width = (gfloat) maxwidth / width;
  else
    scale_width = 1.0;

  if (height > maxheight)
    scale_height = (gfloat) maxheight / height;
  else
    scale_height = 1.0;

  scale = scale_width < scale_height ? scale_width : scale_height;
  scale = scale * 0.90; /* leave some border */
  
  pixmap = gtk_image_new_from_pixbuf(
			    gdk_pixbuf_scale_simple
			    (icon, width * scale, height * scale, GDK_INTERP_BILINEAR));
  
  return pixmap;  
}


/* MacOS X doesn't have dirname()... */
char
*gpe_dirname (char *s) {
  int i;
  for (i=strlen (s); i && s[i]!='/'; i--);
  s[i] = 0;
  return s;
}
