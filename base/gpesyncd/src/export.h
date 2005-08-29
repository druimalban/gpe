/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#ifndef EXPORT_H
#define EXPORT_H

#include "gpesyncd.h"
#include <gpe/tag-db.h>

gchar *get_contact (gpesyncd_context * ctx, guint uid, GError ** error);

gchar *get_event (gpesyncd_context * ctx, guint uid, GError ** error);

gchar *get_todo (gpesyncd_context * ctx, guint uid, GError ** error);

GSList *get_contact_uid_list (gpesyncd_context * ctx, GError ** error);

GSList *get_event_uid_list (gpesyncd_context * ctx, GError ** error);

GSList *get_todo_uid_list (gpesyncd_context * ctx, GError ** error);

#endif /* EXPORT_H */
