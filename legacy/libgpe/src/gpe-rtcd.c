#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>

#include "rtcd.h"

#define RTCD_SOCKET     "/tmp/rtcd"


/*
 * Connect to RTC daemon process
 */
static int connect_rtcd()
{
struct sockaddr rtcd_addr;
int sockdesc;

	if ((sockdesc=socket(PF_UNIX, SOCK_STREAM, 0)) <= 0) {
		perror("socket");
		return(-1);
	};
	rtcd_addr.sa_family=AF_UNIX;
	strcpy(rtcd_addr.sa_data,RTCD_SOCKET);
	if (connect(sockdesc, &rtcd_addr,sizeof(rtcd_addr))!=0) {
		perror("connect");
		return(-1);
	};
return(sockdesc);
}

/*
 * Schedule a RTC alarm specified by date/time as time_t
 */
int rtcd_set_alarm_time_t(char *name, char *comment, time_t when,
                          unsigned int data_length, void *data)
{
struct rtcd_alarm_packet_t alrmpack;
int sockdesc, retval;

	sockdesc=connect_rtcd();
	if (sockdesc == -1)
		return -1;
	alrmpack.func=RTCD_SET_ALARM;
	alrmpack.pid=getpid();
	strcpy(alrmpack.name,name);
	strcpy(alrmpack.comment,comment);
	alrmpack.when=when;
	if (write(sockdesc, &alrmpack, sizeof(struct rtcd_alarm_packet_t))<0)
		perror("write");
	if (write(sockdesc, &data_length, sizeof(data_length)) < 0)
		perror("write");
	if (data_length > 0)
		if (write(sockdesc, data, data_length) < 0)
			perror("write");
	if (read(sockdesc, &retval, sizeof(sizeof(int)))<0)
		perror("read");
	close(sockdesc);

return(retval);
}

/*
 * Delete a RTC alarm specified by date/time as time_t
 */
int rtcd_del_alarm_time_t(char *name, char *comment, time_t when)
{
struct rtcd_alarm_packet_t alrmpack;
int sockdesc, retval;

	sockdesc=connect_rtcd();
	if (sockdesc == -1)
		return -1;
	alrmpack.func=RTCD_DEL_ALARM;
	alrmpack.pid=getpid();
	strcpy(alrmpack.name,name);
	strcpy(alrmpack.comment,comment);
	alrmpack.when=when;
	if (write(sockdesc, &alrmpack, sizeof(struct rtcd_alarm_packet_t))<0)
		perror("write");
	if (read(sockdesc, &retval, sizeof(sizeof(int)))<0)
		perror("read");
	close(sockdesc);

return(retval);
}

/*
 * Schedule a RTC alarm specified by date/time as struct *tm
 */
int rtcd_set_alarm_tm(char *name, char *comment, struct tm *tm_when,
                      unsigned int data_length, void *data)
{
struct rtcd_alarm_packet_t alrmpack;
int sockdesc, retval;
time_t when;

	sockdesc=connect_rtcd();
	if (sockdesc == -1)
		return -1;
	alrmpack.func=RTCD_SET_ALARM;
	alrmpack.pid=getpid();
	strcpy(alrmpack.name,name);
	strcpy(alrmpack.comment,comment);
	when=mktime(tm_when);
	alrmpack.when=when;
	if (write(sockdesc, &alrmpack, sizeof(struct rtcd_alarm_packet_t))<0)
		perror("write");
	if (write(sockdesc, &data_length, sizeof(data_length)) < 0)
		perror("write");
	if (data_length > 0)
		if (write(sockdesc, data, data_length) < 0)
			perror("write");
	if (read(sockdesc, &retval, sizeof(sizeof(int)))<0)
		perror("read");
	close(sockdesc);

return(retval);
}

/*
 * Delete a RTC alarm specified by date/time as struct *tm
 */
int rtcd_del_alarm_tm(char *name, char *comment, struct tm *tm_when)
{
struct rtcd_alarm_packet_t alrmpack;
int sockdesc, retval;
time_t when;

	sockdesc=connect_rtcd();
	if (sockdesc == -1)
		return -1;
	alrmpack.func=RTCD_DEL_ALARM;
	alrmpack.pid=getpid();
	strcpy(alrmpack.name,name);
	strcpy(alrmpack.comment,comment);
	when=mktime(tm_when);
	alrmpack.when=when;
	if (write(sockdesc, &alrmpack, sizeof(struct rtcd_alarm_packet_t)) < 0) {
#ifdef DEBUG
		perror("write");
#endif
	}
	if (read(sockdesc, &retval, sizeof(sizeof(int)))<0)
		perror("read");
	close(sockdesc);

return(retval);
}

/*
 * This is ugly but I dunno how to do it any better...
 * only way to avoid this is to store the whole list temporarily here
 * but this IMHO can create memory leaks when noone frees the list later.
 * On the other hand if the
 * get_alarm_list() -> get_alarm_item() -> final_alarm_list()
 * cycle is not finished properly the socket is dead...
 * Oh well anyway... let's try it this way...
 */
static int listsockdesc = -1;

int rtcd_get_alarm_list(void)
{
struct rtcd_alarm_packet_t alrmpack;

	listsockdesc=connect_rtcd();
	if (listsockdesc == -1)
		return -1;
	memset(&alrmpack,0,sizeof(alrmpack));
	alrmpack.func=RTCD_GET_ALARM;
	if (write(listsockdesc, &alrmpack, sizeof(struct rtcd_alarm_packet_t)) < 0) {
#ifdef DEBUG
		perror("write");
#endif
		return -1;
	}

return 0;
}

int rtcd_get_alarm_item(pid_t *pid, char *name, time_t *when, char *comment,
                        unsigned int *data_length, void *data)
{
struct rtcd_alarm_packet_t alrmpack;
unsigned int rec_data_length;

	memset(&alrmpack,0,sizeof(alrmpack));
	if (read(listsockdesc, &alrmpack, sizeof(struct rtcd_alarm_packet_t)) < 0) {
#ifdef DEBUG
		perror("read");
#endif
	}
	*pid=alrmpack.pid;
	strcpy(name,alrmpack.name);
	*when=alrmpack.when;
	strcpy(comment,alrmpack.comment);
	rec_data_length = 0;
	if (read(listsockdesc, &rec_data_length, sizeof(unsigned int)) < 0) {
#ifdef DEBUG
		perror("read");
#endif
	}
	if (rec_data_length == *data_length) {
#ifdef DEBUG
		fprintf(stderr,"gpe-rtcd: got matching rec_data_length=%d\n",rec_data_length);
#endif
		if (read(listsockdesc, data, rec_data_length) < 0) {
#ifdef DEBUG
			perror("read");
#endif
			data=NULL;
		}
#ifdef DEBUG
		else {
			fprintf(stderr,"gpe-rtcd: got data\n");
		}
#endif
	} else {
		void *tmpbuffer;

		tmpbuffer=malloc(rec_data_length);
		read(listsockdesc, tmpbuffer, rec_data_length);
		free(tmpbuffer);
	}

return 0;
}

int rtcd_set_pid(pid_t pid, char *name)
{
struct rtcd_alarm_packet_t alrmpack;

	listsockdesc=connect_rtcd();
	if (listsockdesc == -1)
		return -1;
	memset(&alrmpack,0,sizeof(alrmpack));
	alrmpack.func=RTCD_SET_PID;
	alrmpack.pid=pid;
	strcpy(alrmpack.name, name);
	if (write(listsockdesc, &alrmpack, sizeof(struct rtcd_alarm_packet_t)) < 0) {
#ifdef DEBUG
		perror("write");
#endif
		return -1;
	}

return 0;
}

void rtcd_final_alarm_list(void)
{
	close(listsockdesc);
}
