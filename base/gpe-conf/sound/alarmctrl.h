#ifndef _ALARMCTRL_H
#define _ALARMCTRL_H

#ifdef __cplusplus
extern "C"
{
#endif

void alarm_save_settings(void);
void alarm_load_settings(void);
void alarm_restore_settings(void);

int get_alarm_level(void);
void set_alarm_level(int newlevel);
int get_alarm_enabled(void);
void set_alarm_enabled(int newenabled);
int get_alarm_automatic(void);
void set_alarm_automatic(int newauto);

void alarm_init(void);
	
#ifdef __cplusplus
}
#endif

#endif /* _ALARMCTRL_H */
