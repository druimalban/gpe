/* GPE Mileage
 * Copyright (C) 2004  Rene Wagner <rw@handhelds.org>
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

/* main.c:
 *  - command args parsing, 
 *  - initialization (gtk, glade),
 *  - cleanup
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include "main_window.h"
#include "preferences.h"

int
main (int argc, char **argv)
{
  bindtextdomain (GETTEXT_PACKAGE, GPEMILEAGELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);

  /* TODO: parse arguments */

  preferences_init ();

  /* open database.  */
  mileage_db_open();

  main_window_init ();

  /* Enter main loop */
  gtk_main ();

  /* close database.  */
  mileage_db_close();

  preferences_shutdown ();

  return 0;
}
