#include "redalert.h"
#include <time.h>

main() {
	//redalert_set_alarm (time_t unixtime, char *program);
	redalert_set_alarm_message (time(NULL)+5, "Hello world");
}
