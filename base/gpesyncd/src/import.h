/*
 *  Copyright (C) 2005 Martin Felis <martin@silef.de>
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#ifndef IMPORT_H
#define IMPORT_H

#include "gpesyncd.h"

gboolean add_item (gpesyncd_context * ctx, guint uid, gchar * type,
		   gchar * data, GError ** error);

gboolean del_item (gpesyncd_context * ctx, guint uid, gchar * type,
		   GError ** error);

#endif /* IMPORT_H */
