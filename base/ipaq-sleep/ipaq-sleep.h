#define MAX_IRQS 255

#define DEFAULTSUSER "/ipaq-sleep.conf"
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
