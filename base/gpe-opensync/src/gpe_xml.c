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
	env->port=6666;
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
			if (!xmlStrcmp(cur->name, (const xmlChar *)"handheld_port")) {
				// convert string to an int
				env->port = atoi(str);
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return TRUE;
}
