%{


#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>

#include "config-parser.h"

Scheme_t schemelist[MAX_SCHEMES];
int	schemecount = 0;

char	schemes[10][32];
int	nrschemes = 0;
int 	count;
int	lcount;

#define DEBUG	0

%}

%union {
	long	lval;
	char	cval[255];
}

%token	<lval>	TOK_CASE		1000
%token	<lval>	TOK_ESAC		1001
%token	<lval>	TOK_INFO 		1002
%token	<lval>	TOK_ESSID		1003
%token	<lval>	TOK_NWID		1004
%token	<lval>	TOK_MODE		1005
%token	<lval>	TOK_FREQ		1006
%token	<lval>	TOK_CHANNEL		1007
%token	<lval>	TOK_SENS		1008
%token	<lval>	TOK_RATE		1009
%token	<lval>	TOK_KEY			1010
%token	<lval>	TOK_RTS			1011
%token	<lval>	TOK_FRAG		1012
%token	<lval>	TOK_IWCONFIG		1013
%token	<lval>	TOK_IWSPY		1014
%token	<lval>	TOK_IWPRIV		1015
%token	<lval>	TOK_IN			1016
%token	<lval>	TOK_EQUALS		1017
%token	<lval>	TOK_COLON		1018
%token	<lval>	TOK_WILDCARD        	1019
%token	<lval>	TOK_SECTEND         	1020
%token	<lval>	TOK_BRACE           	1021
%token	<lval>	TOK_QUOTES          	1022
%token	<cval>	TOK_TEXTVAL         	1023
%token	<cval>	TOK_CASEVAL         	1024
%token  <lval>  TOK_PIPE		1025
%token  <cval>  TOK_KEYVAL		1026

%%

statement_list: statement
	|	statement_list statement
	;

statement:	assignment
	|	case_statement
	;

case_statement:	TOK_CASE TOK_TEXTVAL TOK_IN caselist TOK_ESAC;	{	esac_line = $5;	}
								
caselist:	case_section
	|	caselist case_section
	;

case_section:	case_val_list TOK_BRACE value_list TOK_SECTEND	{
								schemelist[schemecount].scheme_start = sectionstart;
								schemelist[schemecount].scheme_end   = $4;

								//strncpy(schemelist[schemecount].Scheme, $1, 32);
								schemelist[schemecount].lines[L_Scheme]=sectionstart;
								schemelist[schemecount].lines[L_Socket]=sectionstart;
								schemelist[schemecount].lines[L_Instance]=sectionstart;
								schemelist[schemecount].lines[L_HWAddress]=sectionstart;

								for (count=0; count<nrschemes; count++)
								{
									if (count>0) memcpy(&schemelist[schemecount+count], &schemelist[schemecount], sizeof(Scheme_t));
									strncpy(schemelist[schemecount+count].Scheme, strtok(schemes[count] ,","), 32);
									strncpy(schemelist[schemecount+count].Socket, strtok(NULL ,","), 32);
									strncpy(schemelist[schemecount+count].Instance, strtok(NULL ,","), 32);
									strncpy(schemelist[schemecount+count].HWAddress, strtok(NULL ,","), 32);
									
									if (count>0)
									{
										schemelist[schemecount+count].parent_scheme_end = schemelist[schemecount].scheme_end;
										for (lcount=0; lcount<MAX_CONFIG_VALUES; lcount++)
											if (schemelist[schemecount+count].lines[lcount])
												schemelist[schemecount+count].lines[lcount]=LINE_NEW;
										
											
									}

									if (DEBUG==1) printf("yacc: Case section detected: %s,%s,%s,%s\n",
											schemelist[schemecount+count].Scheme,
											schemelist[schemecount+count].Socket,
											schemelist[schemecount+count].Instance,
				 							schemelist[schemecount+count].HWAddress);
								}

								schemecount+=nrschemes;
								memset(schemes, 0, sizeof(schemes));
								nrschemes=0;
							};
case_val_list:	TOK_CASEVAL				{	strncpy(schemes[nrschemes], $1, 32); nrschemes++; }
	|	case_val_list TOK_PIPE TOK_CASEVAL	{	strncpy(schemes[nrschemes], $3, 32); nrschemes++; }
	;

value_list:	assignment
	|	value_list assignment
	;

assignment:	TOK_INFO TOK_EQUALS TOK_TEXTVAL		{
								strncpy(schemelist[schemecount].Info, $3, 64);
								schemelist[schemecount].lines[L_Info]=$1;
								if (DEBUG==1) printf("yacc: INFO = %s\n", $3);
							}
	|	TOK_ESSID TOK_EQUALS TOK_TEXTVAL	{
								strncpy(schemelist[schemecount].ESSID, $3, 32);
								schemelist[schemecount].lines[L_ESSID]=$1;
								if (DEBUG==1) printf("yacc: ESSID = %s\n", $3);
							}
	|	TOK_NWID TOK_EQUALS TOK_TEXTVAL		{
								strncpy(schemelist[schemecount].NWID, $3, 32);
								schemelist[schemecount].lines[L_NWID]=$1;
								if (DEBUG==1) printf("yacc: NWID = %s\n", $3);
							}
	|	TOK_MODE TOK_EQUALS TOK_TEXTVAL		{
								strncpy(schemelist[schemecount].Mode, $3, 16);
								schemelist[schemecount].lines[L_Mode]=$1;
								if (DEBUG==1) printf("yacc: MODE = %s\n", $3);

							}
	|	TOK_FREQ TOK_EQUALS TOK_TEXTVAL		{
								strncpy(schemelist[schemecount].Frequency, $3, 32);
								schemelist[schemecount].lines[L_Frequency]=$1;
								if (DEBUG==1) printf("yacc: FREQUENCY = %s\n", $3);
							}
	|	TOK_CHANNEL TOK_EQUALS TOK_TEXTVAL	{
								strncpy(schemelist[schemecount].Channel, $3, 32);
								schemelist[schemecount].lines[L_Channel]=$1;
								if (DEBUG==1) printf("yacc: CHANNEL = %s\n", $3);
							}
	|	TOK_SENS TOK_EQUALS TOK_TEXTVAL		{
								strncpy(schemelist[schemecount].sens, $3, 32);
								schemelist[schemecount].lines[L_sens]=$1;
								if (DEBUG==1) printf("yacc: SENS = %s\n", $3);	
							}
	|	TOK_RATE TOK_EQUALS TOK_TEXTVAL		{
								strncpy(schemelist[schemecount].Rate, $3, 8);
								schemelist[schemecount].lines[L_Rate]=$1;
								if (DEBUG==1) printf("yacc: RATE = %s\n", $3);
							}
	|	TOK_KEY TOK_EQUALS TOK_TEXTVAL		{
								addkey($3);
								if (DEBUG==1) printf("Evaluated Keys:\n  Key1: %s\n  Key2: %s\n  Key3: %s\n  Key4: %s\n  Active: %s\n  Mode: %s\n  Encryption: %s\n\n",
								schemelist[schemecount].key1,
								schemelist[schemecount].key2,
								schemelist[schemecount].key3,
								schemelist[schemecount].key4,
								schemelist[schemecount].ActiveKey,
								schemelist[schemecount].EncMode,
								schemelist[schemecount].Encryption);
								
								schemelist[schemecount].lines[L_key1] = $1;
								schemelist[schemecount].lines[L_key2] = $1;
								schemelist[schemecount].lines[L_key3] = $1;
								schemelist[schemecount].lines[L_key4] = $1;
								schemelist[schemecount].lines[L_ActiveKey]  = $1;
								schemelist[schemecount].lines[L_EncMode]    = $1;
								schemelist[schemecount].lines[L_Encryption] = $1;

							}
	|	TOK_KEY TOK_EQUALS keyval_list		{
								if (DEBUG==1) printf("Evaluated Keys:\n  Key1: %s\n  Key2: %s\n  Key3: %s\n  Key4: %s\n  Active: %s\n  Mode: %s\n  Encryption: %s\n\n",
								schemelist[schemecount].key1,
								schemelist[schemecount].key2,
								schemelist[schemecount].key3,
								schemelist[schemecount].key4,
								schemelist[schemecount].ActiveKey,
								schemelist[schemecount].EncMode,
								schemelist[schemecount].Encryption);
								
								schemelist[schemecount].lines[L_key1] = $1;
								schemelist[schemecount].lines[L_key2] = $1;
								schemelist[schemecount].lines[L_key3] = $1;
								schemelist[schemecount].lines[L_key4] = $1;
								schemelist[schemecount].lines[L_ActiveKey]  = $1;
								schemelist[schemecount].lines[L_EncMode]    = $1;
								schemelist[schemecount].lines[L_Encryption] = $1;
							}
	|	TOK_RTS TOK_EQUALS TOK_TEXTVAL		{
								strncpy(schemelist[schemecount].rts, $3, 32);
								schemelist[schemecount].lines[L_rts]=$1;
								if (DEBUG==1) printf("yacc: RTS = %s\n", $3);	
							}
	|	TOK_FRAG TOK_EQUALS TOK_TEXTVAL		{
								strncpy(schemelist[schemecount].frag, $3, 32);
								schemelist[schemecount].lines[L_frag]=$1;
								if (DEBUG==1) printf("yacc: FRAG = %s\n", $3);	
							}
	|	TOK_IWCONFIG TOK_EQUALS TOK_TEXTVAL	{
								strncpy(schemelist[schemecount].iwconfig, $3, 255);
								schemelist[schemecount].lines[L_iwconfig]=$1;
								if (DEBUG==1) printf("yacc: IWCONFIG = %s\n", $3);
							}
	|	TOK_IWSPY TOK_EQUALS TOK_TEXTVAL	{
								strncpy(schemelist[schemecount].iwspy, $3, 255);
								schemelist[schemecount].lines[L_iwspy]=$1;
								if (DEBUG==1) printf("yacc: IWSPY = %s\n", $3);
							}
	|	TOK_IWPRIV TOK_EQUALS TOK_TEXTVAL	{
								strncpy(schemelist[schemecount].iwpriv, $3, 255);
								schemelist[schemecount].lines[L_iwpriv]=$1;
								if (DEBUG==1) printf("yacc: IWPRIV = %s\n", $3);
							}
	;

keyval_list:	TOK_KEYVAL				{	addkey($1);	}
	|	keyval_list TOK_KEYVAL			{	addkey($2);	}
	;


%%

void addkey(char *keystr)
{
	int 	count=0;
	int	tempcount=0;

	char 	tempstr[128];
	char	tempkey1[64];
	char	tempkey2[64];
	char	tempkey3[64];
	char    tempkey4[64];
	char    tempkey5[64];
	char    *temp;
	int	keynr;
	int	pos;

	memset(tempkey1, 0, sizeof(tempkey1));
	memset(tempkey2, 0, sizeof(tempkey2));
	memset(tempkey3, 0, sizeof(tempkey3));
	memset(tempkey4, 0, sizeof(tempkey4));
	memset(tempkey5, 0, sizeof(tempkey5));
	sprintf(schemelist[schemecount].Encryption, "on");
	sprintf(schemelist[schemecount].EncMode, "shared");
	sprintf(schemelist[schemecount].ActiveKey, "1");

	while((keystr[count]==' ') || keystr[count]=='"') count++;
	while((count<strlen(keystr)) && (keystr[count]!='"') && (keystr[count]!=0))
	{
		tempstr[tempcount++]=keystr[count++];
	}
	tempstr[tempcount]=0;

	if (strlen(tempstr)==0)
	{
		if (DEBUG==1) printf("yacc: KEY = No Keys found\n");
		sprintf(schemelist[schemecount].Encryption, "off");
		return;
	}
	temp = strtok(tempstr, " ");
	if (temp!=NULL) strncpy(tempkey1, temp, 64);

	temp = strtok(NULL, " ");
	if (temp!=NULL) strncpy(tempkey2, temp, 64);

	temp = strtok(NULL, " ");
	if (temp!=NULL) strncpy(tempkey3, temp, 64);

	temp = strtok(NULL, " ");
	if (temp!=NULL) strncpy(tempkey4, temp, 64);

	temp = strtok(NULL, " ");
	if (temp!=NULL) strncpy(tempkey5, temp, 64);

	if (tempkey1[0]=='[')
	{
		sprintf(schemelist[schemecount].ActiveKey, "%i", tempkey1[1]-'0');
		if (strcmp(tempkey2, "off") == 0) sprintf(schemelist[schemecount].Encryption, "off");
		if (strcmp(tempkey2, "open") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "open");
		}
		if (strcmp(tempkey2, "shared") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "shared");
		}

		return;
	}

	if (strlen(tempkey2)==0)
	{
		if (strcmp(tempkey1, "off") == 0) sprintf(schemelist[schemecount].Encryption, "off");
		if (strcmp(tempkey1, "open") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "open");
			return;
		}
		if (strcmp(tempkey1, "shared") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "shared");
			return;
		}

		keynr = 1;
	}

	keynr = tempkey2[1]-'0';

	if (strlen(tempkey3)==0)
	{
		if (strcmp(tempkey2, "off") == 0) sprintf(schemelist[schemecount].Encryption, "off");
		if (strcmp(tempkey2, "open") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "open");
		}
		if (strcmp(tempkey2, "shared") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "shared");
		}

		keynr = 1;
	}

	pos = 0;
	if (tempkey1[1]==':')
	{
		pos=2;
		schemelist[schemecount].KeyFormat=TRUE;
	}
	switch(keynr)
	{
		case 1: strncpy(schemelist[schemecount].key1, &tempkey1[pos], 64); break;
		case 2: strncpy(schemelist[schemecount].key2, &tempkey1[pos], 64); break;
		case 3: strncpy(schemelist[schemecount].key3, &tempkey1[pos], 64); break;
		case 4: strncpy(schemelist[schemecount].key4, &tempkey1[pos], 64); break;
	}
}

void yyerror(char *errmsg)
{
	fprintf(stderr, "Scanner: %s\n", errmsg);
	input_file_error = TRUE;
}

int parse_input(void)
{
	return(yyparse());
}
