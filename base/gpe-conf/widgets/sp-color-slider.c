#define __SP_COLOR_SLIDER_C__

/*
 * A slider with colored background
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdk.h>
#include <gdk/gdkrgb.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include "sp-color-slider.h"

#define SLIDER_WIDTH 96
#define SLIDER_HEIGHT 10
#define ARROW_SIZE 8

enum {
	GRABBED,
	DRAGGED,
	RELEASED,
	CHANGED,
	LAST_SIGNAL
};

static void sp_color_slider_class_init (SPColorSliderClass *klass);
static void sp_color_slider_init (SPColorSlider *slider);
static void sp_color_slider_destroy (GtkObject *object);

static void sp_color_slider_realize (GtkWidget *widget);
static void sp_color_slider_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_color_slider_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

static gint sp_color_slider_expose (GtkWidget *widget, GdkEventExpose *event);
static gint sp_color_slider_button_press (GtkWidget *widget, GdkEventButton *event);
static gint sp_color_slider_button_release (GtkWidget *widget, GdkEventButton *event);
static gint sp_color_slider_motion_notify (GtkWidget *widget, GdkEventMotion *event);

static void sp_color_slider_adjustment_changed (GtkAdjustment *adjustment, SPColorSlider *slider);
static void sp_color_slider_adjustment_value_changed (GtkAdjustment *adjustment, SPColorSlider *slider);

static void sp_color_slider_paint (SPColorSlider *slider, GdkRectangle *area);
static const guchar *sp_color_slider_render_gradient (gint x0, gint y0, gint width, gint height,
						      gint c[], gint dc[], guint b0, guint b1, guint mask);
static const guchar *sp_color_slider_render_map (gint x0, gint y0, gint width, gint height,
						 guchar *map, gint start, gint step, guint b0, guint b1, guint mask);

static GtkWidgetClass *parent_class;
static guint slider_signals[LAST_SIGNAL] = {0};

GtkType
sp_color_slider_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPColorSlider",
			sizeof (SPColorSlider),
			sizeof (SPColorSliderClass),
			(GtkClassInitFunc) sp_color_slider_class_init,
			(GtkObjectInitFunc) sp_color_slider_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_WIDGET, &info);
	}
	return type;
}

static void
sp_color_slider_class_init (SPColorSliderClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_WIDGET);

	slider_signals[GRABBED] = gtk_signal_new ("grabbed",
						  GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						  GTK_CLASS_TYPE(object_class),
						  GTK_SIGNAL_OFFSET (SPColorSliderClass, grabbed),
						  gtk_marshal_NONE__NONE,
						  GTK_TYPE_NONE, 0);
	slider_signals[DRAGGED] = gtk_signal_new ("dragged",
						  GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						  GTK_CLASS_TYPE(object_class),
						  GTK_SIGNAL_OFFSET (SPColorSliderClass, dragged),
						  gtk_marshal_NONE__NONE,
						  GTK_TYPE_NONE, 0);
	slider_signals[RELEASED] = gtk_signal_new ("released",
						  GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						  GTK_CLASS_TYPE(object_class),
						  GTK_SIGNAL_OFFSET (SPColorSliderClass, released),
						  gtk_marshal_NONE__NONE,
						  GTK_TYPE_NONE, 0);
	slider_signals[CHANGED] = gtk_signal_new ("changed",
						  GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						  GTK_CLASS_TYPE(object_class),
						  GTK_SIGNAL_OFFSET (SPColorSliderClass, changed),
						  gtk_marshal_NONE__NONE,
						  GTK_TYPE_NONE, 0);

	object_class->destroy = sp_color_slider_destroy;

	widget_class->realize = sp_color_slider_realize;
	widget_class->size_request = sp_color_slider_size_request;
	widget_class->size_allocate = sp_color_slider_size_allocate;
/*  	widget_class->draw = sp_color_slider_draw; */
/*  	widget_class->draw_focus = sp_color_slider_draw_focus; */
/*  	widget_class->draw_default = sp_color_slider_draw_default; */

	widget_class->expose_event = sp_color_slider_expose;
	widget_class->button_press_event = sp_color_slider_button_press;
	widget_class->button_release_event = sp_color_slider_button_release;
	widget_class->motion_notify_event = sp_color_slider_motion_notify;
}

static void
sp_color_slider_init (SPColorSlider *slider)
{
	/* We are widget with window */
	GTK_WIDGET_UNSET_FLAGS (slider, GTK_NO_WINDOW);

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
sp_color_slider_destroy (GtkObject *object)
{
	SPColorSlider *slider;

	slider = SP_COLOR_SLIDER (object);

	if (slider->adjustment) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (slider->adjustment), slider);
		gtk_object_unref (GTK_OBJECT (slider->adjustment));
		slider->adjustment = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

static void
sp_color_slider_realize (GtkWidget *widget)
{
	SPColorSlider *slider;
	GdkWindowAttr attributes;
	gint attributes_mask;

	slider = SP_COLOR_SLIDER (widget);

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
sp_color_slider_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	SPColorSlider *slider;

	slider = SP_COLOR_SLIDER (widget);

	requisition->width = SLIDER_WIDTH + widget->style->xthickness * 2;
	requisition->height = SLIDER_HEIGHT + widget->style->ythickness * 2;
}

static void
sp_color_slider_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	SPColorSlider *slider;

	slider = SP_COLOR_SLIDER (widget);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (widget)) {
		/* Resize GdkWindow */
		gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
	}
}

#if 0
static void
sp_color_slider_draw (GtkWidget *widget, GdkRectangle *area)
{
	SPColorSlider *slider;

	slider = SP_COLOR_SLIDER (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		gint width, height;
		width = widget->allocation.width;
		height = widget->allocation.height;
		sp_color_slider_paint (slider, area);
	}
}

static void
sp_color_slider_draw_focus (GtkWidget *widget)
{
	gtk_widget_draw (widget, NULL);
}

static void
sp_color_slider_draw_default (GtkWidget *widget)
{
	gtk_widget_draw (widget, NULL);
}

static gint
sp_color_slider_expose (GtkWidget *widget, GdkEventExpose *event)
{
	gtk_widget_draw (widget, NULL);

	return FALSE;
}
#endif

static gint
sp_color_slider_expose (GtkWidget *widget, GdkEventExpose *event)
{
	SPColorSlider *slider;

	slider = SP_COLOR_SLIDER (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		gint width, height;
		width = widget->allocation.width;
		height = widget->allocation.height;
		sp_color_slider_paint (slider, &event->area);
	}

	return FALSE;
}

static gint
sp_color_slider_button_press (GtkWidget *widget, GdkEventButton *event)
{
	SPColorSlider *slider;

	slider = SP_COLOR_SLIDER (widget);

	if (event->button == 1) {
		gint cx, cw;
		cx = widget->style->xthickness;
		cw = widget->allocation.width - 2 * cx;
		gtk_signal_emit (GTK_OBJECT (slider), slider_signals[GRABBED]);
		slider->dragging = TRUE;
		slider->oldvalue = slider->value;
		gtk_adjustment_set_value (slider->adjustment, CLAMP ((gfloat) (event->x - cx) / cw, 0.0, 1.0));
		gtk_signal_emit (GTK_OBJECT (slider), slider_signals[DRAGGED]);
		gdk_pointer_grab (widget->window, FALSE,
				  GDK_POINTER_MOTION_MASK |
				  GDK_BUTTON_RELEASE_MASK,
				  NULL, NULL, event->time);
	}

	return FALSE;
}

static gint
sp_color_slider_button_release (GtkWidget *widget, GdkEventButton *event)
{
	SPColorSlider *slider;

	slider = SP_COLOR_SLIDER (widget);

	if (event->button == 1) {
		gdk_pointer_ungrab (event->time);
		slider->dragging = FALSE;
		gtk_signal_emit (GTK_OBJECT (slider), slider_signals[RELEASED]);
		if (slider->value != slider->oldvalue) gtk_signal_emit (GTK_OBJECT (slider), slider_signals[CHANGED]);
	}

	return FALSE;
}

static gint
sp_color_slider_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	SPColorSlider *slider;

	slider = SP_COLOR_SLIDER (widget);

	if (slider->dragging) {
		gint cx, cw;
		cx = widget->style->xthickness;
		cw = widget->allocation.width - 2 * cx;
		gtk_adjustment_set_value (slider->adjustment, CLAMP ((gfloat) (event->x - cx) / cw, 0.0, 1.0));
		gtk_signal_emit (GTK_OBJECT (slider), slider_signals[DRAGGED]);
	}

	return FALSE;
}

GtkWidget *
sp_color_slider_new (GtkAdjustment *adjustment)
{
	SPColorSlider *slider;

	slider = gtk_type_new (SP_TYPE_COLOR_SLIDER);

	sp_color_slider_set_adjustment (slider, adjustment);

	return GTK_WIDGET (slider);
}

void
sp_color_slider_set_adjustment (SPColorSlider *slider, GtkAdjustment *adjustment)
{
	g_return_if_fail (slider != NULL);
	g_return_if_fail (SP_IS_COLOR_SLIDER (slider));

	if (!adjustment) {
		adjustment = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.1);
	}

	if (slider->adjustment != adjustment) {
		if (slider->adjustment) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (slider->adjustment), slider);
			gtk_object_unref (GTK_OBJECT (slider->adjustment));
		}

		slider->adjustment = adjustment;
		gtk_object_ref (GTK_OBJECT (adjustment));
		gtk_object_sink (GTK_OBJECT (adjustment));

		gtk_signal_connect (GTK_OBJECT (adjustment), "changed",
				    GTK_SIGNAL_FUNC (sp_color_slider_adjustment_changed), slider);
		gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
				    GTK_SIGNAL_FUNC (sp_color_slider_adjustment_value_changed), slider);

		slider->value = adjustment->value;

		sp_color_slider_adjustment_changed (adjustment, slider);
	}
}

void
sp_color_slider_set_colors (SPColorSlider *slider, guint32 start, guint32 end)
{
	g_return_if_fail (slider != NULL);
	g_return_if_fail (SP_IS_COLOR_SLIDER (slider));

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
sp_color_slider_set_map (SPColorSlider *slider, const guchar *map)
{
	g_return_if_fail (slider != NULL);
	g_return_if_fail (SP_IS_COLOR_SLIDER (slider));

	slider->map = (guchar *) map;

	gtk_widget_queue_draw (GTK_WIDGET (slider));
}

void
sp_color_slider_set_background (SPColorSlider *slider, guint dark, guint light, guint size)
{
	g_return_if_fail (slider != NULL);
	g_return_if_fail (SP_IS_COLOR_SLIDER (slider));

	slider->b0 = dark;
	slider->b1 = light;
	slider->bmask = size;

	gtk_widget_queue_draw (GTK_WIDGET (slider));
}

static void
sp_color_slider_adjustment_changed (GtkAdjustment *adjustment, SPColorSlider *slider)
{
	gtk_widget_queue_draw (GTK_WIDGET (slider));
}

static void
sp_color_slider_adjustment_value_changed (GtkAdjustment *adjustment, SPColorSlider *slider)
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

/* Area is widget relative */

#if 0
static void
print_rect (const char *name, GdkRectangle *r)
{
	g_print ("%s: %d %d %d %d\n", name, r->x, r->y, r->width, r->height);
}
#endif

static void
sp_color_slider_paint (SPColorSlider *slider, GdkRectangle *area)
{
	static GdkPixmap *px;
	static gint pxw = 0;
	static gint pxh = 0;
	GtkWidget *widget;
	GdkRectangle warea, carea, aarea;
	GdkRectangle wpaint, cpaint, apaint;
	const guchar *b;

	widget = GTK_WIDGET (slider);

	warea.x = 0;
	warea.y = 0;
	warea.width = widget->allocation.width;
	warea.height = widget->allocation.height;

	carea.x = widget->style->xthickness;
	carea.y = widget->style->ythickness;
	carea.width = widget->allocation.width - 2 * carea.x;
	carea.height = widget->allocation.height - 2 * carea.y;

	aarea.x = slider->value * carea.width - ARROW_SIZE / 2 + carea.x;
	aarea.width = ARROW_SIZE;
	aarea.y = carea.height - ARROW_SIZE + carea.y * 2;
	aarea.height = ARROW_SIZE;

	if (!gdk_rectangle_intersect (area, &warea, &wpaint)) return;

	if (px && ((pxw < wpaint.width) || (pxh < wpaint.height))) {
		gdk_pixmap_unref (px);
		px = NULL;
	}
	if (!px) {
		px = gdk_pixmap_new (widget->window, wpaint.width, wpaint.height, -1);
		pxw = wpaint.width;
		pxh = wpaint.height;
	}

	b = NULL;

	if (gdk_rectangle_intersect (area, &carea, &cpaint)) {
		if (slider->map) {
			gint s, d;
			/* Render map pixelstore */
			d = (1024 << 16) / carea.width;
			s = (cpaint.x - carea.x) * d;
			b = sp_color_slider_render_map (cpaint.x - carea.x, cpaint.y - carea.y, cpaint.width, cpaint.height,
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

			b = sp_color_slider_render_gradient (cpaint.x - carea.x, cpaint.y - carea.y, cpaint.width, cpaint.height,
							     c, dc,
							     slider->b0, slider->b1, slider->bmask);
		}
	}

	/* Draw shadow */
	gtk_draw_box (widget->style, px,
			 widget->state, GTK_SHADOW_IN,
			 0 - wpaint.x, 0 - wpaint.y,
			 warea.width, warea.height);

	/* Draw pixelstore */
	if (b != NULL) {
		gdk_draw_rgb_image (px, widget->style->black_gc,
				    cpaint.x - wpaint.x, cpaint.y - wpaint.y,
				    cpaint.width, cpaint.height,
				    GDK_RGB_DITHER_MAX,
				    (guchar *) b, cpaint.width * 3);
	}

	if (gdk_rectangle_intersect (area, &aarea, &apaint)) {
		/* Draw arrow */
		gtk_draw_arrow (widget->style, px,
				widget->state, GTK_SHADOW_OUT,
				GTK_ARROW_UP, TRUE,
				aarea.x - wpaint.x, aarea.y - wpaint.y,
				ARROW_SIZE, ARROW_SIZE);
	}

	/* Draw everything */
	gdk_draw_pixmap (widget->window,
			 widget->style->black_gc,
			 px,
			 0, 0,
			 wpaint.x, wpaint.y,
			 wpaint.width, wpaint.height);
}

/* Colors are << 16 */

static const guchar *
sp_color_slider_render_gradient (gint x0, gint y0, gint width, gint height,
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
sp_color_slider_render_map (gint x0, gint y0, gint width, gint height,
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

