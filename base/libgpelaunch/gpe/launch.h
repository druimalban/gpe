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
