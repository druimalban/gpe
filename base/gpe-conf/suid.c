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
#include <errno.h>
#include <crypt.h>

#include "suid.h"
#include "applets.h"
#include "gpe/pixmaps.h"
#include "gpe/render.h"
#include "gpe/picturebutton.h"


int check_root_passwd(char *passwd);

//#define __DARWIN__

#ifdef __DARWIN__ // to compile/test under powerpc macosx
int stime(time_t *t)
{
  printf("Setting time : %ld uid:%d euid:%d\n",*t,getuid(),geteuid());
  return (geteuid()!=0)? -1:0;
}
#endif

int execlp1(char *a1,char *a2,char *a3,char *a4)
{
  printf("%s %s %s\n",a1,a2,a3);
  return 0;
}
int execlp2(char *a1,char *a2,char *a3,char *a4,char *a5)
{
  printf("%s %s %s %s\n",a1,a2,a3,a4);
  return 0;
}
/* this is a very simple way to do the stuff */
/* this avoid the gpe-confsuid bin I dislike.. */
void suidloop(int write,int read) 
{
  char cmd[5];
  FILE *in = fdopen(read,"r");
//iFILE    *out = fdopen(write,"w");
  char *bin = NULL;
  char arg1[100];
  char arg2[100];
  int numarg=0;

  while(!feof(in)) // the prg exits with sigpipe
    {
      fflush(stdout);
      fscanf(in,"%4s",cmd);
      if(!feof(in))
	{
	  cmd[4]=0;
	  bin = NULL;
	  //      printf("%4s\n",cmd);
	  if(strcmp(cmd,"NTPD")==0)
	    {
	      bin = "/usr/sbin/ntpdate";
          sprintf(arg1,"-b");
	      fscanf(in, "%100s",arg2);
	      numarg = 2;
	    }
	  else if(strcmp(cmd,"STIM")==0)
	    {
	      time_t t ;
	      fscanf(in, "%ld",&t);
	      if(stime(&t) == -1)
		fprintf(stderr,"error while setting the time: %d\n",errno);
	    
	    }
	
	  /* of course it is a security hole */
	  /* but certainly enough for PDA..  */
	
	  else if(strcmp(cmd,"CPPW")==0) 
	    {
	      bin = "/bin/cp";
	      strcpy(arg1,"/tmp/passwd");
	      strcpy(arg2,"/etc/passwd");
	      numarg = 2;
	    }
	  else if(strcmp(cmd,"XCAL")==0) 
	    {
	      system("xcalibrate");
	    }
	  if(bin) // fork and exec
	    {
	      int PID;
	      switch(PID = fork())
		{
		case -1:
		  fprintf(stderr, "cant fork\n");
		  exit(errno);
		case 0:
		  switch(numarg)
		    {
		    case 1:
		      execlp(bin,bin,arg1,0);
		      break;
		    case 2:
		      execlp(bin,bin,arg1,arg2,0);
		      break;
		    }
		  exit(0);
		default:
	     	break; 
		}
	    }
	}
    }
}

int check_root_passwd(char *passwd)
{
  struct passwd *pwent;
  setpwent();
  pwent = getpwent();
  while(pwent)
	{
	  if(strcmp(pwent->pw_name,"root")==0)
	    {
	      if (strcmp(crypt(passwd,pwent->pw_passwd ), pwent->pw_passwd) == 0)
		return 1;
	      else
		return 0;
	    }
	  pwent = getpwent();
    }
  return 0;
}
static GtkWidget *passwd_entry;

static int retv;

void verify_pass                (
				 gpointer         user_data)

{
  if(check_root_passwd(gtk_entry_get_text(GTK_ENTRY(passwd_entry))))
    {
      retv = 1;
      gtk_widget_destroy(GTK_WIDGET(user_data)); // close the dialog
    }

}
int ask_root_passwd()
{
  GtkWidget *label, *ok, *cancel, *dialog, *icon;
  GtkWidget *hbox;
  GdkPixbuf *p;

  retv = 0;

  dialog = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(mainw));
  gtk_widget_realize (dialog);

  gtk_window_set_title (GTK_WINDOW(dialog), _("passwd"));

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  label = gtk_label_new (_("You have to know\nthe root password\nto do this!"));
  hbox = gtk_hbox_new (FALSE, 4);

  gtk_widget_realize (dialog);

  p = gpe_find_icon ("lock");
  icon = gpe_render_icon (GTK_DIALOG (dialog)->vbox->style, p);
  gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  passwd_entry = gtk_entry_new ();

  gtk_widget_show (passwd_entry);
  gtk_entry_set_visibility (GTK_ENTRY (passwd_entry), FALSE);


  ok = gpe_picture_button (dialog->style, _("OK"), "ok");
  cancel = gpe_picture_button (dialog->style, _("Cancel"), "cancel");

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), cancel);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), ok);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), passwd_entry);


  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  gtk_signal_connect_object (GTK_OBJECT (ok), "clicked",
			     GTK_SIGNAL_FUNC (verify_pass), 
			     (gpointer)dialog);

  gtk_signal_connect_object (GTK_OBJECT (cancel), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), 
			     (gpointer)dialog);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  gtk_widget_show_all (dialog);

  gtk_main ();

  return retv;
}
