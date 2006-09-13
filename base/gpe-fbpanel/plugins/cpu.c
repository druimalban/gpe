/*
 * CPU usage plugin to fbpanel
 *
 * Copyright (C) 2004 by Alexandre Pereira da Silva <alexandre.pereira@poli.usp.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 */
/*A little bug fixed by Mykola <mykola@2ka.mipt.ru>:) */


#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <stdlib.h>

#include "plugin.h"
#include "panel.h"
#include "gtkbgbox.h"


#define KILOBYTE 1024
#define MAX_WGSIZE 100

//#define DEBUG
#include "dbg.h"
typedef unsigned long tick;

struct cpu_stat {
    tick u, n, s, i;
};


typedef struct {
    GdkGC *gc_cpu;
    GdkColor *ccpu;
    GtkWidget *da;
    GtkWidget *evbox;
    GdkPixmap *pixmap;
    GtkTooltips *tip;

    int timer;
    tick *stats_cpu;
    unsigned int ini_stats;
    int Wwg;
    int Hwg;
    struct cpu_stat cpu_anterior;
} cpu_t;


static int
cpu_update(cpu_t *c)
{
    int cpu_u=0, cpu_s=0, cpu_n=0, cpu_i=100;
    unsigned int i;
    struct cpu_stat cpu, cpu_r;
    FILE *stat;
    float total;
    
    ENTER;
    if(!c->pixmap)
        RET(TRUE); 
     
    stat = fopen("/proc/stat", "r");
    if(!stat)
        RET(TRUE);
    fscanf(stat, "cpu %lu %lu %lu %lu", &cpu.u, &cpu.n, &cpu.s, &cpu.i);
    fclose(stat);

    cpu_r.u = cpu.u - c->cpu_anterior.u;
    cpu_r.n = cpu.n - c->cpu_anterior.n;
    cpu_r.s = cpu.s - c->cpu_anterior.s;
    cpu_r.i = cpu.i - c->cpu_anterior.i;

    total = cpu_r.u + cpu_r.n + cpu_r.s + cpu_r.i;
    cpu_u = cpu_r.u * c->Hwg / total;
    cpu_s = cpu_r.n * c->Hwg / total;
    cpu_n = cpu_r.s * c->Hwg / total;
    cpu_i = cpu_r.i * c->Hwg / total;

    c->cpu_anterior = cpu;
    
    c->stats_cpu[c->ini_stats++] = cpu_u + cpu_s + cpu_n;
    c->ini_stats %= c->Wwg;

    gdk_draw_rectangle(c->pixmap, c->da->style->black_gc, TRUE, 0, 0, c->Wwg, c->Hwg);
    for (i = 0; i < c->Wwg; i++) {
	int val;
	
	val = c->stats_cpu[(i + c->ini_stats) % c->Wwg];
        if (val)
            gdk_draw_line(c->pixmap, c->gc_cpu, i, c->Hwg, i, c->Hwg - val);
    }
    gtk_widget_queue_draw(c->da);
    RET(TRUE);
}

static gint
configure_event(GtkWidget *widget, GdkEventConfigure *event, cpu_t *c)
{
    ENTER;
    if (c->pixmap)
        g_object_unref(c->pixmap);
    c->Wwg = widget->allocation.width;
    c->Hwg = widget->allocation.height;
    if (c->stats_cpu)
        g_free(c->stats_cpu);
    c->stats_cpu = g_new0( typeof(*c->stats_cpu), c->Wwg);
    c->pixmap = gdk_pixmap_new (widget->window,
          widget->allocation.width,
          widget->allocation.height,
          -1);
    gdk_draw_rectangle (c->pixmap,
          widget->style->black_gc,
          TRUE,
          0, 0,
          widget->allocation.width,
          widget->allocation.height);
    
   RET(TRUE);
}


static gint
expose_event(GtkWidget *widget, GdkEventExpose *event, cpu_t *c)
{
    ENTER;
    gdk_draw_drawable (widget->window,
          c->da->style->black_gc,
          c->pixmap,
          event->area.x, event->area.y,
          event->area.x, event->area.y,
          event->area.width, event->area.height);
    
    RET(FALSE);
}

static int
cpu_constructor(plugin *p)
{
    cpu_t *c;

    ENTER;
    c = g_new0(cpu_t, 1);
    p->priv = c;

 
    c->da = gtk_drawing_area_new();
    gtk_widget_set_size_request(c->da, 40, 20);

    gtk_widget_show(c->da);

    c->tip = gtk_tooltips_new();
 
    c->gc_cpu = gdk_gc_new(p->panel->topgwin->window);
    DBG("here1\n");
    c->ccpu = (GdkColor *)malloc(sizeof(GdkColor));
    gdk_color_parse("green",  c->ccpu);
    gdk_colormap_alloc_color(gdk_drawable_get_colormap(p->panel->topgwin->window),  c->ccpu, FALSE, TRUE);
    gdk_gc_set_foreground(c->gc_cpu,  c->ccpu);
    gtk_bgbox_set_background(p->pwid, BG_STYLE, 0, 0);
    gtk_container_add(GTK_CONTAINER(p->pwid), c->da);
    gtk_container_set_border_width (GTK_CONTAINER (p->pwid), 1);
    g_signal_connect (G_OBJECT (c->da),"configure_event",
          G_CALLBACK (configure_event), (gpointer) c);
    g_signal_connect (G_OBJECT (c->da), "expose_event",
          G_CALLBACK (expose_event), (gpointer) c);
    
    c->timer = g_timeout_add(1000, (GSourceFunc) cpu_update, (gpointer) c);
    RET(1);
}

static void
cpu_destructor(plugin *p)
{
    cpu_t *c = (cpu_t *) p->priv;

    ENTER;
    g_object_unref(c->pixmap);
    g_object_unref(c->gc_cpu);
    g_free(c->stats_cpu);
    g_free(c->ccpu);
    g_source_remove(c->timer);
    g_free(p->priv);
    RET();
}


plugin_class cpu_plugin_class = {
    fname: NULL,
    count: 0,

    type : "cpu",
    name : "Cpu usage",
    version: "1.0",
    description : "Display cpu usage",

    constructor : cpu_constructor,
    destructor  : cpu_destructor,
};
