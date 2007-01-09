/* GPE Screenshot
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "glade-utils.h"

static const char *glade_xml_path= PKGDATADIR "/gpe-scap.glade";

GladeXML *
glade_utils_glade_xml_new (const char* elem)
{
  GladeXML *ui = glade_xml_new (glade_xml_path, elem, NULL);
  
  if (!ui)
    {
      g_error ("Failed to get GUI element: %s", elem);
      exit(1);
    }

  return ui;
}

GtkWidget *
glade_utils_glade_xml_get_widget (GladeXML *self,
                          const char *name)
{
  GtkWidget *w = glade_xml_get_widget (self, name);

  if (!w)
    {
      g_error ("Failed to get widget: %s", name);
      exit(1);
    }

  return w;
}
