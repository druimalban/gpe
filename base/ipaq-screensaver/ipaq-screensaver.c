#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/h3600_ts.h>
#include <linux/fb.h>
#include <signal.h>

#define TS_DEV "/dev/h3600_tsraw"
#define FB_DEV "/dev/fb0"

/* internal state
 * 0 = initial, waiting for 1st timeout
 * 1 = first timeout occured, waiting for 2nd
 * 2 = second timeout occured, waiting for 3rd
 * 3 = third timeout occured, waiting for activity
 */
static int state;
static struct h3600_ts_backlight orgstate;

/*
 * 3 timeouts:
 * 1. dim timeout (to minmum 0)
 * 2. switch off backlight
 * 3. switch off display and start led flashing (!)
 */
int tmos[]={20, 60, 90};

void display_power(unsigned char pstate)
{
int fd;

        fd=open(FB_DEV,O_RDWR);
        if (fd==-1) {
                fprintf(stderr,"Could not open fb0\n");
                exit(1);
        }
        if (pstate == 0) {
                if (ioctl(fd,FBIOBLANK,VESA_POWERDOWN)==-1) {
                        fprintf(stderr,"ioctl() failed\n");
                }
        } else if (pstate == 1) {
                if (ioctl(fd,FBIOBLANK,VESA_NO_BLANKING)==-1) {
                        fprintf(stderr,"ioctl() failed\n");
                }
        }
        close(fd);
}

void backlight_get_state(struct h3600_ts_backlight *blstate)
{
int fd;
	fd=open(TS_DEV, O_RDWR);
	if (fd < 0) {
		fprintf(stderr,"could not open touchscreen device '%s'\n", TS_DEV);
		perror("open");
		exit(1);
	};
	ioctl(fd, TS_GET_BACKLIGHT, blstate);
	close(fd);
}

void backlight_set_state(struct h3600_ts_backlight *blstate)
{
int fd;
	fd=open(TS_DEV, O_RDWR);
	if (fd < 0) {
		fprintf(stderr,"could not open touchscreen device '%s'\n", TS_DEV);
		perror("open");
		exit(1);
	};
	ioctl(fd, TS_SET_BACKLIGHT, blstate);
	close(fd);
}

void led_set(unsigned char pstate)
{
#if 0
struct h3600_ts_led ledstate;
int fd;

	if (pstate == 1) {
		ledstate.OffOnBlink=1;
		ledstate.TotalTime=0; /* indefinite? */
		ledstate.OnTime=1;
		ledstate.OffTime=20;
	} else {
		ledstate.OffOnBlink=0;
		ledstate.TotalTime=1;
		ledstate.OnTime=1;
		ledstate.OffTime=20;
	};
	fd=open(TS_DEV, O_RDWR);
	if (fd < 0) {
		fprintf(stderr,"could not open touchscreen device '%s'\n", TS_DEV);
		perror("open");
		exit(1);
	};
	ioctl(fd, LED_ON, &ledstate);
	close(fd);
#else
	if (pstate)
		system("led 1 0 1 20");
	else
		system("led 0 0 0 0");
#endif
}

void timeout_alarm(int signo)
{
struct h3600_ts_backlight setstate;

#ifdef DEBUG
	fprintf(stderr,"timeout on state %d\n",state);
#endif
	switch (state) {
		case 0:
			state=1;
			/* dim */
			backlight_get_state(&orgstate);
			setstate.power=orgstate.power;
			setstate.brightness=0;
			backlight_set_state(&setstate);
			alarm(tmos[state]);
			break;
		case 1:
			state=2;
			/* backlight off */
			setstate.power=0;
			setstate.brightness=0;
			backlight_set_state(&setstate);
			alarm(tmos[state]);
			break;
		case 2:
			state=3;
			/* display off */
			display_power(0);
			/* led on */
			led_set(1);
			break;
	}
}

int get_irq_val()
{
FILE *filp;
static char buffer[255];
static char inum[255];
static char irqs[255];
static char name[255];
int val=0;
int res;

	filp=NULL;
	filp=fopen("/proc/interrupts","r");
	if (filp == NULL) {
		fprintf(stderr,"could not open /proc/interrupts\n");
		return 0;
	}
	do {
		fgets(buffer,255,filp);
		if (buffer != NULL) {
			res=sscanf(buffer,"%s %s %s",inum,irqs,name);
			if (res == 3) {
/*				fprintf(stderr,"get_irq_val() name='%s'\n",name);*/
				if (strcmp(name,"h3600_ts")==0)
					sscanf(irqs,"%d",&val);
			}
		}
	} while (!feof(filp));
	fclose(filp);

return val;
}

void wait_for_change()
{
int localstate;
int val1, val2;

	localstate=state;
	val1=get_irq_val();
	while (1) {
		val2=get_irq_val();
/*		fprintf(stderr,"v1=%d v2=%d ls=%d s=%d\n",val1,val2,localstate,state);*/
		if (val1 != val2) {
			if (localstate == state) {
				return;
			} else {
				localstate = state;
				usleep(450001);
				val1 = get_irq_val();
			}
		}
		usleep(450001);
	}
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		if (argc < 4) {
			fprintf(stderr,"Usage:\n\t%s <t1> <t2> <t3>\n",argv[0]);
		} else {
			int i;
			for (i=1; i<argc; i++)
				tmos[i-1]=atoi(argv[i]);
		}
	}

	signal(SIGALRM, timeout_alarm);
	perror("signal");
	backlight_get_state(&orgstate);
	state=0;
	alarm(tmos[state]);
	led_set(0);
	while(1) {
		wait_for_change();
		if (state != 0) {
			/* restore settings */
			backlight_set_state(&orgstate);
			if (state > 2) {
				/* switch it on again */
				display_power(1);
				/* if led was on switch it off again */
				led_set(0);
			}
			state=0;
		}
		alarm(tmos[0]);
	}
return 0;
}
