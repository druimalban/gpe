/*****************************************************************************
 * gsoko/control.c : menu and callbacks for events
 *****************************************************************************
 * Copyright (C) 2000 Jean-Michel Grimaldi
 *
 * Author: Jean-Michel Grimaldi <jm@via.ecp.fr>
 *
 * Parts of the code are taken from the Gtk+ 1.2 documentation.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *****************************************************************************/

#include <stdio.h>	/* sprintf() */

#include "gsoko.h"

static GtkItemFactoryEntry menu_items[] = {
	{ "/_File",		NULL,		NULL,		0,	"<Branch>"},
	{ "/File/_Restart",	"R",		restart,	0,	NULL},
	{ "/File/_New Game",	"N",		new_game,	0,	NULL},
	{ "/File/sep",		NULL,		NULL,		0,	"<Separator>"},
	{ "/File/_Undo move",	"U",		undo_move,	0,	NULL},
	{ "/File/sep",		NULL,		NULL,		0,	"<Separator>"},
	{ "/File/Quit",		"Q",	goodbye,	0,	NULL},
	{ "/_Help",		NULL,		NULL,		0,	"<LastBranch>"},
	{ "/Help/_About",	NULL,		about,		0,	NULL},
};

/* build the menubar */
void get_main_menu(GtkWidget *window, GtkWidget **menubar)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

	/* attach the new accelerator group to the window. */
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	if (menubar)
		/* finally, return the actual menu bar created by the item factory. */ 
		*menubar = gtk_item_factory_get_widget(item_factory, "<main>");
}

/* redraw darea (from pixmap) */
int expose_event(GtkWidget *widget, GdkEventExpose *event)
{
	gdk_draw_pixmap(
		widget->window,
		widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		darea_pxm,
		event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);

	return FALSE;
}

/* handle keypresses over the window */
void key_press_event (GtkWidget *widget, GdkEventKey *event)
{
//	g_print("Key pressed: %x\n", event->keyval);
	switch(event->keyval)
	{
		case K_LEFT:
			move_s(1);	/* move left */
			break;
		case K_RIGHT:
			move_s(2);	/* move right */
			break;
		case K_UP:
			move_s(3);	/* move up */
			break;
		case K_DOWN:
			move_s(4);	/* move down */
			break;
		case K_RETURN:
			undo_move();
			break;
		/* cheat : 'J' = jump to next level */
/*		case K_J:
			next_level();
			break;*/
	}
}

/* terminate the application */
void goodbye(void)
{
	gtk_main_quit();
}

/* call goodbye */
int delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	goodbye();
	return FALSE;
}

/* display the 'About' dialog */
void about(void)
{
	static GtkWidget *window = NULL;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *bbox;
	GtkWidget *button;

	if (!window)
	{
		/* create a new dialog */
		window = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(window), "About");
		gtk_container_set_border_width(GTK_CONTAINER(window), 5);

		/* insert a frame in the vbox part of the dialog */
		frame = gtk_frame_new(NULL);
		gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox), frame);

		/* put a vbox inside this frame */
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
		gtk_container_add(GTK_CONTAINER(frame), vbox);

		/* put another frame inside this vbox */
		frame = gtk_frame_new(NULL);
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
		gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

		/* put a label inside this frame */
		label = gtk_label_new(W_TITLE);
		gtk_widget_set_usize(label, 300, 30);
		gtk_container_add(GTK_CONTAINER(frame), label);

		/* put another label inside the vbox */
		label = gtk_label_new(
			"(C) 2000-2003 Jean-Michel Grimaldi\n"
			"Author: Jean-Michel Grimaldi <jm@via.ecp.fr>\n"
                        "Contributors:\n"
                        "  Gilles Arnaud\n"
                        "  Göran Uddeborg <goeran@uddeborg.pp.se>\n"
			"  Christopher Gautier\n\n"
			"Sokoban for Gtk+\n"
			"Homepage: http://www.via.ecp.fr/~jm/gsoko.html");
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

		/* create a new buttonbox and put it in the action_area of the dialog */
		bbox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->action_area), bbox);

		/* create a button with the label "OK" and put it into bbox */
		button = gtk_button_new_with_label("OK");
		gtk_container_add(GTK_CONTAINER(bbox), button);

		/* make it the default button (ENTER will activate it) */
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);

		/* handle the deletion of the dialog */
		gtk_signal_connect_object(GTK_OBJECT(button), "clicked", (GtkSignalFunc)gtk_widget_hide, GTK_OBJECT(window));
		gtk_signal_connect(GTK_OBJECT(window), "delete_event", (GtkSignalFunc)gtk_widget_hide, GTK_OBJECT(window));

		gtk_widget_show_all(window);
	}
	else
	{
		if (!GTK_WIDGET_MAPPED(window))
			gtk_widget_show(window);
		else
			gdk_window_raise(window->window);
	}
}

/* display a dialog when all levels are completed */
void udidit(void)
{
	static GtkWidget *window = NULL;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *bbox;
	GtkWidget *button;

	if (!window)
	{
		/* create a new dialog */
		window = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(window), "You did it!");
		gtk_container_set_border_width(GTK_CONTAINER(window), 5);

		/* insert a frame in the vbox part of the dialog */
		frame = gtk_frame_new(NULL);
		gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox), frame);

		/* put a vbox inside this frame */
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
		gtk_container_add(GTK_CONTAINER(frame), vbox);

		/* put another frame inside this vbox */
		frame = gtk_frame_new(NULL);
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
		gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

		/* put a label inside this frame */
		label = gtk_label_new("You did it!");
		gtk_widget_set_usize(label, 300, 30);
		gtk_container_add(GTK_CONTAINER(frame), label);

		/* put another label inside the vbox */
		label = gtk_label_new(
			"Congratulations, you completed all the levels of this version.\n"
			"More are to come.\n\n"
			"You can check for new versions at http://www.via.ecp.fr/~jm/");
		gtk_widget_set_usize(label, 280, 0);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

		/* create a new buttonbox and put it in the action_area of the dialog */
		bbox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->action_area), bbox);

		/* create a button with the label "OK" and put it into bbox */
		button = gtk_button_new_with_label("OK");
		gtk_container_add(GTK_CONTAINER(bbox), button);

		/* make it the default button (ENTER will activate it) */
		GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default(button);

		/* handle the deletion of the dialog */
		gtk_signal_connect_object(GTK_OBJECT(button), "clicked", (GtkSignalFunc)gtk_widget_hide, GTK_OBJECT(window));
		gtk_signal_connect(GTK_OBJECT(window), "delete_event", (GtkSignalFunc)gtk_widget_hide, GTK_OBJECT(window));

		gtk_widget_show_all(window);
	}
	else
	{
		if (!GTK_WIDGET_MAPPED(window))
			gtk_widget_show(window);
		else
			gdk_window_raise(window->window);
	}
}

/* updates the title of the main window */
void make_title(void)
{
#define STITLE_LENGTH (sizeof(W_TITLE)+ 27* 3* 8+ 1) /* 27 is the length of the text below, 8 the size of a reasonably long integer */
	char stitle[STITLE_LENGTH]; 
	snprintf(stitle, STITLE_LENGTH, W_TITLE " / level %i - moves %i - boxes %i", level, nmoves, nbox);
	stitle[STITLE_LENGTH- 1]= 0;
	gtk_window_set_title(GTK_WINDOW(window), stitle);
}
