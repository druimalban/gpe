/*
 *	Lightweight At Daemon
 *
 *	Compile with:
 *		gcc -s -Wall -Wstrict-prototypes -Os atd.c -o atd
 *
 *	Copyright (C) 1996, Paul Gortmaker
 *	Copyright (C) 2001, Russell Nelson
 *	Copyright (C) 2002, Nils Faerber <nils@handhelds.org>
 *
 *	Released under the GNU General Public License, version 2,
 *	included herein by reference.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <pwd.h>

int compare_rtc_to_tm(struct rtc_time *rtc, struct tm *tm)
{
int i;

	i = rtc->tm_year - tm->tm_year;
	if (!i) i = rtc->tm_mon - tm->tm_mon;
	if (!i) i = rtc->tm_mday - tm->tm_mday;
	if (!i) i = rtc->tm_hour - tm->tm_hour;
	if (!i) i = rtc->tm_min - tm->tm_min;
	if (!i) i = rtc->tm_sec - tm->tm_sec;
	if (!i)
		return 0;
	if (i > 0)
		return 1;
return -1;
}

void waitfor(time_t t) {
int rtcfd, tfd, retval= 0;
unsigned long data;
struct rtc_time rtc_tm;
time_t now, then;
struct tm *tm;
struct timeval tv;
int nfds;
fd_set afds;

	printf("waitfor %ld\n", t);
	rtcfd = open ("/dev/rtc", O_RDONLY);

	if (rtcfd ==  -1) {
		perror("/dev/rtc");
		exit(errno);
	}

	/* Read the RTC time/date */
	tfd = open ("trigger", O_RDWR);

	if (tfd ==  -1) {
		perror("trigger");
		exit(errno);
	}

	/* Set the RTC time/date */
	now = time(NULL);
	tm = gmtime(&now);
	rtc_tm.tm_mday = tm->tm_mday;
	rtc_tm.tm_mon = tm->tm_mon;
	rtc_tm.tm_year = tm->tm_year;
	rtc_tm.tm_hour = tm->tm_hour;
	rtc_tm.tm_min = tm->tm_min;
	rtc_tm.tm_sec = tm->tm_sec;
	retval = ioctl(rtcfd, RTC_SET_TIME, &rtc_tm);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	printf("Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
		rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	tm = gmtime(&t);
	printf("Alarm date/time is %d-%d-%d, %02d:%02d:%02d.\n",
		tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	if (t && compare_rtc_to_tm(&rtc_tm, tm) >= 0) {
		close(rtcfd);
		close(tfd);
		return;
	}

	if (t) {
		/* set the alarm */
		rtc_tm.tm_mday = tm->tm_mday;
		rtc_tm.tm_mon = tm->tm_mon;
		rtc_tm.tm_year = tm->tm_year;
		rtc_tm.tm_sec = tm->tm_sec;
		rtc_tm.tm_min = tm->tm_min;
		rtc_tm.tm_hour = tm->tm_hour;
		retval = ioctl(rtcfd, RTC_ALM_SET, &rtc_tm);
		if (retval == -1) {
			perror("ioctl");
			exit(errno);
		}
		printf("Alarm date/time now set to %d-%d-%d, %02d:%02d:%02d.\n",
			rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

		/* Read the current alarm settings */
		retval = ioctl(rtcfd, RTC_ALM_READ, &rtc_tm);
		if (retval == -1) {
			perror("ioctl");
			exit(errno);
		}

		printf("Alarm date/time now in RTC: %d-%d-%d, %02d:%02d:%02d.\n",
			rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
			rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

		/* Enable alarm interrupts */
		retval = ioctl(rtcfd, RTC_AIE_ON, 0);
		if (retval == -1) {
			perror("ioctl");
			exit(errno);
		}
	}

	printf("Waiting for alarm...");
	fflush(stdout);
	/* This blocks until the alarm ring causes an interrupt */
	FD_ZERO(&afds);
	if (t)
		FD_SET(rtcfd, &afds);
	FD_SET(tfd, &afds);
	nfds = rtcfd+1;
	if (tfd > rtcfd)
		nfds = tfd + 1;
	/* Wait up to ten minutes. */
	tv.tv_sec = 10*60;
	tv.tv_usec = 0;
	then = now;
	if (select(nfds, &afds, (fd_set *) 0, (fd_set *) 0, &tv) < 0) {
		if (errno != EINTR)
			perror("select");
		exit(errno);
	}
	now = time(NULL);
	printf("While we were sleeping, %d seconds elapsed, but %d alarms passed\n",
		(int)(now - then), (int)(tv.tv_sec));
	if (FD_ISSET(rtcfd, &afds)) {
		retval = read(rtcfd, &data, sizeof(unsigned long));
		if (retval == -1) {
			perror("read");
			exit(errno);
		}
	}
	if (FD_ISSET(tfd, &afds)) {
		retval = read(tfd, &data, 1);
		if (retval == -1) {
			perror("read");
			exit(errno);
		}
	}
	printf("Something happened!\n");

	/* Disable alarm interrupts */
	retval = ioctl(rtcfd, RTC_AIE_OFF, 0);
	if (retval == -1) {
		perror("ioctl");
		exit(errno);
	}

	close(rtcfd);
	close(tfd);
}


int runjob(char *fn)
{
pid_t pid;
unsigned int status;
struct stat sbuf;

	pid = fork();
	if (pid == -1)
		return -1;
	if (pid == 0) {
		if (getuid()==0 && geteuid()==0) {
			struct passwd *userpwd = NULL;

			printf("I am root so I am going to suid() me to ");
			stat(fn, &sbuf);
			if (setgid(sbuf.st_gid) != 0)
				perror("setgid");
			if (setuid(sbuf.st_uid) != 0)
				perror("setuid");
			printf("uid %d and gid %d\n",sbuf.st_uid,sbuf.st_gid);
			userpwd = getpwuid(sbuf.st_uid);
			if (userpwd != NULL) {
				setenv("USER",userpwd->pw_name,1);
				setenv("LOGNAME",userpwd->pw_name,1);
				setenv("HOME",userpwd->pw_dir,1);
			} else
				perror("getpwuid()");
		}
		printf("running %s\n", fn);
		execl(fn, fn, NULL);
		exit(127);
	}
	do {
		printf("waiting for %d\n", pid);
		if (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR)
				return -1;
		}
		break;
	} while(1);

return 0;
}

unsigned long nameok(const char *name)
{
char *middle;
char *end;
unsigned long this;

	/* check for timestamp.pid */
	this = strtol(name, &middle, 10);
	if (*middle != '.')
		return 0;
	strtol(middle+1, &end, 10);
	if (name + strlen(name) != end)
		return 0;

return this;
}

int scan_i = 0;
DIR *scan_dir = NULL;
char *dirs[] = { "/var/spool/at", /* "/var/spool/cron", */ NULL };


void scan_from_top(void)
{
	if (scan_dir) closedir(scan_dir);
		scan_dir = NULL;
	scan_i = 0;
}

struct dirent *scan(void)
{
struct dirent *dirent;

	if (!scan_dir) {
		if (!dirs[scan_i]) {
			scan_i = 0;
			return NULL;
		} else
			scan_dir = opendir(dirs[scan_i++]);
	}
	dirent = readdir(scan_dir);
	if (dirent)
		return dirent;
	closedir(scan_dir);
	scan_dir = NULL;

return scan();
}

void die(char *s) {
	fprintf(stderr, "%s\n", s);
	exit(2);
}

void die2(char *s, char *a1) {
	fprintf(stderr, s, a1);
	exit(2);
}

void exit_atq(void) {
}

int main(int argc, char *argv[]) {
struct dirent *dirent;
unsigned long this, next, now;
char *argv0;

	argv0 = strrchr(argv[0], '/');
	if (!argv0)
		argv0 = argv[0];

	if (argc != 2)
		die2("usage: %s spooldir\n",argv0);

	if (chdir(argv[1]) < 0)
		die("cannot chdir");
	if (!strcmp(argv0, "atq"))
		exit_atq();
	if (mkfifo("trigger.new", 0777) < 0)
		die("cannot mkfifo trigger.new");
	if (rename("trigger.new","trigger"))
		die("cannot rename trigger.new");
	chmod("trigger", S_IWUSR | S_IWGRP | S_IWOTH);

	while(1) {
		/* run all the jobs in the past */
		now = time(NULL);
		scan_from_top();
		while ((dirent = scan())) {
			this = nameok(dirent->d_name);
			/* run jobs which are in the past, and find the next job in the future */
			/* avoid race conditions. Run jobs scheduled for the next second now */
			if (this)
				if (this <= now + 1)
					runjob(dirent->d_name);
		} /* end while */

		/* find the next job in the future.
		 * A job we just ran might have rescheduled itself.  */
		scan_from_top();
		next = ULONG_MAX;
		while ((dirent = scan())) {
			this = nameok(dirent->d_name);
			/* schedule jobs that we haven't just run */
			if (this) 
				if (this > now + 1)
					if (this < next) next = this;
		} /* end while */

		printf("next: %ld\n", next);
		if (next == ULONG_MAX)
			next = 0;
		 waitfor(next);
	}

return 0;
} 
