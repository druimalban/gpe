/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#define _XOPEN_SOURCE /* Pour GlibC2 */
#include <time.h>
#include "applets.h"
#include "network.h"
#include "gpe/errorbox.h"
#include "parser.h"


GtkWidget *Network_Build_Objects()
{  
  FILE *pipe;
  GtkWidget *label;
  GtkWidget *table;
  gint row = 0,col=2;
  int new=1,i, shift;


  table = gtk_table_new(row,2,FALSE);

  pipe = popen ("/sbin/ifconfig", "r");
  
  if (pipe > 0)
    {
      char buffer[256], buffer2[20];
      fgets (buffer, 255, pipe);
      while (feof(pipe) == 0)
	{
	  if(buffer[0]=='\n' || buffer[0]=='\n')
	    new = 1;
	  if (new && sscanf (buffer, "%s", buffer2) > 0)
	    {
	      row++;
	      new = 0;
	      gtk_table_resize(GTK_TABLE(table),row,col);
	      label = gtk_label_new(buffer2);
	      gtk_table_attach (GTK_TABLE (table), label, 0, 1, row-1, row,
				(GtkAttachOptions) ( GTK_FILL),
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);

	    }
#if __i386__ // my pc is in french :-(
	  // this code is notvery portable.
	  // there is surely some system function to do that..
	  if ((shift = mystrcmp (buffer, "inet adr:")) != -1)
#else
	  if ((shift = mystrcmp (buffer, "inet addr:")) != -1)
#endif	    
	    {
	      for(i=shift;buffer[i]!=' ';i++)
		buffer2[i-shift] = buffer[i];
	      buffer2[i-shift]=0;
	      label = gtk_entry_new();
	      gtk_entry_set_text(GTK_ENTRY(label),buffer2);
	      gtk_entry_set_editable(GTK_ENTRY(label),FALSE);
	      
	      gtk_table_attach (GTK_TABLE (table), label, 1, 2, row-1, row,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 4);

	    }
	  fgets (buffer, 255, pipe);
	}
      pclose (pipe);
    }
  else
    {
      gpe_error_box( "couldn't read ifconfig stats\n");
    }
  
  return table;

}
