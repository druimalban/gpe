#include <stdlib.h>

#include "tetris.h"

void set_block(int x,int y,int color,int next)
{
	gdk_draw_pixmap((!next ? game_area->window : next_block_area->window),
			(!next ? game_area->style->black_gc : next_block_area->style->black_gc),
			blocks_pixmap,
			color*BLOCK_WIDTH,0,
			x*BLOCK_WIDTH,y*BLOCK_HEIGHT,
			BLOCK_WIDTH,BLOCK_HEIGHT);
}

int do_random(int max)
{
	return max*((float)random()/RAND_MAX);
}

void set_label(GtkWidget *label,char *str)
{
	gtk_label_set(GTK_LABEL(label), str);
}

void add_submenu(gchar *name,GtkWidget *menu,GtkWidget *menu_bar,int right)
{
	GtkWidget *sub_menu;
	
	sub_menu = gtk_menu_item_new_with_label(name);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(sub_menu),menu);
	if(right)
		gtk_menu_item_right_justify(GTK_MENU_ITEM(sub_menu));
	gtk_widget_show(sub_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu_bar),sub_menu);
}

GtkWidget *add_menu_item(gchar *name,GtkSignalFunc func,gpointer data,gint state,GtkWidget *menu)
{
	GtkWidget *menu_item;

	if(strlen(name))
		menu_item = gtk_menu_item_new_with_label(name);
	else
		menu_item = gtk_menu_item_new();
	gtk_widget_set_sensitive(menu_item,state);
	gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
			(GtkSignalFunc)func,data);
	gtk_menu_append(GTK_MENU(menu),menu_item);
	gtk_widget_show(menu_item);
	return menu_item;
}

GtkWidget *add_menu_item_toggle(gchar *name,GtkSignalFunc func,gpointer data,gint state,GtkWidget *menu)
{
	GtkWidget *menu_item;

	if(strlen(name))
		menu_item = gtk_check_menu_item_new_with_label(name);
	else	
		menu_item = gtk_check_menu_item_new();
	gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menu_item),state);
	gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
	gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
			(GtkSignalFunc)func,data);
	gtk_menu_append(GTK_MENU(menu),menu_item);
	gtk_widget_show(menu_item);
	return menu_item;
}
