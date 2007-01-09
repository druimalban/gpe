extern GSList *sql_networks;

struct sql_network_server
{
  int id;
  const char *name;
  int port;
};

struct sql_network
{
  int id;
  const char *name;
  const char *nick;
  const char *real_name;
  const char *password;
  GSList *servers;
};

extern int networks_sql_start (void);

extern struct sql_network *new_sql_network (const char *name, const char *nick, const char *real_name, const char *password);
extern struct sql_network_server * new_sql_network_server (const char *name, int port, struct sql_network *network);

extern void edit_sql_network (struct sql_network *network);
extern void edit_sql_network_server (struct sql_network_server *network_server, struct sql_network *network);

extern void del_sql_network (struct sql_network *e);
extern void del_sql_network_server (struct sql_network *e, struct sql_network_server *s);

extern void del_sql_networks_all (void);

extern void networks_sql_close (void);
