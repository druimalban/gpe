#ifndef LIBHAL_NM_BACKEND_H
#define LIBHAL_NM_BACKEND_H

/* libhal-nm-backend.h - Generic declarations for backends.
   Copyright (C) 2006 Milan Plzik <mmp@handhelds.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */



#include <glib/gtypes.h>



struct BackendIface_s
{
	gchar name[16];
	guint16	id;	/* unique ID set by libhal_nm_backend_register */

	gboolean	(*init) (void);
	void    	(*exit) (void);

	/* returns list of device UDIs */
	gchar ** (*find_by_capability)	(const gchar *capability, guint32 * devices_n);

	gboolean (*property_exists)	(const gchar *device, const gchar *property);
	gchar *	 (*get_property_string)	(const gchar *device, const gchar *property);


};

typedef struct BackendIface_s BackendIface;

extern BackendIface *backends[];
extern guint32 backends_registered;

gboolean 
libhal_nm_backend_register (BackendIface *interface);

void libhal_nm_backend_init (void);
void libhal_nm_backend_exit (void);
#endif /* LIBHAL_NM_BACKEND_H */
