#ifndef CONTACTS_H
#define CONTACTS_H

osync_bool gpe_contacts_connect(OSyncContext *ctx);
void gpe_contacts_disconnect(OSyncContext *ctx);
// static osync_bool gpe_contacts_commit_change(OSyncContext *ctx, OSyncChange *change);
void gpe_contacts_get_changes(OSyncContext *ctx);
void gpe_contacts_setup(OSyncPluginInfo *info);

#endif /* CONTACTS_H */
