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

#include <glib.h>
#include "l-grid.h"
#include "l-model.h"

typedef struct {
  LModelNotifyFunc callback;
  gpointer user_data;
} LUpdateInfo;

typedef struct {
  LModelGenerationNotifyFunc callback;
  gpointer user_data;
} LGenerationUpdateInfo;

gboolean
l_model_update_func (gpointer data)
{
  if (data)
    {
      LModel *model = (LModel *) data;

      if (model->notify_callbacks && model->running)
        {
          GSList *iter;
	  
          /* g_print ("model_update_func\n"); */

          l_grid_next_generation (model->grid);
	  model->generation++;

          /* g_print ("next_gen done\n"); */
	  

	  /* update notifications */
	  for (iter = model->notify_callbacks; iter; iter = iter->next)
	    {
	      LUpdateInfo *info = (LUpdateInfo *) iter->data;

	      ((LModelNotifyFunc) (*info->callback)) (info->user_data);
	    }

	  /* generation notifications */
	  for (iter = model->generation_notify_callbacks; iter; iter = iter->next)
	    {
	      LGenerationUpdateInfo *info = (LGenerationUpdateInfo *) iter->data;

	      ((LModelGenerationNotifyFunc) (*info->callback)) (model->generation, info->user_data);
	    }
        }
      
      return TRUE;
    }
  return FALSE;
}

LModel *
l_model_new (guint update_interval)
{
  LModel *model = g_new0 (LModel, 1);

  model->grid = l_grid_new ();
  model->running = FALSE;
  model->generation = 0;

  model->update_interval = update_interval;
  model->event_id = g_timeout_add (update_interval, l_model_update_func, model);

  return model;
}

void
l_model_destroy (LModel *model)
{
  if (model)
    {
      GSList *iter;

      /* stop updating first! */
      g_source_remove (model->event_id);
      l_grid_destroy (model->grid);

      for (iter = model->notify_callbacks; iter; iter = iter->next)
        if (iter->data)
	  g_free ((LUpdateInfo *) iter->data);
      g_slist_free (model->notify_callbacks);

      for (iter = model->generation_notify_callbacks; iter; iter = iter->next)
        if (iter->data)
	  g_free ((LGenerationUpdateInfo *) iter->data);
      g_slist_free (model->generation_notify_callbacks);
    }
}

void
l_model_set_update_interval (LModel *model, guint interval)
{
  if (model)
    {
      g_source_remove (model->event_id);
      model->event_id = g_timeout_add (interval, l_model_update_func, model);
    }
}

guint
l_model_get_update_interval (LModel *model)
{
  guint interval = 0;

  if (model)
    interval = model->update_interval;

  return interval;
}

void
l_model_toggle (LModel *model, gint x, gint y)
{
  /* g_print ("toggle\n"); */
  if (model)
    {
      GSList *iter;
	  
      l_grid_cell_toggle (model->grid, x, y);

      for (iter = model->notify_callbacks; iter; iter = iter->next)
        {
	  LUpdateInfo *info = (LUpdateInfo *) iter->data;

	  ((LModelNotifyFunc) (*info->callback)) (info->user_data);
	}
    }
}

void
l_model_add_notify (LModel *model, LModelNotifyFunc func, gpointer data)
{
  LUpdateInfo *info = g_new0 (LUpdateInfo, 1);

  info->callback = func;
  info->user_data = data;

  model->notify_callbacks = g_slist_append (model->notify_callbacks, info);
}

void
l_model_add_generation_notify (LModel *model, LModelGenerationNotifyFunc func, gpointer data)
{
  LGenerationUpdateInfo *info = g_new0 (LGenerationUpdateInfo, 1);

  info->callback = func;
  info->user_data = data;

  model->generation_notify_callbacks = g_slist_append (model->generation_notify_callbacks, info);
}

void
l_model_run (LModel *model, gboolean run)
{
  if (model)
    {
      model->running = run;
      /* g_print ("run: %d\n",run); */
    }
}

void
l_model_foreach (LModel *model, LModelForeachFunc func, gpointer user_data)
{
  if (model && model->grid)
    l_grid_foreach (model->grid, (LGridForeachFunc) func, user_data);
}

void
l_model_foreach_zombie (LModel *model, LModelForeachFunc func, gpointer user_data)
{
  if (model && model->grid && model->grid->zombies)
    {
      /* g_print ("zombies!\n"); */
      l_grid_foreach (model->grid->zombies, (LGridForeachFunc) func, user_data);
    }
}

void
l_model_get_dimension (LModel *model, gint *width, gint *height)
{
  if (model)
    {
      gint min_x = 0, min_y = 0, max_x = 0, max_y = 0;
      
      l_grid_get_size (model->grid, &min_x, &min_y, &max_x, &max_y);
      (*width) = max_x - min_x;
      (*height) = max_y - min_y;
    }
}

guint
l_model_get_generation (LModel *model)
{
  if (!model)
    return 0;

  return model->generation;
}
