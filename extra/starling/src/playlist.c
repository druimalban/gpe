/* playlist.c - Playlist support.
   Copyright (C) 2007, 2008, 2010 Neal H. Walfield <neal@walfield.org>
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

#define obstack_chunk_alloc g_malloc
#define obstack_chunk_free g_free
#include <obstack.h>

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
  /* The grouping parameter.  */
  enum mdb_fields scope;

  /* The number of entries in the playlist as constrained by CONSTRAINT.  */
  int count;
  /* The number of entries in the playlist.  */
  int total;

  /* Map from indexes to UID.  */
  GArray *idx_uid_map;
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

  if (pl->idx_uid_map)
    g_array_free (pl->idx_uid_map, TRUE);
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

static inline uint64_t
now (void)
{
  struct timeval t;
  struct timezone tz;

  if (gettimeofday( &t, &tz ) == -1)
    return 0;
  return (t.tv_sec * 1000000ULL + t.tv_usec);
}

static enum mdb_fields library_order[]
  = { MDB_ARTIST, MDB_ALBUM, MDB_VOLUME_NUMBER, MDB_TRACK, 
      MDB_TITLE, MDB_SOURCE, 0 };

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
  pl->count = 0;


  GArray *old_idx_uid_map = pl->idx_uid_map;
  pl->idx_uid_map = g_array_sized_new (false, false, sizeof (int), pl->count);

  if (pl->uid_idx_hash)
    g_hash_table_destroy (pl->uid_idx_hash);
  pl->uid_idx_hash = g_hash_table_new (NULL, NULL);

  struct e
  {
    int volume_number;
    int track;
    guint uid;
    int artist_len;
    int album_len;
    int title_len;
  };

  /* Sort the results if we are viewing the library.  */
  bool do_sort = ! pl->list;

  /* We can't get the play list's count.  That is not reliable: there
     is a race; between getting the count and iterating over all of
     the elements, the number of elements may have changed!  */
  GArray *elements;
  if (do_sort)
    elements = g_array_sized_new (false, false,
				  sizeof (struct e *), pl->count);

  struct obstack obstack;
  if (do_sort)
    obstack_init (&obstack);

  pl->count = 0;

  int sorting_cb (int uid, struct music_db_info *info)
  {
    obstack_blank (&obstack, sizeof (struct e));
    struct e *saved = obstack_base (&obstack);
    saved->artist_len = (info->artist ? strlen (info->artist) : 1) + 1;
    saved->album_len = (info->album ? strlen (info->album) : 1) + 1;
    saved->title_len = (info->title ? strlen (info->title) : 1) + 1;
    int source_len = (info->source ? strlen (info->source) : 0) + 1;
    obstack_blank (&obstack,
		   saved->artist_len + saved->album_len
		   + saved->title_len + source_len);
    saved = obstack_finish (&obstack);

    saved->uid = uid;
    saved->volume_number = info->volume_number;
    saved->track = info->track;

    char *artist = (void *) &saved[1];
    char *album = artist + saved->artist_len;
    char *title = album + saved->album_len;
    char *source = title + saved->title_len;

    int i;
    if (info->artist)
      for (i = 0; i < saved->artist_len; i ++)
	artist[i] = tolower (info->artist[i]);
    else
      {
	artist[0] = 255;
	artist[1] = 0;
      }
    if (info->album)
      for (i = 0; i < saved->album_len; i ++)
	album[i] = tolower (info->album[i]);
    else
      {
	album[0] = 255;
	album[1] = 0;
      }
    if (info->title)
      for (i = 0; i < saved->title_len; i ++)
	title[i] = tolower (info->title[i]);
    else
      {
	title[0] = 255;
	title[1] = 0;
      }
    if (info->source)
      memcpy (source, info->source, source_len);
    else
      source[0] = 0;

    g_array_append_val (elements, saved);

    g_hash_table_insert (pl->uid_idx_hash,
			 (gpointer) uid, (gpointer) pl->count);
    pl->count ++;

    return 0;
  }

  int not_sorting_cb (int uid, struct music_db_info *info)
  {
    g_array_append_val (pl->idx_uid_map, uid);
    g_hash_table_insert (pl->uid_idx_hash,
 			 (gpointer) uid, (gpointer) pl->count);
    pl->count ++;

    return 0;
  }
  uint64_t s = now ();
  music_db_for_each (pl->db, pl->list,
		     do_sort ? sorting_cb : not_sorting_cb,
		     NULL, pl->scope, pl->constraint);
  uint64_t d = now () - s;
  printf ("%s: Loading: %d.%06d s\n", __func__,
	  (int) (d / 1000000ULL), (int) (d % 1000000ULL));

  if (do_sort)
    {
      /* Recall: negative value if a < b; zero if a = b; positive
	 value if a > b.  */
      gint element_compare (gconstpointer ag, gconstpointer bg)
      {
	const struct e *const*ap = ag;
	const struct e *a = *ap;
	const struct e *const*bp = bg;
	const struct e *b = *bp;

	char *a_artist = (void *) &a[1];
	char *a_album = a_artist + a->artist_len;
	char *a_title = a_album + a->album_len;
	char *a_source = a_title + a->title_len;

	char *b_artist = (void *) &b[1];
	char *b_album = b_artist + b->artist_len;
	char *b_title = b_album + b->album_len;
	char *b_source = b_title + b->title_len;

	int ret = strcmp (a_artist, b_artist);
	if (ret)
	  return ret;
	ret = strcmp (a_album, b_album);
	if (ret)
	  return ret;
	ret = a->volume_number - b->volume_number;
	if (ret)
	  return ret;
	ret = a->track - b->track;
	if (ret)
	  return ret;
	ret = strcmp (a_title, b_title);
	if (ret)
	  return ret;
	ret = strcmp (a_source, b_source);
	if (ret)
	  return ret;
      }


      s = now ();
      g_array_sort (elements, element_compare);
      d = now () - s;
      printf ("%s: Sorting: %d.%06d s\n", __func__,
	      (int) (d / 1000000ULL), (int) (d % 1000000ULL));

      int i;
      for (i = 0; i < elements->len; i ++)
	{
	  struct e *e = g_array_index (elements, struct e *, i);
	  g_array_append_val (pl->idx_uid_map, e->uid);
	}

      obstack_free (&obstack, NULL);
      g_array_free (elements, TRUE);
    }

  g_assert (pl->count == pl->idx_uid_map->len);
  g_array_set_size (pl->idx_uid_map, pl->count);

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

      gtk_tree_path_free (path);
    }
  else if (old_count > pl->count)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (pl->count, -1);

      int i;
      for (i = pl->count; i < old_count; i ++)
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (pl), path);

      gtk_tree_path_free (path);
    }

  int min_count;
  if (old_count < pl->count)
    min_count = old_count;
  else
    min_count = pl->count;

  s = now ();

  int i;
  for (i = 0; i < min_count; i ++)
    if (g_array_index (pl->idx_uid_map, int, i)
	!= g_array_index (old_idx_uid_map, int, i))
      {
	GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
	GtkTreeIter iter;
	ITER_INIT (pl, &iter, i);

	gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);

	gtk_tree_path_free (path);
      }

  if (old_idx_uid_map)
    g_array_free (old_idx_uid_map, TRUE);

  d = now () - s;
  printf ("%s: Signalling: %d.%06d s\n", __func__,
	  (int) (d / 1000000ULL), (int) (d % 1000000ULL));

  pl->total = -1;

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
int
play_list_index_to_uid (PlayList *pl, int idx)
{
  int count = play_list_count (pl);

  g_assert (idx >= 0);
  g_assert (idx < count || (idx == 0 && count == 0));

  return g_array_index (pl->idx_uid_map, int, idx);
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
changed_entry (MusicDB *db, gint uid, guint changed_mask, gpointer data)
{
  PlayList *pl = PLAY_LIST (data);

  char *m = mdb_fields_mask_to_string (changed_mask);
  printf ("%s: %u changed signal: %s\n", __func__, uid, m);
  g_free (m);

  /* XXX: in a play list, there may be several indexes that reference
     the same track.  This is only emit a row-changed for one of
     them.  */
  int idx = play_list_uid_to_index (pl, uid);
  if (idx == -1)
    return;

  GtkTreePath *path = gtk_tree_path_new_from_indices (idx, -1);
  GtkTreeIter iter;
  ITER_INIT (pl, &iter, idx);

  gtk_tree_model_row_changed (GTK_TREE_MODEL (pl), path, &iter);

  gtk_tree_path_free (path);


  /* See if we need to refresh the play list's contents.  */

  if (0 /* Is filter play list?  */)
    /* If an entry changes, the contents of a filter play list may
       change.  Be conservative and just refresh.  */
    {
      play_list_idx_uid_refresh_schedule (pl, false);
      return;
    }

  if (pl->list)
    /* A change to an entry does not effect a normal play list's
       contents.  Ignore.  */
    return;

  /* A change to an entry may effect the library's order.  If this is
     the case, we schedule a refresh.  Otherwise, we can safely ignore
     the change.  */
  static uint library_order_mask;
  if (! library_order_mask)
    {
      int i;
      for (i = 0; i < sizeof (library_order) / sizeof (library_order[0]); i ++)
	library_order_mask |= library_order[i];
    }
  
  if (! (changed_mask & library_order_mask))
    return;

  m = mdb_fields_mask_to_string (changed_mask & library_order_mask);
  printf ("uid %d changed (%u & %u => %u: %s)\n",
	  uid, changed_mask, library_order_mask,
	  changed_mask & library_order_mask, m);
  g_free (m);
  play_list_idx_uid_refresh_schedule (pl, false);
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

  printf ("%p: %s -> %s\n", pl, pl->list, list);

  g_free (pl->list);
  if (list)
    pl->list = g_strdup (list);
  else
    pl->list = NULL;

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

void
play_list_group_by (PlayList *pl, enum mdb_fields scope)
{
  if (scope == pl->scope)
    return;

  g_return_if_fail (scope == 0 || scope == MDB_ARTIST || scope == MDB_ALBUM);
  pl->scope = scope;

  play_list_idx_uid_refresh_schedule (pl, true);
}

/* Return the currenting grouping parameter.  */
enum mdb_fields
play_list_group_by_get (PlayList *pl)
{
  return pl->scope;
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

  gtk_tree_path_free (path);
}

void
play_list_remove (PlayList *pl, gint idx)
{
  if (! pl->list)
    music_db_remove (pl->db, play_list_index_to_uid (pl, idx));
  else
    music_db_play_list_remove (pl->db, pl->list, idx);
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
    case PL_COL_GENRE:
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

  int uid = play_list_index_to_uid (pl, i);

  struct music_db_info info;

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
	info.fields = MDB_SOURCE;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, info.source);
	return;
      }

    case PL_COL_ARTIST:
      {
	info.fields = MDB_ARTIST;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, info.artist);
	return;
      }

    case PL_COL_ALBUM:
      {
	info.fields = MDB_ALBUM;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, info.album);
	return;
      }

    case PL_COL_TRACK:
      {
	info.fields = MDB_TRACK;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_int (value, info.track);
	return;
      }

    case PL_COL_TITLE:
      {
	info.fields = MDB_TITLE;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, info.title);
	return;
      }

    case PL_COL_DURATION:
      {
	info.fields = MDB_DURATION;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_int (value, info.duration);
	return;
      }

    case PL_COL_GENRE:
      {
	info.fields = MDB_GENRE;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, info.genre);
	return;
      }

    case PL_COL_PLAY_COUNT:
      {
	info.fields = MDB_PLAY_COUNT;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_int (value, info.play_count);
	return;
      }

    case PL_COL_DATE_ADDED:
      {
	info.fields = MDB_DATE_ADDED;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_int (value, info.date_added);
	return;
      }

    case PL_COL_DATE_LAST_PLAYED:
      {
	info.fields = MDB_DATE_LAST_PLAYED;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_int (value, info.date_last_played);
	return;
      }

    case PL_COL_DATE_TAGS_UPDATED:
      {
	info.fields = MDB_DATE_TAGS_UPDATED;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_int (value, info.date_tags_updated);
	return;
      }

    case PL_COL_RATING:
      {
	info.fields = MDB_RATING;
	music_db_get_info (pl->db, uid, &info);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_int (value, info.rating);
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
  if (pos >= play_list_count (pl))
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
