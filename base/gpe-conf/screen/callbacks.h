typedef struct
{
  GtkWidget *table;
  GtkWidget *hbox2;
  GtkWidget *lightl;
  GtkWidget *brightnessl;
  GtkWidget *brightness;
  GtkWidget *displayl;
  GtkWidget *calibratel;
/*  GtkWidget *screensaverl;
  GtkWidget *screensaverl2;
  GtkWidget *screensaverl3;
  GtkWidget *screensaver;
  GtkWidget *screensaverbt1;
  GtkWidget *screensaverbt2;
*/	
#ifndef DISABLE_XRANDR
  GtkWidget *rotation;
  GtkWidget *rotationl;
#endif	
  GtkWidget *touchscreen;
  GtkWidget *calibrate;
  GtkObject* adjSaver;
}tself;

void
on_brightness_hscale_draw              (GtkObject       *adjustment,
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
