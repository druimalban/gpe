#define __SP_COLOR_C__

/*
 * Colors and colorspaces
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include "color.h"

struct _SPColorSpace {
	guchar *name;
};

static const SPColorSpace RGB = {"RGB"};
static const SPColorSpace CMYK = {"CMYK"};

SPColorSpaceClass
sp_color_get_colorspace_class (const SPColor *color)
{
	g_return_val_if_fail (color != NULL, SP_COLORSPACE_CLASS_INVALID);

	if (color->colorspace == &RGB) return SP_COLORSPACE_CLASS_PROCESS;
	if (color->colorspace == &CMYK) return SP_COLORSPACE_CLASS_PROCESS;

	return SP_COLORSPACE_CLASS_UNKNOWN;
}

SPColorSpaceType
sp_color_get_colorspace_type (const SPColor *color)
{
	g_return_val_if_fail (color != NULL, SP_COLORSPACE_TYPE_INVALID);

	if (color->colorspace == &RGB) return SP_COLORSPACE_TYPE_RGB;
	if (color->colorspace == &CMYK) return SP_COLORSPACE_TYPE_CMYK;

	return SP_COLORSPACE_TYPE_UNKNOWN;
}

void
sp_color_copy (SPColor *dst, const SPColor *src)
{
	g_return_if_fail (dst != NULL);
	g_return_if_fail (src != NULL);

	*dst = *src;
}

gboolean
sp_color_is_equal (SPColor *c0, SPColor *c1)
{
	g_return_val_if_fail (c0 != NULL, TRUE);
	g_return_val_if_fail (c1 != NULL, TRUE);

	if (c0->colorspace != c1->colorspace) return FALSE;
	if (c0->v.c[0] != c1->v.c[0]) return FALSE;
	if (c0->v.c[1] != c1->v.c[1]) return FALSE;
	if (c0->v.c[2] != c1->v.c[2]) return FALSE;
	if ((c0->colorspace == &CMYK) && (c0->v.c[3] != c1->v.c[3])) return FALSE;

	return TRUE;
}

void
sp_color_set_rgb_float (SPColor *color, gfloat r, gfloat g, gfloat b)
{
	g_return_if_fail (color != NULL);
	g_return_if_fail (r >= 0.0);
	g_return_if_fail (r <= 1.0);
	g_return_if_fail (g >= 0.0);
	g_return_if_fail (g <= 1.0);
	g_return_if_fail (b >= 0.0);
	g_return_if_fail (b <= 1.0);

	color->colorspace = &RGB;
	color->v.c[0] = r;
	color->v.c[1] = g;
	color->v.c[2] = b;
}

void
sp_color_set_rgb_rgba32 (SPColor *color, guint32 value)
{
	g_return_if_fail (color != NULL);

	color->colorspace = &RGB;
	color->v.c[0] = (value >> 24) / 255.0;
	color->v.c[1] = ((value >> 16) & 0xff) / 255.0;
	color->v.c[2] = ((value >> 8) & 0xff) / 255.0;
}

void
sp_color_set_cmyk_float (SPColor *color, gfloat c, gfloat m, gfloat y, gfloat k)
{
	g_return_if_fail (color != NULL);
	g_return_if_fail (c >= 0.0);
	g_return_if_fail (c <= 1.0);
	g_return_if_fail (m >= 0.0);
	g_return_if_fail (m <= 1.0);
	g_return_if_fail (y >= 0.0);
	g_return_if_fail (y <= 1.0);
	g_return_if_fail (k >= 0.0);
	g_return_if_fail (k <= 1.0);

	color->colorspace = &CMYK;
	color->v.c[0] = c;
	color->v.c[1] = m;
	color->v.c[2] = y;
	color->v.c[3] = k;
}

guint32
sp_color_get_rgba32_ualpha (const SPColor *color, guint alpha)
{
	guint32 rgba;

	g_return_val_if_fail (color != NULL, 0x0);
	g_return_val_if_fail (alpha <= 0xff, 0x0);

	if (color->colorspace == &RGB) {
		rgba = SP_RGBA32_U_COMPOSE (SP_COLOR_F_TO_U (color->v.c[0]), SP_COLOR_F_TO_U (color->v.c[1]), SP_COLOR_F_TO_U (color->v.c[2]), alpha);
	} else {
		gfloat rgb[3];
		sp_color_get_rgb_floatv (color, rgb);
		rgba = SP_RGBA32_U_COMPOSE (SP_COLOR_F_TO_U (rgb[0]), SP_COLOR_F_TO_U (rgb[1]), SP_COLOR_F_TO_U (rgb[2]), alpha);
	}

	return rgba;
}

guint32
sp_color_get_rgba32_falpha (const SPColor *color, gfloat alpha)
{
	g_return_val_if_fail (color != NULL, 0x0);
	g_return_val_if_fail (alpha >= 0.0, 0x0);
	g_return_val_if_fail (alpha <= 1.0, 0x0);

	return sp_color_get_rgba32_ualpha (color, SP_COLOR_F_TO_U (alpha));
}

void
sp_color_get_rgb_floatv (const SPColor *color, gfloat *rgb)
{
	g_return_if_fail (color != NULL);
	g_return_if_fail (rgb != NULL);

	if (color->colorspace == &RGB) {
		rgb[0] = color->v.c[0];
		rgb[1] = color->v.c[1];
		rgb[2] = color->v.c[2];
	} else if (color->colorspace == &CMYK) {
		sp_color_cmyk_to_rgb_floatv (rgb, color->v.c[0], color->v.c[1], color->v.c[2], color->v.c[3]);
	}
}

void
sp_color_get_cmyk_floatv (const SPColor *color, gfloat *cmyk)
{
	g_return_if_fail (color != NULL);
	g_return_if_fail (cmyk != NULL);

	if (color->colorspace == &CMYK) {
		cmyk[0] = color->v.c[0];
		cmyk[1] = color->v.c[1];
		cmyk[2] = color->v.c[2];
		cmyk[3] = color->v.c[3];
	} else if (color->colorspace == &RGB) {
		sp_color_rgb_to_cmyk_floatv (cmyk, color->v.c[0], color->v.c[1], color->v.c[2]);
	}
}

/* Plain mode helpers */

void
sp_color_rgb_to_hsv_floatv (gfloat *hsv, gfloat r, gfloat g, gfloat b)
{
	gdouble max, min, delta;

	max = MAX (MAX (r, g), b);
	min = MIN (MIN (r, g), b);
	delta = max - min;

	hsv[2] = max;

	if (max > 0) {
		hsv[1] = delta / max;
	} else {
		hsv[1] = 0.0;
	}

	if (hsv[1] != 0.0) {
		if (r == max) {
			hsv[0] = (g - b) / delta;
		} else if (g == max) {
			hsv[0] = 2.0 + (b - r) / delta;
		} else {
			hsv[0] = 4.0 + (r - g) / delta;
		}

		hsv[0] = hsv[0] / 6.0;

		if (hsv[0] < 0) hsv[0] += 1.0;
	}
}

void
sp_color_hsv_to_rgb_floatv (gfloat *rgb, gfloat h, gfloat s, gfloat v)
{
	gdouble f, w, q, t, d;

	d = h * 5.99999999;
	f = d - floor (d);
	w = v * (1.0 - s);
	q = v * (1.0 - (s * f));
	t = v * (1.0 - (s * (1.0 - f)));

	if (d < 1.0) {
		*rgb++ = v;
		*rgb++ = t;
		*rgb++ = w;
	} else if (d < 2.0) {
		*rgb++ = q;
		*rgb++ = v;
		*rgb++ = w;
	} else if (d < 3.0) {
		*rgb++ = w;
		*rgb++ = v;
		*rgb++ = t;
	} else if (d < 4.0) {
		*rgb++ = w;
		*rgb++ = q;
		*rgb++ = v;
	} else if (d < 5.0) {
		*rgb++ = t;
		*rgb++ = w;
		*rgb++ = v;
	} else {
		*rgb++ = v;
		*rgb++ = w;
		*rgb++ = q;
	}
}

void
sp_color_rgb_to_cmyk_floatv (gfloat *cmyk, gfloat r, gfloat g, gfloat b)
{
	gfloat c, m, y, k, kd;

	c = 1.0 - r;
	m = 1.0 - g;
	y = 1.0 - b;
	k = MIN (MIN (c, m), y);

	c = c - k;
	m = m - k;
	y = y - k;

	kd = 1.0 - k;

	if (kd > 1e-9) {
		c = c / kd;
		m = m / kd;
		y = y / kd;
	}

	cmyk[0] = c;
	cmyk[1] = m;
	cmyk[2] = y;
	cmyk[3] = k;
}

void
sp_color_cmyk_to_rgb_floatv (gfloat *rgb, gfloat c, gfloat m, gfloat y, gfloat k)
{
	gfloat kd;

	kd = 1.0 - k;

	c = c * kd;
	m = m * kd;
	y = y * kd;

	c = c + k;
	m = m + k;
	y = y + k;

	rgb[0] = 1.0 - c;
	rgb[1] = 1.0 - m;
	rgb[2] = 1.0 - y;
}




