/*
 * Copyright (C) 2002 luc pionchon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef DIALOG_H
#define DIALOG_H

/* Exemple:
   if(confirm_action_dialog_box("Delete document?","Delete")){
     delete_document();
   }
*/
gboolean confirm_action_dialog_box(gchar * text,
                                   gchar * action_button_label);

#endif
