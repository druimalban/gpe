#include "redalert.h"
#include <stdio.h>
#include <glib.h>

void redalert_set_alarm (time_t unixtime, char *program) {
	FILE *f;
	char *filename;
	char *command;
	filename = g_strdup_printf("/var/spool/at/%d.1234", (int)unixtime);
 
	f=fopen(filename, "w");
	if (!f)
		return; /* FIXME: die horribly */

	fprintf(f, "#!/bin/sh\n");
	fprintf (f, "%s\n", program);
	fprintf(f, "/bin/rm $0\n");
	fclose(f);
 
	command = g_strdup_printf ("chmod 755 %s", filename);
	system(command);
	g_free (command);
 
	system ("echo >/var/spool/at/trigger");
	g_free (filename);

}

void redalert_set_alarm_message (time_t unixtime, char *message) {
	char *cmd;
	cmd = g_strdup_printf ("/usr/bin/gpe-announce '%s'", message);
	redalert_set_alarm (unixtime, cmd);
	g_free (cmd);
}
