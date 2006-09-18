#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "dictionary.h"

gboolean
_key_equal (gconstpointer a, gconstpointer b)
{

  return a == b;

}

static void
_dictionary_update_lookup_hash (Dictionary * dict)
{

  GList *node;
  char current_char, new_char;

  if (dict == NULL || dict->list == NULL)
    return;

  assert (dict->lookup_hash != NULL);

  current_char = ((char *) dict->list->data)[0];
  g_hash_table_replace (dict->lookup_hash,
                        GINT_TO_POINTER ((int) current_char), dict->list);

  for (node = dict->list->next; node; node = node->next)
    {

      new_char = ((char *) node->data)[0];

      if (new_char != current_char)
        {

          current_char = new_char;
          g_hash_table_replace (dict->lookup_hash,
                                GINT_TO_POINTER ((int) current_char), node);

        }

    }

}

void
dictionary_destroy (Dictionary * dict)
{

  GList *node;

  if (dict == NULL)
    return;

  for (node = dict->list; node; node = node->next)
    free (node->data);
  g_list_free (dict->list);

  if (dict->last_word)
    free (dict->last_word);

  g_hash_table_destroy (dict->lookup_hash);

}


gint
_string_compare (gconstpointer a, gconstpointer b)
{

  return strcmp (a, b);

}

void
dictionary_add_word (Dictionary * dict, const char *word, gboolean rehash)
{

  GList *first_word;
  char *new_word;

  if (word == NULL || word[0] == '\0')
    return;

  new_word = strdup (word);
  dict->list = g_list_insert_sorted (dict->list, new_word, _string_compare);

  if (rehash)
    {

      first_word =
        g_hash_table_lookup (dict->lookup_hash,
                             GINT_TO_POINTER ((int) word[0]));
      if (strcmp (new_word, (char *) first_word->data) < 0)
        g_hash_table_replace (dict->lookup_hash,
                              GINT_TO_POINTER ((int) new_word[0]),
                              g_list_find (dict->list, new_word));

    }

}

void
dictionary_remove_word (Dictionary * dict, const char *word, gboolean rehash)
{

  GList *node;
  GList *deleted_node;

  if (word == NULL || word[0] == '\0')
    return;

  node =
    g_hash_table_lookup (dict->lookup_hash, GINT_TO_POINTER ((int) word[0]));

  if (node == NULL)
    return;

  if (rehash && strcmp (node->data, word) == 0)
    {

      if (node->next && ((char *) node->next->data)[0] == word[0])
        g_hash_table_replace (dict->lookup_hash,
                              GINT_TO_POINTER ((int) word[0]), node->next);
      else
        g_hash_table_remove (dict->lookup_hash,
                             GINT_TO_POINTER ((int) word[0]));

    }

  for (; node; node = node->next)
    {

      if (strcmp (node->data, word) == 0)
        {

          deleted_node = node;
          dict->list = g_list_remove_link (dict->list, node);

          free (node->data);
          g_list_free (node);

          break;

        }

    }

}

void
dictionary_add_word_list (Dictionary * dictionary, GList * list)
{

  GList *node;

  for (node = dictionary->list; node; node = node->next)
    dictionary_add_word (dictionary, (char *) node->data, FALSE);

  _dictionary_update_lookup_hash (dictionary);

}

void
dictionary_remove_word_list (Dictionary * dictionary, GList * list)
{

  GList *node;

  for (node = dictionary->list; node; node = node->next)
    dictionary_remove_word (dictionary, (char *) node->data, FALSE);

  _dictionary_update_lookup_hash (dictionary);

}



void
dictionary_print (Dictionary * dict)
{

  GList *node;

  for (node = dict->list; node; node = node->next)
    fprintf (stderr, "%s\n", (char *) node->data);

}

Dictionary *
dictionary_new ()
{

  Dictionary *dict;

  dict = (Dictionary *) calloc (1, sizeof (Dictionary));
  dict->last_word = strdup ("");
  dict->lookup_hash = g_hash_table_new (g_direct_hash, _key_equal);
  return dict;

}


Dictionary *
dictionary_new_from_file (char *filename)
{

  Dictionary *dict = dictionary_new ();

  dictionary_load (dict, filename);
  return dict;

}

Dictionary *
dictionary_new_from_list (GList * list)
{

  Dictionary *dict = dictionary_new ();

  dictionary_add_word_list (dict, list);
  return dict;

}

void
dictionary_load (Dictionary * dict, char *filename)
{

  FILE *file;
  int i;
  char buffer[100];

  if ((file = fopen (filename, "r")) == NULL)
    {

      perror ("Error loading dictionary: ");
      return;

    }

  while (fgets (buffer, 100, file))
    {

      // remove trailing \n
      i = 0;
      while (buffer[i])
        i++;
      if (i > 0)
        buffer[i - 1] = '\0';


      dict->list = g_list_prepend (dict->list, strdup (buffer));
      //dictionary_add_word( dict, buffer, FALSE );

    }

  dict->list = g_list_reverse (dict->list);

  fclose (file);

  _dictionary_update_lookup_hash (dict);

}

void
dictionary_predict_reset (Dictionary * dict)
{

  if (dict == NULL)
    return;

  free (dict->last_word);
  dict->last_word = strdup ("");
  dict->last_node = NULL;


}

char *
dictionary_predict_word (Dictionary * dict, char *word)
{

  GList *node;
  int j, last_pos, size;
  char current_char;
  char *current_word;

  if (dict == NULL)
    return NULL;


  if (word == NULL || word[0] == '\0')
    {

      free (dict->last_word);
      dict->last_word = strdup ("");
      dict->last_node = NULL;

      return NULL;

    }

  current_char = tolower (word[0]);
  node =
    g_hash_table_lookup (dict->lookup_hash,
                         GINT_TO_POINTER ((int) current_char));

  if (node == NULL)
    return NULL;

  size = strlen (word);
  last_pos = 0;

  //fprintf( stderr, "%s == %s, %d->%d\n", word, dict->last_word, dict->last_node, ( dict->last_node != NULL ? dict->last_node->next : -1 ) );

  if (dict->last_word && strcmp (dict->last_word, word) == 0
      && dict->last_node && dict->last_node->next)
    {

      current_word = (char *) dict->last_node->next->data;

      if (strncmp (current_word, word, size) == 0)
        {

          free (dict->last_word);
          dict->last_word = strdup (word);

          dict->last_node = dict->last_node->next;
          return current_word;

        }

    }

  //if( dict->last_word && strncmp( dict->last_word, word, sizeof( dict->last_word ) ) == 0 )
  //    node = dict->last_node;

  free (dict->last_word);
  dict->last_word = strdup (word);

  while (node)
    {

      current_word = (char *) node->data;

      if (current_word[0] != current_char)
        break;

      for (j = 0; j < size && current_word[j] == word[j]; j++)
        ;

      if (j == size)
        {

          dict->last_node = node;
          return current_word;

        }
      else if (j < last_pos)
        break;
      else
        last_pos = j;

      node = node->next;

    }

  free (dict->last_word);
  dict->last_word = strdup ("");
  dict->last_node = NULL;

  return NULL;

}
