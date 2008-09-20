/*
   Copyright (C) 2006 Alberto García Hierro
        <skyhusker@handhelds.org>
   Copyright (C) 2007, 2008 Neal H. Walfield <neal@walfield.org>
  
   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */


#include <locale.h>
#include "starling.h"

#include <gtk/gtk.h>
#ifdef ENABLE_GPE
#   include <gpe/init.h>
#endif

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

#ifdef ENABLE_GPE
  gpe_application_init (&argc, &argv);
#else
  gtk_init (&argc, &argv);
#endif

#ifdef IS_HILDON
  osso_context_t *osso_context;

  /* Initialize maemo application */
  osso_context = osso_initialize(APPLICATION_DBUS_SERVICE, "1.0", TRUE, NULL);

  /* Check that initialization was ok */
  if (osso_context == NULL)
    {
      g_critical ("Failed to initialize OSSO context!");
      return OSSO_ERROR;
    }
#endif

  starling_run ();

  gtk_main();

  return 0;
}
