/*
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

GtkWidget *Ownerinfo_Build_Objects();
void Ownerinfo_Free_Objects();
void Ownerinfo_Save();
void Ownerinfo_Restore();

gint
maybe_upgrade_datafile ();

gint
upgrade_to_v2 (guint new_version);

static
void File_Selected (char *file,
		    gpointer data);

void
choose_photofile              (GtkWidget     *button,
			       gpointer       user_data);
GtkWidget*
create_pixmap                          (GtkWidget       *widget,
                                        const gchar     *filename);

char
*my_dirname (char *s);
