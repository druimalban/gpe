/*
 * gpe-logout
 *
 * Logout and shutdown tool for GPE. 
 * 
 * (c) 2008 Florian Boor <florian.boor@kernelconcepts.de>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gpe/spacing.h>
#include <gpe/infoprint.h>

#define _(x) gettext (x)

#define DEFAULT_THEME_DIR PREFIX "/share/gpe/pixmaps/default/"

static void 
cpanel_do_suspend (GtkWidget *button, GtkWidget *panel)
{
  gpe_popup_infoprint(GDK_DISPLAY(), _("Suspend"));
  gtk_widget_destroy (panel);
  g_spawn_command_line_async ("apm -s", NULL); /* apm is suid root on our devices usually */
}

static void 
cpanel_do_poweroff (GtkWidget *button, GtkWidget *panel)
{
  gpe_popup_infoprint(GDK_DISPLAY(), _("Power down..."));
  g_spawn_command_line_async ("gpe-conf task_shutdown", NULL);
  gtk_widget_destroy (panel);
}

static void 
cpanel_do_lock (GtkWidget *button, GtkWidget *panel)
{
  g_spawn_command_line_async ("gpe-lock-display", NULL);
  gtk_widget_destroy (panel);
}

static void 
cpanel_do_logout (GtkWidget *button, GtkWidget *panel)
{
  gpe_popup_infoprint(GDK_DISPLAY(), _("Logout"));
  gtk_widget_destroy (panel);
  g_spawn_command_line_async ("gpe-logout.sh", NULL);
}

void
controlpanel_open (void)
{
  GtkWidget *cpanel, *vbox;
  GtkWidget *bsuspend, *bpoweroff, *block, *bclose, *blogout;
  GtkWidget *imgsuspend, *imgpoweroff, *imglock, *imgclose, *imglogout;
		
  cpanel = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  /* If screen is large enough make the window appear as a dialog */
  if (gdk_screen_width() > 320)
    gtk_window_set_type_hint(GTK_WINDOW(cpanel), GDK_WINDOW_TYPE_HINT_DIALOG);  

  gtk_window_set_keep_above (GTK_WINDOW(cpanel), TRUE);
  gtk_window_set_position (GTK_WINDOW(cpanel), GTK_WIN_POS_CENTER);
  gtk_window_set_decorated (GTK_WINDOW(cpanel), FALSE);

  gtk_window_set_default_size (GTK_WINDOW(cpanel), 240, 300);

  vbox = gtk_vbox_new (TRUE, gpe_get_boxspacing());
  gtk_container_set_border_width (GTK_CONTAINER(vbox), gpe_get_border());

  bsuspend = gtk_button_new_with_label (_("  Suspend"));
  bpoweroff = gtk_button_new_with_label (_("  Turn off"));
  block = gtk_button_new_with_label (_("  Lock"));
  blogout = gtk_button_new_with_label (_("  Logout"));
  bclose = gtk_button_new_with_label (_("  Back"));

  imgsuspend = gtk_image_new_from_file (DEFAULT_THEME_DIR "session-suspend.png");
  imgpoweroff = gtk_image_new_from_file (DEFAULT_THEME_DIR "session-halt.png");
  imglock = gtk_image_new_from_file (DEFAULT_THEME_DIR "session-lock.png");
  imglogout = gtk_image_new_from_file (DEFAULT_THEME_DIR "session-logout.png");
  imgclose = gtk_image_new_from_file (DEFAULT_THEME_DIR "session-back.png");

  gtk_button_set_image (GTK_BUTTON (bsuspend), imgsuspend);
  gtk_button_set_image (GTK_BUTTON (bpoweroff), imgpoweroff);
  gtk_button_set_image (GTK_BUTTON (block), imglock);
  gtk_button_set_image (GTK_BUTTON (blogout), imglogout);
  gtk_button_set_image (GTK_BUTTON (bclose), imgclose);
  gtk_button_set_alignment (GTK_BUTTON (bsuspend), 0.0, 0.5);
  gtk_button_set_alignment (GTK_BUTTON (bpoweroff), 0.0, 0.5);
  gtk_button_set_alignment (GTK_BUTTON (block), 0.0, 0.5);
  gtk_button_set_alignment (GTK_BUTTON (blogout), 0.0, 0.5);
  gtk_button_set_alignment (GTK_BUTTON (bclose), 0.0, 0.5);
		
  gtk_box_pack_start (GTK_BOX (vbox), bsuspend, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), bpoweroff, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), block, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), blogout, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), bclose, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (bsuspend), "clicked", G_CALLBACK (cpanel_do_suspend), cpanel);
  g_signal_connect (G_OBJECT (bpoweroff), "clicked", G_CALLBACK (cpanel_do_poweroff), cpanel);
  g_signal_connect (G_OBJECT (block), "clicked", G_CALLBACK (cpanel_do_lock), cpanel);
  g_signal_connect (G_OBJECT (blogout), "clicked", G_CALLBACK (cpanel_do_logout), cpanel);
  g_signal_connect_swapped (G_OBJECT (bclose), "clicked", G_CALLBACK (gtk_widget_destroy), cpanel);

  g_signal_connect (G_OBJECT (cpanel), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  gtk_container_add (GTK_CONTAINER (cpanel), vbox);
	
  gtk_widget_show_all (cpanel);
  gtk_window_present (GTK_WINDOW (cpanel));
}

int
main (int argc, char *argv[])
{

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  controlpanel_open();
  gtk_main();

  exit(0);
}  

