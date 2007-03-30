/* Dump a vCard file
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: vcard-dump.c 239 2005-11-03 16:39:55Z srittau $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <locale.h>
#include <libintl.h>
#include <glib.h>

#include <mimedir/mimedir-init.h>
#include <mimedir/mimedir-vcard.h>


#ifndef _
#define _(x) dgettext(GETTEXT_PACKAGE, (x))
#endif


static void
usage (const char *program_name)
{
	fprintf (stderr, _("Usage: %s VCARDFILE\n"), program_name);
}


static void
print_vcard (MIMEDirVCard *vcard)
{
	gchar *s, *s_dec;

	s = mimedir_vcard_get_as_string (vcard);
	s_dec = g_locale_from_utf8 (s, -1, NULL, NULL, NULL);

	g_assert (s_dec != NULL);

	printf ("%s", s_dec);

	g_free (s_dec);
	g_free (s);
}


int
main (int argc, char **argv)
{
	GError *error = NULL;
	GList *cardlist, *l;

	/* I18n setup */

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GLOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Glib setup */

	mimedir_init ();

	/* Argument handling */

	if (argc != 2) {
		fprintf (stderr, _("%s: invalid number of arguments\n"),
			 argv[0]);
		usage (argv[0]);
		return 1;
	}

	/* Read the card list */

	cardlist = mimedir_vcard_read_file (argv[1], &error);

	if (error) {
		fprintf (stderr, "%s: %s\n",
			 argv[0], error->message);
		g_error_free (error);
		return 1;
	}

	/* Print the cards */

	for (l = cardlist; l != NULL; l = g_list_next (l)) {
		g_assert (l->data != NULL && MIMEDIR_IS_VCARD (l->data));

		print_vcard (MIMEDIR_VCARD (l->data));

		if (l->next)
			printf ("\n===\n");
	}

	/* Cleanup */

	mimedir_vcard_free_list (cardlist);

	return 0;
}
