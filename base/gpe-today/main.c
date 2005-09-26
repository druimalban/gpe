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
#include <string.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>
#include "xsettings-client.h"
#include "today.h"

struct window_s window;
struct conf_s conf;

extern gboolean start_xsettings(void);

/* Private functions, public ones are in today.h */
static void init_main_window(void);
static void load_modules(void);
static void set_bg_pixmap(const char *path);
static void set_bg_color(const char *color);
static void refresh_widgets(void);

int main(int argc, char **argv)
{
	if (!gpe_application_init(&argc, &argv))
		exit(1);

	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
	
	gtk_rc_parse(DATAPATH(gtkrc));
	
	init_main_window();
	load_modules();
        
        conf.bg = -1;
        start_xsettings();

        if (conf.bg == -1)
            set_background("<mbdesktop>");

	gtk_widget_show_all(window.toplevel);
	
	gtk_main();

	exit(0);
}

static gboolean resize_callback(GtkWidget *wid, GdkEventConfigure *event,
                                gpointer data)
{
	if ((gdk_screen_height() >= gdk_screen_width()) == window.mode) {
		window.mode = !window.mode;
	}

        refresh_widgets();

        return FALSE;
}

static void refresh_widgets(void)
{
	/* refresh various modules, not sure why this is necessary */
	/* weird stuff going on here, MUST draw scroll's BEFORE toplevel */
	gtk_widget_queue_draw(calendar.scroll->draw);
	gtk_widget_queue_draw(todo.scroll->draw);
	gtk_widget_queue_draw(todo.toplevel);
	gtk_widget_queue_draw(calendar.toplevel);
	gtk_widget_queue_draw(window.vpan1);
}

#define MATCHBOX_BG "MATCHBOX/Background"

/* some code adpated from mbdesktop.c copyright matthew allum */
void set_background(const char *spec)
{
        /* img-tiled:<filename>
         * img-streched:<filename> -- will be tiled too
         * col-solid:<filename>
         * <mbdesktop>
         *
         * TODO:
         *  proper img-streched
         *  col-gradient-{vertical|horizontal}
         */

        struct cms {
            char *pat;
            int t;
        } cm[] = {
            { "<mbdesktop>", MBDESKTOP_BG },
            { "img-tiled:", TILED_BG_IMG },
            { "img-streched:", STRECHED_BG_IMG },
            { "col-solid:", SOLID_BG_COL }
        };

        int type, offset, i, nt = (sizeof(cm)/sizeof(struct cms));
        XSettingsSetting *setting;
        const char *arg = NULL;

        if (!spec)
            return;

        for (i=0, type=NO_SPEC, offset=0; i < nt; i++) {
            offset = strlen(cm[i].pat);
            if (offset <= strlen(spec) && !strncmp(cm[i].pat, spec, offset)) {
                type = cm[i].t;
                break;
            }
        }

        conf.bg = type;
        if (type) {
            arg = spec + offset;
        }

        switch (type) {
        case NO_SPEC: /* assume spec == filename */
            set_bg_pixmap(spec);
            break;
        case MBDESKTOP_BG:
            if (conf.xst_client &&
                xsettings_client_get_setting(conf.xst_client, MATCHBOX_BG,
                                             &setting) == XSETTINGS_SUCCESS
                && setting->type == XSETTINGS_TYPE_STRING) {

                /* possible loop here if mb's setting is "<matchbox>" */
				if (strstr(setting->data.v_string, ":") 
					&& strlen(strstr(setting->data.v_string, ":")))
				  set_background(strstr(setting->data.v_string, ":") + 1);
            }
            break;
        case TILED_BG_IMG:
        case STRECHED_BG_IMG:
            set_bg_pixmap(arg);
            break;
        case SOLID_BG_COL:
            set_bg_color(arg);
            break;
        }
}

/*
 * return 0: failure
 * return 1: success
 */
int load_pixmap_non_critical(const char *path, GdkPixmap **pixmap, int alpha)
{
	GdkPixbuf *img, *bg;
	int height, width;

	/* load image from file */
	img = gdk_pixbuf_new_from_file(path, NULL);
	if (!img)
		return 0;
	
	/* determine dimensions */
	width = gdk_pixbuf_get_width(img);
	height = gdk_pixbuf_get_height(img);
	
	/* compose a new pixbuf with background instead of alpha channel to avoid 
	   making random fb background visible. */
	bg = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 
	                    gdk_pixbuf_get_bits_per_sample(img), width, height);
	gdk_pixbuf_fill(bg, 0);
	gdk_pixbuf_composite(img, bg, 0, 0, width, height, 0, 0, 1, 1, 
	                     GDK_INTERP_BILINEAR, alpha);


	*pixmap = gdk_pixmap_new(window.toplevel->window,  width, height, -1);
	gdk_draw_pixbuf(*pixmap, NULL, bg, 0, 0 , 0, 0, -1, -1, 
	                GDK_RGB_DITHER_NORMAL, 1, 1);
	gdk_pixbuf_unref(img);
	gdk_pixbuf_unref(bg);

	return 1;
}

static void set_bg_pixmap(const char *path)
{
    GtkWidget *wid = window.toplevel;
    GdkPixmap *old_pix = NULL, *pix = NULL;

	if (load_pixmap_non_critical(path, &pix, 0xFF))
    if (pix) {
        old_pix = g_object_get_data(G_OBJECT(wid), "bg-pixmap");

        wid->style->bg_pixmap[GTK_STATE_NORMAL] = pix;
        wid->style->bg_pixmap[GTK_STATE_ACTIVE] = pix;
        wid->style->bg_pixmap[GTK_STATE_PRELIGHT] = pix;
        gtk_widget_set_style(wid, wid->style);
        g_object_set_data(G_OBJECT(wid), "bg-pixmap", pix);
    
        if (old_pix) {
            g_object_unref(old_pix);
	}

        refresh_widgets();
    }
}

static void set_bg_color(const char *color)
{
    /* TODO */
}

static void init_main_window(void)
{
	window.toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window.toplevel), _("Summary"));
	gtk_widget_set_name(window.toplevel, "main_window");
	gtk_widget_realize(window.toplevel);
	
	if (gdk_screen_height() > gdk_screen_width())
		window.mode = PORTRAIT;
	else
		window.mode = LANDSCAPE;
	
	window.vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window.toplevel), window.vbox1);

	/* final stuff */
	g_signal_connect(G_OBJECT(window.toplevel), "destroy",
			 G_CALLBACK(gtk_main_quit), NULL);

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
	gtk_paned_pack1(GTK_PANED(window.vpan1), calendar.toplevel, FALSE, FALSE);

	todo_init();
	gtk_paned_pack2(GTK_PANED(window.vpan1), todo.toplevel, FALSE, FALSE);
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
	
	if (continuous)
		g_signal_connect(G_OBJECT(scroll->adjust), "value-changed",
				 G_CALLBACK(myscroll_adjust_cb), scroll->draw);
	else
		g_signal_connect(G_OBJECT(scroll->scrollbar), "button-release-event",
				 G_CALLBACK(myscroll_button_cb), scroll->draw);
	
	g_signal_connect(G_OBJECT(scroll->draw), "configure-event",
			 G_CALLBACK(myscroll_size_cb), scroll);
	
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
