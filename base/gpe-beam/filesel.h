#ifndef _GPE_BEAM_FILESEL_H
#define _GPE_BEAM_FILESEL_H

#include <gtk/gtk.h>

void ask_user_a_file (char *path, char *prompt,
		      void (*File_Selected) (const gchar *filename, gpointer data),
		      void (*Cancel) (gpointer data), gpointer data);


void freedata (GtkWidget * ignored, gpointer user_data);

struct fstruct
{
	GtkWidget *fs;
	void (*File_Selected) (const gchar *filename, gpointer data);
	void (*Cancel) (gpointer data);
	gpointer data;
};

#endif
