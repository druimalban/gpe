#ifndef _SOUNDCTRL_H
#define _SOUNDCTRL_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	int nr;
	char *name;
	char *label;
	int value;
	int initval;
	int backupval;
}
t_mixer;

#define SOUND_SAMPLE PREFIX "/share/gpe-conf/activate.wav"

int set_volume (int channel, int volume);
int get_volume (int channel);
void sound_init(void);
int sound_get_channels(t_mixer **channels);
void sound_save_settings(void);
void sound_load_settings(void);
void sound_restore_settings(void);
void play_sample(char *filename);
void set_mute_status(int set_mute);
int get_mute_status(void);


#ifdef __cplusplus
}
#endif

#endif /* _SOUNDCTRL_H */
