/* gpe-sketchbook -- a notebook to sketch your notes
 * Copyright (C) 2002, 2003, 2004, 2005 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

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

void db_update_title    (const gint id, const gchar *title);
void db_update_timestamp(const gint id, const gint timestamp);

#endif
