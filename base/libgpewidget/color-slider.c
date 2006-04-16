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

#include <gdk/gdk.h>
#include <gdk/gdkrgb.h>
#include <gtk/gtk.h>
#include "gpe/color-slider.h"

#define SLIDER_WIDTH 96
#define SLIDER_HEIGHT 8
#define ARROW_SIZE 8

enum {GRABBED, DRAGGED, RELEASED, CHANGED, LAST_SIGNAL};

static void color_slider_class_init (ColorSliderClass *klass);
static void color_slider_init (ColorSlider *slider);
static void color_slider_dispose (GObject *object);

static void color_slider_realize (GtkWidget *widget);
static void color_slider_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void color_slider_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

static gint color_slider_expose (GtkWidget *widget, GdkEventExpose *event);
static gint color_slider_button_press (GtkWidget *widget, GdkEventButton *event);
static gint color_slider_button_release (GtkWidget *widget, GdkEventButton *event);
static gint color_slider_motion_notify (GtkWidget *widget, GdkEventMotion *event);

static void color_slider_adjustment_changed (GtkAdjustment *adjustment, ColorSlider *slider);
static void color_slider_adjustment_value_changed (GtkAdjustment *adjustment, ColorSlider *slider);

static void color_slider_paint (ColorSlider *slider, GdkRectangle *area);
static const guchar *color_slider_render_gradient (gint x0, gint y0, gint width, gint height,
						      gint c[], gint dc[], guint b0, guint b1, guint mask);
static const guchar *color_slider_render_map (gint x0, gint y0, gint width, gint height,
						 guchar *map, gint start, gint step, guint b0, guint b1, guint mask);

static GtkWidgetClass *parent_class;
static guint slider_signals[LAST_SIGNAL] = {0};

GType
color_slider_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (ColorSliderClass),
			NULL,
            NULL,
            (GClassInitFunc) color_slider_class_init,
            NULL,
            NULL,
			sizeof (ColorSlider),
            0,
			(GInstanceInitFunc) color_slider_init
		};
		type = g_type_register_static (GTK_TYPE_WIDGET, "ColorSlider", &info, 0);
	}
	return type;
}

static void
color_slider_class_init (ColorSliderClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
    g_type_class_add_private (klass, sizeof (ColorSliderPrivate));

	slider_signals[GRABBED] = g_signal_new ("grabbed",
                          G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
						  offsetof (ColorSliderClass, grabbed),
                          NULL, NULL,
						  g_cclosure_marshal_VOID__VOID,
						  G_TYPE_NONE, 0, NULL);
	slider_signals[DRAGGED] = g_signal_new ("dragged",
                          G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
						  offsetof (ColorSliderClass, dragged),
                          NULL, NULL,
						  g_cclosure_marshal_VOID__VOID,
						  G_TYPE_NONE, 0, NULL);
	slider_signals[RELEASED] = g_signal_new ("released",
                          G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
						  offsetof (ColorSliderClass, released),
                          NULL, NULL,
						  g_cclosure_marshal_VOID__VOID,
						  G_TYPE_NONE, 0, NULL);
	slider_signals[CHANGED] = g_signal_new ("changed",
                          G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
						  offsetof (ColorSliderClass, changed),
                          NULL, NULL,
						  g_cclosure_marshal_VOID__VOID,
						  G_TYPE_NONE, 0, NULL);

	object_class->dispose = color_slider_dispose;

	widget_class->realize = color_slider_realize;
	widget_class->size_request = color_slider_size_request;
	widget_class->size_allocate = color_slider_size_allocate;

	widget_class->expose_event = color_slider_expose;
	widget_class->button_press_event = color_slider_button_press;
	widget_class->button_release_event = color_slider_button_release;
	widget_class->motion_notify_event = color_slider_motion_notify;
}

static void
color_slider_init (ColorSlider *slider)
{
	/* We are widget with window */
	GTK_WIDGET_UNSET_FLAGS (slider, GTK_NO_WINDOW);
    slider->priv = G_TYPE_INSTANCE_GET_PRIVATE (slider, TYPE_COLOR_SLIDER, ColorSliderPrivate);
    
	slider->dragging = FALSE;

	slider->adjustment = NULL;
	slider->value = 0.0;

	slider->c0[0] = 0x00;
	slider->c0[1] = 0x00;
	slider->c0[2] = 0x00;
	slider->c0[3] = 0xff;
	slider->c1[0] = 0xff;
	slider->c1[1] = 0xff;
	slider->c1[2] = 0xff;
	slider->c1[3] = 0xff;

	slider->b0 = 0x5f;
	slider->b1 = 0xa0;
	slider->bmask = 0x08;

	slider->map = NULL;
}

static void
color_slider_dispose (GObject *object)
{
	ColorSlider *slider;

	slider = COLOR_SLIDER (object);
    if (slider->priv->dispose_has_run) {
     /* If dispose did already run, return. */
      return;
    }
    slider->priv->dispose_has_run = TRUE;
    
    if (slider->adjustment) {
        g_signal_handlers_disconnect_by_func (G_OBJECT (slider->adjustment), 
                                              color_slider_adjustment_changed, slider);
        g_object_unref (G_OBJECT (slider->adjustment));
        slider->adjustment = NULL;
    }
   /* Chain up to the parent class */
   G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
color_slider_realize (GtkWidget *widget)
{
	ColorSlider *slider;
	GdkWindowAttr attributes;
	gint attributes_mask;

	slider = COLOR_SLIDER (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gdk_rgb_get_visual ();
	attributes.colormap = gdk_rgb_get_cmap ();
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= (GDK_EXPOSURE_MASK |
				  GDK_BUTTON_PRESS_MASK |
				  GDK_BUTTON_RELEASE_MASK |
				  GDK_POINTER_MOTION_MASK |
				  GDK_ENTER_NOTIFY_MASK |
				  GDK_LEAVE_NOTIFY_MASK);
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, widget);

	widget->style = gtk_style_attach (widget->style, widget->window);
}

static void
color_slider_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	ColorSlider *slider;

	slider = COLOR_SLIDER (widget);

	requisition->width = SLIDER_WIDTH + widget->style->xthickness * 2;
	requisition->height = SLIDER_HEIGHT + widget->style->ythickness * 2;
}

static void
color_slider_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	ColorSlider *slider;

	slider = COLOR_SLIDER (widget);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (widget)) {
		/* Resize GdkWindow */
		gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
	}
}

static gint
color_slider_expose (GtkWidget *widget, GdkEventExpose *event)
{
	ColorSlider *slider;

	slider = COLOR_SLIDER (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		gint width, height;
		width = widget->allocation.width;
		height = widget->allocation.height;
		color_slider_paint (slider, &event->area);
	}

	return FALSE;
}

static gint
color_slider_button_press (GtkWidget *widget, GdkEventButton *event)
{
	ColorSlider *slider;

	slider = COLOR_SLIDER (widget);

	if (event->button == 1) {
		gint cx, cw;
		cx = widget->style->xthickness;
		cw = widget->allocation.width - 2 * cx;
		g_signal_emit (G_OBJECT (slider), slider_signals[GRABBED], 0);
		slider->dragging = TRUE;
		slider->oldvalue = slider->value;
		gtk_adjustment_set_value (slider->adjustment, CLAMP ((gfloat) (event->x - cx) / cw, 0.0, 1.0));
		g_signal_emit (G_OBJECT (slider), slider_signals[DRAGGED], 0);
		gdk_pointer_grab (widget->window, FALSE,
				  GDK_POINTER_MOTION_MASK |
				  GDK_BUTTON_RELEASE_MASK,
				  NULL, NULL, event->time);
	}

	return FALSE;
}

static gint
color_slider_button_release (GtkWidget *widget, GdkEventButton *event)
{
	ColorSlider *slider;

	slider = COLOR_SLIDER (widget);

	if (event->button == 1) {
		gdk_pointer_ungrab (event->time);
		slider->dragging = FALSE;
		g_signal_emit (G_OBJECT (slider), slider_signals[RELEASED], 0);
		if (slider->value != slider->oldvalue) g_signal_emit (G_OBJECT (slider), slider_signals[CHANGED], 0);
	}

	return FALSE;
}

static gint
color_slider_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	ColorSlider *slider;

	slider = COLOR_SLIDER (widget);

	if (slider->dragging) {
		gint cx, cw;
		cx = widget->style->xthickness;
		cw = widget->allocation.width - 2 * cx;
		gtk_adjustment_set_value (slider->adjustment, CLAMP ((gfloat) (event->x - cx) / cw, 0.0, 1.0));
		g_signal_emit (G_OBJECT (slider), slider_signals[DRAGGED], 0);
	}

	return FALSE;
}

GtkWidget *
color_slider_new (GtkAdjustment *adjustment)
{
	ColorSlider *slider;

	slider = g_object_new (TYPE_COLOR_SLIDER, NULL);

	color_slider_set_adjustment (slider, adjustment);

	return GTK_WIDGET (slider);
}

void
color_slider_set_adjustment (ColorSlider *slider, GtkAdjustment *adjustment)
{
	g_return_if_fail (slider != NULL);
	g_return_if_fail (IS_COLOR_SLIDER (slider));

	if (!adjustment) {
		adjustment = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.1);
	}

	if (slider->adjustment != adjustment) {
		if (slider->adjustment) {
			g_signal_handlers_disconnect_by_func (G_OBJECT (slider->adjustment), color_slider_adjustment_changed, slider);
			g_object_unref (G_OBJECT (slider->adjustment));
		}

		slider->adjustment = adjustment;
		g_object_ref (G_OBJECT (adjustment));

		g_signal_connect (G_OBJECT (adjustment), "changed",
				    G_CALLBACK (color_slider_adjustment_changed), slider);
		g_signal_connect (G_OBJECT (adjustment), "value_changed",
				    G_CALLBACK (color_slider_adjustment_value_changed), slider);

		slider->value = adjustment->value;

		color_slider_adjustment_changed (adjustment, slider);
	}
}

void
color_slider_set_colors (ColorSlider *slider, guint32 start, guint32 end)
{
	g_return_if_fail (slider != NULL);
	g_return_if_fail (IS_COLOR_SLIDER (slider));

	slider->c0[0] = start >> 24;
	slider->c0[1] = (start >> 16) & 0xff;
	slider->c0[2] = (start >> 8) & 0xff;
	slider->c0[3] = start & 0xff;
	slider->c1[0] = end >> 24;
	slider->c1[1] = (end >> 16) & 0xff;
	slider->c1[2] = (end >> 8) & 0xff;
	slider->c1[3] = end & 0xff;

	gtk_widget_queue_draw (GTK_WIDGET (slider));
}

void
color_slider_set_map (ColorSlider *slider, const guchar *map)
{
	g_return_if_fail (slider != NULL);
	g_return_if_fail (IS_COLOR_SLIDER (slider));

	slider->map = (guchar *) map;

	gtk_widget_queue_draw (GTK_WIDGET (slider));
}

void
color_slider_set_background (ColorSlider *slider, guint dark, guint light, guint size)
{
	g_return_if_fail (slider != NULL);
	g_return_if_fail (IS_COLOR_SLIDER (slider));

	slider->b0 = dark;
	slider->b1 = light;
	slider->bmask = size;

	gtk_widget_queue_draw (GTK_WIDGET (slider));
}

static void
color_slider_adjustment_changed (GtkAdjustment *adjustment, ColorSlider *slider)
{
	gtk_widget_queue_draw (GTK_WIDGET (slider));
}

static void
color_slider_adjustment_value_changed (GtkAdjustment *adjustment, ColorSlider *slider)
{
	GtkWidget *widget;

	widget = GTK_WIDGET (slider);

	if (slider->value != adjustment->value) {
		gint cx, cy, cw, ch;
		cx = widget->style->xthickness;
		cy = widget->style->ythickness;
		cw = widget->allocation.width - 2 * cx;
		ch = widget->allocation.height - 2 * cy;
		if ((gint) (adjustment->value * cw) != (gint) (slider->value * cw)) {
			gint ax, ay;
			gfloat value;
			value = slider->value;
			slider->value = adjustment->value;
			ax = cx + value * cw - ARROW_SIZE / 2 - 1;
			ay = ch - ARROW_SIZE + cy * 2 - 1;
			gtk_widget_queue_draw_area (widget, ax, ay, ARROW_SIZE + 2, ARROW_SIZE + 2);
			ax = cx + slider->value * cw - ARROW_SIZE / 2 - 1;
			ay = ch - ARROW_SIZE + cy * 2 - 1;
			gtk_widget_queue_draw_area (widget, ax, ay, ARROW_SIZE + 2, ARROW_SIZE + 2);
		} else {
			slider->value = adjustment->value;
		}
	}
}

static void
color_slider_paint (ColorSlider *slider, GdkRectangle *area)
{
	GtkWidget *widget;
	GdkRectangle warea, carea, aarea;
	GdkRectangle wpaint, cpaint, apaint;
	const guchar *b;

	widget = GTK_WIDGET (slider);

	/* Widget area */
	warea.x = 0;
	warea.y = 0;
	warea.width = widget->allocation.width;
	warea.height = widget->allocation.height;

	/* Color gradient area */
	carea.x = widget->style->xthickness;
	carea.y = widget->style->ythickness;
	carea.width = widget->allocation.width - 2 * carea.x;
	carea.height = widget->allocation.height - 2 * carea.y;

	/* Arrow area */
	aarea.x = slider->value * carea.width - ARROW_SIZE / 2 + carea.x;
	aarea.width = ARROW_SIZE;
	aarea.y = carea.height - ARROW_SIZE + carea.y * 2;
	aarea.height = ARROW_SIZE;

	/* Actual paintable area */
	if (!gdk_rectangle_intersect (area, &warea, &wpaint)) return;

	b = NULL;

	/* Paintable part of color gradient area */
	if (gdk_rectangle_intersect (area, &carea, &cpaint)) {
		if (slider->map) {
			gint s, d;
			/* Render map pixelstore */
			d = (1024 << 16) / carea.width;
			s = (cpaint.x - carea.x) * d;
			b = color_slider_render_map (cpaint.x - carea.x, cpaint.y - carea.y, cpaint.width, cpaint.height,
							slider->map, s, d,
							slider->b0, slider->b1, slider->bmask);
		} else {
			gint c[4], dc[4];
			gint i;
			/* Render gradient pixelstore */
			for (i = 0; i < 4; i++) {
				c[i] = slider->c0[i] << 16;
				dc[i] = ((slider->c1[i] << 16) - c[i]) / carea.width;
				c[i] += (cpaint.x - carea.x) * dc[i];
			}

			b = color_slider_render_gradient (cpaint.x - carea.x, cpaint.y - carea.y, cpaint.width, cpaint.height,
							     c, dc,
							     slider->b0, slider->b1, slider->bmask);
		}
	}

	/* Draw shadow */
	gtk_paint_shadow (widget->style, widget->window,
			  widget->state, GTK_SHADOW_IN,
			  area, widget, "colorslider",
			  0, 0,
			  warea.width, warea.height);

	/* Draw pixelstore */
	if (b != NULL) {
		gdk_draw_rgb_image (widget->window, widget->style->black_gc,
				    cpaint.x, cpaint.y,
				    cpaint.width, cpaint.height,
				    GDK_RGB_DITHER_MAX,
				    (guchar *) b, cpaint.width * 3);
	}

	if (gdk_rectangle_intersect (area, &aarea, &apaint)) {
		/* Draw arrow */
		gtk_paint_arrow (widget->style, widget->window,
				 widget->state, GTK_SHADOW_IN,
				 area, widget, "colorslider",
				 GTK_ARROW_UP, TRUE,
				 aarea.x, aarea.y,
				 ARROW_SIZE, ARROW_SIZE);
	}
}

/* Colors are << 16 */

static const guchar *
color_slider_render_gradient (gint x0, gint y0, gint width, gint height,
				 gint c[], gint dc[], guint b0, guint b1, guint mask)
{
	static guchar *buf = NULL;
	static gint bs = 0;
	guchar *dp;
	gint x, y;
	guint r, g, b, a;

	if (buf && (bs < width * height)) {
		g_free (buf);
		buf = NULL;
	}
	if (!buf) {
		buf = g_new (guchar, width * height * 3);
		bs = width * height;
	}

	dp = buf;
	r = c[0];
	g = c[1];
	b = c[2];
	a = c[3];
	for (x = x0; x < x0 + width; x++) {
		gint cr, cg, cb, ca;
		guchar *d;
		cr = r >> 16;
		cg = g >> 16;
		cb = b >> 16;
		ca = a >> 16;
		d = dp;
		for (y = y0; y < y0 + height; y++) {
			guint bg, fc;
			/* Background value */
			bg = ((x & mask) ^ (y & mask)) ? b0 : b1;
			fc = (cr - bg) * ca;
			d[0] = bg + ((fc + (fc >> 8) + 0x80) >> 8);
			fc = (cg - bg) * ca;
			d[1] = bg + ((fc + (fc >> 8) + 0x80) >> 8);
			fc = (cb - bg) * ca;
			d[2] = bg + ((fc + (fc >> 8) + 0x80) >> 8);
			d += 3 * width;
		}
		r += dc[0];
		g += dc[1];
		b += dc[2];
		a += dc[3];
		dp += 3;
	}

	return buf;
}

/* Positions are << 16 */

static const guchar *
color_slider_render_map (gint x0, gint y0, gint width, gint height,
			    guchar *map, gint start, gint step, guint b0, guint b1, guint mask)
{
	static guchar *buf = NULL;
	static gint bs = 0;
	guchar *dp, *sp;
	gint x, y;

	if (buf && (bs < width * height)) {
		g_free (buf);
		buf = NULL;
	}
	if (!buf) {
		buf = g_new (guchar, width * height * 3);
		bs = width * height;
	}

	dp = buf;
	for (x = x0; x < x0 + width; x++) {
		gint cr, cg, cb, ca;
		guchar *d;
		sp = map + 4 * (start >> 16);
		cr = *sp++;
		cg = *sp++;
		cb = *sp++;
		ca = *sp++;
		d = dp;
		for (y = y0; y < y0 + height; y++) {
			guint bg, fc;
			/* Background value */
			bg = ((x & mask) ^ (y & mask)) ? b0 : b1;
			fc = (cr - bg) * ca;
			d[0] = bg + ((fc + (fc >> 8) + 0x80) >> 8);
			fc = (cg - bg) * ca;
			d[1] = bg + ((fc + (fc >> 8) + 0x80) >> 8);
			fc = (cb - bg) * ca;
			d[2] = bg + ((fc + (fc >> 8) + 0x80) >> 8);
			d += 3 * width;
		}
		dp += 3;
		start += step;
	}

	return buf;
}
