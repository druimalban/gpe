#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <X11/Xmu/WinUtil.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gpe/spacing.h>
#include <gpe/popup.h>
#include <gpe/launch.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "bg.h"
#include "gtkbgbox.h"

#include "systray.h"
#include "eggtraymanager.h"
#include "fixedtip.h"

#include "dbg.h"

#define _(x) gettext(x)

void 
tray_add_widget (tray *tr, GtkWidget *widget)
{
   gtk_box_pack_start (GTK_BOX (tr->box), widget, TRUE, TRUE, 0);
}

static void
tray_added (EggTrayManager *manager, GtkWidget *icon, void *data)
{
    tray *tr = (tray*)data;

    if (tr->pack_start)
      gtk_box_pack_start (GTK_BOX (tr->box), icon, FALSE, TRUE, 0);
    else
      gtk_box_pack_start (GTK_BOX (tr->box), icon, FALSE, TRUE, 0);
    
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
save_tray_session (GtkWidget *dock)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (dock));
  GList *iter;
	
  for (iter = children; iter; iter=iter->next)
    {
    }
}

static void
tray_destructor(plugin *p)
{
    tray *tr = (tray *)p->priv;
    
    ENTER;
    save_tray_session (tr->box);
    /* Make sure we drop the manager selection */
    if (tr->tray_manager)
        g_object_unref (G_OBJECT (tr->tray_manager));
    fixed_tip_hide ();
    g_free(tr);
    RET();
}

static gboolean
tray_button_press (GtkWidget *box, GdkEventButton *b, gpointer user_data)
{
  tray *tr = user_data;
    
  if (b->button == 1)
    {
       gtk_menu_popup (GTK_MENU (tr->context_menu),
                       NULL, NULL, gpe_popup_menu_position, box, 1, b->time);
       return TRUE;
    }

  return FALSE;
}

static int
tray_constructor(plugin *p)
{
    line s;
    tray *tr;
    GdkScreen *screen;
    GtkWidget *box, *item, *tray_apps_menu;
    
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
    tr->box = p->panel->my_box_new(FALSE, gpe_get_boxspacing());
    gtk_box_pack_start(GTK_BOX (box), tr->box, TRUE, TRUE, 0);
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
    
    /* context menu */
    tr->context_menu = gtk_menu_new ();
    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, NULL);
    gtk_menu_attach (GTK_MENU (tr->context_menu), item, 0, 1, 0, 1);
    
    tray_apps_menu = g_object_get_data (G_OBJECT (p->pwid->parent), 
                                        "tray-apps-menu");
    if (tray_apps_menu)
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), tray_apps_menu);
    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
    gtk_menu_attach (GTK_MENU (tr->context_menu), item, 0, 1, 1, 2);
    item = gtk_separator_menu_item_new ();
    gtk_menu_attach (GTK_MENU (tr->context_menu), item, 0, 1, 2, 3);
    item = gtk_image_menu_item_new_with_label (_("Hide"));
    gtk_menu_attach (GTK_MENU (tr->context_menu), item, 0, 1, 3, 4);
    
    gtk_menu_attach_to_widget (GTK_MENU(tr->context_menu), tr->box, NULL);
    g_signal_connect (tr->box, "button-press-event", 
          G_CALLBACK (tray_button_press), tr);
    
    gtk_widget_show_all (tr->context_menu);

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
