#ifndef DB_H
#define DB_H

typedef enum {
  DB_NOTE_TYPE_SKETCH = 1,
  DB_NOTE_TYPE_TEXT   = 2,
  DB_NOTE_TYPE_VOICE  = 3,
} db_note_type;

typedef struct {
  db_note_type type;
  gint   id;
  gchar *title;
  gint   created;
  gint   updated;
  gchar *url;  
} Note;

int  db_open();
void db_close();

int  db_load_notes();

int  db_insert_note(const Note *note);
void db_delete_note(const gint id); /* before using it, delete associated file! */

#endif
