#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xmu/WinUtil.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "bg.h"
#include "gtkbgbox.h"


#include "eggtraymanager.h"
#include "fixedtip.h"


//#define DEBUG
#include "dbg.h"


typedef struct {
    GtkWidget *mainw;
    plugin *plug;
    GtkWidget *box;
    /////
    EggTrayManager *tray_manager;
    
} tray;

//static void run_gtktray(tray *tr);



static void
tray_added (EggTrayManager *manager, GtkWidget *icon, void *data)
{
    GtkWidget *box = (GtkWidget *)data;

    gtk_box_pack_end (GTK_BOX (box), icon, FALSE, FALSE, 0);
    gtk_widget_show (icon);
}

static void
tray_removed (EggTrayManager *manager, GtkWidget *icon, void *data)
{

}

static void
message_sent (EggTrayManager *manager, GtkWidget *icon, const char *text, glong id, glong timeout,
              void *data)
{
    /* FIXME multihead */
    int x, y;
    
    gdk_window_get_origin (icon->window, &x, &y);
  
    fixed_tip_show (0, x, y, FALSE, gdk_screen_height () - 50, text);
}

static void
message_cancelled (EggTrayManager *manager, GtkWidget *icon, glong id,
                   void *data)
{
  
}



static void
tray_destructor(plugin *p)
{
    tray *tr = (tray *)p->priv;

    ENTER;
    /* Make sure we drop the manager selection */
    if (tr->tray_manager)
        g_object_unref (G_OBJECT (tr->tray_manager));
    fixed_tip_hide ();
    g_free(tr);
    RET();
}

    


static int
tray_constructor(plugin *p)
{
    line s;
    tray *tr;
    GdkScreen *screen;
    GtkWidget *frame;
    
    ENTER;
    s.len = 256;
    while (get_line(p->fp, &s) != LINE_BLOCK_END) {
        ERR("tray: illegal in this context %s\n", s.str);
        RET(0);
    }

    
    tr = g_new0(tray, 1);
    g_return_val_if_fail(tr != NULL, 0);
    p->priv = tr;
    tr->plug = p;

    frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(p->pwid), frame);    
    tr->box = p->panel->my_box_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(frame), tr->box);
    
    gtk_bgbox_set_background(p->pwid, BG_STYLE, 0, 0);
    gtk_container_set_border_width(GTK_CONTAINER(p->pwid), 0);
    screen = gtk_widget_get_screen (GTK_WIDGET (p->panel->topgwin));
    
    if (egg_tray_manager_check_running(screen)) {
        tr->tray_manager = NULL;
        ERR("tray: another systray already running\n");
        RET(1);
    }
    tr->tray_manager = egg_tray_manager_new ();
    if (!egg_tray_manager_manage_screen (tr->tray_manager, screen))
        g_printerr ("tray: System tray didn't get the system tray manager selection\n");
    
    g_signal_connect (tr->tray_manager, "tray_icon_added",
          G_CALLBACK (tray_added), tr->box);
    g_signal_connect (tr->tray_manager, "tray_icon_removed",
          G_CALLBACK (tray_removed), tr->box);
    g_signal_connect (tr->tray_manager, "message_sent",
          G_CALLBACK (message_sent), tr->box);
    g_signal_connect (tr->tray_manager, "message_cancelled",
          G_CALLBACK (message_cancelled), tr->box);
    
    gtk_widget_show_all(frame);
    RET(1);

}


plugin_class tray_plugin_class = {
    fname: NULL,
    count: 0,

    type : "tray",
    name : "tray",
    version: "1.0",
    description : "Old KDE/GNOME Tray",

    constructor : tray_constructor,
    destructor  : tray_destructor,
};
