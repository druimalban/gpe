#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE /* Pour GlibC2 */
#endif
#include <time.h>
#include "applets.h"

#include <gpe/spacing.h>

#define XKBD_DIR "/usr/share/xkbd/"


static GList *options;


void Kbd_Save ()
{
	GList *l;

	l = options;
	while (l)
	{
		char *file;
		file = gtk_object_get_data (GTK_OBJECT(l->data), "file");
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(l->data)))
		{
			gchar *fn;
			FILE *outp;
			fn = g_strdup_printf ("%s/.gpe/gpe-kbd", g_get_home_dir());
			outp = fopen (fn, "w");
			g_free (fn);
			if (!outp)
			{
				perror ("Can't write to ~/.gpe/gpe-kbd");
				exit (1);
			}
			fprintf (outp, "%s\n", file);
			fclose (outp);
			break;
		}
		l = l->next;
	}
}

gboolean setup_current_option_is(char *file)
{
	FILE *inp;
	char temp[1024]="";
	gchar *fn;
	gboolean ret;

	fn = g_strdup_printf ("%s/.gpe/gpe-kbd", g_get_home_dir());
	inp = fopen (fn, "r");
	g_free (fn);
	if (!inp)
		return FALSE;

	fread (temp, 1, 1000, inp);

	/* Strip off newline */
	if (strchr (temp, '\n'))
		*(strchr(temp, '\n')) = 0;

	ret = strncmp (temp, file, 1000) ? FALSE : TRUE;
	fclose (inp);

	return ret;
}

GtkWidget *setup_kb (GtkWidget *box, GtkWidget *opt, char *name, char *file)
{
	GtkWidget *radio;
	if (opt)
		radio = gtk_radio_button_new_with_label_from_widget
			(GTK_RADIO_BUTTON(opt), name);
	else
		radio = gtk_radio_button_new_with_label (NULL, name);

	gtk_object_set_data (GTK_OBJECT(radio), "file", g_strdup(file));

	gtk_box_pack_start (GTK_BOX(box), radio, FALSE, FALSE, 0);

	if (setup_current_option_is(file))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(radio), TRUE);

	options = g_list_append (options, radio);

	return radio;
}


GtkWidget *Kbd_Build_Objects()
{  
  GtkWidget *categories;
  GtkWidget *catvbox1;
  GtkWidget *catlabel1;
  GtkWidget *catconthbox1;
  GtkWidget *catindentlabel1;
  GtkWidget *controlvbox1;

  GtkWidget *opt1;
  char *user_kbdrc;

  guint gpe_catspacing = gpe_get_catspacing ();
  gchar *gpe_catindent = gpe_get_catindent ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  guint gpe_border     = gpe_get_border ();
 
  categories = gtk_vbox_new (FALSE, gpe_catspacing);
  gtk_container_set_border_width (GTK_CONTAINER (categories), gpe_border);

  /* -------------------------------------------------------------------------- */
  catvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (categories), catvbox1, TRUE, TRUE, 0);

  catlabel1 = gtk_label_new (_("Keyboard Geometry/Layout")); /* FIXME: GTK2: make this bold */
  gtk_box_pack_start (GTK_BOX (catvbox1), catlabel1, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (catlabel1), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (catlabel1), 0, 0.5);

  catconthbox1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (catvbox1), catconthbox1, TRUE, TRUE, 0);

  catindentlabel1 = gtk_label_new (gpe_catindent);
  gtk_box_pack_start (GTK_BOX (catconthbox1), catindentlabel1, FALSE, FALSE, 0);

  controlvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (catconthbox1), controlvbox1, TRUE, TRUE, 0);
  
  /* FIXME: this is an ugly mix between Keyboard Geometry and Keyboard Layout */
  opt1 = setup_kb (controlvbox1, NULL, _("Standard"), XKBD_DIR "kbdconfig");
  setup_kb (controlvbox1,        opt1, _("Tiny"),     XKBD_DIR "kbdconfig.tiny");
  setup_kb (controlvbox1,        opt1, _("US"),       XKBD_DIR "kbdconfig.us"); /* FIXME: US? */
  setup_kb (controlvbox1,        opt1, _("Fitaly"),   XKBD_DIR "kbdconfig.fitaly");

  /* If the user has a config in ~/.kbdconfig, add it */
  user_kbdrc = g_strdup_printf ("%s/.kbdconfig", g_get_home_dir());
  if (file_exists(user_kbdrc))
    setup_kb (controlvbox1, opt1, _("User defined"), user_kbdrc);
  g_free (user_kbdrc);

  return categories;
}
