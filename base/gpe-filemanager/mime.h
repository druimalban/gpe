struct MimeType
{
  gchar *name;
  gchar *type;
  gchar *icon;
  gchar **commands;
  gchar **extensions;
};

struct DotDesktop
{
  gchar *filename;
  gchar *lang;
  GHashTable *hash;
};

enum {
  DD_SUCCESS,
  DD_ERROR_FILE_OPEN_FAILED,
  DD_ERROR_NOT_DESKTOP_FILE
};
