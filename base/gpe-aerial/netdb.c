/*
 * gpe-aerial (c) 2003 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * Networks database module.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite.h>
#include <glib.h>

#include "netdb.h"
#include "prismstumbler.h"


#define NETDBNAME 			".gpe/netdb"
#define TABLE_NETWORKS		"networks"

#define SQL_CREATE_TABLE	"create table networks (bssid char(32), ssid char(32), mode int, \
							wep int, dhcp int, wep_key char(48), channel int, ip char(16), \
							netmask char(16), gateway char(16), userset int ,primary key(bssid))"
							
#define SQL_CHECK_TABLE		"select * from networks;"
#define SQL_GETALL			"select ssid,channel,wep,mode from networks;"
#define SQL_SELECTBY_BSSID	"select * from networks where bssid = '%q';"
#define SQL_ADD_NET			"insert into networks (bssid,ssid,mode,wep,dhcp,wep_key,channel,ip,netmask,gateway,userset) \
							values('%q','%q',%i,%i,%i,'%q',%i,'%q','%q','%q',%i);"
#define SQL_UPDATE_NET		"update networks set ssid='%q',wep=%i,dhcp=%i,mode=%i,\
							wep_key='%q',channel=%i, ip='%q', netmask='%q', gateway='%q', userset=%i where bssid = '%q';"
#define SQL_DELETEBY_BSSID	"delete from networks where bssid = '%q';"


static sqlite* netdb = NULL;
static char **myresult;

/*
int getall_networks(sqlite_callback fCallback)
{
	char* errmsg = NULL;
	int retval;

	retval = sqlite_exec(netdb,SQL_GETALL,fCallback,NULL,&errmsg);
	
	if (retval){
		printf("Error get networks: %s\n",errmsg);
		free(errmsg);
		return -1;
	}
	return 0;
}
*/

usernetinfo_t* get_network(char* my_bssid)
{
	char* errmsg = NULL;
	int nrow = 0;
	int ncolumn = 0;
	usernetinfo_t* my_net = malloc(sizeof(usernetinfo_t));

	memset(my_net,0,sizeof(usernetinfo_t));
	if (netdb)
	{
		#ifdef DEBUG
		printf("fetching network...\n");
		#endif
		sqlite_get_table_printf(netdb,SQL_SELECTBY_BSSID,&myresult,&nrow,&ncolumn,&errmsg,my_bssid);

		if (nrow > 0) // we found this net in the database
		{
			// get data
			strncpy(my_net->bssid,myresult[ncolumn],32);
			strncpy(my_net->ssid,myresult[ncolumn+1],32);
			my_net->mode = atol(myresult[ncolumn+2]);
			my_net->wep = atol(myresult[ncolumn+3]);
			my_net->dhcp = atol(myresult[ncolumn+4]);
			strncpy(my_net->wep_key,myresult[ncolumn+5],48);
			my_net->channel = atol(myresult[ncolumn+6]);
			sscanf(myresult[ncolumn+7],"%hhu.%hhu.%hhu.%hhu",&my_net->ip[0],&my_net->ip[1],&my_net->ip[2],&my_net->ip[3]);
			sscanf(myresult[ncolumn+8],"%hhu.%hhu.%hhu.%hhu",&my_net->netmask[0],&my_net->netmask[1],&my_net->netmask[2],&my_net->netmask[3]);
			sscanf(myresult[ncolumn+9],"%hhu.%hhu.%hhu.%hhu",&my_net->gateway[0],&my_net->gateway[1],&my_net->gateway[2],&my_net->gateway[3]);
			my_net->userset = atol(myresult[ncolumn+10]);
			
			#ifdef DEBUG
			printf("found: %i c: %i\n",nrow,ncolumn);
			printf("read: %s\n",my_net->bssid);
			#endif
			sqlite_free_table(myresult);
		}
		else
		{
			free(my_net);
			return NULL;
		}

		if (errmsg){
			printf("Error fetch network: %s\n",errmsg);
			free(errmsg);
			free(my_net);
			return NULL;
		}
	}
	return my_net;
}


int save_network(usernetinfo_t* my_net)
{
	char* errmsg = NULL;
	int nrow = 0;
	int ncolumn = 0;
	int ret;
	static char ip[16],netmask[16],gateway[16];
printf("save called...\n");
	
	if (netdb)
	{
		sprintf(ip,"%hhu.%hhu.%hhu.%hhu",my_net->ip[0],my_net->ip[1],my_net->ip[2],my_net->ip[3]);
		sprintf(netmask,"%hhu.%hhu.%hhu.%hhu",my_net->netmask[0],my_net->netmask[1],my_net->netmask[2],my_net->gateway[3]);
		sprintf(gateway,"%hhu.%hhu.%hhu.%hhu",my_net->gateway[0],my_net->gateway[1],my_net->gateway[2],my_net->gateway[3]);
		#ifdef DEBUG
		printf("saving network...");
		#endif
		sqlite_get_table_printf(netdb,SQL_SELECTBY_BSSID,&myresult,&nrow,&ncolumn,&errmsg,my_net->bssid);
		if (nrow > 0) // we found this net in the database
		{
			#ifdef DEBUG
			printf("updating %c\n",my_net.pvec[0]);
			#endif
			sqlite_free_table(myresult);
			// update network
			ret = sqlite_get_table_printf(netdb,SQL_UPDATE_NET,&myresult,&nrow,&ncolumn,&errmsg,
					my_net->ssid,my_net->wep,my_net->dhcp,my_net->mode,my_net->wep_key,my_net->channel,ip,
					netmask,gateway,my_net->userset,my_net->bssid);
			if (ret){
				printf("Error db update: %s\n",errmsg);
				free(errmsg);
				return -1;
			}
		}
		else // new network
		{
			#ifdef DEBUG
			printf("adding\n");
			#endif
			ret = sqlite_get_table_printf(netdb,SQL_ADD_NET,&myresult,&nrow,&ncolumn,&errmsg,my_net->bssid,
					my_net->ssid,my_net->mode, my_net->wep, my_net->dhcp, my_net->wep_key, 
					my_net->channel,ip,netmask, gateway, my_net->userset);
			if (ret){
				printf("Error db adding network: %s\n",errmsg);
				free(errmsg);
				return -1;
			}
			if (myresult) sqlite_free_table(myresult);
		}
	}
	return 0;
}


int check_table()
{
	char* errmsg = NULL;
	int nrow, ncolumn;

	if (!netdb) return -1;

	sqlite_get_table(netdb,SQL_CHECK_TABLE,&myresult,&nrow,&ncolumn,&errmsg);
	if (myresult) sqlite_free_table(myresult);
	if (errmsg){
		printf("Error db check: %s\n",errmsg);
		free(errmsg);
		return -1;
	}
    return 0;
}


int init_db()
{
	char* errmsg = NULL;
    char* dbname;
	char* envtmp;

	envtmp = getenv("HOME");
	if (!envtmp) return -1;

	dbname = g_strdup_printf("%s/%s",envtmp,NETDBNAME);	
	
	netdb = sqlite_open(dbname, 666, &errmsg);

	free(dbname);

	if (errmsg){
		printf("Error init db: %s\n",errmsg);
		free(errmsg);
		return -1;
	}
	return 0;
}


int create_table()
{
	char* errmsg = NULL;
	int nrow, ncolumn;
	int retval;

	if (!netdb) return -1;

	retval = sqlite_get_table(netdb,SQL_CREATE_TABLE,&myresult,&nrow,&ncolumn,&errmsg);
	if (myresult) sqlite_free_table(myresult);
	
	if (retval){
		printf("Error creating db: %s\n",errmsg);
		free(errmsg);
		return -1;
	}
	return 0;
}


void close_db()
{
	sqlite_close(netdb);
}
