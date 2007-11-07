/* Variable declaration for main.c*/

#ifdef IS_HILDON
#if HILDON_VER > 0
    static gboolean key_press_cb(GtkWidget *w, GdkEventKey *event, GtkWindow *window);
#else
    static gboolean key_press_cb (GtkWidget *w, GdkEventKey *event, HildonApp *app);
#endif /* HILDON_VER */
  static void osso_top_callback (const gchar* arguments, gpointer ptr);
#endif /* IS_HILDON */
