/*
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef GPE_CLOCK_H
#define GPE_CLOCK_H

#include <gtk/gtk.h>
#include <glib-object.h>

#define GPE_TYPE_CLOCK_FACE          (gpe_clock_face_get_type ())
#define GPE_CLOCK_FACE(obj)          G_TYPE_CHECK_INSTANCE_CAST ((obj), gpe_clock_face_get_type(), GpeClockFace)

#define GTK_CLOCK_GET_CLASS(obj)	G_TYPE_INSTANCE_GET_CLASS ((obj), gpe_clock_face_get_type(), GpeClockFaceClass)

typedef struct _GpeClockFace	   GpeClockFace;
typedef struct _GpeClockFaceClass      GpeClockFaceClass;

GtkType		gpe_clock_face_get_type (void);

GtkWidget      *gpe_clock_face_new (GtkAdjustment *, GtkAdjustment *);

#endif
