#ifndef GPE_ADMIN_H
#define GPE_ADMIN_H

GtkWidget *GpeAdmin_Build_Objects();
void GpeAdmin_Free_Objects();
void GpeAdmin_Save();
void GpeAdmin_Restore();

void do_change_user_access(char *acstr);

#define GPE_CONF_CFGFILE "/etc/gpe/gpe-config.conf"

#endif
