#ifndef _CFGFILE_H
#define _CFGFILE_H

#ifdef __cplusplus
extern "C"
{
#endif
	
typedef struct
{
	gboolean warning_disabled;
	gchar *upload_url; /* for future use */
    gint  upload_method;
}
t_gpe_scap_cfg;

	
void load_config (t_gpe_scap_cfg *cfg);
gchar *save_config (t_gpe_scap_cfg *cfg);

#ifdef __cplusplus
}
#endif

#endif
