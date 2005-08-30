#ifndef CONTACTS_H
#define CONTACTS_H

osync_bool gpe_contacts_connect(OSyncContext *ctx);
void gpe_contacts_disconnect(OSyncContext *ctx);
// static osync_bool gpe_contacts_commit_change(OSyncContext *ctx, OSyncChange *change);

osync_bool gpe_contacts_add_item(OSyncContext *ctx, const char *data, gchar **error);
osync_bool gpe_contacts_modify_item(OSyncContext *ctx, unsigned int urn, const char *data, gchar **error);
osync_bool gpe_contacts_delete_item(OSyncContext *ctx, unsigned int urn, gchar **error);
	
void gpe_contacts_get_changes(OSyncContext *ctx);
void gpe_contacts_setup(OSyncPluginInfo *info);

#endif /* CONTACTS_H */
