#ifndef TODO_H
#define TODO_H

osync_bool gpe_todo_connect(OSyncContext *ctx);
void gpe_todo_disconnect(OSyncContext *ctx);
// static osync_bool gpe_todo_commit_change(OSyncContext *ctx, OSyncChange *change);

void gpe_todo_delete_item(OSyncContext *ctx, unsigned int urn);
osync_bool gpe_todo_add_item(OSyncContext *ctx, unsigned int urn, const char *data);
	
void gpe_todo_get_changes(OSyncContext *ctx);
void gpe_todo_setup(OSyncPluginInfo *info);

#endif /* TODO_H */
