/*
 * gpe-sync - A plugin for the opensync framework
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#include "gpe_sync.h"

/*This just opens the config file and returns the settings in the env */
osync_bool gpe_parse_settings(gpe_environment *env, char *data, int size)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	osync_debug("GPE-SYNC", 4, "start: %s", __func__);

	// Set the defaults
	env->device_addr = (char*)malloc(sizeof(char)*10);
	strcpy(env->device_addr, "127.0.0.1");
	env->username = (char*)malloc(sizeof(char)*9);
	strcpy(env->username, "gpeuser");
	
	doc = xmlParseMemory(data, size);
	
	if(!doc) {
		osync_debug("GPE-SYNC", 1, "Could not parse data!\n");
		return FALSE;
	}
	
	cur = xmlDocGetRootElement(doc);

	if(!cur) {
		osync_debug("GPE-SYNC", 0, "data seems to be empty");
		return FALSE;
	}

	if (xmlStrcmp(cur->name, (xmlChar*)"config")) {
		printf("GPE-SYNC data seems not to be a valid configdata.\n");
		xmlFreeDoc(doc);
		return FALSE;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		char *str = (char*)xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"handheld_ip")) {
				// convert the string to an ip
				env->device_addr = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"handheld_user")) {
				env->username = g_strdup(str);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return TRUE;
}
