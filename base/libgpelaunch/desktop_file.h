#include <glib.h>

#ifndef DESKTOP_FILE_H
#define DESKTOP_FILE_H

typedef struct _GnomeDesktopFile GnomeDesktopFile;
typedef enum _GnomeDesktopFileEncoding GnomeDesktopFileEncoding;

typedef void (* GnomeDesktopFileSectionFunc) (GnomeDesktopFile *df,
					      const char       *name,
					      gpointer          data);
typedef void (* GnomeDesktopFileLineFunc) (GnomeDesktopFile *df,
					   const char       *key, /* If NULL, value is comment line */
					   const char       *locale,
					   const char       *value, /* This is raw unescaped data */
					   gpointer          data);

enum _GnomeDesktopFileEncoding {
  GNOME_DESKTOP_FILE_ENCODING_UTF8,
  GNOME_DESKTOP_FILE_ENCODING_LEGACY,
  GNOME_DESKTOP_FILE_ENCODING_UNKNOWN
};

typedef enum 
{
  GNOME_DESKTOP_PARSE_ERROR_INVALID_SYNTAX,
  GNOME_DESKTOP_PARSE_ERROR_INVALID_ESCAPES,
  GNOME_DESKTOP_PARSE_ERROR_INVALID_CHARS
} GnomeDesktopParseError;

#define GNOME_DESKTOP_PARSE_ERROR gnome_desktop_parse_error_quark()
GQuark gnome_desktop_parse_error_quark (void);

GnomeDesktopFile *gnome_desktop_file_new             (GnomeDesktopFileEncoding    encoding);
GnomeDesktopFile *gnome_desktop_file_new_from_string (char                       *data,
						      GError                    **error);
GnomeDesktopFile *gnome_desktop_file_load            (const char                 *filename,
						      GError                    **error);
gboolean          gnome_desktop_file_save            (GnomeDesktopFile           *df,
						      const char                 *path,
                                                      int                         mode,
						      GError                    **error);
char *            gnome_desktop_file_to_string       (GnomeDesktopFile           *df);
void              gnome_desktop_file_free            (GnomeDesktopFile           *df);
void              gnome_desktop_file_launch          (GnomeDesktopFile           *df,
						      char                      **argv,
						      int                         argc,
						      GError                    **error);


GnomeDesktopFileEncoding gnome_desktop_file_get_encoding    (GnomeDesktopFile            *df);
void                     gnome_desktop_file_foreach_section (GnomeDesktopFile            *df,
							     GnomeDesktopFileSectionFunc  func,
							     gpointer                     user_data);
void                     gnome_desktop_file_foreach_key     (GnomeDesktopFile            *df,
							     const char                  *section,
							     gboolean                     include_localized,
							     GnomeDesktopFileLineFunc     func,
							     gpointer                     user_data);
gboolean                 gnome_desktop_file_add_section     (GnomeDesktopFile            *df,
							     const char                  *name);
gboolean                 gnome_desktop_file_remove_section  (GnomeDesktopFile            *df,
							     const char                  *name);
void                     gnome_desktop_file_rename_section  (GnomeDesktopFile            *df,
                                                             const char                  *old_name,
                                                             const char                  *new_name);
gboolean                 gnome_desktop_file_has_section     (GnomeDesktopFile            *df,
                                                             const char                  *name);

/* Gets the raw text of the key, unescaped */
gboolean gnome_desktop_file_get_raw           (GnomeDesktopFile  *df,
					       const char        *section,
					       const char        *keyname,
					       const char        *locale,
					       const char       **val);
gboolean gnome_desktop_file_get_boolean       (GnomeDesktopFile  *df,
					       const char        *section,
					       const char        *keyname,
					       gboolean          *val);
gboolean gnome_desktop_file_get_number        (GnomeDesktopFile  *df,
					       const char        *section,
					       const char        *keyname,
					       double            *val);
gboolean gnome_desktop_file_get_string        (GnomeDesktopFile  *df,
					       const char        *section,
					       const char        *keyname,
					       char             **val);
gboolean gnome_desktop_file_get_locale_string (GnomeDesktopFile  *df,
					       const char        *section,
					       const char        *keyname,
					       char             **val);
gboolean gnome_desktop_file_get_regexp        (GnomeDesktopFile  *df,
					       const char        *section,
					       const char        *keyname,
					       char             **val);

gboolean gnome_desktop_file_get_booleans       (GnomeDesktopFile   *df,
						const char         *section,
						const char         *keyname,
						gboolean          **vals,
						int                *len);
gboolean gnome_desktop_file_get_numbers        (GnomeDesktopFile   *df,
						const char         *section,
						const char         *keyname,
						double            **vals,
						int                *len);
gboolean gnome_desktop_file_get_strings        (GnomeDesktopFile   *df,
						const char         *section,
						const char         *keyname,
                                                const char         *locale,
						char             ***vals,
						int                *len);
gboolean gnome_desktop_file_get_regexps        (GnomeDesktopFile   *df,
						const char         *section,
						const char         *keyname,
						char             ***vals,
						int                *len);

void gnome_desktop_file_set_raw                (GnomeDesktopFile  *df,
                                                const char        *section,
                                                const char        *keyname,
                                                const char        *locale,
                                                const char        *value);
gboolean gnome_desktop_file_set_string        (GnomeDesktopFile  *df,
					       const char        *section,
					       const char        *keyname,
					       const char        *value);
void     gnome_desktop_file_set_strings       (GnomeDesktopFile  *df,
					       const char        *section,
					       const char        *keyname,
                                               const char        *locale,
					       const char       **value);

void gnome_desktop_file_merge_string_into_list  (GnomeDesktopFile *df,
                                                 const char        *section,
                                                 const char        *keyname,
                                                 const char        *locale,
                                                 const char        *value);
void gnome_desktop_file_remove_string_from_list (GnomeDesktopFile *df,
                                                 const char        *section,
                                                 const char        *keyname,
                                                 const char        *locale,
                                                 const char        *value);


const char* desktop_file_get_encoding_for_locale (const char *locale);

void gnome_desktop_file_unset            (GnomeDesktopFile *df,
                                          const char       *section,
                                          const char       *keyname);
void gnome_desktop_file_unset_for_locale (GnomeDesktopFile *df,
                                          const char       *section,
                                          const char       *keyname,
                                          const char       *locale);

void gnome_desktop_file_copy_key (GnomeDesktopFile *df,
                                  const char       *section,
                                  const char       *source_key,
                                  const char       *dest_key);

/* ... More setters ... */

#endif
