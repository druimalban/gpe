/*
 * Copyright (C) 2001, 2002 Petter Knudsen (PaxAnima) <paxanima@handhelds.org>
 * Part of GPE Gallery by Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

extern image_convolve( GdkPixbuf* pixbuf, int* mask, int mask_size, int mask_divisor );
extern void blur( GdkPixbuf* pixbuf );
extern void sharpen( GdkPixbuf* pixbuf );
extern GdkPixbuf* image_rotate( GdkPixbuf* pixbuf, int degrees );
