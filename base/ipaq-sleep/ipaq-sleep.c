/*
 * ipaq sleep daemon
 * 
 * Copyright 2001 Joe McCarthy <mccarthy@engrng.pitt.edu> under the terms of the
 * GNU GPL.
 *
 * blatantly hacked from apm-sleep by
 * Copyright 2000, 2001 Joey Hess <joeyh@kitenet.net> under the terms of the
 * GNU GPL.
 */

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "ipaq-sleep.h"

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

int irqs[MAX_IRQS]; /* irqs to examine have a value of 1 */
long v, irq_count[MAX_IRQS]; /* holds previous counters of the irq's */
int sleep_idle=3 * 60; /* in seconds */
int dim_idle=1 * 60; /* in seconds */
int daemonize=1;
int sleep_time = DEFAULT_SLEEP_TIME;
int cpu=1;
int apm=1;
int dimming=1;
int sleeping=1;
int debug=0;
int probe=1;
double load_check=0.25;
int X=1;

int apmvalue=0, no_sleep=0;
time_t oldpidtime=0;

FILE *dgfp;
		
Display *dpy;
Window root;		    /* The root window (which holds MIT_SCREEN_SAVER
			       info). */
XScreenSaverInfo *info;	    /* The MIT_SCREEN_SAVER info object. */

int init() { 
	int first_event, first_error;
  
	dpy = XOpenDisplay(NULL);
	
	if (dpy==NULL) {
		return (1);
	}
	else {
  		XScreenSaverQueryExtension(dpy, &first_event, &first_error);
  		root = DefaultRootWindow(dpy);
  		info = XScreenSaverAllocInfo();
  		if (debug) 
		  fprintf(dgfp, "X seems to be running....\nUsing display %x\n", dpy);
		return (0);
	}
}

void query_idle(Time *idleTime) {
  
  XScreenSaverQueryInfo(dpy,root, info);
  *idleTime = info->idle;
  
}

void usage () {
	printf("\n");
	
	fprintf(stderr, "usage:  ipaq-sleep -[usnxcCidSh] \n automatic
		sleep/dim daemon for the ipaq. By default the inactivity time
		for dimming is 1 minute and sleeping is 3 minutes. The
		program will probe for IRQs for serial communication, audio
		output, and USB activity and checks for button and
		touchscreen activity via X screen saver extensions. You can
		set a CPU load threshold (defaults to 0.25), or have it
		stay on when plugged into external power (this is on by
		default). Select bits can be over-ridden by changing
		/etc/ipaq-sleep.conf. All auto-sleep activity can be stopped
		by listing \"no-sleep\" PIDs in /var/run/no-sleep/.\n	

	-u (int)\t	set inactivity time for sleep (in seconds) (0 or
	negative means do not suspend)\n
	-o (int)\t	set inactivity time for dim (in seconds) (0 or
	negative means do not dim)\n
	-s	\t	sleep command (in "")	\n
	-S	\t	save new defaults (do NOT run)\n
	-a	\t	ignore apm status	\n
	-n	\t	turn OFF daemon-mode	\n
	-x	\t 	turn OFF X-mode  	\n
	-c	\t 	turn OFF cpu-mode  	\n
	-C (float)\t	 set custom CPU load  	 \n
	-i (int)\t 	set custom IRQs (use with p to remove those already probed) 	\n
	-p 	\t 	do NOT probe for IRQs  	\n
	-d	\t	run in debug mode	\n
	-S	\t	save these values as defaults\n
	-h	\t	show this message  \n\n");

}

/* Signal handler to reap all child processes */
static void reap_children(int signum)
{
    pid_t pid;

    do {
        pid = waitpid(-1, NULL, WNOHANG);
    } while (pid > 0);
}

/* Install the child reaper handler */
static void install_signal_handlers(void)
{
    struct sigaction action;

    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    action.sa_handler = reap_children;
    sigaction(SIGCHLD, &action, NULL);
}


void parse_command_line (int argc, char **argv) {
	extern char *optarg;
	int c=0;
	int i, save=0;
	char dline[64], func[12], value[12];
	FILE *f;
	
	f=fopen(DEFAULTS, "r");
	
	if (! f) fprintf(stderr, "problem opening %s\n", DEFAULTS);
	else {
		while(fgets(dline,sizeof(dline),f)) {
			if (sscanf(dline,"%s = %s", func, value) == 2) {
				if (strcmp(func, uflag)==0) {
					sleep_idle=atoi(value);
					if (debug) fprintf(dgfp, "sleep_idle=%d\n", sleep_idle);
					if (sleep_idle>0) sleeping=1;
					else {
						if (debug) fprintf(dgfp, "sleeping disabled!\n");
						sleeping=0;
					}
				}
				if (strcmp(func, oflag)==0) {
					dim_idle=atoi(value);
					if (debug) fprintf(dgfp, "dim_idle=%d\n", dim_idle);
					if (dim_idle>0) dimming=1;
					else {
						if (debug) fprintf(dgfp, "dimming disabled!\n");
						dimming=0;
					}
				}
				if (strcmp(func, aflag)==0) {
					apm=atoi(value);
					if (debug) fprintf(dgfp, "apm=%d\n", apm);
				}
				if (strcmp(func, cflag)==0) {
					cpu=atoi(value);
					if (debug) fprintf(dgfp, "cpu=%d\n", cpu);
				}
				if (strcmp(func, xflag)==0) {
					X=atoi(value);
					if (debug) fprintf(dgfp, "X=%d\n", cpu);
				}
				if (strcmp(func, dflag)==0) {
					debug=atoi(value);
					if (debug) fprintf(dgfp, "debug=%d\n", cpu);
				}
				if (strcmp(func, Cflag)==0) {
					load_check=atof(value);
					if (debug) fprintf(dgfp, "load_check=%lf\n", load_check);
				}
				if (strcmp(func, pflag)==0) {
					probe=atoi(value);
					if (debug) fprintf(dgfp, "probe=%d\n", probe);
				}
				if (strcmp(func, iflag)==0) {
					i = atoi(value);
					if ((i < 0) || (i >= MAX_IRQS)) 
						fprintf(stderr, "ipaq-sleep: bad irq number %d in /etc/ipaqsleep.conf\n", i);
					irqs[i]=1;
					if (debug) fprintf(dgfp, "IRQ %d set\n", i);
				}
			}
		}
		fclose(f);
	}
			
	while (c != -1) {
		c=getopt(argc,argv, "s:nacxpdSC:u:o:i:h");
		switch (c) {
			case 's':
				sleep_command=strdup(optarg);
				break;
			case 'a':
				apm=0;
				break;
			case 'n':
				daemonize=0;
				break;
			case 'c':
				cpu=0;
				break;
			case 'x':
				X=0;
				break;
			case 'd':
				debug=1;
				break;
			case 'p':
				probe=0;
				break;
			case 'C':
				cpu=1;
				load_check=atof(optarg);
				break;
			case 'u':
				sleep_idle=atoi(optarg);
				if (sleep_idle>0) sleeping=1;
				else {
					if (debug) fprintf(dgfp, "sleeping disabled!\n");
					sleeping=0;
				}
				break;
			case 'o':
				dim_idle=atoi(optarg);
				if (dim_idle>0) dimming=1;
				else {
					if (debug) fprintf(dgfp, "dimming disabled!\n");
					dimming=0;
				}
				break;
			case 'i':
				i = atoi(optarg);
				if ((i < 0) || (i >= MAX_IRQS))
					fprintf(stderr, "ipaq-sleep: bad irq number %d\n", i);
				irqs[atoi(optarg)]=1;
				break;
			case 'S':
				save=1;
				break;
			case 'h':
				usage();
				exit(0);
				break;
		}
	}
	if (optind < argc) {
		usage();
		exit(1);
	}
	
	if (save) {
		
		f=fopen(DEFAULTS, "w");
	
		if (! f) fprintf(stderr, "problem opening %s\n", DEFAULTS);
		else { 
			fprintf(f,"debug = %i\n", debug);
			fprintf(f,"auto-sleep_time = %i\n", sleep_idle);
			fprintf(f,"dim_time = %i\n", dim_idle);
			fprintf(f,"check_apm = %i\n", apm);
			fprintf(f,"check_cpu = %i\n", cpu);
			fprintf(f,"X = %i\n", X);
			fprintf(f,"CPU_value = %lf\n", load_check);
			fprintf(f,"probe_IRQs = %i\n", probe);
			for (i=0;i<MAX_IRQS;i++) 
				if (irqs[i]==1) fprintf(f,"IRQ = %i\n", i);
			fclose(f);	
		}
		
		exit(0);
		
	}
		
}

int probe_IRQs() {
	int i, redo=0, yes=0;
	char iline[64];
  	
	FILE *f;
		
	if (probe) {
			
		f=fopen(INTERRUPTS, "r");
		if (! f) fprintf(stderr, "problem opening %s\n", INTERRUPTS);
		else {
			while(fgets(iline,sizeof(iline),f)) {
			/* Lowercase line. */
				for(i=0;iline[i];i++)
					iline[i]=tolower(iline[i]);
				/* See if it is a serial, audio or USB. */
				if ((strstr(iline, "serial") != NULL ||
			    		strstr(iline, "out") != NULL ||
                            		strstr(iline, "core") != NULL)) yes=1;
				else yes=0;
				if (sscanf(iline,"%d: %ld",&i, &v) == 2 && (yes || irqs[i])) {
					if (irqs[i]!=1) {
						redo=1;
						irqs[i]=1;
						if (debug) fprintf(dgfp,"Found a new IRQ.... %d\n", i);
					}
					irq_count[i] = v;
				}
			}
			fclose(f);
		}
	}
	
	return(redo);
	
}
	
int check_apm() {
	char apmline[64];
	char junk[24];
  	
	FILE *f;
	
	if (apm) {
			
		f=fopen(APM, "r");
		if (! f) fprintf(stderr, "problem opening %s\n", APM);
		else
		{
			while(fgets(apmline,sizeof(apmline),f)) {
				/* Find the external power "string". */
				if ((sscanf(apmline,"%s %s %s %x %s %s %s %s %s", 
					junk, junk, junk, &apmvalue, junk, junk,
					junk, junk, junk)) == 9) {
					if (apmvalue) {
						if (debug) fprintf(dgfp,"You are on external power. NOT sleeping.\n");
						fclose(f);
						return(1);
					}
					else {
						if (debug) fprintf(dgfp,"You are NOT on external power. Its all good.....\n");
					}
				}
				else if (debug) fprintf(dgfp,"Problem checking apm status.\n");
			}
			fclose(f);	    	
		}
	}
	
	return(0);
}		
	
int check_PID() {
	int PID;
	char command[60], pid_file[25];
  	struct stat pid_stat;

	FILE *input, *pid_check;
		
	stat(NO_SLEEP_DIR, &pid_stat);
	if (debug) fprintf(dgfp, "Last time %i, new time %i\n", oldpidtime, pid_stat.st_mtime);  
	if (pid_stat.st_mtime>oldpidtime) {
		
		no_sleep=0;
		if (debug) fprintf(dgfp,"The no-sleep directory has been modified\nChecking for PID locks\n");
		oldpidtime=pid_stat.st_mtime;
		sprintf(command, "/bin/ls %s", NO_SLEEP);
  
  		if((input=popen(command, "r"))==NULL) 
    	  		fprintf(stderr, "Cannot Open Stdin to get no-sleep PIDs\n");
 
  		while((fscanf(input, "%i", &PID))!=-1) {
  
   			sprintf(pid_file, "/proc/%i/cmdline", PID);
  			if ((pid_check=fopen(pid_file, "r"))!=NULL) {
				if (debug) fprintf(dgfp,"PID lock for %d\n", PID);
				no_sleep=1;
   		 		fclose(pid_check);
			}
			else {
			
		  		if (debug) fprintf(dgfp,"Bad PID lock for process %d,removing\n", PID);
				sprintf(command, "/bin/rm %s/%i", NO_SLEEP_DIR,PID);
  				system(command);
			}
		}
		
		pclose(input);
			
  	}
  	if (no_sleep) {
		if (debug) fprintf(dgfp,"Still PID locked\n");
		return(1);
	}
	else return(0);
	
}
	
int check_cpu() {
	int activity=0;
	double load=0.0;
	
	FILE *f;
		
	if (cpu) {
			
		f=fopen(LOADAVG, "r");
                if (! f) fprintf(stderr, "error opening %s\n", LOADAVG);
                else {
			if (load>=load_check) {
				activity=1;
				if (debug) fprintf(dgfp,"CPU activity %f greater than %f\n", load, load_check);
                	}
			else if (debug) fprintf(dgfp,"CPU load is %f lower than %f,
				its all good...\n", load, load_check);
			fclose(f);
		}
                
	}
	
	return(activity);
	
}		

/* Keep checking the interrupts. As long as there is activity, do nothing. */
void main_loop (void) {
	int activity, i, total_unused=0, apm_active=0, old_apm=0;
	int dimmed=0, current_bl=32;
	int newIdle, oldIdle, lastIdle, oldTime, newTime;
	double load=0.0;
	char iline[64];
	char command[60], junk[24], dim_status[3];
  	
	Time idleTime; /* milliseconds */
        FILE *f;
	FILE *input;
  
	sleep_idle=sleep_idle;
	dim_idle=1000*dim_idle;
	
	check_cpu();
	check_PID();
	
	query_idle(&idleTime);
	lastIdle=oldIdle=(int)idleTime;
	if (debug) fprintf(dgfp,"Setting oldIdle %d.\n", oldIdle);
			
	sprintf(command, "/usr/bin/bl");
  	if((input=popen(command, "r"))==NULL) fprintf(stderr, "Cannot Open Stdin for bl command\n");
 	if (debug) fprintf(dgfp,"Current bl value is %d\n Saving this.\n", current_bl);
	if((fscanf(input, "%s %i", dim_status, &current_bl))==-1)
		current_bl=32;
	
	if (strcmp(dim_status, "off")==0) {
		if (debug) fprintf(dgfp,"Currently bl is off...did you want to mean to disable dim handling? Use -o 0\n");
		dimmed=1;
	}
	else {
		if (debug) fprintf(dgfp,"Good the bl is on, leave everything to me!\n");
		dimmed=0;
	}
	
	probe_IRQs();
								
	while (1) {
		activity=0;
		
		f=fopen(INTERRUPTS, "r");
		if (! f) fprintf(stderr, "problem opening %s\n", INTERRUPTS);
		else {
			while(fgets(iline,sizeof(iline),f)) {
				if (sscanf(iline,"%d: %ld",&i, &v) == 2 && 
			   	 irqs[i] && irq_count[i] != v) {
					activity=1;
					if (debug) fprintf(dgfp,"IRQ activity %d\n", i);
					irq_count[i] = v;
				}
			}
			fclose(f);
		}
		
		if (X) {
			query_idle(&idleTime);
			newIdle=(int)idleTime;
			
			if ((newIdle-oldIdle)<(lastIdle-oldIdle)) {
				activity=1;
				oldIdle=0;
				if (debug) fprintf(dgfp,"Recent X activity.\nNewidle=%d, oldidle=%d\n", newIdle, oldIdle);
				if (dimming && dimmed) {
			  	  	if (debug) fprintf(dgfp,"Resetting bl value of %d\n",current_bl);
			  	  	sprintf(command, "/usr/bin/bl %i\n", current_bl);
  			  	  	system(command);
			  	 	dimmed=0;
				}		
						
			}
			
			else if (debug) fprintf(dgfp,"No activity. Newidle=%d, oldidle=%d\n", newIdle, oldIdle);
			
			if (dimming  && !dimmed && !apm_active) {
				if ((newIdle-oldIdle)>=dim_idle) {
					sprintf(command, "/usr/bin/bl");
  					if((input=popen(command, "r"))==NULL) fprintf(stderr, "Cannot Open Stdin for bl command\n");
 					if (debug) fprintf(dgfp,"Current bl value is %d\nSaving this, then Dimming.\n", current_bl);
  					if((fscanf(input, "%s %i", junk, &current_bl))==-1)
						current_bl=32;
					
					if (strcmp(dim_status, "off")==0) 
						if (debug) fprintf(dgfp,"Currently bl is off...but I thought *I* was supposed to do that?\n");
					
					sprintf(command, "/usr/bin/bl off\n");
  					system(command);
					dimmed=1;
				}
			}
			
			lastIdle=newIdle;
	
		}
		
		apm_active=check_apm();
		if (apm_active) {
			activity=1;	
			oldIdle=newIdle;
			if (old_apm==0 && dimming && dimmed) {
				if (debug) fprintf(dgfp,"Resetting bl value of %d\n",current_bl);
			  	sprintf(command, "/usr/bin/bl %i\n", current_bl);
  			  	system(command);
				dimmed=0;	
			}
		}
		old_apm=apm_active;
		
		if (activity) {
			total_unused = 0;
		}
		else {
			total_unused += sleep_time;
			if (total_unused >= sleep_idle && sleeping) {
				
				if (check_cpu() || check_PID() || probe_IRQs()) {
					if (debug) fprintf(dgfp,"You cannot sleep at this time! Not going to sleep....\n");
					total_unused=0;
					oldIdle=newIdle;
				}
				else {
					if (debug) fprintf(dgfp,"Going to sleep....\n");
					if (system(sleep_command) != 0) fprintf(stderr, "%s failed", sleep_command);
					total_unused=0;
				}
			}
			
		}
		
		if (debug) fflush(dgfp);
		sleep(sleep_time);
		
		newTime=time(NULL);
		if (oldTime && newTime-sleep_time > oldTime +1) {
			fprintf(stderr, "%i sec sleep; resetting timer and resetting dimmer...", (int)(newTime - oldTime));
			total_unused=0;
			
			query_idle(&idleTime);
			lastIdle=oldIdle=(int)idleTime;
			if (debug) fprintf(dgfp,"Re-setting oldIdle %d.\n", oldIdle);
			
			if (dimming && dimmed) {
			  	  if (debug) fprintf(dgfp,"Resetting bl value of %d\n",current_bl);
			  	  sprintf(command, "/usr/bin/bl %i\n", current_bl);
  			  	  system(command);
			  	 dimmed=0;
			}
		}
		oldTime=newTime;	
		
	}
}

int main (int argc, char **argv) {
	int try=0, tmpX=0, checkinit=0;
	
	dgfp=fopen("/tmp/ipaq-sleep.log", "a");
	if (! dgfp) {
		fprintf(stderr, "problem opening ipaq-sleep.log\n");
		exit(1);
	}
	
	install_signal_handlers();
	parse_command_line(argc, argv);
	
	if (daemonize) {
		if (daemon(1,1) == -1) {
			fprintf(stderr, "auto-sleep daemon problems\n");
			exit(1);
		}
	}
	
	checkinit=init();
	
	while (try<10) {
		if (checkinit) {
			if (debug) fprintf(dgfp,"Waiting for X....\n");
			tmpX=0;
			sleep(2);
			checkinit=init();
		}
		else {
			tmpX=1;
			break;
		}
		try++;
	}
	
	if(tmpX!=1) {
		
		if (debug) fprintf(dgfp, "It looks like X is not running. You might be somewhat hosed....\n");
		X=tmpX;
	
	}
	
	main_loop();
	
	return(0); // never reached
}
