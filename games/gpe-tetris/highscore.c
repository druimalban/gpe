#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>

#include "tetris.h"

#include <libintl.h>

#define _(_x) gettext (_x)

struct item
{
	char name[10];
	long score;
};

struct item highscore[NUM_HIGHSCORE];

static void
close_window(GtkWidget *widget,
	     GtkWidget *w)
{
  gtk_widget_destroy (w);
}

void read_highscore()
{
	FILE *fp;
	if((fp = fopen(HIGHSCORE_FILE,"r")))
	{
		fread(&highscore,1,sizeof(highscore),fp);
		fclose(fp);
	}
}

void write_highscore()
{
	FILE *fp;
	if(!(fp = fopen(HIGHSCORE_FILE,"w")))
		return;
	fwrite(&highscore,1,sizeof(highscore),fp);
	fclose(fp);
}

void show_highscore(int place)
{
	GtkWidget *highscore_window;
	GtkWidget *highscore_border;
	GtkWidget *label;
	GtkWidget *table;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkStyle *style;
	GdkColormap *colormap;
	GdkColor color;
	int temp;
	char dummy[40];
	
	highscore_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(highscore_window),"Highscore");
	gtk_window_set_policy(GTK_WINDOW(highscore_window),FALSE,FALSE,TRUE);
	gtk_window_set_position(GTK_WINDOW(highscore_window),GTK_WIN_POS_CENTER);
	
	highscore_border = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(highscore_border),GTK_SHADOW_OUT);
	gtk_widget_show(highscore_border);
	gtk_container_add(GTK_CONTAINER(highscore_window),highscore_border);

	vbox = gtk_vbox_new(FALSE,30);
	gtk_container_add(GTK_CONTAINER(highscore_border),vbox);

	table = gtk_table_new(NUM_HIGHSCORE+1,3,FALSE);
	gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,TRUE,0);

	label = gtk_label_new("#");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);
	gtk_widget_set_usize(label,25,0);
	
	label = gtk_label_new("Name:");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,1,2,0,1);
	gtk_widget_set_usize(label,100,0);
	
	label = gtk_label_new("Score:");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table),label,2,3,0,1);
	
	for(temp=0;temp < NUM_HIGHSCORE;temp++)
	{
		sprintf(dummy,"%d",temp+1);
		label = gtk_label_new(dummy);
		gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,temp+1,temp+2);
		gtk_misc_set_alignment(GTK_MISC(label),0.5,0);	
		
		sprintf(dummy,"%s",highscore[temp].name);
		
		label = gtk_label_new(dummy);

		if(place && place-1 == temp)
		{
			style = gtk_style_new();
			colormap = gdk_colormap_get_system();
			color.red = 0;
			color.green = 0;
			color.blue = 0xffff;
			gdk_color_alloc(colormap,&color);
			memcpy(style->fg,&color,sizeof(GdkColor));
			gtk_widget_set_style(label,style);
			gtk_style_unref(style);
		}
		
		gtk_table_attach_defaults(GTK_TABLE(table),label,1,2,temp+1,temp+2);
		gtk_misc_set_alignment(GTK_MISC(label),0,0);
		sprintf(dummy,"%lu",highscore[temp].score);
		label = gtk_label_new(dummy);
		gtk_table_attach_defaults(GTK_TABLE(table),label,2,3,temp+1,temp+2);
		gtk_misc_set_alignment(GTK_MISC(label),0,0);
	}
	
	button = gpe_picture_button (highscore_window->style, _("Close"), "cancel");
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
				GTK_SIGNAL_FUNC(close_window),highscore_window);	
	gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,TRUE,0);
  	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	
	gtk_widget_show_all(highscore_window);
}

int addto_highscore(char *name,long score)
{
	int place = 0;
	int temp,namelen;
	int added = FALSE;
	
	for(temp=NUM_HIGHSCORE-1;temp > -1;temp--)
	{
		if(score > highscore[temp].score || (highscore[temp].score == 0 && strlen(highscore[temp].name) == 0))
		{
			place = temp;
			added = TRUE;
		}
	}
	if(added)
	{
		for(temp=NUM_HIGHSCORE-1;temp > place;temp--)
			memcpy(&highscore[temp],&highscore[temp-1],sizeof(highscore[0]));
		namelen = strlen(name);
		if(namelen > 9)
			namelen = 9;
		memset(&highscore[place].name,0,sizeof(highscore[0].name));
		memcpy(&highscore[place].name,name,namelen);
		highscore[place].score = score;
	}
	return place+1;
}
