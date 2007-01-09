#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <gpe/libgpe.h>
#include <signal.h>


/*
 * Some globals
 */

/* Number of selected row in AlarmCList or -1 if none	*/
static int AlarmListRow = -1;

/* Flag to stop alarm sound thread from playing		*/
static gboolean PlayAlarmStop = TRUE;

/* Flag to show / not show the main menu bar,
   toggled with iPaq Task-Button, cool!			*/
static gboolean MainMenuShown = FALSE;

/* The main window					*/
extern GtkWidget *GPE_Alarm;

/* The preferences */
gboolean PrefAlwaysShowMenubar=FALSE;
gint PrefAlarmVolume=60;
gint PrefAlarmMuteTimeout=60;

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


gboolean
on_GPE_Alarm_de_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *PseudoMain;

	PseudoMain=lookup_widget(widget,"PseudoMain");
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(PseudoMain)) == 0)
		gtk_main_quit();

return TRUE;
}


void
on_NewAlarm_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *PseudoMain;
GtkWidget *widget;
struct timeval mytv;
struct tm *mytm;

	gettimeofday(&mytv, NULL);
	mytm=localtime(&mytv.tv_sec);

	CurrentAlarm=(struct Alarm_t *)malloc(sizeof(struct Alarm_t));
	memset(CurrentAlarm,0,sizeof(struct Alarm_t));
	CurrentAlarm->year=mytm->tm_year + 1900;
	CurrentAlarm->month=mytm->tm_mon;
	CurrentAlarm->day=mytm->tm_mday;
	CurrentAlarm->hour=mytm->tm_hour;
	CurrentAlarm->minute=mytm->tm_min;
	CurrentAlarm->AlarmType=0; /* no accoustic alarm */
	CurrentAlarm->AlarmReoccurence=0; /* once */
	CurrentAlarm->Tone2Enable=TRUE;
	AlarmListRow = -1;

	widget=lookup_widget(GTK_WIDGET(button),"AlarmDate");
	gtk_calendar_select_month(GTK_CALENDAR(widget),mytm->tm_mon, (mytm->tm_year + 1900));
	gtk_calendar_select_day(GTK_CALENDAR(widget),mytm->tm_mday);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmHour");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),(gfloat)mytm->tm_hour);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmMinute");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),(gfloat)mytm->tm_min);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmType");
	gtk_option_menu_set_history (GTK_OPTION_MENU (widget), 0);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmComment");
	gtk_entry_set_text(GTK_ENTRY(widget),"");

	widget=lookup_widget(GTK_WIDGET(button),"Tone1Pitch");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),500);
	CurrentAlarm->Tone1Pitch=500;
	widget=lookup_widget(GTK_WIDGET(button),"Tone1Duration");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),100);
	CurrentAlarm->Tone1Duration=100;
	widget=lookup_widget(GTK_WIDGET(button),"AlTone2On");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);
	CurrentAlarm->Tone2Enable=TRUE;
	widget=lookup_widget(GTK_WIDGET(button),"Tone2Pitch");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),800);
	CurrentAlarm->Tone2Pitch=800;
	widget=lookup_widget(GTK_WIDGET(button),"Tone2Duration");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),100);
	CurrentAlarm->Tone2Duration=100;
	widget=lookup_widget(GTK_WIDGET(button),"ToneCount");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),10);
	CurrentAlarm->ToneAltCount=10;
	widget=lookup_widget(GTK_WIDGET(button),"TonePause");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),1000);
	CurrentAlarm->TonePause=1000;

	widget=lookup_widget(GTK_WIDGET(button),"AlarmTry");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmOptions");
	gtk_widget_set_sensitive(widget, FALSE);

	PseudoMain=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(PseudoMain),1);
}



void
on_AlarmDelete_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *AlarmCList;
struct Alarm_t *Alarm;
struct tm mytm;

	AlarmCList=lookup_widget(GTK_WIDGET(button), "AlarmCList");
	Alarm=(struct Alarm_t *)gtk_clist_get_row_data(GTK_CLIST(AlarmCList), AlarmListRow);

	memset(&mytm,0,sizeof(struct tm));
	mytm.tm_year=Alarm->year - 1900;
	mytm.tm_mon=Alarm->month;
	mytm.tm_mday=Alarm->day;
	mytm.tm_hour=Alarm->hour;
	mytm.tm_min=Alarm->minute;
	mytm.tm_sec=0;
	mytm.tm_isdst=1;
	rtcd_del_alarm_tm("GPE Alarm",Alarm->comment,&mytm);

	free(Alarm);
	gtk_clist_remove(GTK_CLIST(AlarmCList), AlarmListRow);
}


void
on_AlarmCancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *PseudoMain;

	PseudoMain=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(PseudoMain),0);
	free(CurrentAlarm);
	CurrentAlarm=NULL;
}


void
on_AlarmOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *PseudoMain;
GtkWidget *AlarmDate;
GtkWidget *AlarmCList;
gchar *ListEntry[2];
struct timeval mytv;
struct tm mytm;
struct tm *mytmp;

	AlarmDate=lookup_widget(GTK_WIDGET(button),"AlarmDate");
	gtk_calendar_get_date(GTK_CALENDAR(AlarmDate), &CurrentAlarm->year,
		&CurrentAlarm->month,
		&CurrentAlarm->day);
	ListEntry[0]=(gchar *)malloc(32 * sizeof(gchar));
	sprintf(ListEntry[0],"%02d/%02d/%02d %02d:%02d", (CurrentAlarm->month + 1),
		CurrentAlarm->day,
		(CurrentAlarm->year > 1999) ? (CurrentAlarm->year - 2000) : (CurrentAlarm->year - 1900),
		CurrentAlarm->hour, CurrentAlarm->minute);
	ListEntry[1]=(gchar *)malloc(128 * sizeof(gchar));
	sprintf(ListEntry[1],"%s",CurrentAlarm->comment);
	AlarmCList=lookup_widget(GTK_WIDGET(button),"AlarmCList");
	if (AlarmListRow > -1) {
		/* after edit remove old item, append new and sort*/
		gtk_clist_remove(GTK_CLIST(AlarmCList), AlarmListRow);
		gtk_clist_append(GTK_CLIST(AlarmCList), ListEntry);
		gtk_clist_set_row_data(GTK_CLIST(AlarmCList), GTK_CLIST(AlarmCList)->rows-1, (gpointer)CurrentAlarm);
		/*gtk_clist_select_row(GTK_CLIST(AlarmCList), GTK_CLIST(AlarmCList)->rows-1, 0);*/
		gtk_clist_sort(GTK_CLIST(AlarmCList));
		AlarmListRow = -1;

		memset(&mytm,0,sizeof(struct tm));
		gettimeofday(&mytv, NULL);
		mytmp=localtime(&mytv.tv_sec);
		mytm.tm_year=CurrentAlarm->year - 1900;
		mytm.tm_mon=CurrentAlarm->month;
		mytm.tm_mday=CurrentAlarm->day;
		mytm.tm_hour=CurrentAlarm->hour;
		mytm.tm_min=CurrentAlarm->minute;
		mytm.tm_sec=0;
		mytm.tm_isdst=mytmp->tm_isdst;
		rtcd_set_alarm_tm("GPE Alarm",CurrentAlarm->comment,&mytm,sizeof(struct Alarm_t),CurrentAlarm);
	} else {
		/* completely new item, just append and sort */
		gtk_clist_append(GTK_CLIST(AlarmCList), ListEntry);
		gtk_clist_set_row_data(GTK_CLIST(AlarmCList), GTK_CLIST(AlarmCList)->rows-1, (gpointer)CurrentAlarm);
		gtk_clist_sort(GTK_CLIST(AlarmCList));

		memset(&mytm,0,sizeof(struct tm));
		gettimeofday(&mytv, NULL);
		mytmp=localtime(&mytv.tv_sec);
		mytm.tm_year=CurrentAlarm->year - 1900;
		mytm.tm_mon=CurrentAlarm->month;
		mytm.tm_mday=CurrentAlarm->day;
		mytm.tm_hour=CurrentAlarm->hour;
		mytm.tm_min=CurrentAlarm->minute;
		mytm.tm_sec=0;
		mytm.tm_isdst=mytmp->tm_isdst;
		rtcd_set_alarm_tm("GPE Alarm",CurrentAlarm->comment,&mytm,sizeof(struct Alarm_t),CurrentAlarm);
	}
	free(ListEntry[0]);
	free(ListEntry[1]);
	PseudoMain=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(PseudoMain),0);
}


int insert_rtcd_alarms(void)
{
struct Alarm_t *Alarm;
pid_t pid;
char name[128];
time_t when;
struct tm *mytm;
char comment[128];
unsigned int data_length;
void *data;
GtkWidget *AlarmCList;
gchar *ListEntry[2];

	rtcd_set_pid(getpid(), "GPE Alarm");
	if (rtcd_get_alarm_list() == 1)
		return -1;
	do {
		data=NULL;
		data_length=sizeof(struct Alarm_t);
		Alarm=(struct Alarm_t *)malloc(sizeof(struct Alarm_t));
		rtcd_get_alarm_item(&pid,name,&when,comment,&data_length,Alarm);
		if (pid != 0) {
			mytm=localtime(&when);
#ifdef DEBUG
			g_print("  tm %d:%d\n",mytm->tm_hour,mytm->tm_min);
#endif
			AlarmCList=lookup_widget(GPE_Alarm,"AlarmCList");
			if (strcmp(name,"GPE Alarm") == 0) {
#ifdef DEBUG
				g_print("Got alarm from us!\n");
				g_print("  at %d:%d\n",Alarm.hour,Alarm.minute);
#endif
				ListEntry[0]=(gchar *)malloc(32 * sizeof(gchar));
				sprintf(ListEntry[0],"%02d/%02d/%02d %02d:%02d", (Alarm->month + 1),
					Alarm->day,
					(Alarm->year > 1999) ? (Alarm->year - 2000) : (Alarm->year - 1900),
					Alarm->hour, Alarm->minute);
				ListEntry[1]=(gchar *)malloc(128 * sizeof(gchar));
				sprintf(ListEntry[1],"%s",Alarm->comment);
				gtk_clist_append(GTK_CLIST(AlarmCList), ListEntry);
				gtk_clist_set_row_data(GTK_CLIST(AlarmCList), GTK_CLIST(AlarmCList)->rows-1, (gpointer)Alarm);
			}
#ifdef DEBUG
			else {
				g_print("Got alarm from '%s'\n",name);
			}
#endif
		}
#ifdef DEBUG
		else
			g_print("pid==0\n");
#endif
	} while (pid != 0);
	rtcd_final_alarm_list();
return 0;
}


void
on_AlarmOptions_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *PseudoMain;

	PseudoMain=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(PseudoMain),2);
}


void
on_AlTone2On_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
GtkWidget *widget;

	widget=lookup_widget(GTK_WIDGET(togglebutton),"Tone2Pitch");
	gtk_widget_set_sensitive(widget,TRUE);
	widget=lookup_widget(GTK_WIDGET(togglebutton),"Tone2Duration");
	gtk_widget_set_sensitive(widget,TRUE);
	widget=lookup_widget(GTK_WIDGET(togglebutton),"ToneCount");
	gtk_widget_set_sensitive(widget,TRUE);
	CurrentAlarm->Tone2Enable=TRUE;
}


void
on_AlTone2Off_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
GtkWidget *widget;

	widget=lookup_widget(GTK_WIDGET(togglebutton),"Tone2Pitch");
	gtk_widget_set_sensitive(widget,FALSE);
	widget=lookup_widget(GTK_WIDGET(togglebutton),"Tone2Duration");
	gtk_widget_set_sensitive(widget,FALSE);
	widget=lookup_widget(GTK_WIDGET(togglebutton),"ToneCount");
	gtk_widget_set_sensitive(widget,FALSE);
	CurrentAlarm->Tone2Enable=FALSE;
}


void
on_AlarmCList_select_row               (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *widget;

	AlarmListRow = row;
	widget=lookup_widget(GTK_WIDGET(clist),"AlarmEdit");
	gtk_widget_set_sensitive(widget, TRUE);
	widget=lookup_widget(GTK_WIDGET(clist),"AlarmDelete");
	gtk_widget_set_sensitive(widget, TRUE);
}


void
on_AlarmCList_unselect_row             (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *widget;

	AlarmListRow = -1;
	widget=lookup_widget(GTK_WIDGET(clist),"AlarmEdit");
	gtk_widget_set_sensitive(widget, FALSE);
	widget=lookup_widget(GTK_WIDGET(clist),"AlarmDelete");
	gtk_widget_set_sensitive(widget, FALSE);
}


void
on_AlarmEdit_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *AlarmCList;
GtkWidget *widget;
GtkWidget *PseudoMain;
GtkWidget *AlarmTry;
GtkWidget *AlarmOptions;
struct tm mytm;

	AlarmCList=lookup_widget(GTK_WIDGET(button), "AlarmCList");
	CurrentAlarm=gtk_clist_get_row_data(GTK_CLIST(AlarmCList), AlarmListRow);

	memset(&mytm,0,sizeof(struct tm));
	mytm.tm_year=CurrentAlarm->year - 1900;
	mytm.tm_mon=CurrentAlarm->month;
	mytm.tm_mday=CurrentAlarm->day;
	mytm.tm_hour=CurrentAlarm->hour;
	mytm.tm_min=CurrentAlarm->minute;
	mytm.tm_sec=0;
	mytm.tm_isdst=1;
	rtcd_del_alarm_tm("GPE Alarm",CurrentAlarm->comment,&mytm);

	widget=lookup_widget(GTK_WIDGET(button),"AlarmDate");
	gtk_calendar_select_month(GTK_CALENDAR(widget),CurrentAlarm->month, CurrentAlarm->year);
	gtk_calendar_select_day(GTK_CALENDAR(widget),CurrentAlarm->day);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmHour");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),(gfloat)CurrentAlarm->hour);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmMinute");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),(gfloat)CurrentAlarm->minute);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmComment");
	gtk_entry_set_text(GTK_ENTRY(widget),CurrentAlarm->comment);
	widget=lookup_widget(GTK_WIDGET(button),"AlarmType");
	gtk_option_menu_set_history (GTK_OPTION_MENU (widget), CurrentAlarm->AlarmType);
	AlarmTry=lookup_widget(GTK_WIDGET(button),"AlarmTry");
	AlarmOptions=lookup_widget(GTK_WIDGET(button),"AlarmOptions");
	if (CurrentAlarm->AlarmType == 0) {
		gtk_widget_set_sensitive(AlarmTry, FALSE);
		gtk_widget_set_sensitive(AlarmOptions, FALSE);
	} else if ((CurrentAlarm->AlarmType > 0) && (CurrentAlarm->AlarmType < 4)) {
		gtk_widget_set_sensitive(AlarmTry, TRUE);
		gtk_widget_set_sensitive(AlarmOptions, FALSE);
	} else if (CurrentAlarm->AlarmType == 4) {
		gtk_widget_set_sensitive(AlarmTry, TRUE);
		gtk_widget_set_sensitive(AlarmOptions, TRUE);
	}
	widget=lookup_widget(GTK_WIDGET(button),"Tone1Pitch");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),CurrentAlarm->Tone1Pitch);
	widget=lookup_widget(GTK_WIDGET(button),"Tone1Duration");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),CurrentAlarm->Tone1Duration);
	if (CurrentAlarm->Tone2Enable) {
		widget=lookup_widget(GTK_WIDGET(button),"AlTone2On");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);
	} else {
		widget=lookup_widget(GTK_WIDGET(button),"AlTone2Off");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE);
	}
	widget=lookup_widget(GTK_WIDGET(button),"Tone2Pitch");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),CurrentAlarm->Tone2Pitch);
	widget=lookup_widget(GTK_WIDGET(button),"Tone2Duration");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),CurrentAlarm->Tone2Duration);
	widget=lookup_widget(GTK_WIDGET(button),"ToneCount");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),CurrentAlarm->ToneAltCount);
	widget=lookup_widget(GTK_WIDGET(button),"TonePause");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),CurrentAlarm->TonePause);

	PseudoMain=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(PseudoMain),1);
}


void
on_AlarmHour_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
	CurrentAlarm->hour=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
}


void
on_AlarmMinute_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
	CurrentAlarm->minute=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
}


void
on_AlarmComment_changed                (GtkEditable     *editable,
                                        gpointer         user_data)
{
	strcpy(CurrentAlarm->comment, gtk_entry_get_text(GTK_ENTRY(editable)));
}



void
on_AlarmType_selected                  (GtkMenuShell *menu_shell,
                                        gpointer data)
{
GtkWidget *active_item;
GtkWidget *AlarmTry;
GtkWidget *AlarmOptions;
gint item_index;
  
	active_item = gtk_menu_get_active (GTK_MENU (menu_shell));
	item_index = g_list_index (menu_shell->children, active_item);
	/*g_print ("In on_option_selected active: %i\n", item_index);*/
	AlarmTry=lookup_widget(GTK_WIDGET(menu_shell),"AlarmTry");
	AlarmOptions=lookup_widget(GTK_WIDGET(menu_shell),"AlarmOptions");
	if (item_index == 0) {
		gtk_widget_set_sensitive(AlarmTry, FALSE);
		gtk_widget_set_sensitive(AlarmOptions, FALSE);
	} else if ((item_index > 0) && (item_index < 7)) {
		gtk_widget_set_sensitive(AlarmTry, TRUE);
		gtk_widget_set_sensitive(AlarmOptions, FALSE);
	} else if (item_index == 7) {
		gtk_widget_set_sensitive(AlarmTry, TRUE);
		gtk_widget_set_sensitive(AlarmOptions, TRUE);
	}
	CurrentAlarm->AlarmType=item_index;
}


void play_melody(guint Tone1Pitch, guint Tone1Duration,
		guint Tone2Enable, guint Tone2Pitch, guint Tone2Duration,
		guint ToneAltCount, guint TonePause)
{
int gpe_snd_dev=-1;
int i;

	for (i=0; i<5; i++) {
		if ((gpe_snd_dev=gpe_soundgen_init()) == -1) {
			g_print("Couldn't init gpe_soundgen\n");
			sleep (1);
		} else
			break;
	}
	if (gpe_snd_dev == -1)
		return;
	set_audio_volume(PrefAlarmVolume);
	while (!PlayAlarmStop) {
		for (i = 0; i < ToneAltCount; i++) {
			if (PlayAlarmStop)
				break;
			gpe_soundgen_play_tone1(gpe_snd_dev, Tone1Pitch, Tone1Duration);
			if (Tone2Enable) {
				gpe_soundgen_play_tone2(gpe_snd_dev, Tone2Pitch, Tone2Duration);
			}
		}
		gpe_soundgen_pause(gpe_snd_dev, TonePause);
	}
#ifdef DEBUG
	if (PlayAlarmStop)
		g_print(__func__ ": PlayAlarmStop\n");
	else
		g_print(__func__ ": PlayAlarmStop\n");
#endif
	gpe_soundgen_final(gpe_snd_dev);
}


void *play_alarm(void *Alarm)
{
struct Alarm_t *ThisAlarm;

	ThisAlarm=(struct Alarm_t *) Alarm;
	switch (ThisAlarm->AlarmType) {
		case 1:
			play_melody(500,50,0,0,0,1,1000);
			break;
		case 2:
			play_melody(800,50,0,0,0,1,1000);
			break;
		case 3:
			play_melody(1100,50,0,0,0,1,1000);
			break;
		case 4:
			play_melody(500,100,1,800,100,10,2000);
			break;
		case 5:
			play_melody(800,50,1,1100,50,10,2000);
			break;
		case 6:
			play_melody(800,400,1,1100,400,2,2000);
			break;
		case 7:
			play_melody( ThisAlarm->Tone1Pitch,
			ThisAlarm->Tone1Duration,
			ThisAlarm->Tone2Enable,
			ThisAlarm->Tone2Pitch,
			ThisAlarm->Tone2Duration,
			ThisAlarm->ToneAltCount,
			ThisAlarm->TonePause);
			break;
		default:
			break;
	}
	/*
	if (ThisAlarm != NULL)
		free(ThisAlarm);
	*/
	pthread_exit(NULL);

return (NULL);
}

void
on_AlarmTry_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *StopAlarmWin;
pthread_t mythread;

	StopAlarmWin=create_StopAlarmWin();
	gtk_widget_show(StopAlarmWin);
	PlayAlarmStop = FALSE;
	if (pthread_create(&mythread, NULL, play_alarm, (void *)CurrentAlarm) != 0) {
		g_print("pthread_create() failed\n");
	} else
		pthread_detach(mythread);
}


void
on_StopAlarm_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *StopAlarmWin;

	StopAlarmWin=lookup_widget(GTK_WIDGET(button),"StopAlarmWin");
	gtk_widget_hide(StopAlarmWin);
	gtk_widget_destroy(StopAlarmWin);
	PlayAlarmStop = TRUE;
}


void AlarmOptGetOpts(GtkWidget *inside)
{
GtkWidget *widget;

	widget=lookup_widget(inside,"Tone1Pitch");
	CurrentAlarm->Tone1Pitch=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	widget=lookup_widget(inside,"Tone1Duration");
	CurrentAlarm->Tone1Duration=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	widget=lookup_widget(inside,"Tone2Pitch");
	CurrentAlarm->Tone2Pitch=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	widget=lookup_widget(inside,"Tone2Duration");
	CurrentAlarm->Tone2Duration=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	widget=lookup_widget(inside,"ToneCount");
	CurrentAlarm->ToneAltCount=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	widget=lookup_widget(inside,"TonePause");
	CurrentAlarm->TonePause=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}


void
on_AlarmOptTry_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *StopAlarmWin;
pthread_t mythread;

	StopAlarmWin=create_StopAlarmWin();
	gtk_widget_show(StopAlarmWin);
	PlayAlarmStop = FALSE;
	AlarmOptGetOpts(GTK_WIDGET(button));
	if (pthread_create(&mythread, NULL, play_alarm, (void *)CurrentAlarm) != 0) {
		g_print("pthread_create() failed\n");
	} else
		pthread_detach(mythread);
}

void
on_AlarmOptOK_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *PseudoMain;

	AlarmOptGetOpts(GTK_WIDGET(button));
	PseudoMain=lookup_widget(GTK_WIDGET(button),"PseudoMain");
	gtk_notebook_set_page(GTK_NOTEBOOK(PseudoMain),1);
}



void
on_file1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
GtkWidget *widget;

	widget=lookup_widget(GTK_WIDGET(menuitem),"MainMenu");
	gtk_widget_show(widget);
}


void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
GtkWidget *PseudoMain;

	PseudoMain=lookup_widget(GTK_WIDGET(menuitem),"PseudoMain");
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(PseudoMain)) == 0)
		gtk_main_quit();
}


void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
GtkWidget *AlarmsPrefs;
GtkWidget *MainWin;
GtkWidget *widget;

	MainWin=lookup_widget(GTK_WIDGET(menuitem),"GPE_Alarm");
	AlarmsPrefs=create_AlarmsPrefs();
	gtk_window_set_transient_for(GTK_WINDOW(AlarmsPrefs),GTK_WINDOW(MainWin));
	widget = lookup_widget (AlarmsPrefs, "AlarmVolume");
	gtk_adjustment_set_value(GTK_RANGE (widget)->adjustment, PrefAlarmVolume);
	gtk_signal_connect (GTK_OBJECT (GTK_RANGE (widget)->adjustment),
		"value_changed", GTK_SIGNAL_FUNC (on_AlarmVolume_changed), NULL);
	gtk_widget_show(AlarmsPrefs);
}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
GtkWidget *AlarmAbout;
GtkWidget *MainWin;

	MainWin=lookup_widget(GTK_WIDGET(menuitem),"GPE_Alarm");
	AlarmAbout=create_AlarmAbout();
	gtk_window_set_transient_for(GTK_WINDOW(AlarmAbout),GTK_WINDOW(MainWin));
	gtk_widget_show(AlarmAbout);
}


gboolean
on_GPE_Alarm_key_press_event           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
	/*g_print("on_GPE_Alarm_key_press_event() = 0x%x\n",event->keyval);*/
	if (event->keyval == 0x1008ff1a) {
		GtkWidget *MainMenu;

		MainMenu=lookup_widget(GTK_WIDGET(widget),"MainMenu");
		if (MainMenuShown) {
			gtk_widget_hide(MainMenu);
			MainMenuShown = FALSE;
		} else {
			gtk_widget_show(MainMenu);
			MainMenuShown = TRUE;
		}
		return TRUE;
	}

return FALSE;
}


void
on_PrefsOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *AlarmsPrefs;

	AlarmsPrefs=lookup_widget(GTK_WIDGET(button),"AlarmsPrefs");
	gtk_widget_hide(AlarmsPrefs);
	gtk_widget_destroy(AlarmsPrefs);
}


void
on_PrefsCancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *AlarmsPrefs;

	AlarmsPrefs=lookup_widget(GTK_WIDGET(button),"AlarmsPrefs");
	gtk_widget_hide(AlarmsPrefs);
	gtk_widget_destroy(AlarmsPrefs);
}


gboolean
on_AlarmsPrefs_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
/* Ignore detroy and delete events */
return TRUE;
}


void
on_MainMenu_deactivate                 (GtkMenuShell    *menushell,
                                        gpointer         user_data)
{
GtkWidget *MainMenu;

	MainMenu=lookup_widget(GTK_WIDGET(menushell),"MainMenu");
	if (MainMenuShown) {
		gtk_widget_hide(MainMenu);
		MainMenuShown = FALSE;
	} else {
		gtk_widget_show(MainMenu);
		MainMenuShown = TRUE;
	}
}


void
on_AlarmAboutOK_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *AlarmAbout;

	AlarmAbout=lookup_widget(GTK_WIDGET(button),"AlarmAbout");
	gtk_widget_hide(AlarmAbout);
	gtk_widget_destroy(AlarmAbout);
}


gboolean
on_AlarmAbout_de_event                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *AlarmAbout;

	AlarmAbout=lookup_widget(GTK_WIDGET(widget),"AlarmAbout");
	gtk_widget_hide(AlarmAbout);
	gtk_widget_destroy(AlarmAbout);

return FALSE;
}


void RTCD_sighandler(int signo)
{
GtkWidget *AlarmWin;
GtkWidget *AlarmCList;
GtkWidget *widget;
struct Alarm_t *Alarm=NULL;
struct timeval mytv;
struct tm *mytm;
struct tm mmytm;
char tmpstr[32];
int i;
pthread_t mythread;

#ifdef DEBUG
	g_print("got RTCD signal!\n");
#endif
	signal(SIGUSR1, SIG_IGN);

	gettimeofday(&mytv, NULL);
	if (mytv.tv_sec % 60 != 0) {
		mytv.tv_sec += mytv.tv_sec % 60;
	}
	mytm=localtime(&mytv.tv_sec);

	AlarmCList=lookup_widget(GPE_Alarm,"AlarmCList");
	for (i=0; i < GTK_CLIST(AlarmCList)->rows; i++) {
		/* g_print("Looking at list item# %d of %d\n",i,GTK_CLIST(AlarmCList)->rows-1); */
		Alarm=gtk_clist_get_row_data(GTK_CLIST(AlarmCList),i);
#ifdef DEBUG
		g_print("--->\nAlarm->year = %d, mytm->tm_year + 1900 = %d\n", Alarm->year, mytm->tm_year + 1900);
		g_print("Alarm->month = %d, mytm->tm_mon = %d\n", Alarm->month, mytm->tm_mon);
		g_print("Alarm->day = %d, mytm->tm_mday = %d\n", Alarm->day, mytm->tm_mday);
		g_print("Alarm->hour = %d, mytm->tm_hour = %d\n", Alarm->hour, mytm->tm_hour);
		g_print("Alarm->minute = %d, mytm->tm_min = %d\n<---\n", Alarm->minute, mytm->tm_min);
#endif
		if ((Alarm->year == mytm->tm_year + 1900) &&
		    (Alarm->month == mytm->tm_mon) &&
		    (Alarm->day == mytm->tm_mday) &&
		    (Alarm->hour == mytm->tm_hour) &&
		    (Alarm->minute == mytm->tm_min)) {
#ifdef DEBUG
			g_print("Found matching alarm!\n");
#endif
			break;
		} else {
#ifdef DEBUG
			g_print("...did not match\n");
#endif
			Alarm = NULL;
		}
	}
	if (Alarm == NULL) {
		signal(SIGUSR1, RTCD_sighandler);
		return;
	}
	memset(&mmytm,0,sizeof(struct tm));
	mmytm.tm_year=Alarm->year - 1900;
	mmytm.tm_mon=Alarm->month;
	mmytm.tm_mday=Alarm->day;
	mmytm.tm_hour=Alarm->hour;
	mmytm.tm_min=Alarm->minute;
	mmytm.tm_sec=0;
	mmytm.tm_isdst=1;
	rtcd_del_alarm_tm("GPE Alarm",Alarm->comment,&mmytm);
	gtk_clist_remove(GTK_CLIST(AlarmCList),i);

	AlarmWin=create_AlarmWin();
	gtk_object_set_data(GTK_OBJECT (AlarmWin), "Alarm", Alarm);
	widget=lookup_widget(AlarmWin,"AlarmDate");
	sprintf(tmpstr,"%02d/%02d/%02d",(Alarm->month + 1),
		Alarm->day,
		(Alarm->year > 1999) ? (Alarm->year - 2000) : (Alarm->year - 1900));
	gtk_label_set_text(GTK_LABEL(widget),tmpstr);
	widget=lookup_widget(AlarmWin,"AlarmTime");
	sprintf(tmpstr,"%02d:%02d", Alarm->hour, Alarm->minute);
	gtk_label_set_text(GTK_LABEL(widget),tmpstr);
	widget=lookup_widget(AlarmWin,"AlarmComment");
	gtk_entry_set_text(GTK_ENTRY(widget),Alarm->comment);
	if (Alarm->AlarmType == 0) {
		widget=lookup_widget(AlarmWin,"AlarmMute");
		gtk_widget_set_sensitive(widget, FALSE);
	}

	if (Alarm->AlarmType > 0) {
		PlayAlarmStop = FALSE;
		if (pthread_create(&mythread, NULL, play_alarm, (void *)Alarm) != 0) {
			g_print("pthread_create() failed\n");
		} else
			pthread_detach(mythread);
	}

	gtk_window_set_transient_for(GTK_WINDOW(AlarmWin),GTK_WINDOW(GPE_Alarm));
	gtk_widget_show(AlarmWin);

	signal(SIGUSR1, RTCD_sighandler);
}

void
on_AlarmMute_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	PlayAlarmStop = TRUE;
}


void
on_AlarmDelay_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *AlarmWin;
GtkWidget *AlarmCList;
struct timeval mytv;
struct tm *mytm;
struct Alarm_t *Alarm;
gchar *ListEntry[2];

	PlayAlarmStop = TRUE;

	AlarmWin=lookup_widget(GTK_WIDGET(button),"AlarmWin");
	AlarmCList=lookup_widget(GPE_Alarm,"AlarmCList");

	gettimeofday(&mytv, NULL);
	mytv.tv_sec += (5 * 60); /* delay for 5 minutes */
	mytm=localtime(&mytv.tv_sec);

	Alarm=gtk_object_get_data(GTK_OBJECT (AlarmWin), "Alarm");
	Alarm->year=mytm->tm_year + 1900;
	Alarm->month=mytm->tm_mon;
	Alarm->day=mytm->tm_mday;
	Alarm->hour=mytm->tm_hour;
	Alarm->minute=mytm->tm_min;

	ListEntry[0]=(gchar *)malloc(32 * sizeof(gchar));
	sprintf(ListEntry[0],"%02d/%02d/%02d %02d:%02d", (Alarm->month + 1),
		Alarm->day,
		(Alarm->year > 1999) ? (Alarm->year - 2000) : (Alarm->year - 1900),
		Alarm->hour, Alarm->minute);
	ListEntry[1]=(gchar *)malloc(128 * sizeof(gchar));
	sprintf(ListEntry[1],"%s",Alarm->comment);

	gtk_clist_append(GTK_CLIST(AlarmCList), ListEntry);
	gtk_clist_set_row_data(GTK_CLIST(AlarmCList), GTK_CLIST(AlarmCList)->rows-1, (gpointer)Alarm);
	gtk_clist_sort(GTK_CLIST(AlarmCList));

	rtcd_set_alarm_tm("GPE Alarm",CurrentAlarm->comment,mytm,sizeof(struct Alarm_t),CurrentAlarm);
	free(ListEntry[0]);
	free(ListEntry[1]);

	gtk_widget_hide(AlarmWin);
	gtk_widget_destroy(AlarmWin);
}


void
on_AlarmACK_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *AlarmWin;
gpointer Alarm;

	PlayAlarmStop = TRUE;
	AlarmWin=lookup_widget(GTK_WIDGET(button),"AlarmWin");
	Alarm=gtk_object_get_data(GTK_OBJECT (AlarmWin), "Alarm");
	free(Alarm);
	gtk_widget_hide(AlarmWin);
	gtk_widget_destroy(AlarmWin);
}


gboolean
on_GPE_Alarm_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
#ifdef DEBUG
	g_print("button_press_event from button %d\n",event->button);
#endif
	if (event->button == 3) {
		GtkWidget *MainMenu;

		MainMenu=lookup_widget(GTK_WIDGET(widget),"MainMenu");
		if (MainMenuShown) {
			gtk_widget_hide(MainMenu);
			MainMenuShown = FALSE;
		} else {
			gtk_widget_show(MainMenu);
			MainMenuShown = TRUE;
		}
	}	

return FALSE;
}


void
on_AlarmVolume_changed                     (GtkAdjustment       *adj,
                                            gpointer         user_data)
{
int gpe_snd_dev;

	PrefAlarmVolume=(gint)adj->value;
	set_audio_volume(PrefAlarmVolume);
	if ((gpe_snd_dev=gpe_soundgen_init()) != -1) {
		gpe_soundgen_play_tone1(gpe_snd_dev,800,100);
		gpe_soundgen_final(gpe_snd_dev);
	}
}


void set_audio_volume(gint value)
{
int handle,r;

	handle=open("/dev/mixer",O_RDWR);
	if (handle > 0) {
		r=(value << 8) | value;
		ioctl(handle,MIXER_WRITE(SOUND_MIXER_VOLUME),&r);
		close(handle);
	}
}

int get_audio_volume(void)
{
int handle,r;
gint volume=0;

	handle=open("/dev/mixer",O_RDWR);
	if (handle > 0) {
		if (ioctl(handle,MIXER_READ(SOUND_MIXER_VOLUME),&r) == 0) {
			volume=r & 0xff;
		}
		close(handle);
	}

return volume;
}


gboolean
on_AlarmWin_de_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

  return TRUE;
}

