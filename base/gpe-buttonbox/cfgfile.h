#ifndef _CFGFILE_H
#define _CFGFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

#define CONFIGFILE "buttonbox.conf"

typedef struct {
  int nr_slots;
  int slot_width;
  int slot_height;
  int icon_size;
  gboolean myfiles_on;
  gboolean labels_on;
  int fixed_slots;
}t_cfg;

gboolean config_load(void);
gboolean config_save(void);

#ifdef __cplusplus
}
#endif

#endif /* _CFGFILE_H */
