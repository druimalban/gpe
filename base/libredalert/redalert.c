/*
  libredalert - a middle-layer between atd and apps
  Copyright (C) 2002  Robert Mibus <mibus@handhelds.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "redalert.h"
#include <stdio.h>
#include <glib.h>

void redalert_set_alarm (const char *program, guint alarm_id, time_t unixtime, const char *command) {
	FILE *f;
	char *filename;
	filename = g_strdup_printf("/var/spool/at/%d.%s-%lu", (int)unixtime, program, alarm_id);
 
	f=fopen(filename, "w");
	if (!f)
		return; /* FIXME: die horribly */

	fprintf(f, "#!/bin/sh\n");
	// I'm not sure how good an idea this is, but... :
	fprintf(f, "export DISPLAY=:0\n");
	fprintf(f, "%s\n", command);
	fprintf(f, "/bin/rm $0\n");
	fclose(f);

	if (chmod (filename, 0755))
		perror ("chmod");

	f = fopen ("/var/spool/at/trigger", "w");
	if (f) {
		fputc ('\n', f);
		fclose (f);
	}
 
	g_free (filename);
}

void redalert_set_alarm_message (const char *program, guint alarm_id, time_t unixtime, const char *message) {
	char *cmd;
	cmd = g_strdup_printf ("/usr/bin/gpe-announce '%s'", message);
	redalert_set_alarm (program, alarm_id, unixtime, cmd);
	g_free (cmd);
}

void redalert_delete_alarm (char *program, int alarm_id) {
	char *cmd;
	cmd = g_strdup_printf ("rm -f /var/spool/at/*.%s-%d", program, alarm_id);
	system (cmd);
	g_free (cmd);
}

