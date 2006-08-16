#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xmu/WinUtil.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gpe/spacing.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "bg.h"
#include "gtkbgbox.h"

#include "systray.h"
#include "eggtraymanager.h"
#include "fixedtip.h"

#include "dbg.h"

void 
tray_add_widget (tray *tr, GtkWidget *widget)
{
   gtk_box_pack_start (GTK_BOX (tr->startbox), widget, TRUE, TRUE, 0);
}

static void
tray_added (EggTrayManager *manager, GtkWidget *icon, void *data)
{
    tray *tr = (tray*)data;

    if (tr->pack_start)
      gtk_box_pack_start (GTK_BOX (tr->startbox), icon, FALSE, TRUE, 0);
    else
      gtk_box_pack_start (GTK_BOX (tr->endbox), icon, FALSE, TRUE, 0);
    
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
    gint x, y;
    
    gdk_window_get_origin (icon->window, &x, &y);
  
    fixed_tip_show (0, x, y, FALSE, gdk_screen_height () - 50, text);
}

static void
message_cancelled (EggTrayManager *manager, GtkWidget *icon, glong id,
                   void *data)
{
  fixed_tip_hide();
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
    GtkWidget *box;
    
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
    tr->pack_start = TRUE;

    box = p->panel->my_box_new(FALSE, 0);
    tr->startbox = p->panel->my_box_new(FALSE, gpe_get_boxspacing());
    tr->endbox = p->panel->my_box_new(FALSE, gpe_get_boxspacing());
    gtk_box_pack_start(GTK_BOX (box), tr->startbox, TRUE, TRUE, 0);
    gtk_box_pack_end (GTK_BOX (box), tr->endbox, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(p->pwid), box);
    
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
          G_CALLBACK (tray_added), tr);
    g_signal_connect (tr->tray_manager, "tray_icon_removed",
          G_CALLBACK (tray_removed), NULL);
    g_signal_connect (tr->tray_manager, "message_sent",
          G_CALLBACK (message_sent), NULL);
    g_signal_connect (tr->tray_manager, "message_cancelled",
          G_CALLBACK (message_cancelled), NULL);
    
    gtk_widget_show_all(box);
    RET(1);

}


plugin_class tray_plugin_class = {
    fname: NULL,
    count: 0,

    type : "tray",
    name : "tray",
    version: "1.2",
    description : "X Tray/Notification area",

    constructor : tray_constructor,
    destructor  : tray_destructor,
};
