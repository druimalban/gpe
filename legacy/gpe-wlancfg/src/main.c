#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"

GtkWidget *GPE_WLAN;

int
main (int argc, char *argv[])
{
GtkWidget *widget;

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
	add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

	GPE_WLAN = create_GPE_WLANCFG ();
	widget=lookup_widget(GPE_WLAN,"SchemeDelete");
	gtk_widget_set_sensitive(widget,FALSE);
	widget=lookup_widget(GPE_WLAN,"SchemeEdit");
 	gtk_widget_set_sensitive(widget,FALSE);
	widget=lookup_widget(GPE_WLAN,"ListItemUp");
	gtk_widget_set_sensitive(widget,FALSE);
	widget=lookup_widget(GPE_WLAN,"ListItemDown");
 	gtk_widget_set_sensitive(widget,FALSE);
 	widget=lookup_widget(GPE_WLAN,"SchemeList");
 	gtk_clist_column_titles_passive(GTK_CLIST(widget));

	gtk_widget_show (GPE_WLAN);

	gtk_main ();

return 0;
}
