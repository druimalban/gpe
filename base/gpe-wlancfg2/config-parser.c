/*

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Library General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>

#include "interface.h"
#include "support.h"
#include "config-parser.h"

int      input_file_error;

#define DEBUG	0

char 	config_file[255][MAX_CONFIG_LINES];
int	config_linenr[MAX_CONFIG_LINES];
int	line_count = 0;
int	max_config_line;
char 	configline[255] = "";
int	esac_line;
int	delete_list[MAX_SCHEMES][2];
int	delete_list_count = 0;



int parse_configfile(char* cfgname)
{
	FILE 	*inputfile;
	gint    answer;
	GtkWidget *dialog;	
	input_file_error = FALSE;
	
	memset(schemelist, 0, sizeof(schemelist));
	memset(config_file, 0, sizeof(config_file));
	memset(config_linenr, 0, sizeof(config_linenr));
	
	inputfile=fopen(cfgname, "rw");
	if (!inputfile)
	{
		fprintf(stderr, _("Could not open input file %s\n"), cfgname);
		return(0);
	}
	
	line_count = 0;
	while (fgets(config_file[line_count], 255, inputfile))
	{
		config_linenr[line_count]=line_count+1; 
		line_count++;
	}
	rewind(inputfile);
	
	wl_set_inputfile(inputfile);
	parse_input();
	
	answer = GTK_RESPONSE_NO;
	if (schemecount == 0)
	{
       		dialog = gtk_message_dialog_new (GTK_WINDOW (GPE_WLANCFG),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_YES_NO,
                                                 _("The wireless.opts file is broken.\nShall I create a new one?"));
		answer = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	} else 
	if (input_file_error)
	{
       		dialog = gtk_message_dialog_new (GTK_WINDOW (GPE_WLANCFG),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_OK,
                                                 _("The wireless.opts file has syntax errors.\nPerhaps you can't see all entries"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	
	if (answer == GTK_RESPONSE_YES)
	{
		fprintf(stderr,_("Creating new wireless.opts file\n"));	
		
		memset(schemelist, 0, sizeof(schemelist));
		memset(config_file, 0, sizeof(config_file));
		memset(config_linenr, 0, sizeof(config_linenr));

		fclose(inputfile);
		
		inputfile=fopen(cfgname, "wr");
		if (!inputfile)
		{
			fprintf(stderr, _("Could not open input file %s\n"), cfgname);
			return(0);
		}
		
		
		fputs("case \"$ADDRESS\" in\n", inputfile);
		fputs("*,*,*,*)\n", inputfile);
		fputs("\tINFO=\"AnyESSID\"\n", inputfile);
		fputs("\tESSID=\"any\"\n", inputfile);
		fputs("\tMODE=\"auto\"\n", inputfile);
		fputs("\t;;\n", inputfile);
		fputs("esac\n", inputfile);
		
		fclose(inputfile);

		input_file_error = FALSE;
		
		inputfile=fopen(cfgname, "r");
		if (!inputfile)
		{
			fprintf(stderr, _("Could not open input file %s\n"), cfgname);
			return(0);
		}
		
		line_count = 0;
		while (fgets(config_file[line_count], 255, inputfile))
		{
			config_linenr[line_count]=line_count+1; 
			line_count++;
		}
		rewind(inputfile);
		wl_set_inputfile(inputfile);
		parse_input();
	} 
	
	if (DEBUG) printf("Found %i schemes...\n", schemecount);
	
	return(schemecount);
}

void config_insert_line(Scheme_t *Schemes, int scount, int linenr)
{
	int count;
	int valcount;
	for (count=0; count<scount; count++)
	{
		for (valcount=0; valcount<MAX_CONFIG_VALUES; valcount++)
			if ((Schemes[count].lines[valcount]>=linenr) && 
			    (Schemes[count].lines[valcount]!=LINE_NEW)) Schemes[count].lines[valcount]++;
	
		if (Schemes[count].scheme_start) Schemes[count].scheme_start++;	
		if (Schemes[count].scheme_end>=linenr) Schemes[count].scheme_end++;	
		if (Schemes[count].parent_scheme_end>=linenr) Schemes[count].parent_scheme_end++;	
	
	}

	
	for (count=0; count<line_count; count++)
		if (config_linenr[count]>=linenr) config_linenr[count]++;
			
		
	for (count=0; count<delete_list_count; count++)
	{
		if(delete_list[count][0]>=linenr) delete_list[count][0]++;
		if(delete_list[count][1]>=linenr) delete_list[count][1]++;
	}
	
	max_config_line++;
}

char *get_config_line(Scheme_t *Schemes, int scount, int linenr)
{
	int	count;
	int	valcount;
	int	found_configline = FALSE;
	
	strcpy(configline, "");
	for (count=0; count<scount; count++)
		for (valcount=0; valcount<MAX_CONFIG_VALUES; valcount++)
		{
			if (Schemes[count].lines[valcount]==linenr)
			{
				found_configline=TRUE;
				switch(valcount)
				{
						
					case	L_Scheme:
					case	L_Socket:
					case	L_Instance:
					case	L_HWAddress:
						
						sprintf(configline, "%s,%s,%s,%s)\n", Schemes[count].Scheme, Schemes[count].Socket,
							Schemes[count].Instance, Schemes[count].HWAddress);
						break;

					case	L_Info:
						
						sprintf(configline, "\tINFO=\"%s\"\n", Schemes[count].Info);
						break;
					
					case	L_ESSID:
						
						sprintf(configline, "\tESSID=\"%s\"\n",Schemes[count].ESSID);
						break;
					
					case	L_NWID:
						
						sprintf(configline, "\tNWID=\"%s\"\n",Schemes[count].NWID);
						break;
				
					case	L_Mode:
			 			
						sprintf(configline, "\tMODE=\"%s\"\n",Schemes[count].Mode);
						break;
					
					case	L_Channel:
						
						sprintf(configline, "\tCHANNEL=\"%s\"\n",Schemes[count].Channel);
						break;
					
					
					case	L_Frequency:
						
						sprintf(configline, "\tFREQ=\"%s\"\n",Schemes[count].Frequency);
						break;
					
					case	L_Rate:
						
						sprintf(configline, "\tRATE=\"%s\"\n",Schemes[count].Rate);
						break;
					
					case	L_Encryption:
					case	L_EncMode:
					case	L_KeyFormat:
					case	L_key1:
					case	L_key2:
					case	L_key3:
					case	L_key4:
					case	L_ActiveKey:
						
						if (!strcmp(Schemes[count].Encryption,"on"))
						{
							if (Schemes[count].KeyFormat)
							{
								sprintf(configline, "\tKEY=\"");
								if (strlen(Schemes[count].key1)) sprintf(configline, "%ss:%s [1] key ",configline, Schemes[count].key1);
								if (strlen(Schemes[count].key2)) sprintf(configline, "%ss:%s [2] key ",configline, Schemes[count].key2);
								if (strlen(Schemes[count].key3)) sprintf(configline, "%ss:%s [3] key ",configline, Schemes[count].key3);
								if (strlen(Schemes[count].key4)) sprintf(configline, "%ss:%s [4] key ",configline, Schemes[count].key4);
								sprintf(configline,"%s[%s] %s\"\n",configline, Schemes[count].ActiveKey, Schemes[count].EncMode);
							} else
							{
								sprintf(configline, "\tKEY=\"");
								if (strlen(Schemes[count].key1)) sprintf(configline, "%s%s [1] key ",configline, Schemes[count].key1);
								if (strlen(Schemes[count].key2)) sprintf(configline, "%s%s [2] key ",configline, Schemes[count].key2);
								if (strlen(Schemes[count].key3)) sprintf(configline, "%s%s [3] key ",configline, Schemes[count].key3);
								if (strlen(Schemes[count].key4)) sprintf(configline, "%s%s [4] key ",configline, Schemes[count].key4);
								sprintf(configline,"%s[%s] %s\"\n",configline, Schemes[count].ActiveKey, Schemes[count].EncMode);
							}
						}
						break;
						
					case	L_iwconfig:
						sprintf(configline, "\tIWCONFIG=\"%s\"\n",Schemes[count].iwconfig);
						break;
					
					case	L_iwspy:
						sprintf(configline, "\tIWSPY=\"%s\"\n",Schemes[count].iwspy);
						break;
					
					case	L_iwpriv:
						sprintf(configline, "\tIWPRIV=\"%s\"\n",Schemes[count].iwpriv);
						break;
				
				}
			}
		}
			
	if (!found_configline)
	{
		for (count=0; count<line_count; count++)
		{
			if (config_linenr[count]==linenr)
			{
				found_configline=TRUE;
				strncpy(configline, config_file[count], 255);
				break;
			}
		}
	}
			
	if (!found_configline)
	{
		fprintf(stderr, "Error writing back config-line: %i\n", linenr);
		configline[0] = 0;
	}
	return (configline);
}

int line_in_delete_list(int linenr)
{
	int	count;
	
	for (count=0; count<delete_list_count; count++)
		if ((linenr>=delete_list[count][0]) &&
		    (linenr<=delete_list[count][1])) return(TRUE);
		
	return(FALSE);
}

void reset_pcmcia_socket(int sock, gchar *dev)
{
	gchar command[30];
	
	sprintf(command, "exec /sbin/ifdown %s", dev);
	system(command);
	sprintf(command, "exec /sbin/ifconfig %s down", dev);
	system(command);
	sprintf(command, "exec /sbin/cardctl eject %d", sock);
	system(command);
	sprintf(command,  "exec /sbin/cardctl insert %d", sock);
	system(command);	
	usleep(200000);
	sprintf(command, "exec /sbin/ifup %s", dev);
	system(command);	
}

void restart_socket(void)
{
	const char *stabfile;
	char       linebuf[255];
	FILE       *fd;
	int        sock;
	int        socks[10];
	int        sockcount;
	char       drbuf[127];
	int        i;
	char*      sockdevs[10];

	const char *drivers[] = {"orinoco_cs","wvlan_cs","wavelan_cs","prism2_cs","spectrum24_cs","hostap_cs","airo_cs"};
#define NUM_DRIVERS 7

	if (access ("/var/lib/pcmcia", R_OK) == 0)
	{
        	stabfile = "/var/lib/pcmcia/stab";
	}
	else
	{
       		stabfile = "/var/run/stab";
	}

	fd = fopen(stabfile, "r");
	
	if (fd == NULL)
	{
		perror(_("Can't open stab file, is PCMCIA running? "));
		exit(-2);
	}
	
        if (flock (fileno (fd), LOCK_SH) != 0)
        {
		perror(_("Locking stabfile failed."));
		return;
        }

	memset(linebuf, 0, sizeof(255));
	
	sockcount=0;
	while (fgets(linebuf, sizeof(linebuf)-1, fd) != NULL)
	{
		char device[9];
		if (3 == sscanf(linebuf, "%d %*s %s %*s %8s", &sock, drbuf, device))
		{
			for (i = 0; i < NUM_DRIVERS; i++) 
				if (strcmp(drbuf, drivers[i]) == 0)
				  {
					socks[sockcount]=sock;
					sockdevs[sockcount++]=g_strdup(device);
				  }
			if (sockcount==10) break;
		}
	}
	
	flock (fileno(fd), LOCK_UN);
	fclose(fd);	
	
	for (i=0; i<sockcount; i++)
	{
		reset_pcmcia_socket(socks[i], sockdevs[i]);	
		g_free(sockdevs[i]);
	}
}



int write_back_configfile(char* cfgname, Scheme_t *Schemes, int scount)
{
	FILE 	*outputfile;
	int 	count;
	int	valcount;
	int	max_confset_line;

	max_config_line=config_linenr[line_count-1];
	outputfile=fopen(cfgname, "w");
	if (!outputfile)
	{
		fprintf(stderr, "Could not open output file %s\n", cfgname);
		return(0);
	}
		
	for (count=0; count<scount; count++)
	{
		max_confset_line=0;
		if (Schemes[count].lines[L_Scheme]==LINE_NEW)
		{
			if (Schemes[count].parent_scheme_end) 
				max_confset_line=Schemes[count].parent_scheme_end+1;
			else 	max_confset_line=esac_line;
			
			config_insert_line(Schemes, scount, max_confset_line);
			Schemes[count].lines[L_Scheme] = 
			Schemes[count].lines[L_Socket] = 
			Schemes[count].lines[L_Instance] = 
			Schemes[count].lines[L_HWAddress] = max_confset_line;
			for (valcount=L_Info; valcount<MAX_CONFIG_VALUES; valcount++)
				if ((valcount>=L_Encryption) && (valcount<=L_ActiveKey))
				{
					if ((Schemes[count].lines[L_Encryption]==LINE_NEW) ||
					    (Schemes[count].lines[L_EncMode]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_KeyFormat]==LINE_NEW)  ||
					    (Schemes[count].lines[L_key1]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_key2]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_key3]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_key4]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_ActiveKey]==LINE_NEW))
					{
						max_confset_line++;
						config_insert_line(Schemes, scount, max_confset_line);
						Schemes[count].lines[L_Encryption]=max_confset_line;
						Schemes[count].lines[L_EncMode]=max_confset_line;
						Schemes[count].lines[L_KeyFormat]=max_confset_line;
						Schemes[count].lines[L_key1]=max_confset_line;
						Schemes[count].lines[L_key2]=max_confset_line;
						Schemes[count].lines[L_key3]=max_confset_line;
						Schemes[count].lines[L_key4]=max_confset_line;
						Schemes[count].lines[L_ActiveKey]=max_confset_line;
					}
					
				
				} else
				if(Schemes[count].lines[valcount] == LINE_NEW)
				{
					max_confset_line++;
					config_insert_line(Schemes, scount, max_confset_line);
					Schemes[count].lines[valcount] = max_confset_line;
					
				}
			max_confset_line++;
			config_insert_line(Schemes, scount, max_confset_line);	
			strcpy(config_file[line_count], "\t;;\n\n");
			config_linenr[line_count]=max_confset_line;
			line_count++;
			
		} else
		{
		
		
			for (valcount=0; valcount<MAX_CONFIG_VALUES; valcount++)
				if (Schemes[count].lines[valcount]!=LINE_NEW)
				{
					if (max_confset_line<Schemes[count].lines[valcount]) 
						max_confset_line=Schemes[count].lines[valcount];
				}

				
			for (valcount=0; valcount<MAX_CONFIG_VALUES; valcount++)
			{
				if ((valcount>=L_Encryption) && (valcount<=L_ActiveKey))
				{
					if ((Schemes[count].lines[L_Encryption]==LINE_NEW) ||
					    (Schemes[count].lines[L_EncMode]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_KeyFormat]==LINE_NEW)  ||
					    (Schemes[count].lines[L_key1]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_key2]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_key3]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_key4]==LINE_NEW)	   ||
					    (Schemes[count].lines[L_ActiveKey]==LINE_NEW))
					{
						max_confset_line++;
						config_insert_line(Schemes, scount, max_confset_line);
						Schemes[count].lines[L_Encryption]=max_confset_line;
						Schemes[count].lines[L_EncMode]=max_confset_line;
						Schemes[count].lines[L_KeyFormat]=max_confset_line;
						Schemes[count].lines[L_key1]=max_confset_line;
						Schemes[count].lines[L_key2]=max_confset_line;
						Schemes[count].lines[L_key3]=max_confset_line;
						Schemes[count].lines[L_key4]=max_confset_line;
						Schemes[count].lines[L_ActiveKey]=max_confset_line;
					}
					
				
				} else
				if (Schemes[count].lines[valcount]==LINE_NEW)
				{
					max_confset_line++;
					config_insert_line(Schemes, scount, max_confset_line);
					Schemes[count].lines[valcount]=max_confset_line;
				}
			}
				
		}
		
	}
	
	
	for (count=0; count<max_config_line; count++)
		if (!line_in_delete_list(count+1)) fprintf(outputfile, get_config_line(Schemes, scount, count+1));
	
	fclose(outputfile);
		
	restart_socket();

	return(1);
}
