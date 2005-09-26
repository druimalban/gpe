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
#include <ctype.h>
#include <apm.h>

#include "ipaq-sleep.h"

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

#undef DEBUG

int irqs[MAX_IRQS]; /* irqs to examine have a value of 1 */
long v, irq_count[MAX_IRQS]; /* holds previous counters of the irq's */
int sleep_idle=3 * 60; /* in seconds */
int dim_idle=1 * 60; /* in seconds */
int daemonize=1;
int sleep_time = DEFAULT_SLEEP_TIME;
int cpu=1;
int apm=1;
int dimming=1;
int dim_level=4; 
int sleeping=1;
int debug=0;
int probe=1;
double load_check=0.25;
int X=1;
int battery_level = 0;

int apmvalue=0, no_sleep=0;
time_t oldpidtime=0;

#ifdef DEBUG
FILE *dgfp;
#endif
		
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
#ifdef DEBUG
  		if (debug) 
		  fprintf(dgfp, "X seems to be running....\nUsing display %x\n", dpy);
#endif
		return (0);
	}
}

void query_idle(Time *idleTime) {
  
  XScreenSaverQueryInfo(dpy,root, info);
  *idleTime = info->idle;
  
}

int set_backlight (int val)
{
  if (val < 0)
    return system (BL " off");
  else
    {
      char command[256];
      snprintf(command, sizeof (command), BL " %i\n", val);
      return system(command);
    }
}

int read_backlight (void)
{
  int r = 32;
  FILE *input;

  if ((input=popen(BL, "r")) != NULL)
    {
      char buf[32];
      if (fgets (buf, 32, input))
	{
	  if (!strncmp (buf, "on ", 3))
	    r = atoi (buf + 4);
	  else
	    r = -1;
	}
      pclose (input);
    }
  else
    fprintf(stderr, "Cannot Open Stdin for bl command\n");

  return r;
}

void usage () {
	fprintf(stderr, "usage:  ipaq-sleep -[usnxcCidbSh]\n"
	" -u SECONDS\t inactivity time for sleep (0/-ve: don't suspend) [default 300]\n"
	" -o SECONDS\t inactivity time for dim (0/-ve: don't dim) [default 60]\n"
	" -s COMMAND\t sleep command\n"
	" -S\t\t save new defaults (do NOT run)\n"
	" -a\t\t ignore apm status\n"
	" -n\t\t turn OFF daemon-mode\n"
	" -x\t\t turn OFF X-mode\n"
	" -c\t\t turn OFF cpu-mode\n"
	" -C LOAD\t only sleep when load is below this threshold\n"
	" -i INT\t\t set custom IRQs (use with p to remove those already probed)\n"
	" -p\t\t don't probe for IRQs\n"
	" -b MINUTES\t sleep immediately when remaining run time reaches this level\n"
#ifdef DEBUG
	" -d\t\t run in debug mode\n"
#endif
	" -h\t\t show this message  \n\n");
}

static void do_sleep(void)
{
  if (system(sleep_command) != 0) 
    fprintf(stderr, "%s failed\n", sleep_command);
}

void parse_command_line (int argc, char **argv) {
	extern char *optarg;
	int c=0;
	int i, save=0;
	char dline[64], func[12], value[12], userdefaults[256];
	char *home = getenv ("HOME");
	FILE *f;
	
	f=fopen(DEFAULTS, "r");
	
	if (! f) fprintf(stderr, "problem opening %s\n", DEFAULTS);
	else {
		while(fgets(dline,sizeof(dline),f)) {
			if (sscanf(dline,"%s = %s", func, value) == 2) {
				if (strcmp(func, uflag)==0) {
					sleep_idle=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "sleep_idle=%d\n", sleep_idle);
#endif
					if (sleep_idle>0) sleeping=1;
					else {
#ifdef DEBUG
						if (debug) fprintf(dgfp, "sleeping disabled!\n");
#endif
						sleeping=0;
					}
				}
				if (strcmp(func, oflag)==0) {
					dim_idle=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "dim_idle=%d\n", dim_idle);
#endif
					if (dim_idle>0) dimming=1;
					else {
#ifdef DEBUG
						if (debug) fprintf(dgfp, "dimming disabled!\n");
#endif
						dimming=0;
					}
				}
				if (strcmp(func, Lflag)==0) {
					dim_level=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "dim_idle=%d\n", dim_idle);
#endif
				}
				if (strcmp(func, aflag)==0) {
					apm=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "apm=%d\n", apm);
#endif
				}
				if (strcmp(func, cflag)==0) {
					cpu=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "cpu=%d\n", cpu);
#endif
				}
				if (strcmp(func, xflag)==0) {
					X=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "X=%d\n", cpu);
#endif
				}
				if (strcmp(func, dflag)==0) {
					debug=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "debug=%d\n", cpu);
#endif
				}
				if (strcmp(func, Cflag)==0) {
					load_check=atof(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "load_check=%lf\n", load_check);
#endif
				}
				if (strcmp(func, pflag)==0) {
					probe=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "probe=%d\n", probe);
#endif
				}
				if (strcmp(func, bflag)==0) {
					battery_level=atoi (value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "battery=%d\n", battery_level);
#endif
				}
				if (strcmp(func, iflag)==0) {
					i = atoi(value);
					if ((i < 0) || (i >= MAX_IRQS)) 
						fprintf(stderr, "ipaq-sleep: bad irq number %d in /etc/ipaqsleep.conf\n", i);
					irqs[i]=1;
#ifdef DEBUG
					if (debug) fprintf(dgfp, "IRQ %d set\n", i);
#endif
				}
			}
		}
		fclose(f);
	}
	
	sprintf(userdefaults, "%s%s", home, DEFAULTSUSER);
	
	f=fopen(userdefaults, "r");
	
	if (f)
	{
		while(fgets(dline,sizeof(dline),f)) {
			if (sscanf(dline,"%s = %s", func, value) == 2) {
				if (strcmp(func, uflag)==0) {
					sleep_idle=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "sleep_idle=%d\n", sleep_idle);
#endif
					if (sleep_idle>0) sleeping=1;
					else {
#ifdef DEBUG
						if (debug) fprintf(dgfp, "sleeping disabled!\n");
#endif
						sleeping=0;
					}
				}
				if (strcmp(func, oflag)==0) {
					dim_idle=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "dim_idle=%d\n", dim_idle);
#endif
					if (dim_idle>0) dimming=1;
					else {
#ifdef DEBUG
						if (debug) fprintf(dgfp, "dimming disabled!\n");
#endif
						dimming=0;
					}
				}
				if (strcmp(func, aflag)==0) {
					apm=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "apm=%d\n", apm);
#endif
				}
				if (strcmp(func, cflag)==0) {
					cpu=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "cpu=%d\n", cpu);
#endif
				}
				if (strcmp(func, xflag)==0) {
					X=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "X=%d\n", cpu);
#endif
				}
				if (strcmp(func, dflag)==0) {
					debug=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "debug=%d\n", cpu);
#endif
				}
				if (strcmp(func, Cflag)==0) {
					load_check=atof(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "load_check=%lf\n", load_check);
#endif
				}
				if (strcmp(func, pflag)==0) {
					probe=atoi(value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "probe=%d\n", probe);
#endif
				}
				if (strcmp(func, bflag)==0) {
					battery_level=atoi (value);
#ifdef DEBUG
					if (debug) fprintf(dgfp, "battery=%d\n", battery_level);
#endif
				}
				if (strcmp(func, iflag)==0) {
					i = atoi(value);
					if ((i < 0) || (i >= MAX_IRQS)) 
						fprintf(stderr, "ipaq-sleep: bad irq number %d in /etc/ipaqsleep.conf\n", i);
					irqs[i]=1;
#ifdef DEBUG
					if (debug) fprintf(dgfp, "IRQ %d set\n", i);
#endif
				}
			}
		}
		fclose(f);
	}
			
	while (c != -1) {
		c=getopt(argc,argv, "s:nacxpdSC:u:o:i:b:h");
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
#ifdef DEBUG
					if (debug) fprintf(dgfp, "sleeping disabled!\n");
#endif
					sleeping=0;
				}
				break;
			case 'o':
				dim_idle=atoi(optarg);
				if (dim_idle>0) dimming=1;
				else {
#ifdef DEBUG
					if (debug) fprintf(dgfp, "dimming disabled!\n");
#endif
					dimming=0;
				}
				break;
			case 'i':
				i = atoi(optarg);
				if ((i < 0) || (i >= MAX_IRQS))
					fprintf(stderr, "ipaq-sleep: bad irq number %d\n", i);
				irqs[atoi(optarg)]=1;
				break;
			case 'b':
				battery_level = atoi(optarg);
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
			fprintf(f,"dim_level = %i\n", dim_level);
			fprintf(f,"check_apm = %i\n", apm);
			fprintf(f,"check_cpu = %i\n", cpu);
			fprintf(f,"X = %i\n", X);
			fprintf(f,"CPU_value = %f\n", load_check);
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
#ifdef DEBUG
						if (debug) fprintf(dgfp,"Found a new IRQ.... %d\n", i);
#endif
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
	int runtime;
	apm_info info;

	apm_read(&info);
	
	if (info.ac_line_status == AC_LINE_STATUS_ON) {
#ifdef DEBUG
		if (debug) fprintf(dgfp,"You are on external power. NOT sleeping.\n");
#endif
		return(1);
	}

#ifdef DEBUG
	if (debug) fprintf(dgfp,"You are NOT on external power. Its all good.....\n");
#endif
	runtime = info.battery_time;
	if (apm && runtime >= 0 && runtime < battery_level)
	{
#ifdef DEBUG
		if (debug)
			fprintf (dgfp, "Battery level %d below threshold, sleeping\n", runtime);
#endif
		do_sleep ();
	}
	
	return(0);
}		
	
int check_PID() {
	int PID;
	char command[60], pid_file[25];
  	struct stat pid_stat;

	FILE *input;
		
	stat(NO_SLEEP_DIR, &pid_stat);
#ifdef DEBUG
	if (debug) fprintf(dgfp, "Last time %i, new time %i\n", oldpidtime, pid_stat.st_mtime);  
#endif
	if (pid_stat.st_mtime>oldpidtime) {
		
		no_sleep=0;
#ifdef DEBUG
		if (debug) fprintf(dgfp,"The no-sleep directory has been modified\nChecking for PID locks\n");
#endif
		oldpidtime=pid_stat.st_mtime;
		sprintf(command, "/bin/ls %s", NO_SLEEP);
  
  		if((input=popen(command, "r"))==NULL) 
    	  		fprintf(stderr, "Cannot Open Stdin to get no-sleep PIDs\n");
 
  		while((fscanf(input, "%i", &PID))!=-1) {
  
   			sprintf(pid_file, "/proc/%i/cmdline", PID);
  			if (access (pid_file, F_OK) == 0) {
#ifdef DEBUG
				if (debug) fprintf(dgfp,"PID lock for %d\n", PID);
#endif
				no_sleep=1;
			}
			else {
#ifdef DEBUG			
		  		if (debug) fprintf(dgfp,"Bad PID lock for process %d,removing\n", PID);
#endif
				snprintf(command, sizeof (command), "%s/%i", NO_SLEEP_DIR,PID);
  				unlink(command);
			}
		}
		
		pclose(input);
			
  	}
  	if (no_sleep) {
#ifdef DEBUG
		if (debug) fprintf(dgfp,"Still PID locked\n");
#endif
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
#ifdef DEBUG
				if (debug) fprintf(dgfp,"CPU activity %f greater than %f\n", load, load_check);
#endif
                	}
#ifdef DEBUG
			else if (debug) fprintf(dgfp,"CPU load is %f lower than %f,\n"
				"its all good...\n", load, load_check);
#endif
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
	char iline[64];
  	
	Time idleTime; /* milliseconds */
        FILE *f;
  
	sleep_idle=sleep_idle;
	dim_idle=1000*dim_idle;
	
	check_cpu();
	check_PID();
	
	query_idle(&idleTime);
	lastIdle=oldIdle=(int)idleTime;
#ifdef DEBUG
	if (debug) fprintf(dgfp,"Setting oldIdle %d.\n", oldIdle);
#endif

	current_bl = read_backlight ();
				
	if (current_bl < 0) {
#ifdef DEBUG
		if (debug) fprintf(dgfp,"Currently bl is off...did you want to mean to disable dim handling? Use -o 0\n");
#endif
		dimmed=1;
	}
	else {
#ifdef DEBUG
		if (debug) fprintf(dgfp,"Good the bl is on, leave everything to me!\n");
#endif
		dimmed=0;
	}
	
	probe_IRQs();
								
	while (1) {
		activity=0;
		
		if (X) {
			query_idle(&idleTime);
			newIdle=(int)idleTime;
			
			if ((newIdle-oldIdle)<(lastIdle-oldIdle)) {
				activity=1;
				oldIdle=0;
#ifdef DEBUG
				if (debug) fprintf(dgfp,"Recent X activity.\nNewidle=%d, oldidle=%d\n", newIdle, oldIdle);
#endif
				if (dimming && dimmed) {
#ifdef DEBUG
			  	  	if (debug) fprintf(dgfp,"Resetting bl value of %d\n",current_bl);
#endif
					set_backlight (current_bl);
			  	 	dimmed=0;
				}		
						
			}
			
#ifdef DEBUG
			else if (debug) fprintf(dgfp,"No activity. Newidle=%d, oldidle=%d\n", newIdle, oldIdle);
#endif
			
			if (dimming  && !dimmed && !apm_active) {
				if ((newIdle-oldIdle)>=dim_idle) {
					current_bl = read_backlight ();
					set_backlight (dim_level);
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
#ifdef DEBUG
				if (debug) fprintf(dgfp,"Resetting bl value of %d\n",current_bl);
#endif
				set_backlight (current_bl);
				dimmed=0;	
			}
		}
		old_apm=apm_active;
		
		if (activity == 0) {
			f=fopen(INTERRUPTS, "r");
			if (! f) fprintf(stderr, "problem opening %s\n", INTERRUPTS);
			else {
				while(fgets(iline,sizeof(iline),f)) {
					if (sscanf(iline,"%d: %ld",&i, &v) == 2 && 
					    irqs[i] && irq_count[i] != v) {
						activity=1;
#ifdef DEBUG
						if (debug) fprintf(dgfp,"IRQ activity %d\n", i);
#endif
						irq_count[i] = v;
					}
				}
				fclose(f);
			}
		}

		if (activity) {
			total_unused = 0;
		}
		else {
			total_unused += sleep_time;
			if (total_unused >= sleep_idle && sleeping) {
				
				if (check_cpu() || check_PID() || probe_IRQs()) {
#ifdef DEBUG
					if (debug) fprintf(dgfp,"You cannot sleep at this time! Not going to sleep....\n");
#endif
					total_unused=0;
					oldIdle=newIdle;
				}
				else {
#ifdef DEBUG
					if (debug) fprintf(dgfp,"Going to sleep....\n");
#endif
					do_sleep ();
					total_unused=0;
				}
			}
			
		}

#ifdef DEBUG		
		if (debug) fflush(dgfp);
#endif
		sleep(sleep_time);
		
		newTime=time(NULL);
		if (oldTime && newTime-sleep_time > oldTime +1) {
#ifdef DEBUG
			if (debug)
			  fprintf(stderr, "%i sec sleep; resetting timer and resetting dimmer...", (int)(newTime - oldTime));
#endif
			total_unused=0;
			
			query_idle(&idleTime);
			lastIdle=oldIdle=(int)idleTime;
#ifdef DEBUG
			if (debug) fprintf(dgfp,"Re-setting oldIdle %d.\n", oldIdle);
#endif
			
			if (dimming && dimmed) {
#ifdef DEBUG
			  	  if (debug) fprintf(dgfp,"Resetting bl value of %d\n",current_bl);
#endif
				  set_backlight (current_bl);
				  dimmed=0;
			}
		}
		oldTime=newTime;	
		
	}
}

int main (int argc, char **argv) {
	int try=0, tmpX=0, checkinit=0;
	
	signal (SIGCHLD, SIG_IGN);
	parse_command_line(argc, argv);

#ifdef DEBUG
	if (debug) {
		dgfp=fopen("/tmp/ipaq-sleep.log", "a");
		if (! dgfp) {
			fprintf(stderr, "problem opening ipaq-sleep.log\n");
			exit(1);
		}
	}
#endif
	
	if (daemonize) {
		if (daemon(1,1) == -1) {
			fprintf(stderr, "auto-sleep daemon problems\n");
			exit(1);
		}
	}
	
	checkinit=init();
	
	while (try<10) {
		if (checkinit) {
#ifdef DEBUG
			if (debug) fprintf(dgfp,"Waiting for X....\n");
#endif
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
#ifdef DEBUG
		if (debug) fprintf(dgfp, "It looks like X is not running. You might be somewhat hosed....\n");
#endif
		X=tmpX;
	
	}
	
	main_loop();
	
	return(0); // never reached
}
