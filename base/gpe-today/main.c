/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

/* librootimage */
#include <rootpixmap.h>

#include "today.h"
#include "today-prefs.h"

/* Private functions, public ones are in today.h */
static void init_main_window (void);
static void load_modules (void);

int main(int argc, char **argv)
{
	if (!gpe_application_init(&argc, &argv))
		exit(1);

	gtk_rc_parse(DATAPATH(gtkrc));
	
	init_main_window();
	load_modules();
	
	gtk_widget_show_all(window.toplevel);
	
	gtk_main();

	exit(0);
}

static gboolean free_pixmap (Pixmap pix)
{
	XFreePixmap (GDK_DISPLAY(), pix);
	return TRUE;
}

static gboolean resize_callback(GtkWidget *wid, GdkEventConfigure *event,
				gpointer data)
{
	Pixmap rmap;
	GdkPixmap *old_pix = g_object_get_data (G_OBJECT (wid), "bg-pixmap");

	if ((wid->allocation.height >= wid->allocation.width) == window.mode) {
		window.mode = !window.mode;
	}

	rmap = GetRootPixmap(GDK_DISPLAY());

	if (rmap != None) {
		Pixmap pmap = CutWinPixmap(GDK_DISPLAY(),
		                    GDK_WINDOW_XWINDOW(wid->window), rmap,
		                    GDK_GC_XGC(wid->style->black_gc));
		
		if (pmap != None) {
			GdkPixmap *pix = gdk_pixmap_foreign_new(pmap);
			wid->style->bg_pixmap[GTK_STATE_NORMAL] = pix;
			wid->style->bg_pixmap[GTK_STATE_ACTIVE] = pix;
			wid->style->bg_pixmap[GTK_STATE_PRELIGHT] = pix;
			g_object_ref(pix);
			g_object_ref(pix);
			gtk_widget_set_style(wid, wid->style);
			g_object_set_data_full (G_OBJECT (pix), "pixmap", (void *)pmap,
						(GDestroyNotify)free_pixmap);
			g_object_set_data (G_OBJECT (wid), "bg-pixmap", pix);
		}
	}

	/* refresh various modules */
	/* weird stuff going on here, MUST draw scroll's BEFORE toplevel */
	gtk_widget_queue_draw(calendar.scroll->draw);
	gtk_widget_queue_draw(todo.scroll->draw);
	gtk_widget_queue_draw(todo.toplevel);
	gtk_widget_queue_draw(calendar.toplevel);
	gtk_widget_queue_draw(window.vpan1);

	if (old_pix) {
		g_object_unref(old_pix);
		g_object_unref(old_pix);
		g_object_unref(old_pix);
	}

	return FALSE;
}

static void init_main_window(void)
{
	window.toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window.toplevel), _("Summary"));
	gtk_widget_set_name(window.toplevel, "main_window");
	gtk_widget_realize(window.toplevel);
	window.height = window.width = 0;

	window.vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window.toplevel), window.vbox1);

	/* final stuff */
	g_signal_connect(G_OBJECT(window.toplevel), "destroy",
			 G_CALLBACK(gtk_exit), NULL);

	g_signal_connect(G_OBJECT(window.toplevel), "configure-event",
			 G_CALLBACK(resize_callback), NULL);
}

static void load_modules(void)
{
	date_init();
	gtk_box_pack_start(GTK_BOX(window.vbox1), date.toplevel, FALSE, FALSE, 0);

	window.vpan1 = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(window.vbox1), window.vpan1, TRUE, TRUE, 0);

	calendar_init();
	gtk_paned_pack1 (GTK_PANED(window.vpan1), calendar.toplevel, FALSE, FALSE);

	todo_init();
	gtk_paned_pack2 (GTK_PANED(window.vpan1), todo.toplevel, FALSE, FALSE);
}

void load_pixmap(const char *path, GdkPixmap **pixmap, GdkBitmap **mask, int alpha)
{
	if (!load_pixmap_non_critical(path, pixmap, mask, alpha)) {
		gpe_error_box_fmt("Could not load pixmap\n%s", path);
		exit(1);
	}
}

/*
 * return 0: failure
 * return 1: success
 */
int load_pixmap_non_critical(const char *path, GdkPixmap **pixmap,
                             GdkBitmap **mask, int alpha)
{
	GdkPixbuf *img;

	img = gdk_pixbuf_new_from_file(path, NULL);
	
	if (!img)
		return 0;

	gdk_pixbuf_render_pixmap_and_mask(img, pixmap, mask, alpha);
	gdk_pixbuf_unref(img);

	return 1;
}

void myscroll_adjust_cb(GtkAdjustment *adjustment, gpointer draw)
{
	gtk_widget_queue_draw(draw);
}

gboolean myscroll_draw_cb(GtkWidget *wid, GdkEventExpose *event, gpointer data)
{
	GSList *i;
	struct myscroll *scroll = data;
	int ny, y = - scroll->adjust->value;

	/* might be better to iterate over the lines
	 * instead of the whole layouts */

	for (i = scroll->list; i; i = g_slist_next(i)) {
		PangoLayout *pl = i->data;
		
		pango_layout_get_pixel_size(pl, NULL, &ny);
		if (y + ny + scroll->yspacing <= 0) {
			y += ny + scroll->yspacing;
			continue;
		}
		
		gdk_draw_layout(wid->window,
				wid->style->fg_gc[GTK_WIDGET_STATE(wid)],
				0, y, pl);
		
		y += ny + scroll->yspacing;
		
		if (y >= wid->allocation.height)
			break;
	}
	
	return TRUE;
}

gboolean myscroll_button_cb(GtkWidget *wid, GdkEventButton *ev, gpointer draw)
{
	gtk_widget_queue_draw(draw);
    
	return FALSE;
}

void myscroll_update_upper_adjust(struct myscroll *scroll)
{
	int size, y;
	GSList *i;
	
	for (size=0, i = scroll->list; i; i = g_slist_next(i)) {
		PangoLayout *pl = i->data;
		
		pango_layout_get_pixel_size(pl, NULL, &y);
		size += y + scroll->yspacing;
	}
	
	scroll->adjust->upper = size;
	gtk_adjustment_changed(scroll->adjust);
}

gboolean myscroll_size_cb(GtkWidget *wid, GdkEventConfigure *ev, gpointer data)
{
	struct myscroll *scroll = data;
	GSList *i;
	
	scroll->adjust->page_size = ev->height;
	scroll->adjust->value = 0;
	scroll->width = ev->width;
	
	for (i = scroll->list; i; i = g_slist_next(i)) {
		PangoLayout *pl = i->data;   
		pango_layout_set_width(pl, ev->width * PANGO_SCALE);
	}
	
	myscroll_update_upper_adjust(scroll);
	
	if (scroll->adjust->page_size >= scroll->adjust->upper)
		gtk_widget_hide(scroll->scrollbar);
	else if (!GTK_WIDGET_VISIBLE(scroll->scrollbar))
		gtk_widget_show(scroll->scrollbar);
	
	return TRUE;
}

struct myscroll * myscroll_new(gboolean continuous)
{
	struct myscroll *scroll = g_malloc(sizeof(struct myscroll));
	
	scroll->draw = gtk_drawing_area_new();
	scroll->adjust = (GtkAdjustment *) gtk_adjustment_new(0, 0, 0, 10, 10, 1);
	scroll->scrollbar = gtk_vscrollbar_new(scroll->adjust);
	
	g_signal_connect(G_OBJECT(scroll->draw), "expose-event",  
			 G_CALLBACK(myscroll_draw_cb), scroll);
	
	g_signal_connect(G_OBJECT(scroll->draw), "configure-event",
			 G_CALLBACK(myscroll_size_cb), scroll);
	
	if (continuous)
		g_signal_connect(G_OBJECT(scroll->adjust), "value-changed",
				 G_CALLBACK(myscroll_adjust_cb), scroll->draw);
	else
		g_signal_connect(G_OBJECT(scroll->scrollbar), "button-release-event",
				 G_CALLBACK(myscroll_button_cb), scroll->draw);
	
	scroll->list = NULL;
	scroll->yspacing = 2;
	
	return scroll;
}

void markup_printf(PangoLayout *pl, const char *fmt, ...)
{
	va_list ap;
	char *str;

	va_start (ap, fmt);
	vasprintf (&str, fmt, ap);
	va_end (ap);

	pango_layout_set_markup(pl, str, strlen(str));

	g_free (str);
}
