/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003,2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE busybox syslog access module.
 *
 * circular buffer syslog implementation for busybox
 *
 * Copyright (C) 2000 by Gennady Feldman <gfeldman@cachier.com>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/poll.h>

#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "serial.h"
#include "applets.h"
#include "suid.h"


/* --- local types and constants --- */

static const long KEY_ID = 0x414e4547; /*"GENA"*/

static struct shbuf_ds {
	int size;		// size of data written
	int head;		// start of message list
	int tail;		// end of message list
	char data[1];		// data/messages
} *buf = NULL;			// shared memory pointer


// Semaphore operation structures
static struct sembuf SMrup[1] = {{0, -1, IPC_NOWAIT | SEM_UNDO}}; // set SMrup
static struct sembuf SMrdn[2] = {{1, 0}, {0, +1, SEM_UNDO}}; // set SMrdn

static int	log_shmid = -1;	// ipc shared memory id
static int	log_semid = -1;	// ipc semaphore id


/* --- module global variables --- */

static GtkWidget *txLog, *txLog2;
static int logtail = -1;
static int fsession;

/* --- local intelligence --- */

void
printlog (GtkWidget * textview, gchar * str)
{
	GtkTextBuffer *log;
	log = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	gtk_text_buffer_insert_at_cursor (GTK_TEXT_BUFFER (log), str, -1);
}

/*
 * sem_up - up()'s a semaphore.
 */
static inline void sem_up(int semid)
{
	if ( semop(semid, SMrup, 1) == -1 ) 
	{
		fprintf(stderr,"semop[SMrup]");
		return;
	}		
}

/*
 * sem_down - down()'s a semaphore
 */				
static inline void sem_down(int semid)
{
	if ( semop(semid, SMrdn, 2) == -1 )
	{
		fprintf(stderr,"semop[SMrdn]");
		return;
	}		
}


int logread_main()
{
	int i;
	char buffer[256];
	struct pollfd pfd;
		
	if ( (log_shmid = shmget(KEY_ID, 0, 0)) == -1)
	{
		printlog(txLog,_("Can't find circular buffer, is syslog running?"));
		return FALSE;
	}
	// Attach shared memory to our char*
	if ( (buf = shmat(log_shmid, NULL, SHM_RDONLY)) == NULL)
	{
		printlog(txLog,_("Can't get access to syslogd's circular buffer."));
		return FALSE;
	}

	if ( (log_semid = semget(KEY_ID, 0, 0)) == -1)
	{
	    printlog(txLog,_("Can't get access to semaphore(s) for syslogd's circular buffer."));
		return FALSE;
	}

	sem_down(log_semid);	
	// Read Memory 
	if (logtail==-1)				// init condition
	{
		i=buf->head;
	}
	else
	{
		if (logtail != buf->tail)	// append
		{	
			printlog(txLog,"new_item\n");
			i=logtail;
		}
		else 						// nothing to do
		{
			shmdt(buf);
			goto next;
		}
	}
	
	if (buf->head == buf->tail) {
		printlog(txLog,_("<empty syslog>"));
	}
	
	while ( i != buf->tail) {
		printlog(txLog,buf->data+i);
		i+= strlen(buf->data+i) + 1;
		if (i >= buf->size )
			i=0;
	}
	logtail = buf->tail;	
	
	sem_up(log_semid);

	if (log_shmid != -1) 
		shmdt(buf);
next: 	
	/* read session file */
	if (fsession > 0)
	{
		pfd.fd = fsession;
		pfd.events = POLLIN;
		while ((i = poll(&pfd,1,10)) > 0)
		{
		pfd.fd = fsession;
		pfd.events = POLLIN;
		pfd.revents = 0;
			i = read(fsession,buffer,255);
			if (!i) break;
			buffer[i] = 0;
			printlog(txLog2,buffer);
		}
	}	
	return TRUE;		
}


/* --- gpe-conf interface --- */

void
Logread_Free_Objects ()
{
	close(fsession);
}

void
Logread_Save ()
{

}

void
Logread_Restore ()
{
	
}

GtkWidget *
Logread_Build_Objects (void)
{
  GtkWidget *notebook;
  GtkWidget *vbox;
  GtkWidget *tw, *tc;
  gchar *tstr;

  notebook = gtk_notebook_new();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_get_border ());

  /* syslog box */	
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());
  gtk_container_set_border_width (GTK_CONTAINER (vbox), gpe_get_border ());
  tw = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("System Log"));
  gtk_label_set_markup (GTK_LABEL (tw), tstr);
  g_free (tstr);
  gtk_box_pack_start(GTK_BOX(vbox),tw,FALSE,TRUE,0);

  tc = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(tc),GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tc),
		GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vbox),tc,TRUE,TRUE,0);

  tw = gtk_text_view_new();	
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tw),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(tc),tw);
  txLog = tw;
  
  tw = gtk_label_new(_("System Log"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,tw);
  
  /* xsession-errors box */	
  
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());
  gtk_container_set_border_width (GTK_CONTAINER (vbox), gpe_get_border ());
  tw = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(tw),0.0,0.5);
  tstr = g_strdup_printf ("<b>%s</b>", _("Session Log"));
  gtk_label_set_markup (GTK_LABEL (tw), tstr);
  g_free (tstr);
  gtk_box_pack_start(GTK_BOX(vbox),tw,FALSE,TRUE,0);

  tc = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(tc),GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox),tc,TRUE,TRUE,0);

  tw = gtk_text_view_new();	
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tw),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(tc),tw);
  txLog2 = tw;
  
  tw = gtk_label_new(_("Session Log"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,tw);
  
  tstr = g_strdup_printf("%s/.xsession-errors",g_get_home_dir());
  fsession = open(tstr,O_RDONLY);
  fcntl(fsession,O_NONBLOCK);
  if (fsession < 0) printlog(txLog2,_("Could not open X session log."));
  g_free(tstr);
 
  logread_main();
  gtk_timeout_add (2000, (GtkFunction) logread_main, NULL);
  
  return notebook;
}
