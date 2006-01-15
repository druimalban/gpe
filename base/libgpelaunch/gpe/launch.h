#ifndef GPE_LAUNCH_H
#define GPE_LAUNCH_H

#include <glib.h>

enum launch_status
  {
    LAUNCH_STARTING,
    LAUNCH_COMPLETE,
    LAUNCH_FAILED
  };

typedef void (*launch_callback_func) (enum launch_status, void *data);
extern gboolean gpe_launch_program_with_callback (Display *dpy, char *exec, char *name, gboolean startup_notify, launch_callback_func cb, void *data);
extern gboolean gpe_launch_program (Display *dpy, char *exec, char *name);
extern void gpe_launch_install_filter (void);
extern void gpe_launch_set_root (const gchar *root);

extern gboolean gpe_launch_startup_is_pending (Display *dpy, const gchar *binary);
extern Window gpe_launch_get_window_for_binary (Display *dpy, const gchar *binary);
extern gchar *gpe_launch_get_binary_for_window (Display *dpy, Window w);
extern void gpe_launch_monitor_display (Display *dpy);

extern void gpe_launch_activate_window (Display *dpy, Window win);
extern int gpe_launch_process_event (XEvent *xev);

#endif
