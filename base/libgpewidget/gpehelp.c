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
	char *helpcommand;
	pid_t p_help;
	
	helpfile = g_strdup_printf	("%s/%s%s",
									GPE_HELP_PATH,
									book,
									GPE_HELP_FILE_SUFFIX
								);
	if (access(helpfile,R_OK))
		return TRUE;
	if ((access(GPE_HELP_APP1,X_OK)) && (access(GPE_HELP_APP2,X_OK)))
		return TRUE;
	
	helpcommand = g_strdup_printf	("%s%s#%s",
										GPE_HELP_FILE_PREFIX,
										helpfile,
										topic
									);
	p_help = fork();
	switch (p_help)
	{
		case -1: 
			return TRUE;
		break;
		case  0: 
			execlp(GPE_HELP_APP1,helpcommand,NULL);
		break;
		default: 
			g_free(helpcommand);
			g_free(helpfile);
			return FALSE;
		break;
	} 
}
