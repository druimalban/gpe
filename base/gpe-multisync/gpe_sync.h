#include "multisync.h"

#include <nsqlc.h>

/* opie connection definition */
typedef struct {
  client_connection commondata;
  sync_pair *sync_pair;
  char* device_addr;
  char* username;

  nsqlc *n;
} gpe_conn;

#define GPE_DEBUG(conn, x) fprintf (stderr, "%s", (x))

extern gboolean gpe_connect (gpe_conn *conn);
extern gboolean gpe_load_config (gpe_conn *conn);
extern gboolean gpe_save_config (gpe_conn *conn);
