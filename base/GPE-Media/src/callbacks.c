#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <sys/types.h>
#include <signal.h>
#include <wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <string.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "player.h"
#include "io_madplay.h"

static PlayerT *CurrentPlayer = NULL;
static gint ListPos=0;
GtkLabel *TimeLabel;
GtkCList *FileList;

void UpdateUI_cb(void)
{
static char timetext[255];

	if (CurrentPlayer != NULL) {
		sprintf(timetext,"%02d:%02d:%02d",(CurrentPlayer->PosSec / 3600),((CurrentPlayer->PosSec / 60) % 60),(CurrentPlayer->PosSec % 60));
		gtk_label_set_text(TimeLabel, timetext);
	}
}

void sig_child_handler(int signo)
{
int status;
GtkWidget *button;

	fprintf(stderr," SIGCHLD caught, status ");
	if (CurrentPlayer != NULL) {
		waitpid(CurrentPlayer->player_pid, &status, WNOHANG | WUNTRACED);
		fprintf(stderr,"%d\n",status);
		if (WIFSTOPPED(status)) {
			fprintf(stderr,"was stopped by signal %d\n",WSTOPSIG(status));
			return;
		}
	}
	if (WIFEXITED(status)) {
		fprintf(stderr,"child exited with %d\n",WEXITSTATUS(status));
	} else
	if (WIFSIGNALED(status)) {
		fprintf(stderr,"because of signal %d\n",WTERMSIG(status));
	}
	if (CurrentPlayer != NULL) {
		fprintf(stderr,"stopping player !\n");
		gtk_input_remove(CurrentPlayer->input_cb_tag);
		if (CurrentPlayer->PlayerState != 0) {
			madplay_stop();
			CurrentPlayer=NULL;
			ListPos++;
			button=lookup_widget(GTK_WIDGET(FileList),"Start");
			on_Start_clicked(GTK_BUTTON(button),NULL);
			return;
		} else
			CurrentPlayer=NULL;
	} else
		fprintf(stderr,"no player active\n");
	button=lookup_widget(GTK_WIDGET(FileList),"Start");
	gtk_widget_set_sensitive(GTK_WIDGET(button), 1);
}

void sig_pipe_handler(int signo)
{
	fprintf(stderr," SIGPIPE caught\n");
}

void sig_handler(int signo)
{
	fprintf(stderr,"caught signal %d\n", signo);
}

void
on_Start_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
gchar filename[PATH_MAX];

	if (CurrentPlayer == NULL) {
		if (ListPos >= FileList->rows) {
			fprintf(stderr,"on_Start: ListPos=%d FileList=%d\n",ListPos,FileList->rows);
			ListPos=0;
			gtk_clist_select_row(FileList,ListPos,0);
			gtk_widget_set_sensitive(GTK_WIDGET(button), 1);
			return;
		}
		gtk_clist_select_row(FileList,ListPos,0);
		strcpy(filename,(gchar *)gtk_clist_get_row_data(FileList, ListPos));
		signal(SIGCHLD,sig_child_handler);
		signal(SIGPIPE,sig_pipe_handler);
		if (button != NULL)
			gtk_widget_set_sensitive(GTK_WIDGET(button), 0);
		CurrentPlayer=madplay_start(filename);
		if ((CurrentPlayer == NULL) || (CurrentPlayer->input_fd <= 0)) {
			/* something went wrong */
			fprintf(stderr,"error starting player\n");
			return;
		}
		CurrentPlayer->input_cb_tag=gtk_input_add_full(CurrentPlayer->input_fd, GDK_INPUT_READ, CurrentPlayer->input_cb , NULL, NULL, NULL);
		CurrentPlayer->PlayerState = 1;
	}
}


void
on_Stop_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	if (CurrentPlayer != NULL) {
		fprintf(stderr,"stopping...\n");
		CurrentPlayer->PlayerState = 0;
		gtk_input_remove(CurrentPlayer->input_cb_tag);
		madplay_stop();
		CurrentPlayer=NULL;
	}
}


void
on_Pause_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	if (CurrentPlayer != NULL) {
		if (CurrentPlayer->PlayerState == 1) {
			madplay_pause(TRUE);
			CurrentPlayer->PlayerState = 2;
		} else if (CurrentPlayer->PlayerState == 2) {
			madplay_pause(FALSE);
			CurrentPlayer->PlayerState = 1;
		}
	}
}


void
on_Forw_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *StartButton;

	if ((ListPos + 1) >= FileList->rows)
		return;
	if (CurrentPlayer != NULL) {
		fprintf(stderr,"stopping...\n");
		CurrentPlayer->PlayerState = 0;
		gtk_input_remove(CurrentPlayer->input_cb_tag);
		madplay_stop();
		CurrentPlayer = NULL;
	}
	ListPos += 1;
	gtk_clist_select_row(FileList,ListPos,0);
	StartButton=lookup_widget(GTK_WIDGET(button),"Start");
	on_Start_clicked(GTK_BUTTON(StartButton), NULL);
}


void
on_DeleteList_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	if (CurrentPlayer != NULL) {
		madplay_stop();
	}
	gtk_main_quit();
}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_Rew_clicked                         (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *StartButton;

	if ((ListPos - 1) < 0)
		return;
	if (CurrentPlayer != NULL) {
		fprintf(stderr,"stopping...\n");
		CurrentPlayer->PlayerState = 0;
		gtk_input_remove(CurrentPlayer->input_cb_tag);
		madplay_stop();
		CurrentPlayer=NULL;
	}
	ListPos -= 1;
	gtk_clist_select_row(FileList,ListPos,0);
	StartButton=lookup_widget(GTK_WIDGET(button),"Start");
	on_Start_clicked(GTK_BUTTON(StartButton), NULL);
}


void
on_AddList_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *FileSelector;

	FileSelector=create_fileselection();
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(FileSelector),"/mnt/");
	gtk_widget_show(FileSelector);
}

void
on_Slider_changed                     (GtkAdjustment	   *adj,
                                        gpointer	 user_data)
{
}

void     
on_VolScale_changed     	        (GtkAdjustment       *adj,
                                        gpointer	 user_data)
{
int handle,r;

        handle=open("/dev/mixer",O_RDWR);
        if (handle > 0) {
                r=((gint)adj->value << 8) | (gint)adj->value;
                ioctl(handle,MIXER_WRITE(SOUND_MIXER_VOLUME),&r);
                close(handle);
        }
}

void
on_BassScale_changed		         (GtkAdjustment       *adj,
                                        gpointer	 user_data)
{
int handle,r;

        handle=open("/dev/mixer",O_RDWR);
        if (handle > 0) {
                r=((gint)adj->value << 8) | (gint)adj->value;
                ioctl(handle,MIXER_WRITE(SOUND_MIXER_BASS),&r);
                close(handle);
        }
}

void
on_TrebleScale_changed  	           (GtkAdjustment       *adj,
                                        gpointer	 user_data)
{
int handle,r;

        handle=open("/dev/mixer",O_RDWR);
        if (handle > 0) {
                r=((gint)adj->value << 8) | (gint)adj->value;
                ioctl(handle,MIXER_WRITE(SOUND_MIXER_TREBLE),&r);
                close(handle);
        }
}


void
on_filesel_ok_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *FileSelector;
gchar *completename;
gchar *entry[3];

	FileSelector=lookup_widget(GTK_WIDGET(button),"fileselection");
	gtk_widget_hide(FileSelector);
	completename=(gchar *)malloc(PATH_MAX * sizeof(gchar));
	strcpy(completename,gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSelector)));
	entry[0]=(gchar *)malloc(10 * sizeof(gchar));
	entry[1]=(gchar *)malloc(PATH_MAX * sizeof(gchar));
	entry[2]=(gchar *)malloc(10 * sizeof(gchar));
	sprintf(entry[0],"%02d",FileList->rows + 1);
	strcpy(entry[1],basename(completename));
	strcpy(entry[2],"??:??:??");
	gtk_clist_append(FileList, entry);
	gtk_clist_set_row_data(FileList, FileList->rows-1, (gpointer)completename);
	free(entry[0]);
	free(entry[1]);
	free(entry[2]);
	gtk_widget_destroy(FileSelector);
}


void
on_filesel_cancel_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *FileSelector;

	FileSelector=lookup_widget(GTK_WIDGET(button),"fileselection");
	gtk_widget_hide(FileSelector);
	gtk_widget_destroy(FileSelector);
}


void
on_FileList_select_row                 (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	fprintf(stderr,"selected: '%s'\n",(gchar *)gtk_clist_get_row_data(clist, row));
	ListPos=row;
}


void
on_normal1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_repeat_1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_repeat_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_shuffle1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

