extern GHashTable *sql_general_config;

extern int general_sql_start (void);

extern void new_sql_general_tag (const char *property, const char *value);

extern void edit_sql_general_tag (const char *property, const char *value);

extern void general_sql_close (void);
