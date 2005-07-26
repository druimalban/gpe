#ifndef GPE_ADRESSES_H
#define GPE_ADRESSES_H

void gpe_adresses_setup(OSyncPluginInfo *info);
osync_bool gpe_adresses_open(gpe_environment *env, OSyncError **error);
void gpe_adresses_get_changes(OSyncContext *ctx);

#endif /* GPE_ADRESSES_H */
