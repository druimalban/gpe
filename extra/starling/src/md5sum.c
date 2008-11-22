/* md5sum.c - md5sum routine.
   Copyright (C) 2008 Neal H. Walfield <neal@walfield.org>
   Copyright (C) 2006 Alberto García Hierro <skyhusker@handhelds.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include "md5.h"
#include <glib.h>
#include <string.h>

char *
md5sum (const char *str)
{
#ifdef HAVE_G_COMPUTE_CHECKSUM_FOR_DATA
  return g_compute_checksum_for_data (G_CHECKSUM_MD5,
				      (guint8 *) str, strlen (str));
#else
  guchar md5[16];
  gchar md5str[33];
  int i;
 
  md5_buffer (str, strlen (str), md5);
  for (i = 0; i < sizeof (md5); i++)
    sprintf (md5str + 2 * i, "%02x", md5[i]);
  md5str[33] = '\0';
    
  return g_strdup(md5str);
#endif
}
