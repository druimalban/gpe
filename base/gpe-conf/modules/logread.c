/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	         2003, 2004, 2008  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE busybox syslog access module.
 *
 * Based on the circular buffer syslog implementation for busybox
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
	int32_t size;           // size of data - 1
	int32_t tail;           // end of message list
	char data[1];		// messages
} *buf = NULL;			// shared memory pointer


// Semaphore operation structures
static const struct sembuf init_sem[3] = {
	{0, -1, IPC_NOWAIT | SEM_UNDO},
	{1, 0}, {0, +1, SEM_UNDO}
};

struct globals {
	struct sembuf SMrup[1]; // {0, -1, IPC_NOWAIT | SEM_UNDO},
	struct sembuf SMrdn[2]; // {1, 0}, {0, +1, SEM_UNDO}
	struct shbuf_ds *shbuf;
};

/* Providing hard guarantee on minimum size (think of BUFSIZ == 128) */
enum { COMMON_BUFSIZE = (4096 >= 256*sizeof(void*) ? 4096+1 : 256*sizeof(void*)) };
char bb_common_bufsiz1[COMMON_BUFSIZE];

#define G (*(struct globals*)&bb_common_bufsiz1)
#define SMrup (G.SMrup)
#define SMrdn (G.SMrdn)
#define shbuf (G.shbuf)
#define INIT_G() do { \
	memcpy(SMrup, init_sem, sizeof(init_sem)); \
} while (0)


/* --- module global variables --- */

static GtkWidget *txLog, *txLog2;
static int fsession = -1;

/* --- local intelligence --- */


void
printlog (GtkWidget *textview, const gchar *str)
{
	GtkTextBuffer *log;
	if ((str == NULL) || (!strlen(str))) 
          return;

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


int logread_main(void)
{
	unsigned cur;
	int log_semid; /* ipc semaphore id */
	int log_shmid; /* ipc shared memory id */
	int i;
	char buffer[256];
	struct pollfd pfd;
	unsigned shbuf_size;
	unsigned shbuf_tail;
	const char *shbuf_data;
 		
	INIT_G();

	log_shmid = shmget(KEY_ID, 0, 0);
	if (log_shmid == -1) {
		printlog(txLog,
		  _("Can't find circular buffer, is syslog running?\n"));
		return FALSE;
	}

	/* Attach shared memory to our char* */
	shbuf = shmat(log_shmid, NULL, SHM_RDONLY);
	if (shbuf == NULL) {
		printlog(txLog,
		  _("Can't get access to syslogd's circular buffer."));
		return FALSE;
	}

	log_semid = semget(KEY_ID, 0, 0);
	if (log_semid == -1) {
	    printlog(txLog,
		         _("Can't get access to semaphore(s) "\
		         "for syslogd's circular buffer."));
		return FALSE;
	}

	/* Suppose atomic memory read */
	/* Max possible value for tail is shbuf->size - 1 */
	cur = shbuf->tail;
       
	if (semop(log_semid, SMrdn, 2) == -1) {
        	shmdt(shbuf);
            return FALSE;
        }

		/* Copy the info, helps gcc to realize that it doesn't change */
		shbuf_size = shbuf->size;
		shbuf_tail = shbuf->tail;
		shbuf_data = shbuf->data; /* pointer! */
#ifdef DEBUG
    	printf("cur:%d tail:%i size:%i\n",
					cur, shbuf_tail, shbuf_size);
#endif
		/* advance to oldest complete message */
		/* find NUL */
		cur += strlen(shbuf_data + cur);
		if (cur >= shbuf_size) { /* last byte in buffer? */
			cur = strnlen(shbuf_data, shbuf_tail);
			if (cur == shbuf_tail)
				goto unlock; /* no complete messages */
		}
		/* advance to first byte of the message */
		cur++;
		if (cur >= shbuf_size) /* last byte in buffer? */
			cur = 0;

		/* Read from cur to tail */
		while (cur != shbuf_tail) {
			printlog(txLog, shbuf_data + cur);
			cur += (strlen(shbuf_data + cur) + 1);
			if (cur >= shbuf_size)
				cur = 0;
		}
        
unlock:
	/* release the lock on the log chain */
	sem_up(log_semid);

	shmdt(shbuf);

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
			i = read(fsession, buffer, 255);
			if (!i) break;
			buffer[i] = 0;
			printlog(txLog2, buffer);
		}
	}	
	return TRUE;		
}


/* --- gpe-conf interface --- */

void
Logread_Free_Objects (void)
{
	close(fsession);
}

void
Logread_Save (void)
{

}

void
Logread_Restore (void)
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
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tc),
		GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(tc),GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox),tc,TRUE,TRUE,0);

  tw = gtk_text_view_new();	
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tw),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(tc),tw);
  txLog2 = tw;
  
  tw = gtk_label_new(_("Session Log"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,tw);
  
  tstr = g_strdup_printf("%s/.xsession-errors",g_get_home_dir());
  fsession = open(tstr, O_RDONLY);
  fcntl(fsession, O_NONBLOCK);
  if (fsession < 0) 
	  printlog(txLog2,_("Could not open X session log.\n"));
  g_free(tstr);
 
  logread_main();
  gtk_timeout_add (2000, (GtkFunction) logread_main, NULL);
  
  return notebook;
}
