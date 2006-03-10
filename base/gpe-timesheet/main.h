/* Variable declaration for main.c*/

#ifdef IS_HILDON
  static gboolean key_press_cb (GtkWidget *w, GdkEventKey *event, HildonApp *app);
  static void osso_top_callback (const gchar* arguments, gpointer ptr);
#endif
