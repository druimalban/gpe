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

/* Management of windows */

#include "olirc.h"
#include "windows.h"
#include "menus.h"
#include "servers.h"
#include "toolbar.h"
#include "channels.h"
#include "entry.h"
#include "queries.h"
#include "dialogs.h"
#include "prefs.h"
#include "histories.h"
#include "dcc.h"
#include "misc.h"
#include "icons.h"
#include "console.h"
#include "servers_dialogs.h"
#include "prefs.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtkstyle.h>
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#ifdef HAVE_SYS_VARARGS_H
#include <sys/varargs.h>
#endif
#endif

GList *GW_List = NULL;
GList *VW_List = NULL;

GtkStyle *Style_normal = NULL;
GtkStyle *Style_event = NULL;

void VW_Page_Move(Virtual_Window *vw, gint page);
void VW_Activated(Virtual_Window *vw);

static gchar w_tmp[4096];

/* ----- Functions ------------------------------------------------------------------- */

void Sync()
{
	while(gtk_events_pending()) gtk_main_iteration();
}

void VW_Close(Virtual_Window *vw)
{
	GUI_Window *rw;

	g_return_if_fail(vw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Close(%s)\n", vw->Name);
	#endif

	g_return_if_fail(vw->Resource);
	g_return_if_fail(vw->Close);

	rw = vw->rw;

	if (vw->fd_log != -1)
	{
		/* TODO Write the current time in the file */
		close(vw->fd_log); vw->fd_log = -1;
	}

	vw->Close(vw->Resource);
}

/* ------------------------------------------------------------------------------------ */

void VW_Selected(GtkNotebook *notebook, GtkNotebookPage *page)
{
	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Selected(%s)\n", ((Virtual_Window *) gtk_object_get_user_data((GtkObject *) page->child))->Name);
	#endif

	VW_Activated((Virtual_Window *) gtk_object_get_user_data((GtkObject *) page->child));
}

gint Page_Label_button_press_event(GtkObject *w, GdkEventButton *v, gpointer data)
{
	GtkNotebookPage *page = NULL;
	GList *children;
	gint num = 0;

	if (v->button!=3) return FALSE;

	children = ((GtkNotebook *) w)->children;
	while (children)
	{
		page = children->data;
	  
		if (GTK_WIDGET_VISIBLE(page->child) &&
			page->tab_label && GTK_WIDGET_MAPPED(page->tab_label) &&
			(v->x >= page->allocation.x) &&
			(v->y >= page->allocation.y) &&
			(v->x <= (page->allocation.x + page->allocation.width)) &&
			(v->y <= (page->allocation.y + page->allocation.height))) break;

		children = children->next;
		num++;
	}

	if (!children) return FALSE;

	if (v->window != page->tab_label->window) return FALSE;

	/* We don't want that the notebook widget notice the mouse click */

	gtk_signal_emit_stop_by_name(w, "button_press_event");

	Popupmenu_Window_Display(gtk_object_get_data((GtkObject *) page->tab_label, "VW"), v);

	return TRUE;
}

gint Text_button_press_event(GtkObject *w, GdkEventButton *v, gpointer data)
{
	gtk_widget_grab_focus(((Virtual_Window *) data)->entry);

	if (v->button!=3) return FALSE;

	/* The GtkText widget is used to grab the mouse when the user right-click on it.
		To correctly popup our menu, we stop the button_press_event here */

	gtk_signal_emit_stop_by_name(w, "button_press_event");

	/* Now we can safely popup our menu */

	Popupmenu_Window_Display(data, v);

	return TRUE;
}

gint Window_key_press_event(GtkObject *w, GdkEventKey *v, gpointer data)
{
	g_return_val_if_fail(data, FALSE);
	Target_VW = (Virtual_Window *) ((GUI_Window *) data)->vw_active;
	return TRUE;
}

gint Entry_key_press_event(GtkObject *w, GdkEventKey *v, gpointer data)
{
	Virtual_Window *vw;
	gboolean alt = v->state&GDK_MOD1_MASK;
	gboolean shift = v->state&GDK_SHIFT_MASK;
	gboolean control = v->state&GDK_CONTROL_MASK;

	g_return_val_if_fail(data, FALSE);

	vw = (Virtual_Window *) data;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "Entry_key_press_event(%s)\n", vw->Name);
	#endif

	if ((vw->Cmd_Index || vw->Member_Index) && v->keyval != GDK_Tab && *(v->string))
	{
		vw->Cmd_Index = NULL;
		vw->Member_Index = NULL;
		g_free(vw->Before);
		g_free(vw->Completing);
		g_free(vw->After);
	}

	switch (v->keyval)
	{
		case GDK_Escape:
		{
			vw->Hist->pointer = NULL;
			if (vw->Hist->current) g_free_and_NULL(vw->Hist->current);
			gtk_entry_set_text((GtkEntry *) vw->entry, "");
			break;
		}
		case GDK_KP_Enter: Parse_Entry((GtkWidget *) w, vw); break;
		case GDK_Tab: Entry_Tab(vw, ((GtkEditable *) vw->entry)->current_pos, shift); break;
		case GDK_Up: History_Prev(vw); break;
		case GDK_Down: History_Next(vw); break;
		case GDK_Left: if (!alt) return FALSE; VW_Page_Move(vw, vw->Page-1); break;
		case GDK_Right: if (!alt) return FALSE; VW_Page_Move(vw, vw->Page+1); break;
		case GDK_Page_Up: VW_Scroll_Up(vw, shift); break;
		case GDK_Page_Down: VW_Scroll_Down(vw, shift); break;
		case 'u': case 'U': if (control) { VW_entry_insert(vw, "\\u"); break; } else return FALSE;
		case 'r': case 'R': if (control) { VW_entry_insert(vw, "\\r"); break; } else return FALSE;
		case 'o': case 'O': if (control) { VW_entry_insert(vw, "\\o"); break; } else return FALSE;
		case 'k': case 'K': if (control) { VW_entry_insert(vw, "\\k"); break; } else return FALSE;
		case 'b': case 'B': if (control) { VW_entry_insert(vw, "\\b"); break; } else return FALSE;
		case 'g': case 'G': if (control) { VW_entry_insert(vw, "\\g"); break; } else return FALSE;
		case 'w': if (control) { if (vw->Page < (vw->rw->nb_pages-1)) gtk_notebook_next_page((GtkNotebook *) vw->rw->Notebook); else gtk_notebook_set_page((GtkNotebook *) vw->rw->Notebook, 0); break; } else return FALSE;
		case 'W': if (control) { if (vw->Page) gtk_notebook_prev_page((GtkNotebook *) (vw->rw->Notebook)); else gtk_notebook_set_page((GtkNotebook *) vw->rw->Notebook, vw->rw->nb_pages-1); break; } else return FALSE;
		default: return FALSE;
	}

	gtk_signal_emit_stop_by_name(w, "key_press_event");
	return TRUE;
}

/* ----- Management of VW and GW : Output, creation, reorganisation ------------------- */

void VW_entry_insert(Virtual_Window *vw, gchar *text)
{
	GtkEditable *e;
	g_return_if_fail(vw);
	if (!text || !*text) return;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_entry_insert(%s, %s)\n", vw->Name, text);
	#endif

	e = &((GtkEntry *) vw->entry)->editable;
	gtk_editable_insert_text(e, text, strlen(text), &(e->current_pos));
}

void VW_Scroll_value_changed(GtkWidget *w, gpointer data)
{
	GtkAdjustment *adj = (GtkAdjustment *) w;
	Virtual_Window *vw = (Virtual_Window *) data;
	if (adj->page_size<=1 || adj->upper<=adj->page_size) vw->Scrolling = -1;
	else if (vw->Scrolling!=-1 && adj->value<adj->upper-adj->page_size) vw->Scrolling = 1;
	else vw->Scrolling = 0;

	if (vw->rw->vw_active == vw && vw->Scrolling != 1 && Style_normal)
		gtk_widget_set_style(vw->Page_Tab_Label, Style_normal);
}

void VW_Scroll_Up(Virtual_Window *vw, gboolean full)
{
	GtkAdjustment *adj = ((GtkText *) vw->Text)->vadj;
	gint v = adj->value-(adj->page_size/2);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Scroll_Up(%s, %d)\n", vw->Name, full);
	#endif

	if (full) gtk_adjustment_set_value(adj, 0);
	else gtk_adjustment_set_value(adj, (v>0)? v : 0);
}

void VW_Scroll_Down(Virtual_Window *vw, gboolean full)
{
	GtkAdjustment *adj = ((GtkText *) vw->Text)->vadj;
	gint m = adj->upper-adj->page_size;
	gint v = adj->value+(adj->page_size/2);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Scroll_Down(%s, %d)\n", vw->Name, full);
	#endif

	if (full) gtk_adjustment_set_value(adj, m);
	else gtk_adjustment_set_value(adj, (v>m)? m : v);
}

void VW_output_raw(Virtual_Window *vw, gchar *buffer)
{
	/* Output chars in a window, interpreting style codes */

	GdkFont *font = vw->font_normal;
	register gchar *sk;
	guint current = 0, fore = 1, back = 0;
	GdkColor *fg, *bg;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_output_raw(%s)\n", vw->Name);
	#endif

	do
	{
		sk = buffer;

		while (*sk && *sk!=002 && *sk!=003 && *sk!=026 && *sk!=037 && *sk!=017) sk++;

		if ((current & CODE_BOLD) && (current & CODE_UNDERLINE)) font = vw->font_bold_underline;
		else if (current & CODE_UNDERLINE) font = vw->font_underline;
		else if (current & CODE_BOLD) font = vw->font_bold;
		else font = vw->font_normal;

		fg = &Olirc->Colors[fore];
		bg = (back)? &Olirc->Colors[back] : NULL;

		if (!*sk)
		{
			gtk_text_insert((GtkText *) vw->Text, font, fg, bg, buffer, sk - buffer);
		}
		else if (*sk == 003)					/* Color */
		{
			*sk++ = 0;
			gtk_text_insert((GtkText *) vw->Text, font, fg, bg, buffer, sk - buffer - 1);

			if (!*sk || *sk<'0' || *sk>'9')
			{
				current &= ~CODE_COLOR;
				fore = 1; back = 0;
			}
			else
			{
				current |= CODE_COLOR;

				if (sk[1]>='0' && sk[1]<='9')
				{
					fore = (((guint) (*sk++)) - '0') * 10;
					fore += (((guint) (*sk++)) - '0');
				}
				else fore = ((guint) (*sk++)) - '0';

				if (*sk == ',' && sk[1]>='0' && sk[1]<='9')
				{
					sk++;
					if (sk[1]>='0' && sk[1]<='9')
					{
						back = (((guint) (*sk++)) - '0') * 10;
						back += (((guint) (*sk++)) - '0');
					}
					else back = ((guint) (*sk++)) - '0';
				}

				fore = fore%16; back = back%16;
			}
		}
		else if (*sk == 026)					/* Reverse */
		{
			guint swap;
			*sk++ = 0;
			gtk_text_insert((GtkText *) vw->Text, font, fg, bg, buffer, sk - buffer - 1);
			if (current & CODE_REVERSE) current &= ~CODE_REVERSE;
			else current |= CODE_REVERSE;
			swap = fore; fore = back; back =swap;
		}
		else if (*sk == 037)				/* Underline */
		{
			*sk++ = 0;
			gtk_text_insert((GtkText *) vw->Text, font, fg, bg, buffer, sk - buffer - 1);
			if (current & CODE_UNDERLINE) current &= ~CODE_UNDERLINE;
			else current |= CODE_UNDERLINE;
		}
		else if (*sk == 002)				/* Bold */
		{
			*sk++ = 0;
			gtk_text_insert((GtkText *) vw->Text, font, fg, bg, buffer, sk - buffer - 1);
			if (current & CODE_BOLD) current &= ~CODE_BOLD;
			else current |= CODE_BOLD;
		}
		else if (*sk == 017)				/* Plain Text */
		{
			*sk++ = 0;
			gtk_text_insert((GtkText *) vw->Text, font, fg, bg, buffer, sk - buffer - 1);
			current = 0;
			fore = 1; back = 0;
		}

		buffer = sk;

	} while (*buffer);
}

void VW_output_line(Virtual_Window *vw, struct w_line *line, gboolean highlight)
{
	static gchar buffer[4096];
	static gchar tmp[256];
	static guint8 indexes[256];
	gchar *txt, *s, *b;
	guint8 n = 0;

	g_return_if_fail(vw);
	g_return_if_fail(vw->Text);
	g_return_if_fail(line);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_output_line(%s, %p, %d)\n", vw->Name, line, highlight);
	#endif

	memset(indexes, 255, sizeof(indexes));
	s = line->args_type;
	while (*s) if (line->args[n]) indexes[(guint8) *s++] = n++; else { s++; n++; }

	/* <PREFS> The user will be able to customise all of these settings */
	/* This will allow the user to specify a certain format for a certain userhost mask
	 * (for friends...) ...
	 * by instance: prefs_get_gchar(PT_T_CHAN_NOTICE, struct pmask) -> "\0037-%n-\017 %t"
	 *
	 * %n = nickname
	 * %u = userhost
	 * %t = text
	 * %d = target name
	 * %s = server name
	 */

	switch (line->type)
	{
		case T_PRIV_MSG:			txt = "*%n* %t"; break;
		case T_CHAN_MSG:			txt = "<%n> %t"; break;
		case T_DCC_MSG:			txt = "=%n= %t"; break;

		case T_PRIV_ACTION:		txt = "\0036* %n %t"; break;
		case T_CHAN_ACTION:		txt = "\0036* %n %t"; break;
		case T_DCC_ACTION:		txt = "\0036* %n %t"; break;

		case T_PRIV_NOTICE:		txt = "\0037-%n- %t"; break;
		case T_CHAN_NOTICE:		txt = "\0037-%n-\017 %t"; break;

		case T_OWN_MSG:			txt = "\0035*\0032%n\0035*\017 %t"; break;
		case T_OWN_CHAN_MSG:		txt = "\0035<\0032%n\0035>\017 %t"; break;
		case T_OWN_DCC_MSG:		txt = "\0035=\0032%n\0035=\017 %t"; break;

		case T_OWN_ACTION:		txt = "\0035*\0032 %n\0036 %t"; break;
		case T_OWN_CHAN_ACTION:	txt = "\0035*\0032 %n\0036 %t"; break;
		case T_OWN_DCC_ACTION:	txt = "\0035*\0032 %n\0036 %t"; break;

		case T_OWN_SEND_MSG:		txt = "\0033=> %d:\0032 %t"; break;
		case T_OWN_SEND_NOTICE:	txt = "\0037=> %d:\0032 %t"; break;

		case T_WARNING:			txt = "\0035,00%t"; break;
		case T_NORMAL:				txt = "\0031,00%t"; break;
		case T_ERROR:				txt = "\0034,00%t"; break;
		case T_JOIN:				txt = "\0035-\0033 %t"; break;
		case T_PART:				txt = "\0035-\0033 %t"; break;
		case T_QUIT:				txt = "\0035-\0033 %t"; break;
		case T_KICK:				txt = "\0035-\0033 %t"; break;
		case T_MODE:				txt = "\0035-\0036 %t"; break;
		case T_NAMES:				txt = "\0035-\0036 %t"; break;
		case T_TOPIC:				txt = "\0035-\0036 %t"; break;
		case T_NICK:				txt = "\0035-\0033 %t"; break;
		case T_INVITE:				txt = "\0035-\0037 %t"; break;
		case T_CTCP_SEND:			txt = "\0033- %t"; break;
		case T_CTCP_REQUEST:		txt = "\0034- %t"; break;
		case T_CTCP_ANSWER:		txt = "\0036- %t"; break;
		case T_ANSWER:				txt = "\0035***\0036 %t"; break;

		default:	g_warning("VW_output_line(): type %d not implemented\n", line->type); return;
	}

	*buffer = 0;

	if (!vw->Empty) strcat(buffer, "\n");

	if ((vw->pmask.w_type != W_CONSOLE && vw->pmask.w_type != W_SERVER))
	{
		/* <PREFS> prefs_get_gchar(PT_DISPLAY_TIME) gives the format wanted to display the time */
		/* (if it return an empty string, we don't have to display the time) */

		struct tm *t = localtime(&(line->date));

	#ifdef HAVE_STRFTIME
		strftime(tmp, sizeof(tmp), "\00314,00%H:%M\017 ", t);
	#else
		sprintf(tmp, "\00314,00%.2d:%.2d\017 ", t->tm_hour, t->tm_min);
	#endif

		strcat(buffer, tmp);
	}

	*w_tmp = 0; s = w_tmp;

	for (; *txt; txt++)
	{
		if (*txt != '%') { *s++ = *txt; continue; }

		switch (*++txt)
		{
			case '%':	*s++ = '%'; continue;

			case 's':	/* server pointer */
			{
				Server *server = (Server *) line->args[indexes[(guint8) *txt]];
				if (!server) { *s++ = '?'; continue; }
				else { b = server->fs->Name; while ((*s = *b++)) s++; }
				continue;
			}

			default:

				if (indexes[(guint8) *txt] == 255) { *s++ = '?'; continue; }
				b = (gchar *) line->args[indexes[(guint8) *txt]];
				while ((*s = *b++)) s++;
		}
	}

	*s = 0; strcat(buffer, w_tmp);

	if (vw->fd_log != -1)
	{
		gchar *s;
		if (vw->Empty) s = Strip_Codes(buffer, CODES_ALL);
		else s = Strip_Codes(buffer + 1, CODES_ALL);
		write(vw->fd_log, s, strlen(s));
		write(vw->fd_log, "\n", 1);
		g_free(s);
	}

	gtk_text_freeze((GtkText *) vw->Text);
	VW_output_raw(vw, buffer);
	gtk_text_thaw((GtkText *) vw->Text);

	if (vw->Scrolling != 1) VW_Scroll_Down(vw, TRUE);

	vw->Empty = FALSE;
	vw->Eol = TRUE;

	/* Highlight the tab of the vw if needed */

	// if (highlight && !(Olirc->Flags & FLAG_STARTING) && (vw->rw->vw_active != vw || vw->Scrolling == 1))
	if (highlight && (vw->rw->vw_active != vw || vw->Scrolling == 1))
		if (Style_event) gtk_widget_set_style(vw->Page_Tab_Label, Style_event);
}

void VW_output(Virtual_Window *vw, guint type, gchar *fmt, ...)
{
	va_list va;
	struct w_line *line;
	guint n = 0;
	static gchar buffer[4096];
	static gchar final_fmt[256];
	gchar *f_fmt, *format;
	gpointer dummy;
	gboolean highlight = TRUE;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_output(%s, %d, %s, ...)\n", vw->Name, type, fmt);
	#endif

	line = g_malloc0(sizeof(struct w_line));
	line->date = time(NULL);
	line->type = type;
	line->new = TRUE;

	f_fmt = final_fmt;

	va_start(va, fmt);

	/*		s	server pointer
	 *		n	nickname
	 *		u userhost
	 *		d	target (channel name, nick)
	 *		t text
	 *		F vsprintf() format string
	 *		- arg for sprintf() - 
	 *		# don't highlight the window's tab
	 */

	for (; *fmt; fmt++)
	{
		switch (*fmt)
		{
			case '#':	highlight = FALSE; break;

			case 'F':
				format = va_arg(va, gchar *);

				vsprintf(buffer, format, va);
				line->args[n++] = (gpointer) g_strdup(buffer);
				*f_fmt++ = 't';
				break;

			case '-': dummy = va_arg(va, gpointer); break;

			case 's':
				line->args[n++] = va_arg(va, gpointer);
				*f_fmt++ = *fmt;
				break;

			default:
				line->args[n++] = (gpointer) g_strdup(va_arg(va, gchar *));
				*f_fmt++ = *fmt;
		}
	}

	va_end(va);

	*f_fmt = 0; line->args_type = g_strdup(final_fmt);

	vw->lines = g_list_append(vw->lines, (gpointer) line);

	VW_output_line(vw, line, highlight);
}

void VW_Activate(Virtual_Window *vw)
{
	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Activate(%s)\n", vw->Name);
	#endif

	if (vw->Page!=-1) gtk_notebook_set_page((GtkNotebook *) vw->rw->Notebook, vw->Page);
	VW_Scroll_Down(vw, TRUE);
}

void VW_Activated(Virtual_Window *vw)
{
	/* Activate a VW. */

	g_return_if_fail(vw);
	g_return_if_fail(vw->rw);
	g_return_if_fail(vw->Resource);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Activated(%s)\n", vw->Name);
	#endif

	if (vw->rw->vw_active && vw->rw->vw_active != vw)
		vw->rw->last_vw_active = vw->rw->vw_active->object_id;

	vw->rw->vw_active = vw;

	switch (vw->pmask.w_type)
	{
		case W_SERVER: ((Server *) vw->Resource)->vw_active = vw; break;
		case W_CHANNEL: ((Channel *) vw->Resource)->server->vw_active = vw; break;
		case W_QUERY: ((Query *) vw->Resource)->server->vw_active = vw; break;
	}

	if (!(Olirc->Flags & FLAG_STARTING))
		gtk_window_set_title((GtkWindow *) vw->rw->Gtk_Window, vw->Title);

	if (vw->Scrolling != 1 && Style_normal) gtk_widget_set_style(vw->Page_Tab_Label, Style_normal);

	gtk_widget_grab_focus(vw->entry);

	VW_Status(vw);
	gtk_widget_queue_resize(vw->Frame_Box);
}

void VW_clear_text_widget(Virtual_Window *vw)
{
	g_return_if_fail(vw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_clear_text_widget(%s)\n", vw->Name);
	#endif

	if (vw->Text)
	{
		GtkText *t = (GtkText *) vw->Text;
		gtk_text_freeze((GtkText *) vw->Text);
		gtk_text_set_point(t, 0);
		gtk_text_forward_delete(t, t->text_end - t->gap_size);
		vw->Eol = FALSE;
		vw->Scrolling = 0;
		vw->Empty = TRUE;
		gtk_text_thaw((GtkText *) vw->Text);
		if (Style_normal) gtk_widget_set_style(vw->Page_Tab_Label, Style_normal);
	}
}

void VW_Clear(Virtual_Window *vw)
{
	struct w_line *line;
	gchar *s;
	guint n;

	g_return_if_fail(vw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Clear(%s)\n", vw->Name);
	#endif

	VW_clear_text_widget(vw);

	while (vw->lines)
	{
		line = (struct w_line *) vw->lines->data;
		/* TODO line->args_type is ALWAYS set normally */
		if (line->args_type) for (s = line->args_type, n = 0; *s; s++, n++) if (*s != 's') g_free(line->args[n]);
		vw->lines = g_list_remove(vw->lines, vw->lines->data);
	}
}

void VW_redraw(Virtual_Window *vw)
{
	GList *l;
	struct w_line *line;

	g_return_if_fail(vw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_redraw(%s)\n", vw->Name);
	#endif

	if (!(vw->lines)) return;

	l = vw->lines;

	gtk_text_freeze((GtkText *) vw->Text);
	while (l)
	{
		line = (struct w_line *) l->data;
		VW_output_line(vw, line, FALSE);
		l = l->next;
	}
	gtk_text_thaw((GtkText *) vw->Text);
	VW_Scroll_Down(vw, TRUE);
}

void VW_Status(Virtual_Window *vw)
{
	g_return_if_fail(vw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Satus(%s)\n", vw->Name);
	#endif

	g_return_if_fail(vw->Resource);
	g_return_if_fail(vw->Infos);

	if (vw->rw->vw_active == vw) vw->Infos(vw->Resource);
}

void VW_Page_Move(Virtual_Window *vw, gint page)
{
	Virtual_Window *vw_tmp;
	GList *l;

	if (page < 0 || page >= g_list_length(vw->rw->vw_list)) return;

	gtk_widget_ref(vw->Page_Box);
	gtk_widget_ref(vw->Page_Tab_Box);

	gtk_signal_disconnect_by_func((GtkObject *) vw->rw->Notebook, (GtkSignalFunc) VW_Selected, 0);
	gtk_notebook_remove_page((GtkNotebook *) vw->rw->Notebook, vw->Page);

	l = vw->rw->vw_list;
	while (l)
	{
		vw_tmp = (Virtual_Window *) l->data;
		if (vw_tmp->Page == page)
		{
			vw_tmp->Page = vw->Page;
			break;
		}
		l = l->next;
	}
	vw->Page = page;

	gtk_notebook_insert_page((GtkNotebook *) vw->rw->Notebook, vw->Page_Box, vw->Page_Tab_Box, page);
	gtk_signal_connect((GtkObject *) vw->rw->Notebook, "switch_page", (GtkSignalFunc) VW_Selected, 0);

	VW_Activate(vw);

	gtk_widget_unref(vw->Page_Box);
	gtk_widget_unref(vw->Page_Tab_Box);
}

void VW_Add_To_GW(Virtual_Window *vw, GUI_Window *rw)
{
	/* Add a VW to a GW. The VW isn't activated. */

	g_return_if_fail(vw);
	g_return_if_fail(rw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Add_To_GW(%s,%p)\n", vw->Name, rw);
	#endif

	vw->rw = rw;
	rw->vw_list = g_list_append(rw->vw_list, vw);

	vw->Page = rw->nb_pages++;
	gtk_object_set_user_data((GtkObject *) vw->Page_Box, (gpointer) vw);

	gtk_notebook_append_page((GtkNotebook *) rw->Notebook, vw->Page_Box, vw->Page_Tab_Box);

	if (vw->Referenced)
	{
		gtk_widget_unref(vw->Page_Box);
		gtk_widget_unref(vw->Page_Tab_Box);
		vw->Referenced = FALSE;
	}

	if (!(GPrefs.Flags & PREF_HISTORIES_SEPARATED)) vw->Hist = rw->Hist;

	gtk_widget_show_all(rw->Notebook);

	VW_Status(vw);
}

void VW_Remove_From_GW(Virtual_Window *vw, gboolean keep_widgets)
{
	/* Remove a VW from its current GW. */

	GUI_Window *rw;
	gint page;

	g_return_if_fail(vw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Remove_From_GW(%s,%s)\n", vw->Name, (keep_widgets)? "TRUE" : "FALSE");
	#endif

	rw = vw->rw;
	page = vw->Page;

	if (!(GPrefs.Flags & PREF_HISTORIES_SEPARATED)) vw->Hist = NULL;

	vw->rw = NULL;
	vw->Page = -1;
		
	if (Target_VW == vw) Target_VW = NULL;

	rw->vw_list = g_list_remove(rw->vw_list, vw);
	rw->nb_pages--;

	if (keep_widgets && !vw->Referenced)
	{
		/* We reference the Page_Box and the Page_Tab_Box to prevent them from being destroyed */

		gtk_widget_ref(vw->Page_Box);
		gtk_widget_ref(vw->Page_Tab_Box);
		gtk_object_set_user_data((GtkObject *) vw->Page_Box, NULL);
		vw->Referenced = TRUE;
	}

	/* We remove the page from the notebook. If the VW is not 'referenced', it
	 * will be freed *NOW* (by VW_destroy()) */

	gtk_notebook_remove_page((GtkNotebook *) rw->Notebook, page);

	if (!(rw->nb_pages))
	{
		/* If the GW contains no more VW, we close it. */

		gtk_widget_destroy(rw->Gtk_Window);
	}
	else /* We decrement the other pages that need it */
	{
		GList *l = rw->vw_list;
		while (l)
		{
			if (((Virtual_Window *) l->data)->Page > page) ((Virtual_Window *) l->data)->Page--;
			l = l->next;
		}
	}
}

void VW_unload_fonts(Virtual_Window *vw)
{
	if (!vw) return;

	VW_clear_text_widget(vw);

	if (vw->font_normal) { gdk_font_unref(vw->font_normal); vw->font_normal = NULL; }
	if (vw->font_bold) { gdk_font_unref(vw->font_bold); vw->font_bold = NULL; }
	if (vw->font_underline) { gdk_font_unref(vw->font_underline); vw->font_underline = NULL; }
	if (vw->font_bold_underline) { gdk_font_unref(vw->font_bold_underline); vw->font_bold_underline = NULL; }
}

void VW_load_fonts(Virtual_Window *vw)
{
	if (!vw) return;
	vw->font_entry = gdk_font_load(prefs_get_gchar(PT_FONT_ENTRY, &(vw->pmask)));
	vw->font_normal = gdk_font_load(prefs_get_gchar(PT_FONT_NORMAL, &(vw->pmask)));
	vw->font_bold = gdk_font_load(prefs_get_gchar(PT_FONT_BOLD, &(vw->pmask)));
	vw->font_underline = gdk_font_load(prefs_get_gchar(PT_FONT_UNDERLINE, &(vw->pmask)));
	vw->font_bold_underline = gdk_font_load(prefs_get_gchar(PT_FONT_BOLD_UNDERLINE, &(vw->pmask)));

	VW_redraw(vw);

	if (vw->font_entry)
	{
		gint pos;
		gchar *text;
	   pos = ((GtkEditable *) vw->entry)->current_pos;
		text = g_strdup(gtk_entry_get_text((GtkEntry *) vw->entry));
		gtk_entry_set_text((GtkEntry *) vw->entry, "");
		vw->style_entry->font = vw->font_entry;
		gtk_widget_queue_resize(vw->entry);
		gtk_entry_set_text((GtkEntry *) vw->entry, text);
	   gtk_entry_set_position((GtkEntry *) vw->entry, pos);
		g_free(text);
	}
}

void VW_reload_prefs()
{
	GList *l = VW_List;
	Virtual_Window *vw;

	while (l)
	{
		vw = (Virtual_Window *) l->data;
		VW_unload_fonts(vw);
		VW_load_fonts(vw);
		l = l->next;
	}
}

void VW_destroy(GtkWidget *Widget, gpointer data)
{
	Virtual_Window *vw = (Virtual_Window *) data;

	g_return_if_fail(vw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_destroy(%s)\n", vw->Title);
	#endif

	VW_List = g_list_remove(VW_List, (gpointer) vw);

	if (GPrefs.Flags & PREF_HISTORIES_SEPARATED && vw->Hist) History_Clear(vw->Hist);
	VW_unload_fonts(vw);

	g_free(vw);
}

Virtual_Window *VW_find_by_id(guint32 id)
{
	GList *l = VW_List;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_find_by_id(%d)\n", id);
	#endif

	while (l)
	{
		if (((Virtual_Window *) l->data)->object_id == id) return (Virtual_Window *) l->data;
		l = l->next;
	}
	return NULL;
}

Virtual_Window *VW_new()
{
	static guint32 vw_id = 0;
	Virtual_Window *vw;
	
	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_new()\n");
	#endif

	vw = (Virtual_Window *) g_malloc0(sizeof(Virtual_Window));
	if (!vw) return NULL;
	vw->object_id = vw_id++;
	VW_List = g_list_append(VW_List, (gpointer) vw);
	return vw;
}

void VW_Init(Virtual_Window *vw, GUI_Window *trw, gint nframes)
{
	/*	Initialize a VW. If rw==NULL, a new GW is also created. */

	GUI_Window *rw;
	gint i;
	GtkWidget *wtmp;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Init(%s)\n", vw->Title);
	#endif

	vw->Page = -1;
	vw->Scrolling = -1;
	vw->Empty = TRUE;
	vw->fd_log = -1;

	if (!trw) /* We must create a new GW */
	{
		rw = GW_New(vw->pmask.w_type == W_CONSOLE);
		if (!(GPrefs.Flags & PREF_HISTORIES_SEPARATED)) vw->Hist = rw->Hist;
	}
	else rw = trw;

	vw->Page_Box = gtk_vbox_new(FALSE,0);

	vw->Head_Box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start((GtkBox *) vw->Page_Box, vw->Head_Box, FALSE, FALSE, 0);

	vw->TextBox = gtk_hbox_new(FALSE,0);
	vw->pane = gtk_hpaned_new ();
	gtk_paned_pack1 (GTK_PANED (vw->pane), vw->TextBox, TRUE, TRUE);
	
	gtk_box_pack_start((GtkBox *) vw->Page_Box, vw->pane, TRUE, TRUE, 0);

	vw->Text = gtk_text_new(NULL, NULL);			/* The VW Text */

	/* <PREFS> Should the lines be word-wrapped ? */

	gtk_text_set_word_wrap((GtkText *) vw->Text, 1);

	/* <PREFS> Possibility to use an horizontal scrollbar instead of wrapping the text ? */
	/*	gtk_text_set_line_wrap((GtkText *) vw->Text, 0); */

	GTK_WIDGET_UNSET_FLAGS(vw->Text, GTK_CAN_FOCUS);
	gtk_signal_connect((GtkObject *) vw->Text, "button_press_event", (GtkSignalFunc) Text_button_press_event, (gpointer) vw);
	gtk_signal_connect((GtkObject *) ((GtkText *) vw->Text)->vadj, "value_changed", (GtkSignalFunc) VW_Scroll_value_changed, (gpointer) vw);
	gtk_signal_connect((GtkObject *) vw->Text, "destroy", (GtkSignalFunc) VW_destroy, (gpointer) vw);
	gtk_box_pack_start((GtkBox *) vw->TextBox, vw->Text, TRUE, TRUE, 0);

	vw->Scroll = gtk_vscrollbar_new(((GtkText *) vw->Text)->vadj);		/* The VW Scrollbar */
	GTK_WIDGET_UNSET_FLAGS(vw->Scroll, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) vw->TextBox, vw->Scroll, FALSE, FALSE, 0);

	if (!console_xpm)
		console_xpm = gdk_pixmap_create_from_xpm_d(rw->Gtk_Window->window, &console_xpm_mask, NULL, console_xpm_data);

	if (!server_xpm)
		server_xpm = gdk_pixmap_create_from_xpm_d(rw->Gtk_Window->window, &server_xpm_mask, NULL, server_xpm_data);

	if (!channel_xpm)
		channel_xpm = gdk_pixmap_create_from_xpm_d(rw->Gtk_Window->window, &channel_xpm_mask, NULL, channel_xpm_data);

	if (!query_xpm)
		query_xpm = gdk_pixmap_create_from_xpm_d(rw->Gtk_Window->window, &query_xpm_mask, NULL, query_xpm_data);

	if (!dcc_chat_xpm)
		dcc_chat_xpm = gdk_pixmap_create_from_xpm_d(rw->Gtk_Window->window, &dcc_chat_xpm_mask, NULL, dcc_chat_xpm_data);

	vw->Page_Tab_Box = gtk_hbox_new(FALSE, 0);
	gtk_object_set_data((GtkObject *) vw->Page_Tab_Box, "VW", (gpointer) vw);

	switch(vw->pmask.w_type)
	{
		case W_CONSOLE: vw->Page_Tab_Pixmap = gtk_pixmap_new(console_xpm, console_xpm_mask); break;
		case W_SERVER: vw->Page_Tab_Pixmap = gtk_pixmap_new(server_xpm, server_xpm_mask); break;
		case W_CHANNEL: vw->Page_Tab_Pixmap = gtk_pixmap_new(channel_xpm, channel_xpm_mask); break;
		case W_QUERY: vw->Page_Tab_Pixmap = gtk_pixmap_new(query_xpm, query_xpm_mask); break;
		case W_DCC: vw->Page_Tab_Pixmap = gtk_pixmap_new(dcc_chat_xpm, dcc_chat_xpm_mask); break;
	}

	vw->Frame_Box = gtk_hbox_new(FALSE, 0);
	//	gtk_box_pack_start((GtkBox *) vw->Page_Box, vw->Frame_Box, FALSE, FALSE, 0);

	gtk_box_pack_end((GtkBox *) vw->Frame_Box, gtk_label_new(""), TRUE, TRUE, 0);

	gtk_box_pack_start((GtkBox *) vw->Page_Tab_Box, vw->Page_Tab_Pixmap, FALSE, TRUE, 0);
	gtk_misc_set_padding((GtkMisc *) vw->Page_Tab_Pixmap, 3, 0);

	vw->Page_Tab_Label = gtk_label_new(vw->Name);
	gtk_box_pack_start ((GtkBox *) vw->Page_Tab_Box, vw->Page_Tab_Label, FALSE, TRUE, 0);

	if (GPrefs.Flags & PREF_HISTORIES_SEPARATED) vw->Hist = g_malloc0(sizeof(History));

	vw->entry = gtk_entry_new_with_max_length(2048);
	gtk_box_pack_start((GtkBox *) vw->Page_Box, vw->entry, FALSE, FALSE, 0);
	gtk_signal_connect((GtkObject *) vw->entry, "key_press_event", (GtkSignalFunc) Entry_key_press_event, vw);
	gtk_signal_connect((GtkObject *) vw->entry, "activate", (GtkSignalFunc) Parse_Entry, (gpointer) vw);

	vw->style_entry = gtk_style_copy(gtk_widget_get_style(vw->entry));
	gtk_widget_set_style(vw->entry, vw->style_entry);

	VW_load_fonts(vw);

	gtk_widget_show_all(vw->Page_Tab_Box);
	gtk_widget_show_all(vw->Page_Box);

	for (i=0; i<nframes; i++)
	{
		vw->WF[i] = gtk_frame_new(NULL);
		gtk_box_pack_start((GtkBox *) vw->Frame_Box, vw->WF[i], FALSE, FALSE, 0);

		wtmp = gtk_hbox_new(FALSE, 0);
		gtk_container_add((GtkContainer *) vw->WF[i], wtmp);

		vw->WFL[i] = gtk_label_new("");
		gtk_box_pack_start((GtkBox *) wtmp, vw->WFL[i], TRUE, TRUE, 4);
	}

	VW_Add_To_GW(vw, rw);

	if (!trw) gtk_widget_show(rw->Gtk_Window);

	if (!Style_normal) Style_normal = gtk_style_copy(gtk_widget_get_style(vw->Page_Tab_Label));

	if (!Style_event && Style_normal)
	{
		Style_event = gtk_style_copy(Style_normal);
		/* <PREFS> The user should be allowed to choose the color he/she wants for highlighting */
		Style_event->fg[0] = Olirc->Colors[4];
	}
}

void VW_Move_To_GW(Virtual_Window *vw, GUI_Window *rw)
{
	/*	Move a VW into a GW. If rw==NULL, a new GW is created. */

	GUI_Window *crw;

	g_return_if_fail(vw);

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "VW_Move_To_GW(%s,%p)\n", vw->Name, rw);
	#endif
	
	crw = vw->rw;

	g_return_if_fail(crw->vw_list);

	if (!rw && !(crw->vw_list->next))
	{
		/* The source GW contains only one VW.
		 * It's stupid to recreate another new GW for this VW, so we give up silently. */
		 return;
	}

	VW_Remove_From_GW(vw, TRUE);

	if (!rw) /* We must create a new GW */
	{
		GUI_Window *rw = GW_New(FALSE);
		VW_Add_To_GW(vw, rw);
		gtk_widget_show(rw->Gtk_Window);
	}
	else
	{
		if (!rw->Tabs_visible) GW_Windows_List_Switch(rw);
		VW_Add_To_GW(vw, rw);
	}

	VW_Activate(vw);
}

/* ----- Operations on GUI Windows ---------------------------------------------------- */

gboolean GW_Activate_Last_VW(GUI_Window *rw)
{
	Virtual_Window *vw = VW_find_by_id(rw->last_vw_active);
	if (vw && vw->rw == rw) { VW_Activate(vw); return TRUE; }
	return FALSE;
}

void GW_destroy(GtkWidget *Widget, gpointer data)
{
	GUI_Window *rw = (GUI_Window *) data;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "GW_destroy()\n");
	#endif

	g_return_if_fail(rw);

	GW_List = g_list_remove(GW_List, rw);

	if (rw->Hist) History_Clear(rw->Hist);
	g_free(rw);

	if (!GW_List && !(Olirc->Flags & FLAG_QUITING)) olirc_quit(NULL, FALSE);
}

void GW_delete_event(GtkWidget *widget, GdkEvent *v, gpointer data)
{
	/* A GW has received a close request. */

	GList *l, *g = NULL;
	Virtual_Window *vw;
	GUI_Window *rw;

	g_return_if_fail(data);

	rw = (GUI_Window *) data;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "GW_Delete_event(%p)\n", rw);
	#endif

	/* If this is the last GW, we quit Ol-Irc */

	if (!(GW_List->next)) { olirc_quit(NULL, /* <PREFS> True or false here */ FALSE); return; }

	/* We check if the GW contains any connected servers */

	l = rw->vw_list;
	while (l)
	{
		vw = (Virtual_Window *) l->data; l = l->next;

		if (vw->pmask.w_type == W_SERVER && ((Server *) vw->Resource)->State == SERVER_CONNECTED)
			g = g_list_append(g, vw->Resource);
	}

	/* If the GW contains one or more connected servers, we ask the user */

	if (g) { dialog_quit_servers(g, NULL, NULL, rw, FALSE); return; }

	/* We close all VW contained in the GW */

	while (rw->vw_list) VW_Close((Virtual_Window *) rw->vw_list->data);
}

GUI_Window *GW_find_by_id(guint32 id)
{
	GList *l = GW_List;
	while (l)
	{
		if (((GUI_Window *) l->data)->object_id == id) return (GUI_Window *) l->data;
		l = l->next;
	}
	return NULL;
}

GUI_Window *GW_New(gboolean show_menus)
{
	/* Creation and setup of the window */

	static guint32 rw_id = 0;

	GUI_Window *rw = (GUI_Window *) g_malloc0(sizeof(GUI_Window));

	rw->object_id = rw_id++;

	#ifdef DEBUG_WINDOWS
	fprintf(stdout, "GW_New(%s)\n", (show_menus)? "TRUE" : "FALSE");
	#endif

	rw->Gtk_Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_container_border_width((GtkContainer *) rw->Gtk_Window, 0);

	if (!(GPrefs.Flags & PREF_HISTORIES_SEPARATED)) rw->Hist = g_malloc0(sizeof(History));

	gtk_window_set_position((GtkWindow *) rw->Gtk_Window, GTK_WIN_POS_CENTER);
	gtk_signal_connect((GtkObject *) rw->Gtk_Window, "delete_event", (GtkSignalFunc) GW_delete_event, (gpointer) rw);
	gtk_signal_connect((GtkObject *) rw->Gtk_Window, "destroy", (GtkSignalFunc) GW_destroy, (gpointer) rw);
	gtk_window_set_policy((GtkWindow *) rw->Gtk_Window, TRUE, TRUE, FALSE);
	gtk_signal_connect((GtkObject *) rw->Gtk_Window, "key_press_event", (GtkSignalFunc) Window_key_press_event, (gpointer) rw);

	/* <PREFS> The size of the window must be chosen by the user */
	gtk_widget_set_usize(rw->Gtk_Window, 800, 500);

	/*
	gtk_window_set_default_size((GtkWindow *) rw->Gtk_Window, 800, 500);
	gtk_widget_set_uposition(rw->Gtk_Window, 50, 50);
	*/

	gtk_widget_show(rw->Gtk_Window);

	/*
	gdk_window_get_position(rw->Gtk_Window->window, &x, &y);
	*/

	/* We create the main pack box for all the widgets */

	rw->Main_Box = gtk_vbox_new(FALSE, 0);

	/* Initialization of the menu bar */

	Menubar_Init(rw);
	if (show_menus) Menus_Show(rw);

	/* Creation of the tool bar */

	rw->sep1 = gtk_hseparator_new();
	gtk_box_pack_start((GtkBox *) rw->Main_Box, rw->sep1, FALSE, FALSE, 2);

	rw->Toolbar_Box = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width((GtkContainer *) rw->Toolbar_Box, 2);
	gtk_box_pack_start((GtkBox *) rw->Main_Box, rw->Toolbar_Box, FALSE, FALSE, 0);

	rw->sep2 = gtk_hseparator_new();
	gtk_box_pack_start((GtkBox *) rw->Main_Box, rw->sep2, FALSE, FALSE, 2);

	if (GPrefs.Flags & PREF_TOOLBAR_SHOW) Toolbar_Switch(rw);

	if (!GW_List) rw->Tabs_visible = TRUE;

	rw->Notebook = gtk_notebook_new();
	rw->nb_pages = 0;
	GTK_WIDGET_UNSET_FLAGS(rw->Notebook, GTK_CAN_FOCUS);
	gtk_notebook_set_tab_pos((GtkNotebook *) rw->Notebook, GPrefs.Tab_Pos);
	gtk_box_pack_start((GtkBox *) rw->Main_Box, rw->Notebook, TRUE, TRUE, 0);
	gtk_signal_connect((GtkObject *) rw->Notebook, "switch_page", (GtkSignalFunc) VW_Selected, 0);
	gtk_signal_connect((GtkObject *) rw->Notebook, "button_press_event", (GtkSignalFunc) Page_Label_button_press_event, (gpointer) rw);
	gtk_notebook_set_show_tabs((GtkNotebook *) rw->Notebook, rw->Tabs_visible);
	gtk_widget_show(rw->Notebook);

	/* We pack the main box into the window */

	gtk_container_add((GtkContainer *) rw->Gtk_Window, rw->Main_Box);

	/* We finally show the main box */

	gtk_widget_show(rw->Main_Box);

	GW_List = g_list_append(GW_List, rw);

	Toolbar_Init(rw);

	return rw;
}

void GW_Windows_List_Switch(GUI_Window *rw)
{
	rw->Tabs_visible = !rw->Tabs_visible;
	gtk_notebook_set_show_tabs((GtkNotebook *) rw->Notebook, rw->Tabs_visible);
	GTK_WIDGET_UNSET_FLAGS(rw->Notebook, GTK_CAN_FOCUS);
}

/* vi: set ts=3: */

