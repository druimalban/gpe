#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "player.h"

static PlayerT MadPlayer;

void madplay_input (gpointer data, gint source, GdkInputCondition condition)
{
char buffer[1024];
int r;

	r=read(source,buffer,1023);
	if (r > 0) {
		buffer[r]='\0';
		if (buffer[3]==':') {
			int hours,minutes,seconds;
			sscanf(buffer+1,"%d:%d:%d",&hours,&minutes,&seconds);
			MadPlayer.PosSec=(hours * 3600) + (minutes * 60) + seconds;
			UpdateUI_cb();
		}
	}
}

/* Starts player */
PlayerT *madplay_start(char *filename)
{
static int ppair[2];
pid_t ppid;

	if (pipe(ppair) != 0) {
		perror("pipe");
		return NULL;
	}
	fprintf(stderr,"pipe returned %d and %d\n",ppair[0],ppair[1]);
	ppid=fork();
	if (ppid == 0) {
		/* child */
		/* close stdin */
		close(0);
		/* redirect stdout to pipe */
		if (dup2(ppair[1], 1) == -1) {
			perror("dup2()");
			exit(1);
		}
		/* redirect stderr to pipe */
		if (dup2(ppair[1], 2) == -1) {
			perror("dup2()");
			exit(1);
		}
		/* throw away input filedesc from pipe */
		if (close(ppair[0]) == -1) {
			perror("close(ppair[0])");
			exit(1);
		}
		execlp("madplay","madplay","-v",filename,NULL);
	} else if (ppid < 0) {
		close(ppair[0]);
		close(ppair[1]);
		return(NULL);
	} else {
/*		close(ppair[1]);*/
		MadPlayer.input_fd=ppair[0];
		MadPlayer.PosSec=0;
		MadPlayer.input_cb=madplay_input;
		MadPlayer.player_pid=ppid;
		MadPlayer.PlayerState=1;
		return &MadPlayer;
	}
return NULL;
}

/* Stop the player */
int madplay_stop(void)
{
	kill(MadPlayer.player_pid, SIGINT);
	waitpid(MadPlayer.player_pid,NULL,0);
	MadPlayer.player_pid=0;
	MadPlayer.PosSec=0;
	MadPlayer.PlayerState=0;
	close(MadPlayer.input_fd);
	UpdateUI_cb();

return 0;
}

/* pause player, True=pause, False=restart */
int madplay_pause(gboolean pause)
{
	if (pause)
		kill(MadPlayer.player_pid, SIGSTOP);
	else
		kill(MadPlayer.player_pid, SIGCONT);

return 0;
}

/* seek to a specific file position */
int madplay_seek(unsigned int seconds)
{

return 0;
}
