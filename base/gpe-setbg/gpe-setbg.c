#include <stdio.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gpe/pixmaps.h>

static struct gpe_icon my_icons[] = {
  { "default_bg", PREFIX "/share/pixmaps/gpe-login-bg.png" },
  { NULL, NULL }
};

int 
main (int argc, char *argv[])
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  
  gdk_init (&argc, &argv);

  if (gpe_load_icons (my_icons) == FALSE) exit (1);
  if (gpe_find_icon_pixmap ("default_bg", &pixmap, &bitmap)) {
    gdk_window_set_back_pixmap (
				(GdkWindow *) GDK_ROOT_WINDOW (),
				//(GdkWindow *) RootWindow (GDK_DISPLAY (), DefaultScreen (GDK_DISPLAY ())),
				pixmap,
				0);
  }

  return (0);

}
