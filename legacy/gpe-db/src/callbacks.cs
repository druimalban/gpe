#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include <stdlib.h>
#include <sqlite.h>
#include <stdio.h>
#include "db-funcs.h"

extern GtkWidget *GPE_DB_Main;
gint DBTableListSelectedRow = -1;

void DBSelected(GtkWidget *button);


void create_tabs(guint nrentries, gchar *names[])
{
GtkWidget *widget;
GtkWidget *notebook;
GtkWidget *scrolled_window;
GtkWidget *edit_table;
GtkWidget *label;
GtkWidget *entry;
guint i;

	/*
	 * First create the entries list in the second tab
	 */
	notebook=lookup_widget(GPE_DB_Main,"notebook1");
	widget=gtk_clist_new_with_titles(nrentries, names);
	gtk_widget_set_name (widget, "EntryCList");    
	gtk_widget_ref(widget);
	gtk_object_set_data_full (GTK_OBJECT (GPE_DB_Main), "EntryCList", widget,
                              (GtkDestroyNotify) gtk_widget_unref);
        gtk_clist_column_titles_passive(GTK_CLIST(widget));
	gtk_widget_show(widget);
	scrolled_window=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_name(scrolled_window,"Entries");
	gtk_widget_ref(scrolled_window);
	gtk_widget_show(scrolled_window);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window),widget);
	gtk_container_add(GTK_CONTAINER(notebook),scrolled_window);

	widget=gtk_label_new(_("Entries"));
	gtk_widget_show(widget);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1), widget);

	/*
	 * Second create the edit tab
	 */
	scrolled_window=gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_set_name(scrolled_window,"EntryEdit");
	gtk_widget_ref(scrolled_window);
	gtk_widget_show(scrolled_window);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	edit_table=gtk_table_new(nrentries,2,FALSE);
	gtk_widget_ref(edit_table);
	gtk_widget_show(edit_table);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), edit_table);

	for (i=0; i<nrentries; i++) {
		label=gtk_label_new(names[i]);
		gtk_widget_ref(label);
		gtk_widget_show(label);
		entry=gtk_entry_new();
		gtk_widget_ref(entry);
		gtk_widget_show(entry);
		gtk_table_attach(GTK_TABLE(edit_table), label, 0, 1, i, i+1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 0, 0);
		gtk_table_attach(GTK_TABLE(edit_table), entry, 1, 2, i, i+1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 0, 0);
	}
	gtk_container_add(GTK_CONTAINER(notebook),scrolled_window);
	widget=gtk_label_new(_("Edit"));
	gtk_widget_show(widget);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 2), widget);
}


void
on_select_db1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	DBSelected(GTK_WIDGET(menuitem));
}


void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	close_db();
	gtk_main_quit();
}


void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

void DBSelected(GtkWidget *button)
{
gchar **tables = NULL;
gint tcount = 0;
gchar *entry[1];

	close_db();
	if (open_db("/root/.gpe/address") == -1)
		return;
	tables=get_tables(&tcount);
	if (tcount != 0 && tables != NULL) {
		gint i;
		GtkWidget *DBTableCList;
	
		DBTableCList=lookup_widget(GTK_WIDGET(button),"DBTableCList");
		gtk_clist_clear(GTK_CLIST(DBTableCList));
		gtk_widget_set_sensitive(GTK_WIDGET(DBTableCList),TRUE);
		for (i=1; i<=tcount; i++) {
#ifdef DEBUG
			g_print("table %d = '%s'\n",i,tables[i]);
#endif
			entry[0]=(gchar *)malloc(strlen(tables[i])+1 * sizeof(gchar));
			strcpy(entry[0],tables[i]);
			gtk_clist_append(GTK_CLIST(DBTableCList),entry);
			free(entry[0]);
		}
	}
}


void
on_SelectDB_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	DBSelected(GTK_WIDGET(button));
}


void
on_DBTableCList_select_row             (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *OpenTable;

	OpenTable=lookup_widget(GTK_WIDGET(clist),"OpenTable");
	gtk_widget_set_sensitive(OpenTable, TRUE);
	DBTableListSelectedRow = row;
}


void
on_DBTableCList_unselect_row           (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
GtkWidget *OpenTable;

	OpenTable=lookup_widget(GTK_WIDGET(clist),"OpenTable");
	gtk_widget_set_sensitive(OpenTable, FALSE);
	DBTableListSelectedRow = -1;
}


void
on_DBSelectionOK_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_DBSelectionCancel_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{

}

static int
entries_callback (void *arg, int argc, char **argv, char **names)
{
#ifdef DEBUG
unsigned int i;
 
	g_print("argc=%d\n",argc);
	for (i=0; i<argc; i++) {
		g_print("argv[%d]='%s'\n",i,argv[i]);
	}
#endif
	gtk_clist_append(GTK_CLIST((GtkWidget *)arg), argv);

return 0;
}

void
on_OpenTable_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *DBTableClist;
GtkWidget *DBEntryCList;
gchar *entry;
gchar **fields;
gint ecount;
char query[128];

	DBTableClist = lookup_widget(GTK_WIDGET(button),"DBTableCList");
	gtk_clist_get_text(GTK_CLIST(DBTableClist), DBTableListSelectedRow, 0, &entry);
	ecount=-1;
	fields=get_fields(entry, &ecount);
	if (ecount > 0) {
#ifdef DEBUG
		gint i;

		for (i=0; i<ecount; i++) {
			g_print("column(%d)='%s'\n",i,fields[i]);
		}
#endif
		create_tabs(ecount,fields);
	}
	sprintf(query,"select * from %s",entry);
	DBEntryCList=lookup_widget(GTK_WIDGET(button),"EntryCList");
	if (DBEntryCList == NULL) {
		g_print("EntryCList not found!¸n");
		return;
	}
	gtk_clist_freeze(GTK_CLIST(DBEntryCList));
	db_exec(query, entries_callback, (void *)DBEntryCList, NULL);
	gtk_clist_columns_autosize (GTK_CLIST(DBEntryCList));
	gtk_clist_thaw(GTK_CLIST(DBEntryCList));
}


gboolean
on_GPE_DB_Main_de_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	close_db();
	gtk_main_quit();

return FALSE;
}
