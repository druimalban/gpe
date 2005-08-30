#ifndef CALENDAR_H
#define CALENDAR_H

osync_bool gpe_calendar_connect(OSyncContext *ctx);
void gpe_calendar_disconnect(OSyncContext *ctx);
// static osync_bool gpe_calendar_commit_change(OSyncContext *ctx, OSyncChange *change);

void gpe_calendar_delete_item(OSyncContext *ctx, unsigned int urn);
osync_bool gpe_calendar_add_item(OSyncContext *ctx, unsigned int urn, const char *data);
	
void gpe_calendar_get_changes(OSyncContext *ctx);
void gpe_calendar_setup(OSyncPluginInfo *info);

#endif /* CALENDAR_H */
