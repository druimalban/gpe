#ifndef _BRIGHTNESS_H
#define _BRIGHTNESS_H

void backlight_init(void);
void backlight_set_brightness (int brightness);
int backlight_get_brightness (void);
void backlight_set_power (gboolean power);
gboolean backlight_get_power (void);
int ipaq_get_level(void);

#endif
