extern GSList *mime_types;

struct mime_type
{
  int id;
  const char *mime_name;
  const char *description;
  const char *extension;
  const char *icon;
};

extern int sql_start (void);

extern struct mime_type *new_mime_type (const char *mime_name, const char *description, const char *extension, const char *icon);

extern void del_mime_type (struct mime_type *e);

extern void del_mime_all (void);

extern void sql_close (void);
