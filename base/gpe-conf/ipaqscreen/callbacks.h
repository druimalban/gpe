typedef struct
{
  GtkWidget *table;
  GtkWidget *hbox2;
  GtkWidget *lightl, *lightstl;
  GtkWidget *brightnessl;
  GtkWidget *brightness;
  GtkWidget *screensaverl;
  GtkWidget *screensaverl2;
  GtkWidget *screensaver;
  GtkWidget *screensaverbt;
  GtkWidget *rotation;
  GtkWidget *rotationl;
  GtkWidget *touchscreen;
  GtkWidget *calibrate;
  GtkWidget *rbLightswitch1, *rbLightswitch2;
}tself;

void
on_brightness_hscale_draw              (GtkObject       *adjustment,
                                        gpointer         user_data);
void
on_screensaver_hscale_draw              (GtkWidget       *adjustment,
                                        gpointer         user_data);
void
on_screensaver_button_clicked              (GtkWidget       *widget,
                                        GdkRectangle    *area,
                                        gpointer         user_data);

void
on_rotation_entry_changed              (GtkWidget     *editable,
                                        gpointer         user_data);

void
on_calibrate_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

gint on_light_check(gpointer adj);

void on_light_on (GtkWidget *sender, gpointer user_data);
