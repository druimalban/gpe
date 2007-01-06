/* gtkcolombo - colombo widget for gtk+
 * Copyright 1997 Paolo Molaro
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef __GTK_COLOMBO_H__
#define __GTK_COLOMBO_H__

#include <gtk/gtkentry.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkarrow.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_COLOMBO(obj)			GTK_CHECK_CAST (obj, gtk_colombo_get_type (), GtkColombo)
#define GTK_COLOMBO_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gtk_colombo_get_type (), GtkColomboClass)
#define GTK_IS_COLOMBO(obj)       GTK_CHECK_TYPE (obj, gtk_colombo_get_type ())

typedef struct _GtkColombo		GtkColombo;
typedef struct _GtkColomboClass	GtkColomboClass;

typedef enum {
	GTK_COLOMBO_EVERY_VALUE,
	GTK_COLOMBO_IN_LIST_VALUE
} GtkColomboFlag;

/* this func retrieves a static string from a listitem */
typedef gchar* (*GtkColomboFunc)(GtkWidget* item);
typedef gchar* (*GtkColomboExternFunc)(char *i);

struct _GtkColombo {
	GtkEntry entry;
	GtkWidget *popup;
	GtkWidget *popwin;
	GtkWidget *list;

	/*
	GtkColomboFunc func;


	gint list_change_id;
	gint flag;
	*/
	gint entry_change_id;
	GList *glist;
	GtkColomboExternFunc extfunc;
};

struct _GtkColomboClass {
	GtkEntryClass parent_class;
};

guint      gtk_colombo_get_type (void);
GtkWidget *gtk_colombo_new      (void);
void       gtk_colombo_set_func (GtkColombo* colombo, 
                               GtkColomboExternFunc func);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_COLOMBO_H__ */

