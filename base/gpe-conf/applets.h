#include <gtk/gtk.h>


#ifndef APPLETS_H
#define APPLETS_H
struct Applet
{
  GtkWidget *(*Build_Objects)(); // store his objets in a static self struct
  void (*Free_Objects)(); // free all but the Gtk Widgets
  void (*Save)();
  void (*Restore)();
  char * label;
  char * name;
  char * frame_label;
};

/* When translation will be implemented...*/
#define _(x) gettext(x)

/* Return 1 if a file exists & can be read, 0 otherwise.*/

int file_exists (char *fn);
GtkWidget *make_menu_from_dir(char *path, int(*entrytest)(char* path), char *current);
GList *make_items_from_dir(char *path);

// I just want to get a file !
void ask_user_a_file(char *path, char *prompt,
		     void (*File_Selected)(char *filename, gpointer data),
		     void (*Cancel)(gpointer data),
		     gpointer data);

int mystrcmp(char *s, char *c);

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
