/*
 * gpe-sync - A plugin for the opensync framework
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#ifndef UTILS_H
#define UTILS_H

int get_type_uid (const char *string);

osync_bool parse_value_modified (gchar *string, gchar **uid, gchar **modified);

osync_bool report_change (OSyncContext *ctx, gchar *type, gchar *uid, gchar *hash, gchar *data);


#endif /* UTILS_H */
