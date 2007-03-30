/* RFC 2426 vCard MIME Directory Utilities
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-utils.c 94 2002-09-13 01:59:04Z srittau $
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

#include <glib.h>
#include <glib-object.h>

#include "mimedir-utils.h"


/**
 * mimedir_utils_strcat_list:
 * @list: a list of gchar pointers
 * @separator: the delimiter to use between the concatenated strings or
 *             %NULL if no delimiter is desired
 *
 * Concatenates a list of strings, optionally separating them with another
 * string. The returned string must be freed with g_free().
 *
 * Return value: the concatenated string
 **/
gchar *
mimedir_utils_strcat_list (GSList *list, const gchar *separator)
{
	GString *s;

	s = g_string_new ("");

	for (; list != NULL; list = g_slist_next (list)) {
		s = g_string_append (s, (const gchar *) list->data);
		if (list->next && separator != NULL)
			g_string_append (s, separator);
	}

	return g_string_free (s, FALSE);
}

/**
 * mimedir_utils_copy_string_list:
 * @list: a list of gchar pointers
 *
 * Makes a deep copy of the supplied #GList, i.e. it copies the list,
 * treating its elements as string pointers and copying them. The returned
 * list may be freed using mimedir_utils_free_string_list().
 *
 * Return value: the copied list
 **/
GList *
mimedir_utils_copy_string_list (const GList *list)
{
	GList *l = NULL;

	for (; list != NULL; list = g_list_next (list)) {
		const gchar *string;

		string = (const gchar *) list->data;
		l = g_list_append (l, g_strdup (string));
	}

	return l;
}

/**
 * mimedir_utils_free_string_list:
 * @list: a list of gchar pointers
 *
 * Frees the supplied #GSList and the strings it contains.
 **/
void
mimedir_utils_free_string_list (GList *list)
{
	GList *l;

	for (l = list; l != NULL; l = g_list_next (l))
		g_free ((gchar *) l->data);

	g_list_free (list);
}

/**
 * mimedir_utils_copy_string_slist:
 * @list: a list of gchar pointers
 *
 * Makes a deep copy of the supplied #GSList, i.e. it copies the list,
 * treating its elements as string pointers and copying them. The returned
 * list may be freed using mimedir_utils_free_string_slist().
 *
 * Return value: the copied list
 **/
GSList *
mimedir_utils_copy_string_slist (const GSList *list)
{
	GSList *l = NULL;

	for (; list != NULL; list = g_slist_next (list)) {
		const gchar *string;

		string = (const gchar *) list->data;
		l = g_slist_append (l, g_strdup (string));
	}

	return l;
}

/**
 * mimedir_utils_free_string_slist:
 * @list: a list of gchar pointers
 *
 * Frees the supplied #GSList and the strings it contains.
 **/
void
mimedir_utils_free_string_slist (GSList *list)
{
	GSList *l;

	for (l = list; l != NULL; l = g_slist_next (l))
		g_free ((gchar *) l->data);

	g_slist_free (list);
}

/**
 * mimedir_utils_copy_object_list:
 * @list: a list of objects
 *
 * Makes a shallow copy of the supplied #GSList, i.e. it copies the list,
 * treating its elements as objects and reffing them. The returned list
 * may be freed using mimedir_utils_free_object_list().
 *
 * Return value: the copied list
 **/
GList *
mimedir_utils_copy_object_list (const GList *list)
{
	GList *l = NULL;

	for (; list != NULL; list = g_list_next (list)) {
		GObject *object;

		g_return_val_if_fail (G_IS_OBJECT (list->data), l);

		object = G_OBJECT (list->data);
		g_object_ref (object);
		l = g_list_append (l, object);
	}

	return l;
}

/**
 * mimedir_utils_free_object_list:
 * @list: a list of objects
 *
 * Frees the supplied #GList and unrefs the objects it contains.
 **/
void
mimedir_utils_free_object_list (GList *list)
{
	GList *l;

	for (l = list; l != NULL; l = g_list_next (l)) {
		if (!l || !G_IS_OBJECT (l->data)) {
			g_critical ("list does not contain GObjects");
			return;
		}
		g_object_unref (G_OBJECT (l->data));
	}

	g_list_free (list);
}

/**
 * mimedir_utils_copy_object_slist:
 * @list: a list of objects
 *
 * Makes a shallow copy of the supplied #GSList, i.e. it copies the list,
 * treating its elements as objects and reffing them. The returned list
 * may be freed using mimedir_utils_free_object_slist().
 *
 * Return value: the copied list
 **/
GSList *
mimedir_utils_copy_object_slist (const GSList *list)
{
	GSList *l = NULL;

	for (; list != NULL; list = g_slist_next (list)) {
		GObject *object;

		g_return_val_if_fail (G_IS_OBJECT (list->data), l);

		object = G_OBJECT (list->data);
		g_object_ref (object);
		l = g_slist_append (l, object);
	}

	return l;
}

/**
 * mimedir_utils_free_object_slist:
 * @list: a list of objects
 *
 * Frees the supplied #GSList and unrefs the objects it contains.
 **/
void
mimedir_utils_free_object_slist (GSList *list)
{
	GSList *l;

	for (l = list; l != NULL; l = g_slist_next (l)) {
		if (!l || !G_IS_OBJECT (l->data)) {
			g_critical ("list does not contain GObjects");
			return;
		}
		g_object_unref (G_OBJECT (l->data));
	}

	g_slist_free (list);
}

/**
 * mimedir_utils_is_token_char:
 * @c: a character
 *
 * Checks whether the given character is a token character (i.e. in the
 * range a-z, A-Z, or 0-9 or a dash).
 *
 * Return value: %TRUE if the character is a token character, %FALSE otherwise
 **/
gboolean
mimedir_utils_is_token_char (gchar c)
{
	if ((c >= 'A' && c <= 'Z') ||
	    (c >= 'a' && c <= 'z') ||
	    (c >= '0' && c <= '9') ||
	    (c == '-'))
		return TRUE;
	else
		return FALSE;
}

/**
 * mimedir_utils_is_token:
 * @string: a string
 *
 * Checks whether the supplied string matches the criteria of a token
 * (i.e. matches the pattern 1*(ALPHA / DIGIT / "-")).
 *
 * Return value: TRUE is the string is a token, FALSE otherwise
 **/
gboolean
mimedir_utils_is_token (const gchar *string)
{
	g_return_val_if_fail (string != NULL, FALSE);

	if (string[0] == '\0')
		return FALSE;

	for (; string[0] != '\0'; string++) {
		if (!mimedir_utils_is_token_char (string[0]))
			return FALSE;
	}

	return TRUE;
}

/**
 * mimedir_utils_is_safe:
 * @string: a string
 *
 * Checks whether the supplied string contains only "safe" characters
 * (i.e. any character except CTLs, DQUOTE, ";", ":", ",").
 *
 * Return value: TRUE is the string contains only safe characters, FALSE
 *               otherwise
 **/
gboolean
mimedir_utils_is_safe (const gchar *string)
{
	for (; string[0] != '\0'; string++) {
		if ((string[0] >= 0x01 && string[0] <= 0x08) || /* CTL */
		    (string[0] >= 0x0a && string[0] <= 0x1f) || /* CTL */
		    (string[0] == '"') || (string[0] == ',') ||
		    (string[0] == ':') || (string[0] == ';'))
			return FALSE;
	}

	return TRUE;
}

/**
 * mimedir_utils_is_qsafe:
 * @string: a string
 *
 * Checks whether the supplied string contains only "qsafe" characters
 * (i.e. any character except CTLs, DQUOTE).
 *
 * Return value: TRUE is the string contains only qsafe characters, FALSE
 *               otherwise
 **/
gboolean
mimedir_utils_is_qsafe (const gchar *string)
{
	for (; string[0] != '\0'; string++) {
		if ((string[0] >= 0x01 && string[0] <= 0x08) || /* CTL */
		    (string[0] >= 0x0a && string[0] <= 0x1f) || /* CTL */
		    (string[0] == '"'))
			return FALSE;
	}

	return TRUE;
}

/**
 * mimedir_utils_compare_tokens:
 * @token1: a token
 * @token2: another token
 *
 * Compares the two tokens, ignoring any non-token character.
 *
 * Return value: %TRUE if the tokens are equal, %FALSE otherwise.
 **/
gboolean
mimedir_utils_compare_tokens (const gchar *token1, const gchar *token2)
{
	for (;; token1++, token2++) {
		/* Skip non-token characters */

		while (token1[0] && !mimedir_utils_is_token_char (token1[0]))
			token1++;
		while (token2[0] && !mimedir_utils_is_token_char (token2[0]))
			token2++;

		/* Compare */

		if (token1[0] == '\0' && token2[0] == '\0')
			return TRUE;

		if (token1[0] == token2[0])
			;
		else if (token1[0] >= 'A' && token1[0] <= 'Z' &&
			 token2[0] >= 'a' && token2[0] <= 'z') {
			if (token1[0] - 'A' != token2[0] - 'a')
				return FALSE;
		} else if (token1[0] >= 'a' && token1[0] <= 'z' &&
			   token2[0] >= 'A' && token2[0] <= 'Z') {
			if (token1[0] - 'a' != token2[0] - 'A')
				return FALSE;
		} else
			return FALSE;
	}
}

/**
 * mimedir_utils_set_property_string:
 * @dest: string destination
 * @value: a #GValue
 *
 * Frees the string located at @dest and copies the string from @value into
 * it.
 **/
void
mimedir_utils_set_property_string (gchar **dest, const GValue *value)
{
	const gchar *string;

	string = g_value_get_string (value);
	g_free (*dest);
	*dest = g_strdup (string);
}
