/*
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define GPE_OWNERINFO_DATA   "/etc/gpe/gpe-ownerinfo.data"
#define GPE_OWNERINFO_TMP   "/tmp/gpe-ownerinfo.data"

GtkWidget *Ownerinfo_Build_Objects();
void Ownerinfo_Free_Objects();
void Ownerinfo_Save();
void Ownerinfo_Restore();

gint
maybe_upgrade_datafile ();

gint
upgrade_to_v2 (guint new_version);

void
choose_photofile              (GtkWidget     *button,
			       gpointer       user_data);
GtkWidget*
create_pixmap                          (GtkWidget       *widget,
                                        const gchar     *filename);

char
*my_dirname (char *s);

char
*my_basename (char *s);
