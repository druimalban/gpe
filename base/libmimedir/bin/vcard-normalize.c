/* Normalize a vCard file
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: vcard-normalize.c 176 2005-02-26 22:46:04Z srittau $
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


int
main (int argc, char **argv)
{
	GError *error = NULL;
	GList *cardlist, *l;
	GIOChannel *channel;

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
		fprintf (stderr, _("%s: error reading card file: %s\n"),
			 argv[0], error->message);
		g_error_free (error);
		return 1;
	}

	/* Normalize the cards */

	channel = g_io_channel_unix_new (1);

	for (l = cardlist; l != NULL; l = g_list_next (l)) {
		MIMEDirVCard *vcard;

		g_assert (l->data != NULL && MIMEDIR_IS_VCARD (l->data));
		vcard = MIMEDIR_VCARD (l->data);

		if (!mimedir_vcard_write_to_channel (vcard, channel, &error)) {
			fprintf (stderr, _("%s: error writing card file: %s\n"),
				 argv[0], error->message);
			g_error_free (error);
		}
	}

	g_io_channel_unref (channel);

	/* Cleanup */

	mimedir_vcard_free_list (cardlist);

	return 0;
}
