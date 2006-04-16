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

#ifndef __GPE_COLOR_DIALOG_H__
#define __GPE_COLOR_DIALOG_H__

#include <gtk/gtkdialog.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GPE_TYPE_COLOR_DIALOG                  (gpe_color_dialog_get_type ())
#define GPE_COLOR_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPE_TYPE_COLOR_DIALOG, GpeColorDialog))
#define GPE_COLOR_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GPE_TYPE_COLOR_DIALOG, GpeColorDialogClass))
#define GPE_IS_COLOR_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPE_TYPE_COLOR_DIALOG))
#define GPE_IS_COLOR_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GPE_TYPE_COLOR_DIALOG))
#define GPE_COLOR_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GPE_TYPE_COLOR_DIALOG, GpeColorDialogClass))

#define COL_STR_LEN  8
#define COLOR_F_TO_U(v) ((unsigned int) ((v) * 255.9999))
#define RGBA32_U_COMPOSE(r,g,b,a) ((((r) & 0xff) << 24) | (((g) & 0xff) << 16) | (((b) & 0xff) << 8) | ((a) & 0xff))
#define RGBA32_F_COMPOSE(r,g,b,a) RGBA32_U_COMPOSE (COLOR_F_TO_U (r), COLOR_F_TO_U (g), COLOR_F_TO_U (b), COLOR_F_TO_U (a))
    
typedef struct _GpeColorDialog        GpeColorDialog;
typedef struct _GpeColorDialogClass   GpeColorDialogClass;

struct _GpeColorDialog
{
  /*< private >*/
  
  GtkDialog parent_instance;
  
  GtkWidget *colorbox;
  GtkWidget *previewbutton;
  gchar cur_color_str[COL_STR_LEN];
  GdkColor cur_color_gdk;
};

struct _GpeColorDialogClass
{
  GtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
};

GType      gpe_color_dialog_get_type (void) G_GNUC_CONST;

GtkWidget* gpe_color_dialog_new   (GtkWindow *parent, GtkDialogFlags flags, const gchar *initcolor);

G_CONST_RETURN gchar *gpe_color_dialog_get_color_str (GpeColorDialog *color_dialog);
void gpe_color_dialog_set_color_str (GpeColorDialog *color_dialog, const gchar *colordesc);
G_CONST_RETURN GdkColor* gpe_color_dialog_get_color_gdk (GpeColorDialog *color_dialog);
void gpe_color_dialog_set_color_gdk (GpeColorDialog *color_dialog, const GdkColor *new_color);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GPE_COLOR_DIALOG_H__ */
