#define MAX_IRQS 255

#define DEFAULTSUSER "/.ipaq-sleep.conf"
#define DEFAULTS "/etc/ipaq-sleep.conf"
#define INTERRUPTS "/proc/interrupts"
#define LOADAVG "/proc/loadavg"
#define NO_SLEEP_DIR "/var/run/no-sleep"
#define NO_SLEEP "/var/run/no-sleep/ 2> /dev/null"
#define DEFAULT_SLEEP_TIME 2
#define BL "/usr/bin/bl"

#define uflag	"auto-sleep_time"
#define oflag	"dim_time"
#define aflag	"check_apm"
#define cflag	"check_cpu"
#define dflag	"debug"
#define Lflag	"dim_level"
#define xflag	"X"
#define Cflag	"CPU_value"
#define pflag	"probe_IRQs"
#define iflag	"IRQ"
#define bflag	"low_battery"

const char *sleep_command="/usr/bin/apm --suspend";

/* APM stuff */

/* We need to define some of these headers because seemingly
 * they aren't defined in every apm. Very mysterious
  */
  
  #ifndef AC_LINE_STATUS_OFF
  #define AC_LINE_STATUS_OFF      (0)
  #endif
  
  #ifndef AC_LINE_STATUS_ON
  #define AC_LINE_STATUS_ON       (1)
  #endif
  
  #ifndef BATTERY_FLAGS_CHARGING
  #define BATTERY_FLAGS_CHARGING  (0x8)
  #endif
  
  #ifndef BATTERY_FLAGS_ABSENT
  #define BATTERY_FLAGS_ABSENT    (0x80)
  #endif
  
  #ifndef BATTERY_PERCENTAGE_UNKNOWN
  #define BATTERY_PERCENTAGE_UNKNOWN  (-1)
  #endif
  
  #ifndef BATTERY_TIME_UNKNOWN
  #define BATTERY_TIME_UNKNOWN        (-1)
  #endif
  
  /* These are the headers that are in apm.h - define above 
   * as we need them.
   *
   * #define AC_LINE_STATUS_OFF      (0)
   * #define AC_LINE_STATUS_ON       (1)
   * #define AC_LINE_STATUS_BACKUP   (2)
   * #define AC_LINE_STATUS_UNKNOWN  (0xff)
   * #define BATTERY_STATUS_HIGH     (0)
   * #define BATTERY_STATUS_LOW      (1)
   * #define BATTERY_STATUS_CRITICAL (2)
   * #define BATTERY_STATUS_ABSENT   (4) 
   * #define BATTERY_STATUS_UNKNOWN  (0xff) 
   * #define BATTERY_FLAGS_HIGH      (0x1)
   * #define BATTERY_FLAGS_LOW       (0x2)
   * #define BATTERY_FLAGS_CRITICAL  (0x4)
   * #define BATTERY_FLAGS_ABSENT    (0x80)
   */
