#ifndef _MAIN_H
#define _MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
  gchar *filename;
  GnomeVFSFileInfo *vfs;
} FileInformation;

extern gchar *current_directory;

void refresh_current_directory (void);


#ifdef __cplusplus
}
#endif

#endif /* _MAIN_H */
