/*
   Copyright (C) 2006 Alberto García Hierro
        <skyhusker@handhelds.org>
   Copyright (C) 2007, 2008, 2009 Neal H. Walfield <neal@walfield.org>
  
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

#include "starling.h"
#include "config.h"

#include <locale.h>
#include <string.h>

#include <handoff.h>

#include <gtk/gtk.h>
#ifdef ENABLE_GPE
#   include <gpe/init.h>
#endif

#if HAVE_HILDON
/* Hildon includes */
# if HAVE_HILDON_VERSION > 0
#  include <hildon/hildon-program.h>
#  include <hildon/hildon-window.h>
#  include <hildon/hildon-file-chooser-dialog.h>
# else
#  include <hildon-widgets/hildon-app.h>
#  include <hildon-widgets/hildon-appview.h>
#  include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
# endif /* HAVE_HILDON_VERSION */

# include <libosso.h>
# define APPLICATION_DBUS_SERVICE "starling"
#endif /* HAVE_HILDON */

static Starling *st;

/* Another instance started and has passed some state to us.  */
static void
handoff_callback (Handoff *handoff, char *data)
{
  char *line = data;
  while (line && *line)
    {
      char *var = line;

      char *end = strchr (line, '\n');
      if (! end)
        {
          end = line + strlen (line);
          line = 0;
        }
      else
        line = end + 1;
      *end = 0;

      char *equal = strchr (var, '=');
      if (equal)
        *equal = 0;

      char *value;
      if (equal)
        value = equal + 1;
      else
        value = NULL;

      if (strcmp (var, "FOCUS") == 0)
	starling_come_to_front (st);
      else
	g_warning ("%s: Unknown command: %s", __func__, var);
    }
}

/* Serialize our state: another instance will take over (e.g. on
   another display).  */
static char *
handoff_serialize (Handoff *handoff)
{
  starling_quit (st);
  return NULL;
}

#ifdef HAVE_HILDON
static void
osso_top_callback (const gchar *arguments, gpointer data)
{
  handoff_callback (NULL, arguments);
}
#endif

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (! g_thread_supported ())
    g_thread_init (NULL);
  gdk_threads_init ();

#ifdef ENABLE_GPE
  gpe_application_init (&argc, &argv);
#else
  gtk_init (&argc, &argv);
#endif

  /* See if there is another instance of Starling already running.  If
     so, try to handoff any arguments and exit.  Otherwise, take
     over.  */
  Handoff *handoff = handoff_new ();

#define RENDEZ_VOUS CONFIGDIR "/rendezvous"
  const char *home = g_get_home_dir ();
  char *rendez_vous = alloca (strlen (home) + strlen (RENDEZ_VOUS) + 1);
  sprintf (rendez_vous, "%s/" RENDEZ_VOUS, home);

  g_signal_connect (G_OBJECT (handoff), "handoff",
                    G_CALLBACK (handoff_callback), NULL);

  if (handoff_handoff (handoff, rendez_vous, "FOCUS", TRUE,
		       handoff_serialize, NULL))
    exit (0);


#if HAVE_HILDON
  osso_context_t *osso_context;

  /* Initialize maemo application */
  osso_context = osso_initialize(APPLICATION_DBUS_SERVICE, "1.0", TRUE, NULL);

  /* Check that initialization was ok */
  if (osso_context == NULL)
    {
      g_critical ("Failed to initialize OSSO context!");
      return OSSO_ERROR;
    }

  osso_application_set_top_cb (osso_context, osso_top_callback, NULL);
#endif

  st = starling_run ();

  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();

  return 0;
}
