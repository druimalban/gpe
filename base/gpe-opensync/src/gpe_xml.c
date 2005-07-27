#include "gpe_sync.h"

/* This function converts a IP adress, given in a string
 *  * into an array of char. Should be secure.
 *   */
void parse_ip(char *ip_str, char *ip_char)
{
	int len = strlen(ip_str);
	int i=0, i2=0, bi=0;
	char buf[3]="   ";

	for(i=0; (i<len) && (i2 < 4); i++) {
		if( (ip_str[i] == '.') || (bi > 3) ) {
			ip_char[i2]=atoi(buf);
			i2++;
			bi=0;
			strcpy(buf,"   ");
		}
		else {
			buf[bi]=ip_str[i];
			bi++;
		}
	}
	ip_char[i2]=atoi(buf);
}

/*This just opens the config file and returns the settings in the env */
osync_bool gpe_parse_settings(gpe_environment *env, char *data, int size)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	osync_debug("GPE-SYNC", 4, "start: %s", __func__);

	// Set the defaults
	env->ip[0]=127;
	env->ip[1]=0;
	env->ip[2]=0;
	env->ip[3]=1;
	env->port=6666;
	env->user = (char*)malloc(sizeof(char)*9);
	strcpy(env->user, "gpeuser");
	
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
				parse_ip(str, env->ip);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"handheld_user")) {
				env->user = g_strdup(str);
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
