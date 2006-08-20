#ifndef BACKEND_SIMPLE_H
#define BACKEND_SIMPLE_H

/* backend-net.c - Simple /proc-based network backend implementation.
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

/* Initializes libhal_nm backend */
void backend_net_main (void);

#endif /* BACKEND_SIMPLE */
