#ifndef APPLETS_H
#define APPLETS_H

#include <gtk/gtk.h>
#include <gpe/pixmaps.h>

struct Applet
{
  GtkWidget *(*Build_Objects)(); // store his objets in a static self struct
  void (*Free_Objects)(); // free all but the Gtk Widgets
  void (*Save)();
  void (*Restore)();
  char * label;
  char * name;
  char * frame_label;
  char * icon_file;
};

#define _(x) gettext(x)


void change_cfg_value (const gchar *file, const gchar *var, const gchar* val, gchar seperator);
void file_set_line (const gchar *file, const gchar *pattern, const gchar *newline);
void printlog(GtkWidget *textview, gchar *str);

/* Return 1 if a file exists & can be read, 0 otherwise.*/
int file_exists (char *fn);
GList *make_items_from_dir(char *path, char *filter);

// I just want to get a file !
void ask_user_a_file(char *path, char *prompt,
		     void (*File_Selected)(char *filename, gpointer data),
		     void (*Cancel)(gpointer data),
		     gpointer data);

int runProg(char *cmd);

// one usefull macro..
int system_and_gfree(gchar *cmd);
#define system_printf(y...)   system_and_gfree(g_strdup_printf(y))

void
freedata                               (GtkWidget       *ignored,
                                        gpointer         user_data); // to free data on destroy

extern struct gpe_icon my_icons[];
extern GtkStyle *wstyle; // for pixmaps
extern GtkWidget *mainw; // for dialogs

struct fstruct
{
  GtkWidget *fs;
  void (*File_Selected)(char *filename, gpointer data);  
  void (*Cancel)(gpointer data);
  gpointer data;
};

extern int suid_exec(const char* cmd,const char* params);

#endif
