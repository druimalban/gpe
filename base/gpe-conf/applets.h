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
#define _(x) (x)

/* Return 1 if a file exists & can be read, 0 otherwise.*/

int file_exists (char *fn);
GtkWidget *make_menu_from_dir(char *path, int(*entrytest)(char* path), char *current);

// I just want to get a file !
void ask_user_a_file(char *path, char *prompt,
		     void (*File_Selected)(char *filename, gpointer data),
		     void (*Cancel)(gpointer data),
		     gpointer data);

int mystrcmp(char *s, char *c);

// one usefull macro..
void system_and_gfree(gchar *cmd);
#define system_printf(x,y...)   system_and_gfree(g_strdup_printf(x,y))

extern struct gpe_icon my_icons[];
extern GtkStyle *wstyle; // for pixmaps

#endif
