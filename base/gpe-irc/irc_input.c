#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h>

#include "dictionary.h"

GtkWidget *irc_input_entry;
GtkWidget *irc_input_quick_button;
GtkWidget *irc_input_smiley_button;
Dictionary *word_dictionary = NULL;
Dictionary *nick_dictionary = NULL;

static guchar word[100];
static char *predicted_word;
static char *predicted_word_at_cycle_start;
static Dictionary *predicted_word_cycle_dictionary;
static int predicted_word_is_nick;
static int predicted_word_cycle;
static int word_pos = 0;
static int entry_insert_text_connection_id;

static GtkListStore *irc_input_quick_list_store = NULL;
static GtkListStore *irc_input_smiley_store = NULL;
static char *quick_list_filename;
static char *smiley_list_filename;

/*
 * oh I feel sooo dirty ;)
 *
 * The function is called from an idle timer, a hack to make the irc_input_entry scroll to the right
 * if a word completes outside of the entry. It's called first to scroll to the right, then called 
 * right after to put the cursor back where it was.
 *
 * Thanks to danielk on #gtk, irc.gimp.org
 *
 */
gboolean
i_feel_dirty (gpointer data)
{
  gtk_editable_set_position (GTK_EDITABLE (irc_input_entry), (int) data + 1);

  return FALSE;
}


void
irc_input_predict_word ()
{

  int current_pos, old_pos;

  old_pos = current_pos =
    gtk_editable_get_position (GTK_EDITABLE (irc_input_entry));

  if (predicted_word)
    {

      gtk_editable_delete_text (GTK_EDITABLE (irc_input_entry), current_pos,
                                current_pos + strlen (predicted_word) -
                                word_pos + 1);

    }

  if (predicted_word_cycle)
    {


      predicted_word =
        dictionary_predict_word (predicted_word_cycle_dictionary, word);

      //fprintf( stderr, "%s, %s\n", predicted_word, predicted_word_at_cycle_start );

      if (predicted_word == predicted_word_at_cycle_start)
        {

          //fprintf( stderr, "Loop\n" );
          if (predicted_word_cycle_dictionary == nick_dictionary)
            {

              predicted_word =
                dictionary_predict_word (word_dictionary, word);

              if (predicted_word == NULL)
                predicted_word = predicted_word_at_cycle_start;
              else
                {

                  predicted_word_at_cycle_start = predicted_word;
                  predicted_word_is_nick = 0;
                  predicted_word_cycle_dictionary = word_dictionary;

                }

            }
          else
            {

              predicted_word =
                dictionary_predict_word (nick_dictionary, word);

              if (predicted_word == NULL)
                predicted_word = predicted_word_at_cycle_start;
              else
                {

                  predicted_word_at_cycle_start = predicted_word;
                  predicted_word_is_nick = 1;
                  predicted_word_cycle_dictionary = nick_dictionary;

                }

            }

        }

    }
  else
    {

      //fprintf( stderr, "hei, %s\n", word );
      predicted_word = dictionary_predict_word (nick_dictionary, word);
      predicted_word_is_nick = 0;
      if (predicted_word == NULL)
        predicted_word = dictionary_predict_word (word_dictionary, word);
      else
        predicted_word_is_nick = 1;


    }


  if (predicted_word)
    {

      gtk_signal_handler_block (irc_input_entry,
                                entry_insert_text_connection_id);
      gtk_editable_insert_text (GTK_EDITABLE (irc_input_entry),
                                &predicted_word[word_pos],
                                strlen (predicted_word) - word_pos,
                                &current_pos);

      gtk_signal_handler_unblock (irc_input_entry,
                                  entry_insert_text_connection_id);

      gtk_editable_set_position (GTK_EDITABLE (irc_input_entry), 0);

      // black magic. Code adopted to C from regexxer by danielk on #gtk, irc.gimp.org
      g_idle_add_full (G_PRIORITY_HIGH_IDLE + 17, i_feel_dirty,
                       GINT_TO_POINTER (current_pos), NULL);
      g_idle_add_full (G_PRIORITY_HIGH_IDLE + 18, i_feel_dirty,
                       GINT_TO_POINTER (old_pos), NULL);

    }

}

gboolean
irc_input_entry_insert_text (GtkEditable * editable, const gchar * text,
                             gint length, gint * position, gpointer data)
{

  int current_pos;

  if (predicted_word_cycle)
    {

      word_pos = 0;
      word[0] = '\0';
      predicted_word = NULL;

    }

  predicted_word_cycle = 0;

  if (length > 1 || isalpha (text[0]) == 0)
    {

      if (text[0] == ' ')
        {

          current_pos =
            gtk_editable_get_position (GTK_EDITABLE (irc_input_entry));
          if (predicted_word)
            {

              gtk_editable_delete_text (GTK_EDITABLE (irc_input_entry),
                                        current_pos,
                                        current_pos +
                                        strlen (predicted_word) - word_pos +
                                        1);

            }

        }

      word_pos = 0;
      predicted_word = NULL;
      dictionary_predict_reset (word_dictionary);
      dictionary_predict_reset (nick_dictionary);

    }
  else
    {

      word[word_pos++] = text[0];

      irc_input_predict_word ();

    }

  word[word_pos] = '\0';

  return TRUE;

}

void
irc_input_complete_word ()
{

  int current_pos, end_pos;
  static int old_end, old_current = -10;

  if (predicted_word)
    {

      if (predicted_word_cycle != 1)
        predicted_word_cycle = 2;

      current_pos =
        gtk_editable_get_position (GTK_EDITABLE (irc_input_entry));
      end_pos = current_pos + strlen (predicted_word) - word_pos;

      if (predicted_word_cycle == 2)
        {

          old_current = current_pos;
          predicted_word_cycle = 1;
          predicted_word_at_cycle_start = predicted_word;

          if (predicted_word_is_nick)
            predicted_word_cycle_dictionary = nick_dictionary;
          else
            predicted_word_cycle_dictionary = word_dictionary;

        }
      else if (predicted_word_cycle == 1)
        {

          gtk_editable_delete_text (GTK_EDITABLE (irc_input_entry),
                                    old_current, old_end);
          irc_input_predict_word ();

          end_pos = current_pos + strlen (predicted_word) - word_pos;

        }

      if (predicted_word_is_nick
          && (gtk_editable_get_position (GTK_EDITABLE (irc_input_entry)) ==
              word_pos || (predicted_word_cycle && old_current == word_pos)))
        {

          gtk_signal_handler_block (irc_input_entry,
                                    entry_insert_text_connection_id);
          gtk_editable_insert_text (GTK_EDITABLE (irc_input_entry), ": ", 2,
                                    &end_pos);
          gtk_signal_handler_unblock (irc_input_entry,
                                      entry_insert_text_connection_id);

        }


      // oh lord, need to hack _again_
      //gtk_editable_set_position( GTK_EDITABLE( irc_input_entry ), end_pos );
      g_idle_add_full (G_PRIORITY_HIGH_IDLE + 19, i_feel_dirty,
                       GINT_TO_POINTER (end_pos), NULL);


      old_end = end_pos;
      //fprintf( stderr, "HEI, %d\n", word_pos );

    }


}

gboolean
irc_input_entry_key_press (GtkWidget * widget, GdkEventKey * event,
                           gpointer data)
{
  if (event->keyval == GDK_Tab)
    {

      irc_input_complete_word ();

      return TRUE;

    }
  else if (event->keyval == GDK_BackSpace)
    {

      if (word_pos > 0)
        word_pos--;
      word[word_pos] = '\0';

    }
  else if (event->keyval == GDK_Left || event->keyval == GDK_Right)
    {

      word_pos = 0;
      word[word_pos] = '\0';
      predicted_word = NULL;

    }


  return FALSE;
}

/*
 * Creates an item list from the lines in the file _filename_
 *
 */
GtkListStore *
input_popup_create_list_from_file (char *filename)
{


  GtkListStore *store;
  GtkTreeIter iter;


  FILE *file;
  char buffer[100];
  int i;

  if ((file = fopen (filename, "r")) == NULL)
    {

      fprintf (stderr, "Error loading list from %s\n", filename);
      return NULL;

    }
  store = gtk_list_store_new (1, G_TYPE_STRING);


  while (fgets (buffer, 100, file))
    {

      i = 0;
      while (buffer[i])
        i++;
      if (i > 0)
        buffer[i - 1] = '\0';

      gtk_list_store_append (store, &iter);     /* Acquire an iterator */
      gtk_list_store_set (store, &iter, 0, buffer, -1);

    }

  fclose (file);

  return store;

}

gboolean
input_popup_list_button_press_event (GtkWidget * widget,
                                     GdkEventButton * event, gpointer data)
{

  GtkTreePath *path;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *smiley;
  int current_pos;

  if (gtk_tree_view_get_path_at_pos
      (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL))
    {

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
      if (gtk_tree_model_get_iter (model, &iter, path))
        {


          gtk_tree_model_get (model, &iter, 0, &smiley, -1);

          current_pos =
            gtk_editable_get_position (GTK_EDITABLE (irc_input_entry));
          gtk_signal_handler_block (irc_input_entry,
                                    entry_insert_text_connection_id);
          gtk_editable_insert_text (GTK_EDITABLE (irc_input_entry), smiley,
                                    strlen (smiley), &current_pos);
          gtk_signal_handler_unblock (irc_input_entry,
                                      entry_insert_text_connection_id);
          gtk_widget_grab_focus (irc_input_entry);
          gtk_editable_select_region (GTK_EDITABLE (irc_input_entry),
                                      current_pos, current_pos);


          g_free (smiley);

          gtk_widget_destroy ((GtkWidget *) data);

        }
      gtk_tree_path_free (path);

    }

  /* TODO: Check */
  return TRUE;
}

GtkWidget *
input_popup (GtkListStore * store)
{

  GtkWidget *popup_window;
  GtkWidget *popup_list;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  int x_pos, y_pos;

  popup_window = gtk_window_new (GTK_WINDOW_POPUP);
  //gtk_window_set_modal( GTK_WINDOW( popup_window ), TRUE );


  if (store == NULL)
    return NULL;

  popup_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_signal_connect (GTK_OBJECT (popup_list), "button-press-event",
                      GTK_SIGNAL_FUNC (input_popup_list_button_press_event),
                      popup_window);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Smileys",
                                                     renderer,
                                                     "text", 0, NULL);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (popup_list), FALSE);

  gtk_tree_view_append_column (GTK_TREE_VIEW (popup_list), column);
  gtk_container_add (GTK_CONTAINER (popup_window), popup_list);
  gtk_widget_show (popup_list);
  gtk_tree_view_columns_autosize (GTK_TREE_VIEW (popup_list));


  gtk_widget_realize (popup_window);

  gdk_window_get_origin (irc_input_entry->window, &x_pos, &y_pos);
  gtk_window_move (GTK_WINDOW (popup_window), x_pos,
                   y_pos - popup_window->allocation.height);

  gtk_widget_show_all (popup_window);

  return popup_window;

}

void
irc_input_quick_button_clicked (GtkWidget * widget, gpointer data)
{

  static GtkWidget *popup_window = NULL;

  if (popup_window)
    gtk_widget_destroy (popup_window);
  else
    {

      if (irc_input_quick_list_store == NULL)
        irc_input_quick_list_store =
          input_popup_create_list_from_file (quick_list_filename);

      popup_window = input_popup (irc_input_quick_list_store);
      g_object_add_weak_pointer (G_OBJECT (popup_window),
                                 (gpointer *) & popup_window);

    }

}

void
irc_input_smiley_button_clicked (GtkWidget * widget, gpointer data)
{

  static GtkWidget *popup_window = NULL;

  if (popup_window)
    gtk_widget_destroy (popup_window);
  else
    {

      if (irc_input_smiley_store == NULL)
        irc_input_smiley_store =
          input_popup_create_list_from_file (smiley_list_filename);

      popup_window = input_popup (irc_input_smiley_store);
      g_object_add_weak_pointer (G_OBJECT (popup_window),
                                 (gpointer *) & popup_window);

    }

}


void
irc_input_cleanup ()
{

  dictionary_destroy (word_dictionary);

  if (irc_input_smiley_store)
    gtk_list_store_clear (irc_input_smiley_store);
  if (irc_input_quick_list_store)
    gtk_list_store_clear (irc_input_quick_list_store);

}

void
irc_input_set_nick_dictionary (Dictionary * dictionary)
{

  nick_dictionary = dictionary;

}

void
irc_input_create (char *dict_filename,
                  GtkWidget * entry,
                  GtkWidget * quick_button, char *quick_filename,
                  GtkWidget * smiley_button, char *smiley_filename)
{

  word_dictionary = dictionary_new_from_file (dict_filename);

  if (quick_button != NULL && quick_filename != NULL
      && quick_filename[0] != '\0')
    {

      quick_list_filename = quick_filename;
      irc_input_quick_button = quick_button;
      gtk_signal_connect (GTK_OBJECT (irc_input_quick_button), "clicked",
                          GTK_SIGNAL_FUNC (irc_input_quick_button_clicked),
                          NULL);

    }

  if (smiley_button != NULL && smiley_filename != NULL
      && smiley_filename[0] != '\0')
    {

      smiley_list_filename = smiley_filename;
      irc_input_smiley_button = smiley_button;
      gtk_signal_connect (GTK_OBJECT (irc_input_smiley_button), "clicked",
                          GTK_SIGNAL_FUNC (irc_input_smiley_button_clicked),
                          NULL);

    }


  irc_input_entry = entry;
  entry_insert_text_connection_id =
    gtk_signal_connect (GTK_OBJECT (irc_input_entry), "insert-text",
                        GTK_SIGNAL_FUNC (irc_input_entry_insert_text), NULL);

  gtk_signal_connect (GTK_OBJECT (irc_input_entry), "key-press-event",
                      GTK_SIGNAL_FUNC (irc_input_entry_key_press), NULL);

  predicted_word = NULL;
  predicted_word_cycle = 0;

}
