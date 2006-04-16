#ifndef __COLOR_SLIDER_H__
#define __COLOR_SLIDER_H__
/*
 * Copyright (C) 2006 Florian Boor <florian@kernelconcepts.de>
 *
 * Derived from SpColorSlider by Lauris Kaplinski <lauris@kaplinski.com>
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

#include <gtk/gtk.h>

typedef struct _ColorSlider ColorSlider;
typedef struct _ColorSliderClass ColorSliderClass;

#define TYPE_COLOR_SLIDER (color_slider_get_type ())
#define COLOR_SLIDER(o) (GTK_CHECK_CAST ((o), TYPE_COLOR_SLIDER, ColorSlider))
#define COLOR_SLIDER_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), TYPE_COLOR_SLIDER, ColorSliderClass))
#define IS_COLOR_SLIDER(o) (GTK_CHECK_TYPE ((o), TYPE_COLOR_SLIDER))
#define IS_COLOR_SLIDER_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), TYPE_COLOR_SLIDER))

struct _ColorSliderPrivate {
  gboolean dispose_has_run;
};

typedef struct _ColorSliderPrivate ColorSliderPrivate;

struct _ColorSlider {
	GtkWidget parent;

	guint dragging : 1;

	GtkAdjustment *adjustment;

	gfloat value;
	gfloat oldvalue;
	guchar c0[4], c1[4];
	guchar b0, b1;
	guchar bmask;

	gint mapsize;
	guchar *map;
    ColorSliderPrivate *priv;
};

struct _ColorSliderClass {
	GtkWidgetClass parent_class;

	void (* grabbed) (ColorSlider *slider);
	void (* dragged) (ColorSlider *slider);
	void (* released) (ColorSlider *slider);
	void (* changed) (ColorSlider *slider);
};

GtkType color_slider_get_type (void);

GtkWidget *color_slider_new (GtkAdjustment *adjustment);

void color_slider_set_adjustment (ColorSlider *slider, GtkAdjustment *adjustment);
void color_slider_set_colors (ColorSlider *slider, guint32 start, guint32 end);
void color_slider_set_map (ColorSlider *slider, const guchar *map);
void color_slider_set_background (ColorSlider *slider, guint dark, guint light, guint size);

#endif
