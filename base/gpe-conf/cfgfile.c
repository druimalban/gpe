
/*

	Configfile I/O routines

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gpe/errorbox.h>

#include "cfgfile.h"

static FILE* configfile;
static gchar** configtext;
static gint configlen;
NWInterface_t* iflist = NULL;
gint iflen = 0;

gint set_file_open(gint openon);
gint get_section_start(gchar* section);
gint get_section_end(gchar* section);
gint get_section_nr(gchar* section);
void empty_section(gint s_start, gint s_end);
gint read_section(gint, NWInterface_t *Scheme);
gint rewrite_section(NWInterface_t *Scheme, gint startpos);

gint get_section_text(gchar* section);
gint get_file_text();
gint get_scheme_list();

gint write_sections();

// some helper for parsing strings
gchar last_char(gchar* s);
gint count_char(gchar* s, gchar c);
gint get_first_char(gchar* s);
gint get_param_val(gchar* line, gchar* param, gchar* value);
gint subst_val(gchar* line, gchar* value);


gint subst_val(gchar* line, gchar* value)
{
	gchar param[255];
	gchar tmpval[255];
	
	if (get_first_char(value) != -1)
	{
		get_param_val(line,param,tmpval);
		sprintf(line,"\t%s %s",param,value);
	}
	else	// remove line
	{
		sprintf(line,"\0"); 
	}	
	return 0;
}

gint get_param_val(gchar* line, gchar* param, gchar* value)
{
	gint st,sep,a,b;
	
	st=get_first_char(line);
	if (line[st] == '#') return -1; // mark comment 
	for (sep=st;sep<strlen(line);sep++)
		if (line[sep]==' ') break;
	for (a=sep;a<strlen(line);a++)
		if ((line[a]!=' ') && (line[a]!='\t')) break;
	for (b=a+1;b<strlen(line);b++)
		if ((line[b]=='\n') || (line[b]=='\0')) break;
	param=strncpy(param,&line[st],sep-st);
	param[sep-st]='\0';	
	if ((b-a) > 1) value=strncpy(value,&line[a],b-a);	
	value[b-a]='\0';
	return 0;
}


gint read_section(gint iface, NWInterface_t *Iface)
{
	return 0;
}


gint get_first_char(gchar* s)
{
	gint i=0;
	for (i=0;i<strlen(s);i++)
		if ((s[i] != ' ') && (s[i] != '\t') && (s[i] != '\n') && (s[i] != '\0')) return i;
	return -1;
}


gint count_char(gchar* s, gchar c)
{
	gint a = 0;
	gint i;
	for (i=0;i<strlen(s);i++)
		if (s[i] == c) a++;
	return a;
}


gchar last_char(gchar* s)
{
	gint i;
	if (strlen(s)<=0) return (gchar)0;
	for (i=1;i<strlen(s);i++)
		if ((s[strlen(s)-i] != ' ') && (s[strlen(s)-i] != '\t')) return s[strlen(s)-i];
	return (gchar)0;
}


gint get_file_text()
{
	gchar* ret;
	gint i = 0;
	gchar buf[256];

	configtext=NULL;		
	do
	{
		ret = fgets(buf,255,configfile);
		if (ret) {
			i++;
			buf[255]='\0';
			configtext=realloc(configtext,i*sizeof(gchar*));
			configtext[i-1]=(gchar*)malloc(sizeof(gchar)*(strlen(ret)+2));				
			strcpy(configtext[i-1],buf);
		}
	}	
	while(ret != NULL);
	return i;	
}


gint get_scheme_list()
{
	gchar ifname[255]  = {0};
	gchar header[255]  = {0};
	gchar paramval[255];
	gchar option1[32] = {0};
	gchar option2[32] = {0};
	gint i, j, k;
	gint l=0;
	for (i=0;i<configlen;i++)
	{
		if (get_param_val(configtext[i],paramval,ifname) == -1) continue; 	// find interface definition?
		if (!strcmp("iface",paramval))
		{
			l++;
			k=0;
			for (j=0;j<strlen(configtext[i]);j++)
			{
				if (configtext[i][j]==' ') {
					switch (k) {
						case 0:header[j]='\0';
						break;
						case 1:ifname[j-strlen(header)-1]='\0';
						break;
						case 2:option1[j-strlen(header)-strlen(ifname)-2]='\0';
						break;
					}
					j++; k++;
				}
				if (j==(strlen(configtext[i])-1)) {
						option2[j-strlen(header)-strlen(ifname)-strlen(option1)-3]='\0';
						break;
					break;
				}
				switch (k) {
					case 0:header[j]=configtext[i][j];
					break;
					case 1:ifname[j-strlen(header)-1]=configtext[i][j];
					break;
					case 2:option1[j-strlen(header)-strlen(ifname)-2]=configtext[i][j];
					break;
					case 3:option2[j-strlen(header)-strlen(ifname)-strlen(option1)-3]=configtext[i][j];
					break;
				}
			}

			iflist=(NWInterface_t*)realloc(iflist,l*sizeof(NWInterface_t));
			strcpy(iflist[l-1].name,ifname);
			
			iflist[l-1].isstatic = FALSE;
			iflist[l-1].isinet = FALSE;
			iflist[l-1].isloop = FALSE;
			iflist[l-1].isdhcp = FALSE;
			iflist[l-1].isppp = FALSE;
			
			// strstr could make this easier!
			if (!strcmp("inet",option1)) iflist[l-1].isinet = TRUE;
			if (!strcmp("inet",option2)) iflist[l-1].isinet = TRUE;
			if (!strcmp("static",option1)) iflist[l-1].isstatic = TRUE;
			if (!strcmp("static",option2)) iflist[l-1].isstatic = TRUE;
			if (!strcmp("loopback",option1)) iflist[l-1].isloop = TRUE;
			if (!strcmp("loopback",option2)) iflist[l-1].isloop= TRUE;
			if (!strcmp("dhcp",option1)) iflist[l-1].isdhcp = TRUE;
			if (!strcmp("dhcp",option2)) iflist[l-1].isdhcp= TRUE;
			if (!strcmp("ppp",option1)) iflist[l-1].isppp = TRUE;
			if (!strcmp("ppp",option2)) iflist[l-1].isppp= TRUE;

			strcpy(iflist[l-1].netmask,"\0");
			strcpy(iflist[l-1].network,"\0");
			strcpy(iflist[l-1].gateway,"\0");
			strcpy(iflist[l-1].broadcast,"\0");
			strcpy(iflist[l-1].hostname,"\0");
			strcpy(iflist[l-1].clientid,"\0");
			strcpy(iflist[l-1].provider,"\0");
			
			iflist[l-1].firstline=i;
			iflist[l-1].lastline=i;
			//if ((l>1) && (!iflist[l-2].lastline)) iflist[l-1].lastline=i-1;
		}
		else // parse rest and find end
		{
			if (!iflist) continue; // we are before any sections
/*			if (!strcmp("auto",paramval))
			{
				iflist[l-1].lastline=i-1;
			}
*/			if (!strcmp("address",paramval)) {
				strcpy(iflist[l-1].address,ifname);
				iflist[l-1].lastline=i;
			}
			if (!strcmp("netmask",paramval)) {
				strcpy(iflist[l-1].netmask,ifname);
				iflist[l-1].lastline=i;
			}
			if (!strcmp("gateway",paramval)) {
				strcpy(iflist[l-1].gateway,ifname);
					iflist[l-1].lastline=i;
			}
			if (!strcmp("broadcast",paramval)) {
				strcpy(iflist[l-1].broadcast,ifname);
					iflist[l-1].lastline=i;
			}
			if (!strcmp("network",paramval)) {
				strcpy(iflist[l-1].network,ifname);
					iflist[l-1].lastline=i;
			}
			if (!strcmp("hostname",paramval)) {
				strcpy(iflist[l-1].hostname,ifname);
				iflist[l-1].lastline=i;
			}
			if (!strcmp("client",paramval)) {
				strcpy(iflist[l-1].clientid,ifname);
					iflist[l-1].lastline=i;
			}
			if (!strcmp("provider",paramval)) {
				strcpy(iflist[l-1].provider,ifname);
					iflist[l-1].lastline=i;
			}
						
		} // else
	} // for
	iflen = l;
	return l;
}


void empty_section(gint s_start, gint s_end)
{
	
}


gint get_section_nr(gchar* section)
{
	gint i;
	for (i=0;i<iflen;i++)
		if (!strcmp(iflist[i].name,section)) return i;
	return -1;
}


gint get_section_start(gchar* section)
{
	int i;
	for (i=0;i<iflen;i++)
		if (!strcmp(iflist[i].name,section)) return iflist[i].firstline;
	return -1;
}


gint get_section_end(gchar* section)
{
	gint i;
	for (i=0;i<iflen;i++)
		if (!strcmp(iflist[i].name,section)) return iflist[i].lastline;
	return -1;
}


gint set_file_open(gint openon)
{
	if (openon){
		configfile = fopen(NET_CONFIGFILE,"r");
		if (configfile)
			configlen = get_file_text();
		fclose(configfile);
	}
	else
	{
		configfile = NULL;
		free(configtext);
	}
	return (gint)configfile;
}


void add_line(gint pos, gchar* line)
{
	gint a;
	gchar *tmp;
	configlen++;
	configtext=realloc(configtext,configlen*sizeof(gchar*));
	configtext[configlen-1] = (gchar*)malloc(sizeof(gchar)*(strlen(configtext[configlen-2])+1));
	tmp = configtext[configlen-1];
	for (a=configlen-1;a>pos;a--)
	{
		configtext[a]=configtext[a-1];
	}
	
	configtext[pos] = tmp;
	strcpy(configtext[pos],line);
}

gint write_sections()
{
	gint i,j;
	gint l=0;
	gint in_section = FALSE;
	gchar outstr[255];
	gchar paramval[255];
	gchar ifname[255];
	gint svd[10];
	gint lastwpos = 0;
	gint last_i;
	
	for (i=0;i<configlen;i++)
	{
		get_param_val(configtext[i],paramval,ifname);	// get next tokens
		if (!strcmp("iface",paramval))
		{
			in_section = TRUE;
			l++;
			memset(&svd,(gint)0,10*sizeof(gint));
			lastwpos = i;
			sprintf(outstr,"iface %s",	iflist[l-1].name);
			if (iflist[l-1].isinet) strcat(outstr," inet");
			if (iflist[l-1].isstatic) strcat(outstr," static");
			if (iflist[l-1].isloop) strcat(outstr," loopback");
			if (iflist[l-1].isppp) strcat(outstr," ppp");
			if (iflist[l-1].isdhcp) strcat(outstr," dhcp");
			strcat(outstr,"\n\0"); // tm?
			strcpy(configtext[i],outstr);
		}
		else 
		{
			if (!in_section) continue; // we are before any sections
			if (!strcmp("address",paramval)) 
			{
				subst_val(configtext[i],iflist[l-1].address);
				svd[Saddress]=TRUE;
				lastwpos = i;
			}
			if (!strcmp("netmask",paramval)) 
			{
				subst_val(configtext[i],iflist[l-1].netmask);
				svd[Snetmask]=TRUE;
				lastwpos = i;
			}
			if (!strcmp("gateway",paramval)) 
			{
				subst_val(configtext[i],iflist[l-1].gateway);
				svd[Sgateway]=TRUE;
				lastwpos = i;
			}
			if (!strcmp("broadcast",paramval)) 
			{
				subst_val(configtext[i],iflist[l-1].broadcast);
				svd[Sbroadcast]=TRUE;
				lastwpos = i;
			}
			if (!strcmp("network",paramval)) 
			{
				subst_val(configtext[i],iflist[l-1].network);			
				svd[Snetwork]=TRUE;
				lastwpos = i;
			}
			if (!strcmp("hostname",paramval)) 
			{
				subst_val(configtext[i],iflist[l-1].hostname);
				svd[Shostname]=TRUE;
				lastwpos = i;
			}			
			if (!strcmp("client",paramval))
			{
				subst_val(configtext[i],iflist[l-1].clientid);
				svd[Sclientid]=TRUE;
				lastwpos = i;
			}
			if (!strcmp("provider",paramval)) 
			{
				subst_val(configtext[i],iflist[l-1].provider);
				svd[Sprovider]=TRUE;
				lastwpos = i;
			}
				
		} // else
			// handle new parameters at section end
			if (i == iflist[l-1].lastline)
			{
				last_i = i;
				in_section = FALSE;
				lastwpos++;
				if ((!svd[Saddress]) && (get_first_char(iflist[l-1].address)>=0))
				{
					sprintf(outstr,"\taddress %s",iflist[l-1].address);
					add_line(lastwpos,outstr);
					i++;
				}
				if ((!svd[Snetmask]) && (get_first_char(iflist[l-1].netmask)>=0))
				{
					sprintf(outstr,"\tnetmask %s",iflist[l-1].netmask);
					add_line(lastwpos,outstr);
					i++;
				}
				if ((!svd[Snetwork]) && (get_first_char(iflist[l-1].network)>=0))
				{
					sprintf(outstr,"\tnetwork %s",iflist[l-1].network);
					add_line(lastwpos,outstr);
					i++;
				}
				if ((!svd[Sbroadcast]) && (get_first_char(iflist[l-1].broadcast)>=0))
				{
					sprintf(outstr,"\tbroadcast %s",iflist[l-1].broadcast);
					add_line(lastwpos,outstr);
					i++;
				}
				if ((!svd[Shostname]) && (get_first_char(iflist[l-1].hostname)>=0))
				{
					sprintf(outstr,"\thostname %s",iflist[l-1].hostname);
					add_line(lastwpos,outstr);
					i++;
				}
				if ((!svd[Sclientid]) && (get_first_char(iflist[l-1].clientid)>=0))
				{
					sprintf(outstr,"\tclient %s",iflist[l-1].clientid);
					add_line(lastwpos,outstr);
					i++;
				}
				if ((!svd[Sprovider]) && (get_first_char(iflist[l-1].provider)>=0))
				{
					sprintf(outstr,"\tprovider %s",iflist[l-1].provider);
					add_line(lastwpos,outstr);
					i++;
				}
				if ((!svd[Sgateway]) && (get_first_char(iflist[l-1].gateway)>=0))
				{
					sprintf(outstr,"\tgateway %s",iflist[l-1].gateway);
					add_line(lastwpos,outstr);
					i++;
				}
				for (j=l;j<iflen;j++)
					iflist[j].lastline+=(i-last_i);
			} // if last line
	} // for

	configfile = fopen(NET_CONFIGFILE,"w");
	if (configfile)
	{
		for (i=0;i<configlen;i++)
		{
			if (configtext[i][0] != '\0')
			{
				if (count_char(configtext[i],'\n')>0)
					fprintf(configfile,"%s",configtext[i]);
				else
					fprintf(configfile,"%s\n",configtext[i]);
			}	
		}
		fclose(configfile);
	}
	else
		gpe_error_box( "No write access to network configuration.\n");
			
	return l;	
}
