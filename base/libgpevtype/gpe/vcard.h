/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef GPE_VCARD_H
#define GPE_VCARD_H

#include <glib.h>
#include <mimedir/mimedir-vcard.h>

#include <gpe/tag-db.h>

extern MIMEDirVCard *vcard_from_tags (GSList *tags);
extern GSList *vcard_to_tags (MIMEDirVCard *vcard);

#endif
