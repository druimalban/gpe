/* playlist.c - Playlist support.
   Copyright (C) 2007, 2008 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro <skyhusker@handhelds.org>

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
#include <gst/gst.h>
#include <gst/audio/gstaudiosink.h>

#include "playlist.h"
#include "musicdb.h"

#define ITER_INIT(model, iter, i) \
  do \
    { \
      (iter)->stamp =  GPOINTER_TO_INT ((model)); \
      (iter)->user_data = GINT_TO_POINTER ((i)); \
    } \
  while (0)

struct _PlayList {
  GObject parent;

  MusicDB *db;

  char *list;

  /* The visible entries.  */
  char *constraint;

  /* The number of entries in the playlist as constrained by CONSTRAINT.  */
  int count;
  /* The number of entries in the playlist.  */
  int total;

  /* Number of cells in the IDX_UID_MAP (COUNT <= SIZE).  */
  int size;
  /* Map from indexes to UID.  */
  int *idx_uid_map;
  /* Hash mapping UIDs to indexes.  If in queue mode, the maps to one
     index.  */
  GHashTable *uid_idx_hash;

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

static void play_list_dispose (GObject *obj);
static void play_list_finalize (GObject *object);
static void treemodel_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (PlayList, play_list, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                treemodel_iface_init));

static void
play_list_class_init (PlayListClass *klass)
{
  play_list_parent_class = g_type_class_peek_parent (klass);

  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = play_list_finalize;
  object_class->dispose = play_list_dispose;

  // PlayListClass *play_list_class = PLAY_LIST_CLASS (klass);
}

static void
play_list_init (PlayList *pl)
{
  pl->total = -1;
}

static void
play_list_dispose (GObject *obj)
{
  PlayList *pl = PLAY_LIST (obj);

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

  g_free (pl->constraint);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (play_list_parent_class)->dispose (obj);
}

static void
play_list_finalize (GObject *object)
{
  PlayList *pl = PLAY_LIST (object);

  G_OBJECT_CLASS (play_list_parent_class)->finalize (object);

  free (pl->idx_uid_map);
  if (pl->uid_idx_hash)
    g_hash_table_destroy (pl->uid_idx_hash);

  g_free (pl->list);
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
  PlayList *pl = PLAY_LIST (data);

  if (pl->reschedule_requests > 10)
    /* There have been a lot of reschedule requests recently.  Delay
       the actual reschedule a bit.  */
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

  int old_count = pl->count;

  int *old_idx_uid_map = pl->idx_uid_map;

  if (pl->uid_idx_hash)
    g_hash_table_destroy (pl->uid_idx_hash);
  pl->uid_idx_hash = g_hash_table_new (NULL, NULL);

  pl->count = music_db_count (pl->db, pl->list, pl->constraint);

  pl->size = pl->count + 128;
  pl->idx_uid_map = malloc (pl->size * sizeof (pl->idx_uid_map[0]));

  int idx = 0;

  int cb (int uid, struct music_db_info *info)
  {
    pl->idx_uid_map[idx] = uid;
    g_hash_table_insert (pl->uid_idx_hash, (gpointer) uid, (gpointer) idx);
    idx ++;

    return 0;
  }
  enum mdb_fields library_order[]
    = { MDB_ARTIST, MDB_ALBUM, MDB_TRACK, MDB_TITLE, MDB_SOURCE, 0 };
  music_db_for_each (pl->db, pl->list, cb,
		     pl->list ? NULL : library_order,
		     pl->constraint);

  /* We need to emit some signals now so the thing using this model
     will stay in sync.  We brute force it as calculating the
     differences is a) non-trivial and b) likely more expensive than
     the naive approach.  */

  if (old_count < pl->count)
    /* There are some new rows.  Add those first.  */
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (old_count, -1);
      GtkTreeIter iter;
      ITER_INIT (pl, &iter, old_count);

      int i;
      for (i = old_count; i < pl->count; i ++)
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (pl), path, &iter);

      g_free (path);
    }
  else if (old_count > pl->count)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (pl->count, -1);

      int i;
      for (i = pl->count; i < old_count; i ++)
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (pl), path);

      g_free (path);
    }

  int min_count;
  if (old_count < pl->count)
    min_count = old_count;
  else
    min_count = pl->count;

  int i;
  for (i = 0; i < min_count; i ++)
    if (pl->idx_uid_map[i] != old_idx_uid_map[i])
      {
	GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
	GtkTreeIter iter;
	ITER_INIT (pl, &iter, i);

	gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);

	g_free (path);
      }

  free (old_idx_uid_map);

  /* Remove timeout source.  */
  pl->reschedule_timeout = 0;
  pl->reschedule_requests = 0;

  gettimeofday (&pl->last_refresh, NULL);

  return FALSE;
}

/* Schedule a playlist refresh.  We do this at most once every few
   seconds.  */
static void
play_list_idx_uid_refresh_schedule (PlayList *pl, bool now)
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

/* Return the UID of the record at index IDX.  */
static int
play_list_idx_to_uid (PlayList *pl, int idx)
{
  int count = play_list_count (pl);

  g_assert (idx >= 0);
  g_assert (idx < count || (idx == 0 && count == 0));

  return pl->idx_uid_map[idx];
}

int
play_list_uid_to_index (PlayList *pl, int uid)
{
  gpointer key;
  gpointer value;
  if (! g_hash_table_lookup_extended (pl->uid_idx_hash, (gpointer) uid,
				      &key, &value))
    return -1;
  else
    return (int) value;
}

static void
new_entry (MusicDB *db, gint uid, gpointer data)
{
  PlayList *pl = PLAY_LIST (data);

  if (! pl->list)
    play_list_idx_uid_refresh_schedule (pl, false);
}

static void
changed_entry (MusicDB *db, gint uid, gpointer data)
{
  PlayList *pl = PLAY_LIST (data);

  /* In queue mode, there may be several indexes that reference the
     same track.  The refresh will catch the others.  */
  int idx = play_list_uid_to_index (pl, uid);
  if (idx == -1)
    return;

  GtkTreePath *path = gtk_tree_path_new_from_indices (idx, -1);
  GtkTreeIter iter;
  ITER_INIT (pl, &iter, idx);

  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);

  play_list_idx_uid_refresh_schedule (pl, false);

  g_free (path);
}

static void
deleted_entry (MusicDB *db, gint uid, gpointer data)
{
  PlayList *pl = PLAY_LIST (data);

  play_list_idx_uid_refresh_schedule (pl, false);
}

static void
cleared (MusicDB *db, gpointer data)
{
  PlayList *pl = PLAY_LIST (data);

  play_list_idx_uid_refresh_schedule (pl, true);
}

static void
added_to_play_list (MusicDB *db, const char *list, gint offset, gpointer data)
{
  PlayList *pl = PLAY_LIST (data);

  if (pl->list && strcmp (list, pl->list) == 0)
    play_list_idx_uid_refresh_schedule (pl, false);
}

static void
removed_from_play_list (MusicDB *db, const char *list, gint offset,
			gpointer data)
{
  PlayList *pl = PLAY_LIST (data);

  if (pl->list && strcmp (list, pl->list) == 0)
    play_list_idx_uid_refresh_schedule (pl, false);
}


PlayList *
play_list_new (MusicDB *db, const char *list)
{
  PlayList *pl = PLAY_LIST (g_object_new (PLAY_LIST_TYPE, NULL));

  if (list)
    pl->list = g_strdup (list);
  else
    pl->list = NULL;

  g_object_ref (db);
  pl->db = db;

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

  return pl;
}

void
play_list_set (PlayList *pl, const char *list)
{
  if (list && ! *list)
    list = NULL;

  if (! pl->list && ! list)
    return;
  if (pl->list && list && strcmp (pl->list, list) == 0)
    return;

  g_free (pl->list);
  if (list)
    pl->list = g_strdup (list);
  else
    pl->list = NULL;

  pl->total = -1;
  play_list_idx_uid_refresh_schedule (pl, true);
}

const char *
play_list_get (PlayList *pl)
{
  return pl->list;
}

void
play_list_constrain (PlayList *pl, const char *constraint)
{
  if (constraint && ! *constraint)
    constraint = NULL;

  if (! pl->constraint && ! constraint)
    return;
  if (pl->constraint && constraint && strcmp (pl->constraint, constraint) == 0)
    return;

  g_free (pl->constraint);
  if (constraint)
    pl->constraint = g_strdup (constraint);
  else
    pl->constraint = NULL;

  play_list_idx_uid_refresh_schedule (pl, true);
}

const char *
play_list_constraint_get (PlayList *pl)
{
  return pl->constraint;
}

gint
play_list_count (PlayList *pl)
{
  if (! pl->idx_uid_map)
    play_list_idx_uid_refresh_schedule (pl, true);

  return pl->count;
}

gint
play_list_total (PlayList *pl)
{
  if (pl->total == -1)
    pl->total = music_db_count (pl->db, play_list_get (pl), NULL);

  return pl->total;
}

void
play_list_force_changed (PlayList *pl, gint n)
{
  GtkTreePath *path = gtk_tree_path_new_from_indices (n, -1);
  GtkTreeIter iter;
  ITER_INIT (pl, &iter, n);

  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);

  g_free (path);
}

void
play_list_remove (PlayList *pl, gint idx)
{
  if (! pl->list)
    music_db_remove (pl->db, play_list_idx_to_uid (pl, idx));
  else
    music_db_play_list_remove (pl->db, pl->list, idx);
}

bool
play_list_get_info (PlayList *pl, int idx, int *uidp,
		    char **source,
		    char **artist, char **album,
		    int *track, char **title, int *duration)
{
  g_assert (idx >= 0);

  int uid = play_list_idx_to_uid (pl, idx);
  if (uid == -1)
    return false;

  if (uidp)
    *uidp = uid;

  if (! source && ! artist && ! album && ! track && ! title && ! duration)
    /* Nothing else to get.  */
    return true;

  return music_db_get_info (pl->db, uid, source, artist, album,
			    track, title, duration);
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
  return PL_COL_COUNT;
}

static GType
get_column_type (GtkTreeModel *tree_model, gint col)
{
  g_return_val_if_fail (0 <= col && col < PL_COL_COUNT, G_TYPE_INVALID);

  switch (col)
    {
    case PL_COL_INDEX:
    case PL_COL_TRACK:
    case PL_COL_DURATION:
    case PL_COL_UID:
      return G_TYPE_INT;

    case PL_COL_SOURCE:
    case PL_COL_ARTIST:
    case PL_COL_TITLE:
    case PL_COL_ALBUM:
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

  if (indices[0] >= play_list_count (PLAY_LIST (tree_model)))
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
  PlayList *pl = PLAY_LIST (tree_model);
  int i = (int) iter->user_data;

  g_return_if_fail (i != -1);

  int uid = play_list_idx_to_uid (pl, i);

  switch (column)
    {
    case PL_COL_INDEX:
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, i);
      return;

    case PL_COL_UID:
      {
	g_value_init (value, G_TYPE_INT);
	g_value_set_int (value, uid);
	return;
      }

    case PL_COL_SOURCE:
      {
	char *s;
	music_db_get_info (pl->db, uid, &s, NULL, NULL, NULL, NULL, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    case PL_COL_ARTIST:
      {
	char *s;
	music_db_get_info (pl->db, uid, NULL, &s, NULL, NULL, NULL, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    case PL_COL_ALBUM:
      {
	char *s;
	music_db_get_info (pl->db, uid, NULL, NULL, &s, NULL, NULL, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    case PL_COL_TRACK:
      {
	int t;
	music_db_get_info (pl->db, uid, NULL, NULL, NULL, &t, NULL, NULL);
	g_value_init (value, G_TYPE_INT);
	g_value_set_int (value, t);
	return;
      }

    case PL_COL_TITLE:
      {
	char *s;
	music_db_get_info (pl->db, uid, NULL, NULL, NULL, NULL, &s, NULL);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, s);
	return;
      }

    case PL_COL_DURATION:
      {
	int d;
	music_db_get_info (pl->db, uid, NULL, NULL, NULL, NULL, NULL, &d);
	g_value_init (value, G_TYPE_INT);
	g_value_set_int (value, d);
	return;
      }

    default:
      g_assert (! "Bad column.");
      return;
    }
}

static gboolean
iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  g_return_val_if_fail (iter->user_data != (gpointer) -1, FALSE);

  PlayList *pl = PLAY_LIST (tree_model);

  int pos = 1 + (int) iter->user_data;
  ITER_INIT (tree_model, iter, pos);
  if (pos == play_list_count (pl))
    {
      ITER_INIT (tree_model, iter, -1);
      return FALSE;
    }
  return TRUE;
}

static gboolean
iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter,
	       GtkTreeIter *parent)
{
  g_return_val_if_fail (iter->user_data != (gpointer) -1, FALSE);

  iter->stamp = -1;
  iter->user_data = (gpointer) -1;

  if (! parent && play_list_count (PLAY_LIST (tree_model)) > 0)
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
  g_assert (iter->user_data != (gpointer) -1);

  if (! iter)
    return play_list_count (PLAY_LIST (tree_model));
  return 0;
}

static gboolean
iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
		GtkTreeIter *parent, gint n)
{
  if (! parent)
    {
      if (n >= play_list_count (PLAY_LIST (tree_model)))
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
