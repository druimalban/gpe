
typedef struct _dotdesktopitem
{
  gchar *filename;
  gchar *lang;
  struct hash *h;

} DotDesktop;

struct MimeType
{
  gchar *name;
  gchar *type;
  gchar *icon;
  gchar *exec;
  gchar **extensions;

};

enum {
  DD_SUCCESS,
  DD_ERROR_FILE_OPEN_FAILED,
  DD_ERROR_NOT_DESKTOP_FILE
};

DotDesktop *
dotdesktop_new_from_file(const char *filename, char *lang, char *section);

unsigned char *
dotdesktop_get(DotDesktop *dd, char *field);

void
dotdesktop_set(DotDesktop *dd, const char *field, const unsigned char *value);

void
dotdesktop_free(DotDesktop *dd);

extern GList *
get_mime_types (void);
