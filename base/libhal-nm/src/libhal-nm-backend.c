
#include <stdlib.h>
#include <glib.h>

#include "libhal-nm-backend.h"
#include "backend-net.h"

#define NBACKENDS 1

BackendIface *backends[NBACKENDS];
guint32 backends_registered = 0;

gboolean
libhal_nm_backend_register (BackendIface *interface)
{
	g_return_val_if_fail (backends_registered < NBACKENDS, FALSE);
	
	backends[backends_registered ++] = interface;
	backends[backends_registered - 1]->id = backends_registered - 1;

	return TRUE;
}

void
libhal_nm_backend_init (void)
{
	int i;

	backend_net_main ();

	for (i = 0; i < backends_registered; i++) 
		if (backends[i]->init) backends[i]->init();
}

void libhal_nm_backend_exit (void)
{
}
