/*
 * Copyright (C) Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA. 
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <unistd.h>

#define GPE_HELP_PATH PREFIX "/share/doc/gpe"
#define GPE_HELP_FILE_SUFFIX ".html"
#define GPE_HELP_FILE_PREFIX "file://"
#define GPE_HELP_APP1 "/usr/bin/dillo"
#define GPE_HELP_APP2 "/usr/bin/minimo"

/*
 *	This function provides a generic interface for displaying
 *  full text online help. It is intended to be independent from
 *  file format and location.
 *  Return value is FALSE if help is found and displayed, TRUE
 *  if an error occurs.
 */
gboolean
gpe_show_help(const char* book, const char* topic)
{
	char *helpfile;
	char *helpadress;
	char *app = NULL;
	pid_t p_help;
	
	/* construction of help file name */
	helpfile = g_strdup_printf	("%s/%s%s",
									GPE_HELP_PATH,
									book,
									GPE_HELP_FILE_SUFFIX
								);
	
	/* check if the file is readable */
	if (access(helpfile, R_OK))
		return TRUE;
	
	/* check if we are able to execute one of the displaying applications */
	if (!access(GPE_HELP_APP1, X_OK)) app = GPE_HELP_APP1;
	else if (!access(GPE_HELP_APP2, X_OK)) app = GPE_HELP_APP2;
	
	/* return if no app is available */
	if (app == NULL) 
		return TRUE;
	
	/* construct the complete help address */
	if ((topic) && strlen(topic))
		helpadress = g_strdup_printf("%s%s#%s",
										GPE_HELP_FILE_PREFIX,
										helpfile,
										topic
									);
	else
		helpadress = g_strdup_printf("%s%s",
										GPE_HELP_FILE_PREFIX,
										helpfile
									);
		
	/* fork and exec displaying application */
	p_help = fork();
	switch (p_help)
	{
		case -1: 
			return TRUE;
		break;
		case  0: 
				execlp(app, app, helpadress, NULL);
		break;
		default: 
			g_free(helpadress);
			g_free(helpfile);
			return FALSE;
		break;
	} 
	/* we should never get there, help the compiler - it doesn't know */
	return TRUE;
}
