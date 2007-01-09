/*
 * ol-irc - A small irc client using GTK+
 *
 * Copyright (C) 1998, 1999 Yann Grossel [Olrick]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "olirc.h"
#include "histories.h"
#include "misc.h"

#include <string.h>

void History_Add(Virtual_Window *vw, gchar *line)
{
	if ((!vw->Hist->lines) || strcmp(line, (gchar *) g_list_last(vw->Hist->lines)->data))
	{
		gchar *dup_line = g_strdup(line);
		vw->Hist->lines = g_list_append(vw->Hist->lines, dup_line);
	}
	vw->Hist->pointer = NULL;
}

void History_Prev(Virtual_Window *vw)
{
	if (!vw->Hist->lines) return;

	if (!vw->Hist->pointer)
	{
		vw->Hist->current = g_strdup(gtk_entry_get_text((GtkEntry *) vw->entry));
		vw->Hist->pointer = g_list_last(vw->Hist->lines);
	}
	else if (vw->Hist->pointer->prev)
	{
		gchar *line = gtk_entry_get_text((GtkEntry *) vw->entry);
		if (strcmp(line, (gchar *) vw->Hist->pointer->data))
		{
			g_free(vw->Hist->pointer->data);
			vw->Hist->pointer->data = g_strdup(line);
		}
		vw->Hist->pointer = vw->Hist->pointer->prev;
	}
	else return;

	gtk_entry_set_text((GtkEntry *) vw->entry, vw->Hist->pointer->data);
}

void History_Next(Virtual_Window *vw)
{
	gchar *line = gtk_entry_get_text((GtkEntry *) vw->entry);

	if (!vw->Hist->lines) return;
	if (!vw->Hist->pointer) return;

	if (strcmp(line, (gchar *) vw->Hist->pointer->data))
	{
		g_free(vw->Hist->pointer->data);
		vw->Hist->pointer->data = g_strdup(line);
	}

	if (vw->Hist->pointer != g_list_last(vw->Hist->lines))
	{
		vw->Hist->pointer = vw->Hist->pointer->next;
		gtk_entry_set_text((GtkEntry *) vw->entry, vw->Hist->pointer->data);
	}
	else
	{
		vw->Hist->pointer = NULL;
		gtk_entry_set_text((GtkEntry *) vw->entry, vw->Hist->current);
		g_free(vw->Hist->current);
	}
}

void History_Clear(History *hist)
{
	if (!hist) return;

	while (hist->lines)
	{
		g_free(hist->lines->data);
		hist->lines = g_list_remove(hist->lines, hist->lines->data);
	}
}

/* vi: set ts=3: */

