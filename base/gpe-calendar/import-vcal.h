/* import-vcal.h - Import calendar interface.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#ifndef IMPORT_VCAL_H
#define IMPORT_VCAL_H

#include <gpe/event-db.h>

/* Import the list of files (NULL terminated) into calendars EC.
   Either may be NULL in which case the user will be prompted.  */
int import_vcal (EventCalendar *ec, const char *filename[]);

#endif
