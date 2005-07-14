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

#ifndef GCONF_GCONF_H
#define GCONF_GCONF_H
                      
#include <glib.h>

/* Utility function converts enumerations to and from strings */
typedef struct _GConfEnumStringPair GConfEnumStringPair;
                                                   
struct _GConfEnumStringPair {                      
  gint enum_value;                                 
  const gchar* str;                                
};

gboolean     gconf_string_to_enum (GConfEnumStringPair  lookup_table[],
                                   const gchar         *str,
                                   gint                *enum_value_retloc);
const gchar* gconf_enum_to_string (GConfEnumStringPair  lookup_table[],
                                   gint                 enum_value);

#endif /*GCONF_GCONF_H*/
