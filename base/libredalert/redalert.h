/* 'redalert' is a temporary name.
   (unless no-one can come up with a better one.
   other possibilities discussed on IRC:
   mibus: libnoisy? libbeeper? libillgetrightbacktoyou? libalarmnotify? libalarm?
   PaxAnima: lib-wake-me-up-before-you-go-go, libbeep
*/

#include <time.h>
#include <glib.h>

void redalert_set_alarm (const char *program, guint alarm_id, time_t unixtime, const char *command);
void redalert_set_alarm_message (const char *program, guint alarm_id, time_t unixtime, const char *message);
