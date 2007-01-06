#include <gtk/gtk.h>
#include "gtkcolombo.h"
#include <stdio.h>
#include <string.h>

#ifdef TEST
#include <stdio.h>
#endif

#define COLOMBO_LIST_MAX_HEIGHT 300

static GtkEntryClass* parent_class = NULL;

static void gtk_colombo_destroy(GtkObject* colombo) {

	if (GTK_COLOMBO(colombo)->popwin)
	{
		gtk_widget_destroy(GTK_COLOMBO(colombo)->popwin);
        	//gtk_widget_unref(GTK_COLOMBO(colombo)->popwin);
		GTK_COLOMBO(colombo)->popwin=NULL;
	}

        if ( GTK_OBJECT_CLASS(parent_class)->destroy )
                (*GTK_OBJECT_CLASS(parent_class)->destroy)(colombo);
}

void gtk_colombo_class_init (GtkColomboClass *klass) {
        GtkObjectClass* oclass;

        parent_class = gtk_type_class(gtk_entry_get_type());
        oclass = (GtkObjectClass*)klass;

        oclass->destroy = gtk_colombo_destroy;
}

static void
gtk_colombo_update_entry(GtkList* list, GtkColombo* colombo) {
	GtkListItem* li;
	GtkWidget* label;
	char* text;


	//gtk_grab_remove(GTK_WIDGET(colombo));
	if ( list->selection ) {
		li=list->selection->data;
		label = GTK_BIN(li)->child;
		if ( !label || !GTK_IS_LABEL(label) ) text=NULL;
		else 
		{
			text=NULL;
			gtk_label_get(GTK_LABEL(label), &text);
		}
		if ( !text )
			text = "";
		gtk_signal_handler_block(GTK_OBJECT(colombo), colombo->entry_change_id);
		gtk_entry_set_text(GTK_ENTRY(colombo), text);
		gtk_signal_handler_unblock(GTK_OBJECT(colombo), colombo->entry_change_id);
	}
	gtk_widget_hide(colombo->popwin);
	//gtk_grab_remove(colombo->popwin);
	//gdk_pointer_ungrab(GDK_CURRENT_TIME);
}

static void gtk_colombo_get_pos(GtkColombo* colombo, gint* x, gint* y, gint* height, gint* width) {
	GtkAllocation *pos = &(GTK_WIDGET(colombo)->allocation);
	GtkRequisition req1;
	GtkRequisition req2;
	GtkRequisition req3;

	gtk_widget_size_request(colombo->popup, &req1);
	gtk_widget_size_request(GTK_SCROLLED_WINDOW(colombo->popup)->hscrollbar, &req2);

	gtk_widget_size_request(colombo->list, &req3);
	*height = req1.height-req2.height+req3.height; /* it's a pain */
	gdk_window_get_origin(GTK_WIDGET(colombo)->window, x, y);
	*y += pos->height;
	*height = MIN(COLOMBO_LIST_MAX_HEIGHT, *height);
	*height = MIN(*height, gdk_screen_height() - *y);
	*width = pos->width;  /* this is wrong when it's resized: who knows why? */
}




static int gtk_colombo_popup_list2 (GtkColombo *colombo)
{
	gint height, width, x, y;
	char *sp;
	char sp2[100];
	GtkWidget *li;

	// first remove items
	gtk_list_clear_items(GTK_LIST(colombo->list),0,-1);
	if (colombo->glist!=NULL) g_list_free((GList *)(colombo->glist));
	colombo->glist=NULL;
	// now add items
	sp=NULL;
	if (NULL!=colombo->extfunc)
	{
		strcpy(sp2,gtk_entry_get_text(GTK_ENTRY(colombo)));
		//printf("text=%s\n",sp2);
		//sp=colombo->extfunc(gtk_entry_get_text(GTK_ENTRY(colombo)));
		sp=colombo->extfunc(sp2);
	}
	while (sp!=NULL)
	{
		printf("colombo: %s\n",sp);
		li=gtk_list_item_new_with_label(sp);
		colombo->glist=(GList *)g_list_append(colombo->glist,li);
		gtk_container_add(GTK_CONTAINER(colombo->list),li);
		sp=colombo->extfunc(NULL);
	}
	gtk_colombo_get_pos(colombo, &x, &y, &height, &width);
	gtk_widget_set_usize(colombo->popwin, width, height);
	gtk_widget_set_uposition(colombo->popwin, x, y);
	gtk_widget_show_all(colombo->popwin);
	return 0;

}

static void gtk_colombo_popup_list (GtkEntry *entry, GdkEventKey * event, GtkColombo *colombo)
{
         gtk_idle_add((GtkFunction)gtk_colombo_popup_list2,colombo);
}

gboolean gtk_colombo_hide_list(GtkWidget *widget, GdkEventFocus *event, GtkColombo *colombo)
{
	gtk_widget_hide(GTK_WIDGET(colombo->popwin));
	return 0;
}
void gtk_colombo_init (GtkColombo *colombo) {
	colombo->extfunc=NULL;
	colombo->glist=NULL;
	colombo->popwin = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_policy(GTK_WINDOW(colombo->popwin), 1, 1, 0);
	colombo->popup = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(colombo->popup),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	colombo->list = gtk_list_new();
	gtk_container_add(GTK_CONTAINER(colombo->popwin), colombo->popup);
        //gtk_container_add(GTK_CONTAINER(colombo->popup), colombo->list);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(colombo->popup),colombo->list);
        gtk_widget_show_all(colombo->popup);
	//colombo->list_change_id=
	gtk_signal_connect(GTK_OBJECT(colombo->list), "selection_changed",
		(GtkSignalFunc)gtk_colombo_update_entry, colombo);
	colombo->entry_change_id=gtk_signal_connect(GTK_OBJECT(colombo),
		"key_release_event",
		(GtkSignalFunc)gtk_colombo_popup_list,colombo);
	//colombo->entry_change_id=
	gtk_signal_connect(GTK_OBJECT(colombo),"focus_out_event",
		(GtkSignalFunc)gtk_colombo_hide_list,colombo);
}

guint gtk_colombo_get_type ()
{
        static guint colombo_type = 0;

        if (!colombo_type) {
                GtkTypeInfo colombo_info = {
                        "GtkColombo",
                        sizeof(GtkColombo),
                        sizeof(GtkColomboClass),
                        (GtkClassInitFunc) gtk_colombo_class_init,
                        (GtkObjectInitFunc) gtk_colombo_init,
                        NULL,
			NULL,
			(GtkClassInitFunc) NULL,
                };

                colombo_type = gtk_type_unique (gtk_entry_get_type (), &colombo_info);
        }
        return colombo_type;
}

GtkWidget* gtk_colombo_new()
{
        return GTK_WIDGET(gtk_type_new(gtk_colombo_get_type()));
}

void gtk_colombo_set_func ( GtkColombo *colombo,GtkColomboExternFunc extfunc)
{
	colombo->extfunc=extfunc;
}
#ifdef TEST


gchar *ff(char *i)
{
	static char count;
	static char s[100];
	static char *sp;

	if (i!=NULL) {sp=i;count=3;}
	if (count>0) 
	{
		sprintf(s,"%s#%d",sp,count);
		count--;
		return s;
	}
	return NULL;

}

int main(int argc, char **argv)
{
   GtkWidget *w,*col;
   // set locale
   gtk_set_locale();
   
   // init gtk
   gtk_init(&argc, &argv);

   // main window
   w=gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_signal_connect (GTK_OBJECT(w), "delete-event", 
      (GtkSignalFunc) gtk_main_quit, NULL);
   gtk_signal_connect (GTK_OBJECT(w), "destroy", 
      (GtkSignalFunc) gtk_main_quit, NULL);

   col=gtk_colombo_new();
   gtk_colombo_set_func(GTK_COLOMBO(col),ff);
   gtk_container_add (GTK_CONTAINER (w), col);
   gtk_widget_show_all(w);
   gtk_main();
   gtk_exit(0);

}
#endif
