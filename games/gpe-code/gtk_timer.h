/* gtk_timer.h
 * Forward declarations and data structures for the functions in gtk_timer.c
 */ 


/* data structure for the timer */
typedef struct {
  GtkWidget *label;
  gint timer;
  int interval_count;
  char *interval_description;
  int is_running;
} type_timer;


void initialize_timer (type_timer *new_timer, GtkWidget *label, 
		       char *description);
gint timer_callback (gpointer timer_ptr);

void reset_timer (type_timer *app_timer, int starting_value);
void start_timer (type_timer *app_timer, int ms_timeout);
void stop_timer (type_timer *app_timer);

int timer_is_running (type_timer *timer);
int timer_current_value (type_timer *app_timer);
