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
