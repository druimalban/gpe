/* 'redalert' is a temporary name.
   (unless no-one can come up with a better one.
   other possibilities discussed on IRC:
   mibus: libnoisy? libbeeper? libillgetrightbacktoyou? libalarmnotify? libalarm?
   PaxAnima: lib-wake-me-up-before-you-go-go, libbeep
*/

#include <time.h>

void redalert_set_alarm (time_t unixtime, char *program);
void redalert_set_alarm_message (time_t unixtime, char *message);
