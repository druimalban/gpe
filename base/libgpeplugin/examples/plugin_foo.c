#include <gpe/gpe-plugin.h>




void render_body()
{
	printf("Hello world body\n");
	
}


void render_tail()
{
	printf("Hello world tail\n");
	
}

void render_header()
{
	printf("Hello world header\n");
	
}

GtkWidget *plugin_widget()
{
	GtkWidget *apply, *label;
	GtkWidget *plug_widget ;
	plug_widget = gtk_vbox_new (TRUE,1);
	apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	label = gtk_label_new ("Provaccia bla bla bla\n");
	printf ("Plugin widget initialiser\n");	
	gtk_box_pack_start (GTK_BOX (plug_widget), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (plug_widget), apply, FALSE, FALSE, 0);
	gtk_widget_show_all (plug_widget);
	return plug_widget;
}



DECL_PLUGIN
DEFINE_NAME("foobar module")
DEFINE_RENDER_BODY(render_body)
DEFINE_RENDER_HEADER(render_header)
DEFINE_RENDER_TAIL(render_tail)
DEFINE_WIDGET(plugin_widget)
DECL_END
