/* GConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gconf-utility.h"

/*
 * Enumeration conversions
 */

gboolean
gconf_string_to_enum (GConfEnumStringPair lookup_table[],
                      const gchar* str,
                      gint* enum_value_retloc)
{
  int i = 0;

  while (lookup_table[i].str != NULL)
    { 
      if (g_ascii_strcasecmp (lookup_table[i].str, str) == 0)
        { 
          *enum_value_retloc = lookup_table[i].enum_value;
          return TRUE;
        }
      
      ++i;
    }

  return FALSE;
}

const gchar*
gconf_enum_to_string (GConfEnumStringPair lookup_table[],
                      gint enum_value)
{
  int i = 0;

  while (lookup_table[i].str != NULL)
    { 
      if (lookup_table[i].enum_value == enum_value)
        return lookup_table[i].str;

      ++i;
    }

  return NULL;
}
