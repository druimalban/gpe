/* gpe-rtcd.h - Communication API for the RTC daemon

   Copyright (C) 2002 by Nils Faerber <nils@kernelconcepts.de>

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _GPE_RTCD_H
#define _GPE_RTCD_H

/*
 * Prototypes for the rtcd client library
 */

/*
 * Connect to the rtc daemon process
 * normally not needed in a client program
 */
#if 0
static int connect_rtcd();
#endif

/*
 * Set an rtc alarm
 * char *name:    Descriptive name of the calling process, up to 128 chars
 * char *comment: Additional comment for this alarm, up to 128 chars
 * time_t when:   The alarm time/date given as a time_t value
 * unsigned int data_length:		Length of an arbitrary client data buffer
 * void *data:		The data buffer itself
 */
int rtcd_set_alarm_time_t(char *name, char *comment, time_t when,
                          unsigned int data_length, void *data);

/*
 * Delete an rtc alarm
 * char *name:    Descriptive name of the calling process, up to 128 chars
 * char *comment: Additional comment for this alarm, up to 128 chars
 * time_t when:   The alarm time/date given as a time_t value
 */
int rtcd_del_alarm_time_t(char *name, char *comment, time_t when);

/*
 * Set an rtc alarm
 * char *name
 *	Descriptive name of the calling process, up to 128 chars
 * char *comment
 *	Additional comment for this alarm, up to 128 chars
 * struct tm tm_when
 *	The alarm time/date given as a tm struct
 * unsigned int data_length
 *	Length of an arbitrary client data buffer
 * void *data
 *	The data buffer itself
 */
int rtcd_set_alarm_tm(char *name, char *comment, struct tm *tm_when,
                      unsigned int data_length, void *data);

/*
 * Set an rtc alarm
 * char *name:    Descriptive name of the calling process, up to 128 chars
 * char *comment: Additional comment for this alarm, up to 128 chars
 * struct tm tm_when:   The alarm time/date given as a tm struct
 */
int rtcd_del_alarm_tm(char *name, char *comment, struct tm *tm_when);


/*
 * Start retrieving the list of current pending RTCD alarms
 */
int rtcd_get_alarm_list(void);

/*
 * Retrieves an alarm item from the list.
 * After the call to rtcd_get_alarm_list() this call must be called as
 * long as non-NULL item are returned!
 * After that rtcd_final_alarm_list() must be called!
 */
int rtcd_get_alarm_item(pid_t *pid, char *name, time_t *when, char *comment,
                        unsigned int *data_length, void *data);

/*
 * Change PID of possibly existing alarms to new PID,
 * useful if process had to be restarted
 */
int rtcd_set_pid(pid_t pid, char *name);

/*
 * Finalize a get_alarm_list() call
 * Before calling this all items must have been retrieved using
 * rtcd_get_alarm_item()!
 */
void rtcd_final_alarm_list(void);

#endif
