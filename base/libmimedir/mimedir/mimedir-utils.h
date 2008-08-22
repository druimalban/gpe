/* RFC 2426 vCard MIME Directory Utilities
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-utils.h 94 2002-09-13 01:59:04Z srittau $
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
 */

#ifndef __MIMEDIR_UTILS_H__
#define __MIMEDIR_UTILS_H__

#include <glib.h>
#include <glib-object.h>


gchar	*mimedir_utils_strcat_list (GSList *list, const gchar *separator);

GList	*mimedir_utils_copy_string_list		(const GList  *list);
void	 mimedir_utils_free_string_list		(GList	      *list);
GSList	*mimedir_utils_copy_string_slist	(const GSList *list);
void	 mimedir_utils_free_string_slist	(GSList       *list);
GList	*mimedir_utils_copy_object_list		(const GList  *list);
void	 mimedir_utils_free_object_list		(GList        *list);
GSList	*mimedir_utils_copy_object_slist	(const GSList *list);
void	 mimedir_utils_free_object_slist	(GSList       *list);

gboolean mimedir_utils_is_token_char		(gunichar c);

gboolean mimedir_utils_is_token			(const gchar *string);
gboolean mimedir_utils_is_safe			(const gchar *string);
gboolean mimedir_utils_is_qsafe			(const gchar *string);

gboolean mimedir_utils_compare_tokens		(const gchar *token1, const gchar *token2);

void	 mimedir_utils_set_property_string	(gchar **dest, const GValue *value);

#endif /* __MIMEDIR_UTILS_H__ */
