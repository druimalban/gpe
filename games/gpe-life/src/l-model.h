/* GPE Life
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __L_MODEL_H__
#define __L_MODEL_H__

#include <glib.h>
#include "l-grid.h"

struct _LModel
{
  LGrid *grid;
  GSList *notify_callbacks;
  GSList *generation_notify_callbacks;
  gboolean running;
  guint event_id;
  guint generation;
  guint update_interval;
};
typedef struct _LModel LModel;

typedef void (*LModelNotifyFunc) (gpointer);

typedef void (*LModelGenerationNotifyFunc) (guint, gpointer);

typedef void (*LModelForeachFunc) (gint, gint, gpointer);

LModel *
l_model_new (guint);

void
l_model_destroy (LModel *);

void
l_model_toggle (LModel *, gint, gint);

void
l_model_add_notify (LModel *, LModelNotifyFunc, gpointer);

void
l_model_add_generation_notify (LModel *model, LModelGenerationNotifyFunc func, gpointer data);

void
l_model_run (LModel *model, gboolean run);

void
l_model_foreach (LModel *, LModelForeachFunc, gpointer);

void
l_model_foreach_zombie (LModel *, LModelForeachFunc, gpointer);

void
l_model_get_dimension (LModel *, gint *, gint *);

void
l_model_set_update_interval (LModel *model, guint interval);

guint
l_model_get_update_interval (LModel *model);

guint
l_model_get_generation (LModel *model);

#endif /* __L_MODEL_H__ */
