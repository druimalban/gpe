#include <gtk/gtk.h>

gint bells_and_whistles ( );

gint
on_ok_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

gint
on_mute_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

gint
on_snooze_clicked                     (GtkButton       *button,
                                        char         *user_data);

GtkWidget* create_window (char *announcetext);

#define TIMEFMT "%X"
#define DATEFMT "%x"

struct Alarm_t {
	guint year;
	guint month;
	guint day;
	unsigned int hour;
	unsigned int minute;
	unsigned int AlarmType;
	unsigned int AlarmReoccurence;
	char comment[128];
	unsigned int Tone1Pitch;
	unsigned int Tone1Duration;
	unsigned int Tone2Enable;
	unsigned int Tone2Pitch;
	unsigned int Tone2Duration;
	unsigned int ToneAltCount;
	unsigned int TonePause;
} *CurrentAlarm;

