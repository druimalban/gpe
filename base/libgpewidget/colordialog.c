/*
 * Copyright (C) 2006 Florian Boor <florian@kernelconcepts.de>
 *
 * Derived from GtkMessageDialog
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* TODO:
   - Escape key binding.
 */
#include <string.h>
#include <libintl.h>
#include <gtk/gtk.h>

#include "gpe/color-slider.h"
#include "gpe/spacing.h"
#include "gpe/colordialog.h"

#define _(x) gettext(x)

#define GPE_COLOR_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GPE_TYPE_COLOR_DIALOG, GpeColorDialogPrivate))

typedef struct _GpeColorDialogPrivate GpeColorDialogPrivate;

struct _GpeColorDialogPrivate
{
  ColorSlider *sliders[3];
  GtkWidget   *previewbutton;
};

static void gpe_color_dialog_class_init (GpeColorDialogClass *klass);
static void gpe_color_dialog_init       (GpeColorDialog      *dialog);

static void gpe_color_dialog_set_property (GObject *object, guint prop_id,
                                           const GValue *value, GParamSpec *pspec);

static void gpe_color_dialog_get_property (GObject *object, guint prop_id,
                                           GValue *value, GParamSpec *pspec);

enum {
  PROP_0,
  PROP_COLOR_STR,
  PROP_COLOR_GDK
};

enum {
  SLIDER_RED,
  SLIDER_GREEN,
  SLIDER_BLUE
};
static gpointer parent_class;

GType
gpe_color_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (!dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GpeColorDialogClass),
        NULL,		/* base_init */
        NULL,		/* base_finalize */
        (GClassInitFunc) gpe_color_dialog_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        sizeof (GpeColorDialog),
        0,		/* n_preallocs */
        (GInstanceInitFunc) gpe_color_dialog_init,
      };

      dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "GpeColorDialog",
                                            &dialog_info, 0);
    }

  return dialog_type;
}

static void
gpe_color_dialog_class_init (GpeColorDialogClass *class)
{
  GtkWidgetClass *widget_class;
  GObjectClass *gobject_class;

  widget_class = GTK_WIDGET_CLASS (class);
  gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = gpe_color_dialog_set_property;
  gobject_class->get_property = gpe_color_dialog_get_property;
  
  /**
   * GpeColorDialog::color_str
   *
   * Currently selected colour as string description.
   *
   * Since: 0.111
   */
  g_object_class_install_property (gobject_class, PROP_COLOR_STR,
                                   g_param_spec_string ("color_str",
                                     _("Selected colour"),
                                     _("Selected colour value in the dialog."),
                                     "#FFFFFF",
                                     G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT ));
  /**
   * GpeColorDialog::color_gdk
   *
   * Currently selected colour as pointer to a #GdkColor.
   *
   * Since: 0.111
   */
  g_object_class_install_property (gobject_class, PROP_COLOR_GDK,
                                   g_param_spec_pointer ("color_gdk",
                                     _("Selected colour"),
                                     _("Selected colour value in the dialog."),
                                     G_PARAM_READABLE | G_PARAM_WRITABLE));
                                 
  g_type_class_add_private (gobject_class, sizeof (GpeColorDialogPrivate));
}

void
set_widget_color_str (GtkWidget *widget, const gchar *colstr)
{
  GdkColormap *map;
  static GdkColor colour;
    
  gdk_color_parse (colstr, &colour);
  map = gdk_colormap_get_system ();
  gdk_colormap_alloc_color (map, &colour, FALSE, TRUE);
  gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, &colour);
  gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, &colour);
}

void
set_widget_color_gdk (GtkWidget *widget, GdkColor color)
{
  gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, &color);
  gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, &color);
}

static void
color_selector_update_sliders (ColorSlider *slider, gpointer data)
{
  GpeColorDialog *dialog = data;
  GpeColorDialogPrivate *priv;
  static GdkColor new_color;
  static gint channel = SLIDER_RED;
  static gint r,g,b;
  
  priv = GPE_COLOR_DIALOG_GET_PRIVATE (dialog);
  
  if (slider == priv->sliders[SLIDER_RED])
    channel = SLIDER_RED;
  else if (slider == priv->sliders[SLIDER_GREEN])
    channel = SLIDER_GREEN;
  else
    channel = SLIDER_BLUE;
  
  if (channel != SLIDER_RED)
    {
      /* Update red values */
      color_slider_set_colors (COLOR_SLIDER (priv->sliders[SLIDER_RED]),
				  RGBA32_F_COMPOSE (0.0, priv->sliders[SLIDER_GREEN]->adjustment->value,
						       priv->sliders[SLIDER_BLUE]->adjustment->value, 1.0),
				  RGBA32_F_COMPOSE (1.0, priv->sliders[SLIDER_GREEN]->adjustment->value,
						       priv->sliders[SLIDER_BLUE]->adjustment->value, 1.0));
    }
  if (channel != SLIDER_GREEN)
    {
      /* Update green values */
      color_slider_set_colors (COLOR_SLIDER (priv->sliders[SLIDER_GREEN]),
				  RGBA32_F_COMPOSE (priv->sliders[SLIDER_RED]->adjustment->value, 0.0,
						       priv->sliders[SLIDER_BLUE]->adjustment->value, 1.0),
				  RGBA32_F_COMPOSE (priv->sliders[SLIDER_RED]->adjustment->value, 1.0,
						       priv->sliders[SLIDER_BLUE]->adjustment->value, 1.0));
    }
  if (channel != SLIDER_BLUE)
    {
      /* Update blue values */
      color_slider_set_colors (COLOR_SLIDER (priv->sliders[SLIDER_BLUE]),
				  RGBA32_F_COMPOSE (priv->sliders[SLIDER_RED]->adjustment->value,
						       priv->sliders[SLIDER_GREEN]->adjustment->value, 0.0, 1.0),
				  RGBA32_F_COMPOSE (priv->sliders[SLIDER_RED]->adjustment->value,
						       priv->sliders[SLIDER_GREEN]->adjustment->value, 1.0, 1.0));
    }
	
  r = (gint)(priv->sliders[0]->adjustment->value * 255.0);
  g = (gint)(priv->sliders[1]->adjustment->value * 255.0);
  b = (gint)(priv->sliders[2]->adjustment->value * 255.0);

  snprintf (dialog->cur_color_str, COL_STR_LEN, "#%02x%02x%02x", r, g, b);
  new_color.pixel = 0;
  new_color.red   = r * 256;
  new_color.green = g * 256;
  new_color.blue  = b * 256;
  if (gdk_colormap_alloc_color (gdk_colormap_get_system(), &new_color, FALSE, TRUE))
    {
      dialog->cur_color_gdk = new_color;
      set_widget_color_gdk (priv->previewbutton, new_color);
    }
  else
    {
      g_printerr ("Warning: unable to allocate colour.\n");
    }
}

static GtkWidget *
build_colorbox (GpeColorDialog *dialog, GpeColorDialogPrivate *priv)
{
  GtkWidget *label, *button, *slider, *preview;
  GtkWidget *colorbox;
  guint gpe_border = gpe_get_border ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  gchar *tstr = NULL;
  gint i;

  colorbox = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (colorbox), gpe_border);
  gtk_table_set_row_spacings (GTK_TABLE (colorbox), gpe_boxspacing);
  gtk_table_set_col_spacings (GTK_TABLE (colorbox), gpe_boxspacing);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
  
  preview = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (button), preview);
  priv->previewbutton = preview;
  gtk_widget_set_size_request(button, 30, 30 + 6 * gpe_boxspacing);
  gtk_table_attach (GTK_TABLE (colorbox), button, 2, 3, 0, 3,
			        GTK_FILL, GTK_FILL, 0, 0);

  for (i = 0; i < 3; i++)
    {
      label = gtk_label_new (NULL);
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      switch (i)
        {
        case 0:
          tstr = g_strdup_printf ("<b>%s</b>", _("Red"));
          break;
        case 1:
          tstr = g_strdup_printf ("<b>%s</b>", _("Green"));
          break;
        case 2:
          tstr = g_strdup_printf ("<b>%s</b>", _("Blue"));
          break;
        }
      gtk_label_set_markup (GTK_LABEL (label), tstr);
      g_free (tstr);
      gtk_table_attach (GTK_TABLE (colorbox), label, 0, 1, i, i + 1,
                        GTK_FILL, GTK_FILL, 0, 0);
      slider = color_slider_new (NULL);
      priv->sliders[i] = COLOR_SLIDER (slider);
      g_signal_connect (G_OBJECT (priv->sliders[i]), "changed", 
                        G_CALLBACK (color_selector_update_sliders), dialog);

      color_slider_set_map (COLOR_SLIDER (slider), NULL);
      color_slider_set_colors (COLOR_SLIDER (slider), 1.0, 65534.0);
      gtk_table_attach (GTK_TABLE (colorbox), slider, 1, 2, i, i + 1,
                        GTK_FILL, GTK_FILL, 0, 0);
    }
    
  return colorbox;
}

static void
gpe_color_dialog_init (GpeColorDialog *dialog)
{
  GtkWidget *colorbox;
  GpeColorDialogPrivate *priv;

  priv = GPE_COLOR_DIALOG_GET_PRIVATE (dialog);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    
  colorbox = build_colorbox (dialog, priv);
  gtk_widget_show_all (colorbox);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), colorbox,
                      FALSE, FALSE, 0);
}

/**
 * gpe_color_dialog_get_color_str:
 * @dialog: a #GpeColorDialog
 *
 * Get a pointer to the string GRB representation of the currently selected 
 * colour. The description has the form #rrggbb.
 *
 * Since: 0.111
 **/
G_CONST_RETURN gchar *
gpe_color_dialog_get_color_str (GpeColorDialog *color_dialog)
{
  g_return_val_if_fail (GPE_IS_COLOR_DIALOG (color_dialog), NULL);
    
  return color_dialog->cur_color_str;
}

/**
 * gpe_color_dialog_set_color_str:
 * @color_dialog: a #GpeColorDialog
 * @new_color: new colour
 *
 * Set the selection dialog to the given colour defined by a string value.
 * This may be any string description XParseColor is able to interpret. e.g. 
 * HTML-like values #rrggbb or any definition from rgb.txt.
 *
 * Since: 0.111
 **/
void 
gpe_color_dialog_set_color_str(GpeColorDialog *color_dialog, 
                               const gchar *colordesc)
{
  GpeColorDialogPrivate *priv;
  GdkColor new_color;

  priv = GPE_COLOR_DIALOG_GET_PRIVATE (color_dialog);
  
  gdk_color_parse (colordesc, &new_color);
  
  gtk_adjustment_set_value(priv->sliders[SLIDER_RED]->adjustment,   (gdouble)new_color.red / 65536.0); 
  gtk_adjustment_set_value(priv->sliders[SLIDER_GREEN]->adjustment, (gdouble)new_color.green / 65536.0); 
  gtk_adjustment_set_value(priv->sliders[SLIDER_BLUE]->adjustment,  (gdouble)new_color.blue / 65536.0); 
  color_selector_update_sliders (priv->sliders[SLIDER_RED], color_dialog);
  color_selector_update_sliders (priv->sliders[SLIDER_GREEN], color_dialog);
}

/**
 * gpe_color_dialog_get_color_gdk:
 * @color_dialog: a #GpeColorDialog
 *
 * Get a pointer to the currently selected and allocated colour #GdkColor value.
 * The value must not be altered or freed.
 *
 * Since: 0.111
 *
 * Returns: A pointer to a #GdkColor struct.
 **/
G_CONST_RETURN GdkColor*
gpe_color_dialog_get_color_gdk (GpeColorDialog *color_dialog)
{
  g_return_val_if_fail (GPE_IS_COLOR_DIALOG (color_dialog), NULL);
    
  return &color_dialog->cur_color_gdk;
}

/**
 * gpe_color_dialog_set_color_gdk:
 * @color_dialog: a #GpeColorDialog
 * @new_color: new colour
 *
 * Set the selection dialog to the given colour defined by a #GdkColor value.
 *
 * Since: 0.111
 **/
void 
gpe_color_dialog_set_color_gdk (GpeColorDialog *color_dialog, 
                                const GdkColor *new_color)
{
  GpeColorDialogPrivate *priv;
  
  g_return_if_fail (new_color != NULL);
    
  priv = GPE_COLOR_DIALOG_GET_PRIVATE (color_dialog);
  
  gtk_adjustment_set_value(priv->sliders[SLIDER_RED]->adjustment,   (gdouble)new_color->red / 65536.0); 
  gtk_adjustment_set_value(priv->sliders[SLIDER_GREEN]->adjustment, (gdouble)new_color->green / 65536.0); 
  gtk_adjustment_set_value(priv->sliders[SLIDER_BLUE]->adjustment,  (gdouble)new_color->blue / 65536.0); 
  color_selector_update_sliders (priv->sliders[SLIDER_RED], color_dialog);
  color_selector_update_sliders (priv->sliders[SLIDER_GREEN], color_dialog);
}

static void 
gpe_color_dialog_set_property (GObject *object, guint prop_id,
                               const GValue *value, GParamSpec *pspec)
{
  GpeColorDialog *dialog;
  
  dialog = GPE_COLOR_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_COLOR_STR:
      gpe_color_dialog_set_color_str (dialog, g_value_get_string (value));
      break;
    case PROP_COLOR_GDK:
      gpe_color_dialog_set_color_gdk (dialog, g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
gpe_color_dialog_get_property (GObject *object, guint prop_id,
                               GValue  *value,  GParamSpec *pspec)
{
  GpeColorDialog *dialog;
  
  dialog = GPE_COLOR_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_COLOR_STR:
      g_value_set_string (value, gpe_color_dialog_get_color_str (dialog));
      break;
    case PROP_COLOR_GDK:
      g_value_set_pointer (value, gpe_color_dialog_get_color_gdk (dialog));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/**
 * gpe_color_dialog_new:
 * @parent: transient parent, or NULL for none 
 * @initcolor: initial colour
 *
 * Creates a small colour selection dialog suitable for use on small 
 * screens. 
 * When the user clicks a button a "response"
 * signal is emitted with response IDs from #GtkResponseType. See
 * #GtkDialog for more details.
 *
 * Since: 0.111
 *
 * Return value: a new #GpeColorDialog
 **/
GtkWidget* 
gpe_color_dialog_new (GtkWindow *parent, GtkDialogFlags flags, const gchar *initcolor)
{
  GtkWidget *widget;
  GtkDialog *dialog;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

  widget = g_object_new (GPE_TYPE_COLOR_DIALOG, NULL);
  dialog = GTK_DIALOG (widget);

  if (parent != NULL)
    gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (parent));
  
  if (flags & GTK_DIALOG_MODAL)
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  
  if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  if (flags & GTK_DIALOG_NO_SEPARATOR)
    gtk_dialog_set_has_separator (dialog, FALSE);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Select colour"));
  gpe_color_dialog_set_color_str (GPE_COLOR_DIALOG (dialog), initcolor);
  
  return widget;
}
