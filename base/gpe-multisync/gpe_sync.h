/* opie connection definition */
typedef struct {
  client_connection commondata;
  sync_pair *sync_pair;
  char* device_addr;
  char* username;
} gpe_conn;

#define GPE_DEBUG(conn, x) fprintf (stderr, "%s", (x))

