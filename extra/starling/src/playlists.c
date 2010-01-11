/* playlists.c - A treemodel for playlists.
   Copyright (C) 2007, 2008, 2010 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#define ERROR_DOMAIN() g_quark_from_static_string ("playlist")

#include <string.h>
#include <sys/time.h>
#include <glib.h>
#include <gtk/gtktreemodel.h>

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>

#include "playlists.h"
#include "musicdb.h"

#define ITER_INIT(model, iter, i) \
  do \
    { \
      (iter)->stamp =  GPOINTER_TO_INT ((model)); \
      (iter)->user_data = GINT_TO_POINTER ((i)); \
    } \
  while (0)

struct play_list
{
  struct play_list *next;

  /* The number of entries in the play list.  If -1, then invalid.  */
  int total;

  /* The last time this entry was updated.  If VERSION !=
     PLS->VERSION, then TOTAL may be out of sync.  */
  unsigned int total_version;

  char name[];
};

struct _PlayLists {
  GObject parent;

  MusicDB *db;

  struct obstack data_obstack;

  struct play_list *play_lists;

  /* Whether to include the queue in the list of play lists.  */
  gboolean include_queue;

  /* See the definition of version in the struct play_list above.  */
  unsigned int version;

  /* The number of entries in the playlist.  */
  int total;

  gint new_entry_signal_id;
  gint changed_entry_signal_id;
  gint deleted_entry_signal_id;
  gint cleared_signal_id;
  gint added_to_play_list_signal_id;
  gint removed_from_play_list_signal_id;

  gint reschedule_timeout;
  /* Number of reschedules requests since last reschedule.  */
  gint reschedule_requests;
  struct timeval last_refresh;
};

static void play_lists_dispose (GObject *obj);
static void play_lists_finalize (GObject *object);
static void treemodel_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (PlayLists, play_lists, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                treemodel_iface_init));

static void
play_lists_class_init (PlayListsClass *klass)
{
  play_lists_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = play_lists_finalize;
  object_class->dispose = play_lists_dispose;

  // PlayListsClass *play_lists_class = PLAY_LISTS_CLASS (klass);
}

static void
play_lists_init (PlayLists *pl)
{
  pl->total = -1;

  obstack_init (&pl->data_obstack);
}

static void
play_lists_dispose (GObject *obj)
{
  PlayLists *pl = PLAY_LISTS (obj);

  if (pl->new_entry_signal_id)
    g_signal_handler_disconnect (pl->db, pl->new_entry_signal_id);
  if (pl->changed_entry_signal_id)
    g_signal_handler_disconnect (pl->db, pl->changed_entry_signal_id);
  if (pl->deleted_entry_signal_id)
    g_signal_handler_disconnect (pl->db, pl->deleted_entry_signal_id);
  if (pl->cleared_signal_id)
    g_signal_handler_disconnect (pl->db, pl->cleared_signal_id);
  if (pl->added_to_play_list_signal_id)
    g_signal_handler_disconnect (pl->db, pl->added_to_play_list_signal_id);
  if (pl->removed_from_play_list_signal_id)
    g_signal_handler_disconnect (pl->db, pl->removed_from_play_list_signal_id);

  if (pl->reschedule_timeout)
    g_source_remove (pl->reschedule_timeout);

  if (pl->db)
    g_object_unref (pl->db);

  obstack_free (&pl->data_obstack, NULL);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (play_lists_parent_class)->dispose (obj);
}

static void
play_lists_finalize (GObject *object)
{
  // PlayLists *pl = PLAY_LISTS (object);

  G_OBJECT_CLASS (play_lists_parent_class)->finalize (object);
}

/* From glibc manual.  */
static int
timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
  /* Perform the carry for the later subtraction by updating Y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     `tv_usec' is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

static gboolean
do_refresh (gpointer data)
{
  PlayLists *pl = PLAY_LISTS (data);

  if (pl->reschedule_requests > 10)
    /* There have been a lot of reschedule requests recently.  Delay
       the actual reschedule a bit on the assumption that this will
       continue for a while and then eventually settle.  */
    {
      pl->reschedule_requests >>= 1;
      return TRUE;
    }

  {
    struct timeval now;
    gettimeofday (&now, NULL);

    struct timeval diff;
    timeval_subtract (&diff, &now, &pl->last_refresh);

    if (diff.tv_sec == 0)
      /* Less than a second since the last refresh.  Wait.  */
      return TRUE;
  }

  /* Forget timeout source.  */
  pl->reschedule_timeout = 0;
  pl->reschedule_requests = 0;

  int old_total = pl->total;

  if (pl->play_lists)
    obstack_free (&pl->data_obstack, pl->play_lists);
  pl->play_lists = NULL;

  pl->total = 0;

  struct play_list *last = NULL;

  bool saw_queue = ! pl->include_queue;
  int cb (const char *list)
  {
    if (strcmp (list, "queue") == 0)
      {
	if (saw_queue)
	  return 0;
	saw_queue = true;
      }

    pl->total ++;

    int len = strlen (list);
    struct play_list *l = obstack_alloc (&pl->data_obstack,
					 sizeof (*l) + len + 1);
    if (! pl->play_lists)
      pl->play_lists = l;

    /* Give it a version furthest in the future.  */
    l->total_version = pl->version;
    l->total = -1;
    memcpy (l->name, list, len + 1);

    l->next = NULL;
    if (last)
      last->next = l;
    last = l;

    return 0;
  }

  cb ("Library");
  cb ("queue");
  music_db_play_lists_for_each (pl->db, cb);
  pl->version ++;

  /* We need to emit some signals now so the thing using this model
     will stay in sync.  We brute force it as calculating the
     differences is a) non-trivial and b) likely more expensive than
     the naive approach.  */

  /* The first time through, we don't emit new change events.  This is
     because the rows are not new: any attach control has not seem
     them yet.  The result would be (at least in some situation, for
     instance, a combo box created with gtk_combo_box_new_with_model),
     double counting.  */
  if (old_total != -1)
    {
      if (old_total < pl->total)
	/* There are some new rows.  Add those first.  */
	{
	  GtkTreePath *path = gtk_tree_path_new_from_indices (old_total, -1);
	  GtkTreeIter iter;
	  ITER_INIT (pl, &iter, old_total);

	  int i;
	  for (i = old_total; i < pl->total; i ++)
	    gtk_tree_model_row_inserted (GTK_TREE_MODEL (pl), path, &iter);

	  gtk_tree_path_free (path);
	}
      else if (old_total > pl->total)
	{
	  GtkTreePath *path = gtk_tree_path_new_from_indices (pl->total, -1);

	  int i;
	  for (i = pl->total; i < old_total; i ++)
	    gtk_tree_model_row_deleted (GTK_TREE_MODEL (pl), path);

	  gtk_tree_path_free (path);
	}

      int min_total;
      if (old_total < pl->total)
	min_total = old_total;
      else
	min_total = pl->total;

      int i;
      for (i = 0; i < min_total; i ++)
	{
	  GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
	  GtkTreeIter iter;
	  ITER_INIT (pl, &iter, i);

	  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);

	  gtk_tree_path_free (path);
	}
    }

  gettimeofday (&pl->last_refresh, NULL);

  return FALSE;
}

/* Schedule a playlist refresh.  We do this at most once every few
   seconds.  */
static void
refresh_schedule (PlayLists *pl, bool now)
{
  if (now)
    {
      if (pl->reschedule_timeout)
	{
	  g_source_remove (pl->reschedule_timeout);
	  pl->reschedule_timeout = 0;
	}

      pl->reschedule_requests = 0;
      memset (&pl->last_refresh, 0, sizeof (pl->last_refresh));
      do_refresh (pl);
    }
  else if (! pl->reschedule_timeout)
    pl->reschedule_timeout = g_timeout_add (100, do_refresh, pl);
  else
    pl->reschedule_requests ++;
}

/* If a refresh is pending, do it now.  */
static void
refresh_flush (PlayLists *pl)
{
  if (! pl->reschedule_timeout)
    /* No refresh pending.  */
    return;

  g_source_remove (pl->reschedule_timeout);
  pl->reschedule_timeout = 0;
  pl->reschedule_requests = 0;
  memset (&pl->last_refresh, 0, sizeof (pl->last_refresh));
  do_refresh (pl);
}

static void
new_entry (MusicDB *db, gint uid, gpointer data)
{
  PlayLists *pl = PLAY_LISTS (data);

  refresh_schedule (pl, false);
}

static void
changed_entry (MusicDB *db, guint uid, guint changed_mask, gpointer data)
{
  PlayLists *pl = PLAY_LISTS (data);

  refresh_schedule (pl, false);
}

static void
deleted_entry (MusicDB *db, gint uid, gpointer data)
{
  PlayLists *pl = PLAY_LISTS (data);

  refresh_schedule (pl, false);
}

static void
cleared (MusicDB *db, gpointer data)
{
  PlayLists *pl = PLAY_LISTS (data);

  refresh_schedule (pl, true);
}

static void
added_to_play_list (MusicDB *db, const char *list, gint offset, gpointer data)
{
  PlayLists *pl = PLAY_LISTS (data);

  refresh_schedule (pl, false);
}

static void
removed_from_play_list (MusicDB *db, const char *list, gint offset,
			gpointer data)
{
  PlayLists *pl = PLAY_LISTS (data);

  refresh_schedule (pl, false);
}


PlayLists *
play_lists_new (MusicDB *db, gboolean include_queue)
{
  PlayLists *pl = PLAY_LISTS (g_object_new (PLAY_LISTS_TYPE, NULL));

  g_object_ref (db);
  pl->db = db;

  pl->include_queue = include_queue;

  pl->new_entry_signal_id
    = g_signal_connect (G_OBJECT (db), "new-entry",
			G_CALLBACK (new_entry), pl);

  pl->changed_entry_signal_id
    = g_signal_connect (G_OBJECT (db), "changed-entry",
			G_CALLBACK (changed_entry), pl);

  pl->deleted_entry_signal_id
    = g_signal_connect (G_OBJECT (db), "deleted-entry",
			G_CALLBACK (deleted_entry), pl);

  pl->cleared_signal_id
    = g_signal_connect (G_OBJECT (db), "cleared",
			G_CALLBACK (cleared), pl);

  pl->added_to_play_list_signal_id
    = g_signal_connect (G_OBJECT (db), "added-to-play-list",
			G_CALLBACK (added_to_play_list), pl);

  pl->removed_from_play_list_signal_id
    = g_signal_connect (G_OBJECT (db), "removed-from-play-list",
			G_CALLBACK (removed_from_play_list), pl);

  refresh_schedule (pl, false);

  return pl;
}

gint
play_lists_count (PlayLists *pl)
{
  refresh_flush (pl);
  return pl->total;
}

gint
play_lists_index_of (PlayLists *pls, const char *play_list)
{
  if (! play_list)
    return 0;

  refresh_flush (pls);
  int i;
  struct play_list *l;
  for (i = 0, l = pls->play_lists; l; i ++, l = l->next)
    if (strcmp (play_list, l->name) == 0)
      return i;
  return -1;
}

gboolean
play_lists_iter_of (PlayLists *pls, GtkTreeIter *iter, const char *play_list)
{
  int i = play_lists_index_of (pls, play_list);
  if (i == -1)
    return FALSE;

  ITER_INIT (pls, iter, i);
  return TRUE;
}

void
play_lists_force_changed (PlayLists *pl, gint n)
{
  refresh_flush (pl);
  if (n >= pl->total)
    return;

  GtkTreePath *path = gtk_tree_path_new_from_indices (n, -1);
  GtkTreeIter iter;
  ITER_INIT (pl, &iter, n);

  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);

  gtk_tree_path_free (path);
}

/* Tree model interface.  */
static GtkTreeModelFlags
get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_LIST_ONLY;
}

static gint
get_n_columns (GtkTreeModel *tree_model)
{
  return PLS_COL_COUNT;
}

static GType
get_column_type (GtkTreeModel *tree_model, gint col)
{
  g_return_val_if_fail (0 <= col && col < PLS_COL_COUNT, G_TYPE_INVALID);

  switch (col)
    {
    case PLS_COL_INDEX:
    case PLS_COL_TRACKS:
      return G_TYPE_INT;

    case PLS_COL_NAME:
      return G_TYPE_STRING;

    default:
      g_assert (! "Bad column.");
      return 0;
    }
}

static gboolean
get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
  ITER_INIT (tree_model, iter, -1);

  if (gtk_tree_path_get_depth (path) != 1)
    return FALSE;

  gint *indices = gtk_tree_path_get_indices (path);
  if (! indices)
    return FALSE;

  refresh_flush (PLAY_LISTS (tree_model));
  if (indices[0] >= play_lists_count (PLAY_LISTS (tree_model)))
    return FALSE;

  ITER_INIT (tree_model, iter, indices[0]);

  return TRUE;
}

static GtkTreePath *
get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (iter->user_data != (gpointer) -1, NULL);

  return gtk_tree_path_new_from_indices ((int) iter->user_data, -1);
}

static void
get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column,
	   GValue *value)
{
  PlayLists *pl = PLAY_LISTS (tree_model);
  int i = (int) iter->user_data;

  g_return_if_fail (i != -1);

  refresh_flush (pl);

  g_return_if_fail (i < pl->total);

  if (column == PLS_COL_INDEX)
    {
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, i);
      return;
    }

  struct play_list *l;
  for (l = pl->play_lists; i > 0; i --, l = l->next)
    g_assert (l && l->next);

  if (column == PLS_COL_NAME)
    {
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, l->name);
      return;
    }

  switch (column)
    {
    case PLS_COL_TRACKS:
      if (l->total_version != pl->version)
	{
	  l->total = music_db_count (pl->db,
				     (strcmp (l->name, "Library") == 0
				      ? NULL : l->name),
				     NULL);
	  l->total_version = pl->version;
	}

      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, l->total);
      return;

    default:
      g_assert (! "Bad column.");
      return;
    }
}

static gboolean
iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (iter->user_data != (gpointer) -1, FALSE);

  PlayLists *pl = PLAY_LISTS (tree_model);

  int pos = 1 + (int) iter->user_data;
  if (pos >= play_lists_count (pl))
    {
      ITER_INIT (tree_model, iter, -1);
      return FALSE;
    }
  else
    ITER_INIT (tree_model, iter, pos);

  return TRUE;
}

static gboolean
iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter,
	       GtkTreeIter *parent)
{
  g_return_val_if_fail (iter->user_data != (gpointer) -1, FALSE);

  iter->stamp = -1;
  iter->user_data = (gpointer) -1;

  if (parent)
    return FALSE;

  if (play_lists_count (PLAY_LISTS (tree_model)) > 0)
    {
      ITER_INIT (tree_model, iter, 0);
      return TRUE;
    }
  return FALSE;
}

static gboolean
iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_assert (iter->user_data != (gpointer) -1);

  return FALSE;
}

static gint
iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  if (! iter)
    return play_lists_count (PLAY_LISTS (tree_model));
  return 0;
}

static gboolean
iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
		GtkTreeIter *parent, gint n)
{
  if (! parent)
    {
      if (n >= play_lists_count (PLAY_LISTS (tree_model)))
	/* Not a valid node.  */
	return FALSE;

      ITER_INIT (tree_model, iter, n);
      return TRUE;
    }

  g_assert (parent->user_data != (gpointer) -1);

  return FALSE;
}

static gboolean
iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
  g_assert (child->user_data != (gpointer) -1);
  ITER_INIT (tree_model, iter, -1);
  return FALSE;
}

static void
ref_node (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
}

static void
unref_node (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
}

static void
treemodel_iface_init (gpointer g_iface, gpointer iface_data)
{
  GtkTreeModelIface *iface = (GtkTreeModelIface *) g_iface;

  iface->get_flags = get_flags;
  iface->get_n_columns = get_n_columns;
  iface->get_column_type = get_column_type;
  iface->get_iter = get_iter;
  iface->get_path = get_path;
  iface->get_value = get_value;
  iface->iter_next = iter_next;
  iface->iter_children = iter_children;
  iface->iter_has_child = iter_has_child;
  iface->iter_n_children = iter_n_children;
  iface->iter_nth_child = iter_nth_child;
  iface->iter_parent = iter_parent;
  iface->ref_node = ref_node;
  iface->unref_node = unref_node;
}
