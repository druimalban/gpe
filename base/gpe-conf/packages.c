/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE configuration package manager module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>

#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "packages.h"
#include "applets.h"
#include "suid.h"
#include "parser.h"


/* --- local types and constants --- */


/* --- module global variables --- */

static GtkWidget *notebook;
static GtkWidget *txLog, *txLog2;
static GtkWidget *bUpdate, *bInstall, *bRemove;
static GtkWidget *ePackage;
static int timeout = -1;

/* --- local intelligence --- */


/*
 *  This function is called from suid task to perform a package install.  
 *  Any output is returned through pipe.
 */
int do_package_install(const char *package,int remove)
{
  FILE *pipe;
  static char cur[256];
		
  if (setvbuf(nsreturn,NULL,_IONBF,0) != 0) 
    fprintf(stderr,"gpe-conf: error setting buffer size!");
  if (remove)
  {
    fprintf(nsreturn,"%s %s\n",_("Removing package"),package);
    snprintf(cur,256,"ipkg -force-defaults remove %s 2>&1",package);
  }
  else
  {	
   fprintf(nsreturn,"%s %s\n",_("Installing package"),package);
   snprintf(cur,256,"ipkg -force-defaults install %s 2>&1",package);
  }
  pipe = popen(cur, "r");

  if (pipe > 0)
    {
      while ((!feof (pipe)) && (!ferror (pipe)))
	  {
	    if (fgets(cur, 255, pipe) > 0)
			fprintf(nsreturn,"%s",cur);
	  }
	  pclose(pipe);
    } 
	
  fprintf(nsreturn,"<end>\n");
  fflush(nsreturn);
  fsync(nsreturnfd);
  return TRUE;
}


/* 
 * Checks if the given packeage is installed.
 */
int do_package_check(const char *package)
{
	char s1[100], s2[100], s3[100];
	char *command = g_strdup_printf("/usr/bin/ipkg status %s",package);
	if (parse_pipe(command,"Status: %s %s %s",s1,s2,s3))
	{
		return FALSE;
	}
	else		
	{
		printf("s1: %s, s2: %s, s3: %s\n",s1,s2,s3);
		if (!strcmp(s1,"install") && !strcmp(s2,"ok") && !strcmp(s3,"installed"))
			return TRUE;
		else
			return FALSE;
	}
}


gboolean poll_log_pipe_generic(void callback(char*))
{
  static char str[256];
  struct pollfd pfd[1];

  pfd[0].fd = suidinfd;
  pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
  while (poll(pfd,1,0))
  {
     fgets (str, 255, suidin);
	 {
       callback(str);
	 }
     if (strstr(str,"<end>")) 
     {
		 return FALSE;
	 }
  }
  return TRUE;	
}


static gboolean poll_log_pipe_update()
{
  static char str[256];
  struct pollfd pfd[1];

  pfd[0].fd = suidinfd;
  pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
    
  while (poll(pfd,1,0))
  {
     fgets (str, 255, suidin);
     if (strstr(str,"<end>")) 
     {
       gtk_button_set_label(GTK_BUTTON(bUpdate),_("Start"));
       gtk_timeout_remove(timeout);
       gtk_widget_set_sensitive(bInstall,TRUE);
       gtk_widget_set_sensitive(bUpdate,TRUE);
       gtk_widget_set_sensitive(bRemove,TRUE);
       printlog(txLog,_("Update finished. Please check log messages for errors."));
	 }
	 else
	 {
       printlog(txLog,str);
	 }
  }
  return TRUE;	
}


static gboolean poll_log_pipe_install()
{
  static char str[256];
  struct pollfd pfd[1];

  pfd[0].fd = suidinfd;
  pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
    
  while (poll(pfd,1,0))
  {
     fgets (str, 255, suidin);
     if (strstr(str,"<end>")) 
     {
       gtk_timeout_remove(timeout);
       gtk_widget_set_sensitive(bInstall,TRUE);
       gtk_widget_set_sensitive(bUpdate,TRUE);
       gtk_widget_set_sensitive(bRemove,TRUE);
       gtk_button_set_label(GTK_BUTTON(bInstall),_("Install"));
       gtk_button_set_label(GTK_BUTTON(bRemove),_("Remove"));
       printlog(txLog2,_("Install/Removal finished. Please check log messages for errors."));
	 }
	 else
	 {
       printlog(txLog2,str);
	 }
  }
  return TRUE;	
}
/*
 *  This function is called from suid task to perform the update.  
 *  Any output is returned through pipe.
 */
void do_package_update()
{
  FILE *pipe;
  static char cur[256];
		
  if (setvbuf(nsreturn,NULL,_IONBF,0) != 0) 
    fprintf(stderr,"gpe-conf: error setting buffer size!");
  fprintf(nsreturn,_("Update using \"ipkg upgrade\" started.\n"));
  pipe = popen("ipkg update 2>&1 && ipkg -force-defaults upgrade 2>&1", "r");

  if (pipe > 0)
    {
      while (!feof (pipe))
	  {
	    fgets (cur, 255, pipe);
		fprintf(nsreturn,"%s",cur);
	  }
	  pclose(pipe);
    } 
	
  fprintf(nsreturn,"<end>\n");
  fflush(nsreturn);
  fsync(nsreturnfd);
}


void on_network_update_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextBuffer *logbuf;
  GtkTextIter start,end;
	
  gtk_button_set_label(button,_("Running..."));
  gtk_widget_set_sensitive(bUpdate,FALSE);
  gtk_widget_set_sensitive(bInstall,FALSE);
  gtk_widget_set_sensitive(bRemove,FALSE);
	
  /* clear log */	
  logbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txLog));
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(logbuf),&start);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(logbuf),&end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(logbuf),&start,&end);

  timeout = gtk_timeout_add (1000, (GtkFunction) poll_log_pipe_update, NULL);
	
  suid_exec("NWUD","NWUD");
	
}


void on_package_install_clicked(GtkButton *button, gpointer user_data)
{
  GtkTextBuffer *logbuf;
  GtkTextIter start,end;
  gchar *pname;
	
  gtk_button_set_label(button,_("Running..."));
  gtk_widget_set_sensitive(bInstall,FALSE);
  gtk_widget_set_sensitive(bUpdate,FALSE);
  gtk_widget_set_sensitive(bRemove,FALSE);
	
  /* clear log */	
  logbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txLog2));
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(logbuf),&start);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(logbuf),&end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(logbuf),&start,&end);

  pname = gtk_editable_get_chars(GTK_EDITABLE(ePackage),0,-1);

  timeout = gtk_timeout_add (1000, (GtkFunction) poll_log_pipe_install, NULL);
  
  if (user_data) // install or remove?
    suid_exec("NWRM",pname);
  else
    suid_exec("NWIS",pname);

  g_free(pname);	
}


/* --- gpe-conf interface --- */

void
Packages_Free_Objects ()
{
}

void
Packages_Save ()
{

}

void
Packages_Restore ()
{
	
}

GtkWidget *
Packages_Build_Objects (void)
{
  GtkWidget *vbox;
  GtkWidget *cur;
  GtkTooltips *tooltips;
  char *tmp;
		
  tooltips = gtk_tooltips_new ();
	
  notebook = gtk_notebook_new();	
  gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_get_border ());
  
  gtk_object_set_data(GTK_OBJECT(notebook),"tooltips",tooltips);

  /* update tab */	
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());

  cur = gtk_label_new(_("Update"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,cur);

  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Update from network"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	
	
  cur = gtk_button_new_with_label(_("Start"));
  bUpdate = cur;
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,FALSE,gpe_get_boxspacing());	
  g_signal_connect(G_OBJECT (cur), "clicked",G_CALLBACK(on_network_update_clicked),NULL);
  gtk_tooltips_set_tip (tooltips, cur, _("Update entire system over an internet connection."), NULL);
  
  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Activity log"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	
  
  cur = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cur),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cur),GTK_SHADOW_IN);
  gtk_tooltips_set_tip (tooltips, cur, _("This window shows all output from the packet manager that performs the update."), NULL);
  
  txLog = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txLog),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(cur),txLog);
  gtk_box_pack_start_defaults(GTK_BOX(vbox),cur);	
   
  /* install tab */
  vbox = gtk_vbox_new(FALSE,gpe_get_boxspacing());

  cur = gtk_label_new(_("Install/Remove"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,cur);

  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Install/Remove a package"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	

  ePackage = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(vbox),ePackage,FALSE,TRUE,0);	
  gtk_tooltips_set_tip (tooltips, ePackage, _("Enter name of package to be installed here."), NULL);
  
  cur = gtk_button_new_with_label(_("Install"));
  bInstall = cur;
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,FALSE,gpe_get_boxspacing());	
  g_signal_connect(G_OBJECT (cur), "clicked",G_CALLBACK(on_package_install_clicked),(gpointer)FALSE);
  gtk_tooltips_set_tip (tooltips, cur, _("Install a package from a configured source."), NULL);
  
  cur = gtk_button_new_with_label(_("Remove"));
  bRemove = cur;
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,FALSE,gpe_get_boxspacing());	
  g_signal_connect(G_OBJECT (cur), "clicked",G_CALLBACK(on_package_install_clicked),(gpointer)TRUE);
  gtk_tooltips_set_tip (tooltips, cur, _("Remove the package."), NULL);
  
  cur = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(cur),0.0,0.5);
  tmp = g_strdup_printf("<b>%s</b>",_("Activity log"));
  gtk_label_set_markup(GTK_LABEL(cur),tmp);
  free(tmp);
  gtk_box_pack_start(GTK_BOX(vbox),cur,FALSE,TRUE,0);	
  
  cur = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cur),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(cur),GTK_SHADOW_IN);
  gtk_tooltips_set_tip (tooltips, cur, _("This window shows all output from the packet manager installing the package."), NULL);
  
  txLog2 = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txLog2),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(cur),txLog2);
  gtk_box_pack_start_defaults(GTK_BOX(vbox),cur);	
  
  /* todo: config tab */
   
  /* change buffering of suid process input pipe */
  if (setvbuf(suidin,NULL,_IONBF,0) != 0) 
    fprintf(stderr,"gpe-conf: error setting buffer size!");

  return notebook;
}
