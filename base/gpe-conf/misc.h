#ifndef CONF_MISC_H
#define CONF_MISC_H
/*
 * Miscellaneous functions for gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */


int is_network_up();

GtkWidget*
gpe_create_pixmap                      (GtkWidget       *widget,
                                        const gchar     *filename,
					const guint      pxwidth,
					const guint      pxheight);

char
*gpe_dirname (char *s);

#endif
