
typedef struct _iniitem
{
  char *filename;
  char *lang;
  char *type;
  struct hash *h;

} IniFile;

enum {
  INI_SUCCESS,
  INI_ERROR_FILE_OPEN_FAILED,
  INI_ERROR_NOT_MIME_FILE
};

IniFile *
ini_new_from_file (const char *filename, char *lang);

unsigned char *
ini_get (IniFile *ini, char *field);

void
ini_set (IniFile *ini, const char *field, const unsigned char *value);

void
ini_free (IniFile *ini);
