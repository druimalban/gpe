#include "gpe_sync.h"

osync_bool gpe_adresses_open(gpe_environment *env, OSyncError **error);
void gpe_adresses_get_changes(OSyncContext *ctx);

void gpe_adresses_setup(OSyncPluginInfo *info)
{
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "file", NULL);
}
