/*
 * in:
 *   unsigned char func	-	function that should be performed
 *				0 - set this alarm
 *				1 - delete this alarm
 *				3 - get a list of all pending alarms
 *   pid_t pid		-	who is calling us
 *				we will watch this using waitpid()
 *   char[128] name	-	optional name of caller, not necessarily
 *				the real processe's name,
 *				empty if N/A
 *   time_t when	-	time of alarm (in seconds since epoch)
 *   char[128] comment	-	optional comment that can be shown in
 *				an alarm synopsis
 */

#ifndef _RTCD_H
#define _RTCD_H

/*
 * Some configuration definitions
 */
#define RTC_DEVICE      "/dev/rtc"      /* name of RTC device			*/
#define RTCD_SOCKET     "/tmp/rtcd"     /* name of socket    			*/
#define RTCD_POLL_INT   1               /* RTC polling interval (seconds)	*/


#define	RTCD_SET_ALARM	0
#define	RTCD_DEL_ALARM	2
#define	RTCD_GET_ALARM	3
#define	RTCD_SET_PID	10


/*
 * one alarm entry
 * as received as data packet from client
 */
struct rtcd_alarm_packet_t {
	unsigned char	func;
	pid_t		pid;
	char		name[128];
	time_t		when;
	char		comment[128];
};

/*
 * one alarm entry
 * for the internal list of alarms
 */
struct rtcd_alarm_t {
	pid_t		pid;
	char		name[128];
	time_t		when;
	char		comment[128];
};

/*
 * the internal list of alarms
 */
struct rtcd_alarms_t {
	struct rtcd_alarms_t *next;
	struct rtcd_alarm_t *alarm;
};

#endif /* _RTCD_H */
