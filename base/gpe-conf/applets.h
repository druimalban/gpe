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
};

/* When translation will be implemented...*/
#define _(x) (x)

/* Return 1 if a file exists & can be read, 0 otherwise.*/

int file_exists (char *fn);
GtkWidget *make_menu_from_dir(char *path, int(*entrytest)(char* path), char *current);
int mystrcmp(char *s, char *c);

#endif
