#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "rtcd.h"


/* filedescriptor for the RTC device, needs to be global because we use it */
/* inside the signal handler */
static int rtcfd;
static unsigned long rtcd_recv_tmo=0;

/*
 * This stores the list of alarms,
 * global so that we can access it inside a handler
 */
static struct rtcd_alarms_t *rtcd_alarms;


time_t rtc_get_time()
{
struct tm mytm;

	ioctl(rtcfd, RTC_RD_TIME, &mytm);
	mytm.tm_isdst=1;

return (mktime(&mytm));
}


/*
 * Is called when an alarm was added or deleted in order
 * to update the hardware RTC alarm if necessary
 * (the new alarm might be nearer than the current one or
 *  the deleted alarm was the current pending RTC alarm).
 */
int update_rtc_alarm(void)
{
struct tm *alrm_time;
struct tm rtc_tm;
time_t RTC_ALARM,ttmp;
struct rtcd_alarms_t *walker;

	if (rtcd_alarms!=NULL && rtcd_alarms->alarm!=NULL) {
		ttmp=rtc_get_time();
		/* when the actual alarm is now then there is no need to set it */
		walker=rtcd_alarms;
		while ((walker != NULL) && (walker->alarm->when <= ttmp)) {
#ifdef DEBUG
			fprintf(stderr, __func__ ": alarm->when (%d) <= rtc_get_time (%d)\n",(unsigned int)rtcd_alarms->alarm->when, (unsigned int)ttmp);
#endif
			walker=walker->next;
		}
		if (walker == NULL)
			return -1;
		RTC_ALARM=walker->alarm->when;
		alrm_time=localtime(&RTC_ALARM);
		if (ioctl(rtcfd, RTC_AIE_OFF, 0) < 0) {
			perror("ioctl RTC_AIE_OFF");
			return -1;
		}
		/* set the alarm */
		if (ioctl(rtcfd, RTC_ALM_SET, alrm_time) < 0) {
			perror("ioctl RTC_ALM_SET");
			return -1;
		}
		ioctl(rtcfd, RTC_ALM_READ, &rtc_tm);
#ifdef DEBUG
		fprintf(stderr, __func__ ": Alarm time now set to %02d:%02d:%02d.\n",
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
#endif
		/* enable RTC alarms */
		if (ioctl(rtcfd, RTC_AIE_ON, 0) < 0) {
			perror("ioctl RTC_AIE_ON");
			return -1;
		}
	}
#ifdef DEBUG
	 else {
		fprintf(stderr,__func__ ": update_rtc_alarm() called but list empty\n");
	}
#endif
return 0;
}


/*
 * add an alarm to the list
 */
int rtcd_add_alarm(struct rtcd_alarm_t *rtcd_alarm)
{
struct rtcd_alarms_t *walker;
struct rtcd_alarms_t *walker_prev;
struct rtcd_alarms_t *new_alrm;

	walker = rtcd_alarms;
	walker_prev = NULL;
	if (walker->alarm != NULL) {
		/* prepare a new list entry */
		new_alrm=(struct rtcd_alarms_t *)malloc(sizeof(struct rtcd_alarms_t));
		if (new_alrm == NULL)
			return -1;
		new_alrm->alarm=rtcd_alarm;
		new_alrm->next=NULL;

		/* walk through the list */
		do {
			if (rtcd_alarm->when < walker->alarm->when) {
#ifdef DEBUG
				fprintf(stderr, __func__ ": new alarm < list alarm\n");
#endif
				break;
			}
			if (walker->next != NULL) {
				walker_prev = walker;
				walker = walker->next;
			} else
				break;
		} while (1) ;

		/* if the new alarm is next link it in here */
		if ((walker->next != NULL) && (walker != rtcd_alarms)) {
#ifdef DEBUG
			fprintf(stderr,__func__ ": somewhere in list\n");
#endif
			if (walker_prev != NULL) {
#ifdef DEBUG
				fprintf(stderr, __func__ ": got predecessor\n");
#endif
				new_alrm->next=walker_prev->next;
				walker_prev->next=new_alrm;
			} else {
#ifdef DEBUG
				fprintf(stderr, __func__ ": got NO predecessor\n");
#endif
				new_alrm->next=walker->next;
				walker->next=new_alrm;
			}
		} else if (walker != rtcd_alarms) {
#ifdef DEBUG
			fprintf(stderr,__func__ ": end of list\n");
#endif
			if (rtcd_alarm->when < walker->alarm->when) {
				if (walker_prev != NULL) {
					new_alrm->next = walker;
					walker_prev->next=new_alrm;
				} else
					walker->next=new_alrm;
			} else {
				walker->next=new_alrm;
			}
		} else {
#ifdef DEBUG
			fprintf(stderr,__func__ ": beginning of list\n");
#endif
			if (rtcd_alarm->when < walker->alarm->when) {
				new_alrm->next = rtcd_alarms;
				rtcd_alarms = new_alrm;
			} else {
				rtcd_alarms->next = new_alrm;
			}
		}
	} else {
#ifdef DEBUG
		fprintf(stderr,__func__ ": first of list\n");
#endif
		rtcd_alarms->alarm=rtcd_alarm;
		rtcd_alarms->next=NULL;
	}
	update_rtc_alarm();

return 0;
}


/*
 * delete a specific alarm from the list
 * alarm is specified by calling processe's PID and time of the alarm
 */
int rtcd_del_alarm(struct rtcd_alarm_t *rtcd_alarm)
{
struct rtcd_alarms_t *walker;
struct rtcd_alarms_t *prev;

	prev=NULL;
	walker=rtcd_alarms;
	while (walker->alarm != NULL) {
		if ((walker->alarm->pid == rtcd_alarm->pid) &&
		    (walker->alarm->when == rtcd_alarm->when))
			break;
		prev=walker;
		if (walker->next != NULL)
			walker=walker->next;
		else
			break;
	};
	/* found one in the list ? */
	if (walker->alarm == NULL)
		return -1;
	if (walker!=NULL && prev!=NULL) {
		prev->next=walker->next;
		if (walker->alarm->data_length != 0)
			free(walker->alarm->data);
		free(walker->alarm);
		free(walker);
		return 0;
	};
	/* found first */
	if (walker!=NULL && prev==NULL) {
		if (walker->next!=NULL) {
			walker=walker->next;
			if (rtcd_alarms->alarm->data_length != 0)
				free(rtcd_alarms->alarm->data);
			free(rtcd_alarms->alarm);
			rtcd_alarms->alarm=walker->alarm;
			rtcd_alarms->next=walker->next;
			free(walker);
		} else {
			if (walker->alarm->data_length != 0)
				free(walker->alarm->data);
			free(walker->alarm);
			rtcd_alarms->alarm=NULL;
			rtcd_alarms->next=NULL;
		};
		update_rtc_alarm();
		return 0;
	} else {
		return -1; /* not found at all */
	}

return 0;
}


/*
 * Dump the current list of alarms to stderr
 * only used for debugging
 */
void dump_list()
{
struct rtcd_alarms_t *rtcd_tmp;

	rtcd_tmp=rtcd_alarms;
	while((rtcd_tmp!=NULL) && (rtcd_tmp->alarm != NULL)) {
#ifdef DEBUG
		fprintf(stderr,__func__": from %s (pid %d) at %s\n",rtcd_tmp->alarm->name,rtcd_tmp->alarm->pid,ctime(&rtcd_tmp->alarm->when));
#endif
		rtcd_tmp=rtcd_tmp->next;
	}
}

/*
 * Similar to dump_list() but sends the current list of alarms back over
 * the socket to a client process
 */
void rtcd_send_alarmlist(int sockdesc)
{
struct rtcd_alarms_t *rtcd_tmp;
struct rtcd_alarm_packet_t alrmpack;
unsigned int nullval=0;

	rtcd_tmp=rtcd_alarms;
	while((rtcd_tmp!=NULL) && (rtcd_tmp->alarm != NULL)) {
		alrmpack.func=RTCD_GET_ALARM;
		alrmpack.pid=rtcd_tmp->alarm->pid;
		strcpy(alrmpack.name,rtcd_tmp->alarm->name);
		strcpy(alrmpack.comment,rtcd_tmp->alarm->comment);
		alrmpack.when=rtcd_tmp->alarm->when;
		if (write(sockdesc,&alrmpack,sizeof(struct rtcd_alarm_packet_t)) == -1)
			return;
		if (write(sockdesc,&rtcd_tmp->alarm->data_length,sizeof(unsigned int)) == -1)
			return;
		if (rtcd_tmp->alarm->data_length != 0) {
			if (write(sockdesc,rtcd_tmp->alarm->data,rtcd_tmp->alarm->data_length) == -1)
				return;
		}
		rtcd_tmp=rtcd_tmp->next;
	}
	alrmpack.func=RTCD_GET_ALARM;
	alrmpack.pid=0;
	strcpy(alrmpack.name,"");
	strcpy(alrmpack.comment,"");
	alrmpack.when=0;
	write(sockdesc,&alrmpack,sizeof(struct rtcd_alarm_packet_t));
	write(sockdesc,&nullval,sizeof(unsigned int));
}


/*
 * Change PIDs of a client,
 * Client sends its name and new PID, check complete list for matching
 * alarm entries and update PIDs
 */
int rtcd_set_pid(struct rtcd_alarm_t *rtcd_alarm)
{
struct rtcd_alarms_t *walker;

#ifdef DEBUG
	fprintf(stderr, __func__ ": for '%s' to PID %d\n",rtcd_alarm->name,rtcd_alarm->pid);
#endif
	walker=rtcd_alarms;
	while (walker != NULL) {
		if (walker->alarm != NULL)
			if (walker->alarm->name != NULL)
				if (strcmp(walker->alarm->name,rtcd_alarm->name)==0) {
#ifdef DEBUG
					fprintf(stderr, __func__ ": found alarm, changin PID %d to %d\n",walker->alarm->pid,rtcd_alarm->pid);
#endif
					walker->alarm->pid = rtcd_alarm->pid;
				}
		walker=walker->next;
	}

return 0;
}

/*
 * Check RTC's alarm status
 * If an alarm rang send the corresponding process a signal and
 * update the RTC alarm again
 */
void rtcd_check_rtcalarm(void)
{
unsigned long rtc_data=0;
struct rtcd_alarm_t *rtcd_alarm;
struct rtcd_alarms_t *walker;
time_t ttmp;

	if (read(rtcfd,&rtc_data,sizeof(unsigned long))!=0 && (rtc_data!=0)) {
#ifdef DEBUG
		fprintf(stderr,__func__": RTC alarm rang %ld %d\n",rtc_data,errno);
#endif
		ioctl(rtcfd, RTC_AIE_OFF, 0);
		/* Search for corresponding alarm, first match */
		walker=rtcd_alarms;
		ttmp=rtc_get_time(); /*time(NULL);*/
		while ((walker != NULL) && (walker->alarm != NULL)) {
			if (walker->alarm->when == ttmp) {
				rtcd_alarm=walker->alarm;
				/* Notify the process who registered this alarm */
#ifdef DEBUG
				fprintf(stderr, __func__ ": kill(%d, SIGUSER1)\n",rtcd_alarms->alarm->pid);
#endif
				if (kill(rtcd_alarms->alarm->pid, SIGUSR1) < 0)
					perror("kill SIGUSR1");
			}
			walker=walker->next;
		}
		/* look for new alarm */
		update_rtc_alarm();
	}
#ifdef DEBUG
	else {
		fprintf(stderr,__func__": RTC returned nothing\n");
	};
	dump_list();
#endif
}


/*
 * Called when rtcd is interrupted, just cleans up and exits
 */
void rtcd_sigint(int sig)
{
	unlink(RTCD_SOCKET);
	exit(sig);
}

/*
 * Create the rtcd daemon process waiting for connections
 */
pid_t rtcd_daemonize(int flags)
{
pid_t dpid;
int sockdesc,recv_sock;
struct sockaddr my_addr;
struct rtc_time rtc_tm;
struct sockaddr client_addr;
int client_addrl,retval;
int readlen,recvlen,selres;
struct rtcd_alarm_packet_t recv_packet;
struct rtcd_alarm_t *rtcd_alarm;
fd_set sockfdset;
struct timeval seltmo;
unsigned int data_length;
void *data;
/*unsigned long tmplong;*/

	if (flags) {
#ifdef DEBUG
		fprintf(stderr,__func__ ": would go into background\n");
#endif
		if ((dpid=fork())!=0) {
			return dpid;
		}
	}
#ifdef DEBUG
	else {
		fprintf(stderr,__func__ ": staying in foreground...\n");
	}
#endif

	/* open the RTC device */
	rtcfd=open(RTC_DEVICE, O_RDONLY | O_NONBLOCK);
	if (rtcfd <= 0) {
		perror(RTC_DEVICE);
		exit(errno);
	};

	/* create our socket */
	if ((sockdesc=socket(PF_UNIX, SOCK_STREAM, 0)) <= 0) {
		perror("socket");
		close(rtcfd);
		exit(errno);
	};

	/* bind the socket to a local Unix domain socket */
	my_addr.sa_family=AF_UNIX;
	strcpy(my_addr.sa_data,RTCD_SOCKET);
	if (bind(sockdesc, &my_addr, sizeof(my_addr)) < 0) {
		perror("bind " RTCD_SOCKET);
		close(rtcfd);
		close(sockdesc);
		exit(errno);
	};

	if (signal(SIGINT,rtcd_sigint)==SIG_ERR) {
		perror("signal SIGINT");
		close(rtcfd);
		close(sockdesc);
		exit(errno);
	};
	
	if (signal(SIGTERM,rtcd_sigint)==SIG_ERR) {
		perror("signal SIGINT");
		close(rtcfd);
		close(sockdesc);
		exit(errno);
	};
	
	ioctl(rtcfd, RTC_RD_TIME, &rtc_tm);
#ifdef DEBUG
	fprintf(stderr, __func__ ": RTC time is %02d:%02d:%02d.\n",
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
#endif

	/* start listening on the socket and wait for an incoming connection */
	/* we will have to poll for RTC events */
	listen(sockdesc, 10);
	while (1) {
		client_addrl=sizeof(client_addr);
		FD_ZERO(&sockfdset);
		FD_SET(sockdesc,&sockfdset);
		FD_SET(rtcfd,&sockfdset);
		selres=select(sockdesc + 1,&sockfdset,NULL,NULL,NULL);
		if ((selres > 0) && FD_ISSET(sockdesc, &sockfdset)) {
			recv_sock=accept(sockdesc,&client_addr,&client_addrl);
		} else if ((selres > 0) && FD_ISSET(rtcfd, &sockfdset)) {
			/*read(rtcfd,&tmplong,sizeof(unsigned long));*/
			rtcd_check_rtcalarm();
			continue;
		} else {
#ifdef DEBUG
			fprintf(stderr,__func__": 1st select timeout\n");
#endif
			continue;
		}
#ifdef DEBUG
		fprintf(stderr,__func__": accepted connection!\n");
#endif
		rtcd_recv_tmo=time(NULL)+10;
		recvlen=0;
		FD_ZERO(&sockfdset);
		FD_SET(recv_sock,&sockfdset);
		while ((time(NULL) <= rtcd_recv_tmo) && (recvlen < sizeof(struct rtcd_alarm_packet_t))) {
			seltmo.tv_sec=1;
			seltmo.tv_usec=0;
			selres=select(recv_sock+1,&sockfdset,NULL,NULL,&seltmo);
			if ((selres > 0) && FD_ISSET(recv_sock,&sockfdset)) {
				readlen=read(recv_sock,&recv_packet+recvlen,sizeof(struct rtcd_alarm_packet_t));
				if (readlen!=0)
					recvlen+=readlen;
				else
					break;
#ifdef DEBUG
				fprintf(stderr,__func__": recved %d\n",recvlen);
#endif
			}
			if (selres == 0) {
				rtcd_check_rtcalarm();
			}
			if (selres < 0) {
#ifdef DEBUG
				fprintf(stderr,__func__": select error\n");
				perror("select");
#endif
				break;
			}
			FD_ZERO(&sockfdset);
			FD_SET(recv_sock,&sockfdset);
		}
#ifdef DEBUG
		fprintf(stderr,__func__": Ending receive...\n");
#endif
		if (recvlen == sizeof(struct rtcd_alarm_packet_t)) {
			/* the packet was complete so we process the request */
#ifdef DEBUG
			fprintf(stderr,__func__ ": packet complete...\nfrom '%s' (%d) at %d for '%s'\n",recv_packet.name, recv_packet.pid, (unsigned int)recv_packet.when, recv_packet.comment);
#endif
			rtcd_alarm=(struct rtcd_alarm_t *) malloc (sizeof (struct rtcd_alarm_t));
			rtcd_alarm->pid=recv_packet.pid;
			strcpy(rtcd_alarm->name,recv_packet.name);
			rtcd_alarm->when=recv_packet.when;
			strcpy(rtcd_alarm->comment,recv_packet.comment);
			if (recv_packet.func == RTCD_SET_ALARM) {
#ifdef DEBUG
				fprintf(stderr,__func__": Func RTCD_SET_ALARM %ld (%ld)\n",(unsigned long)rtcd_alarm->when,(unsigned long)time(NULL));
#endif
				read(recv_sock,&data_length,sizeof(data_length));
#ifdef DEBUG
				fprintf(stderr,__func__": additional data_length = %d\n",data_length);
#endif
				if (data_length != 0) {
					data=malloc(data_length);
					read(recv_sock,data,data_length);
#ifdef DEBUG
					fprintf(stderr,__func__":   got data\n");
#endif
					rtcd_alarm->data_length=data_length;
					rtcd_alarm->data=data;
				} else {
					rtcd_alarm->data_length=0;
					rtcd_alarm->data=NULL;
				}
				retval=rtcd_add_alarm(rtcd_alarm);
				write(recv_sock,&retval,sizeof(int));
			} else
			if (recv_packet.func == RTCD_DEL_ALARM) {
#ifdef DEBUG
				fprintf(stderr,__func__": Func RTCD_DEL_ALARM\n");
#endif
				retval=rtcd_del_alarm(rtcd_alarm);
				free(rtcd_alarm);
				write(recv_sock,&retval,sizeof(int));
			} else
			if (recv_packet.func == RTCD_GET_ALARM) {
#ifdef DEBUG
				fprintf(stderr,__func__": Func RTCD_GET_ALARM\n");
#endif
				rtcd_send_alarmlist(recv_sock);
				free(rtcd_alarm);
			} else
			if (recv_packet.func == RTCD_SET_PID) {
#ifdef DEBUG
				fprintf(stderr,__func__": Func RTCD_SET_PID\n");
#endif
				rtcd_set_pid(rtcd_alarm);
				free(rtcd_alarm);
			}


		}
		close(recv_sock);
		recvlen=0;
	}

	/* cleanup */
	if (close(sockdesc) != 0)
		perror("close(sockdesc)");
	unlink(RTCD_SOCKET);
	close(rtcfd);
return 0;
}


/*
 * main - the obvious ;)
 */
int main(int argc, char **argv)
{
pid_t dpid;

	rtcd_alarms=(struct rtcd_alarms_t *)malloc(sizeof(struct rtcd_alarms_t));
	if (rtcd_alarms==NULL) {
		fprintf(stderr,__func__": malloc of alarm list failed\n");
		exit(1);
	}
	rtcd_alarms->alarm=NULL;
	rtcd_alarms->next=NULL;

	/* option -d stays in foreground */
	if (argc>1 && strcmp(argv[1],"-d")==0)
		rtcd_daemonize(0);
	else {
		dpid=rtcd_daemonize(1);
		return dpid;
	}

return 0;
}
