/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include <openobex/obex.h>

#include "obexserver.h"

#define _(x)  (x)

void
import_unknown (const char *name, const gchar *data, size_t len)
{
}

