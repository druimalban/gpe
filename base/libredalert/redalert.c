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
	fprintf (f, "%s\n", command);
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

