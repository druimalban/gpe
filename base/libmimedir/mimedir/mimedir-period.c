/* RFC 2445 iCal Period Object
 * Copyright (C) 2002, 2003  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-period.c 147 2003-07-08 14:49:37Z srittau $
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libintl.h>

#include "mimedir-period.h"
#include "mimedir-vfreebusy.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_period_class_init		(MIMEDirPeriodClass	*klass);
static void	 mimedir_period_init		(MIMEDirPeriod	*period);
static void	 mimedir_period_dispose		(GObject		*object);
static void	 mimedir_period_set_property	(GObject		*object,
						 guint			 property_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	 mimedir_period_get_property	(GObject		*object,
						 guint			 property_id,
						 GValue			*value,
						 GParamSpec		*pspec);


enum {
	PROP_FBTYPE = 1
};

struct _MIMEDirPeriodPriv {
	MIMEDirVFreeBusyType fbtype;
};

static GObjectClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_period_get_type (void)
{
	static GType mimedir_period_type = 0;

	if (!mimedir_period_type) {
		static const GTypeInfo mimedir_period_info = {
			sizeof (MIMEDirPeriodClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_period_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirPeriod),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_period_init,
		};

		mimedir_period_type = g_type_register_static (G_TYPE_OBJECT,
								  "MIMEDirPeriod",
								  &mimedir_period_info,
								  0);
	}

	return mimedir_period_type;
}


static void
mimedir_period_class_init (MIMEDirPeriodClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_PERIOD_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_period_dispose;
	gobject_class->set_property = mimedir_period_set_property;
	gobject_class->get_property = mimedir_period_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Properties */

	pspec = g_param_spec_uint ("fbtype",
				   _("Free/busy type"),
				   _("Whether this object signifies a free or a busy period"),
				   MIMEDIR_VFREEBUSY_FREE,
				   MIMEDIR_VFREEBUSY_BUSY_TENTATIVE,
				   MIMEDIR_VFREEBUSY_BUSY,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_FBTYPE, pspec);
}


static void
mimedir_period_init (MIMEDirPeriod *period)
{
	MIMEDirPeriodPriv *priv;

	g_return_if_fail (period != NULL);
	g_return_if_fail (MIMEDIR_IS_PERIOD (period));

	priv = g_new0 (MIMEDirPeriodPriv, 1);
	period->priv = priv;
}


static void
mimedir_period_dispose (GObject *object)
{
	MIMEDirPeriod *period;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_PERIOD (object));

	period = MIMEDIR_PERIOD (object);

	g_free (period->priv);
	period->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
mimedir_period_set_property (GObject		*object,
			     guint		 property_id,
			     const GValue	*value,
			     GParamSpec		*pspec)
{
	MIMEDirPeriod *period;
	MIMEDirPeriodPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_PERIOD (object));

	period = MIMEDIR_PERIOD (object);
	priv = period->priv;

	switch (property_id) {
	case PROP_FBTYPE:
		priv->fbtype = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
mimedir_period_get_property (GObject	*object,
			     guint	 property_id,
			     GValue	*value,
			     GParamSpec	*pspec)
{
	MIMEDirPeriod *period;
	MIMEDirPeriodPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_PERIOD (object));

	period = MIMEDIR_PERIOD (object);
	priv = period->priv;

	switch (property_id) {
	case PROP_FBTYPE:
		g_value_set_uint (value, priv->fbtype);
		break;
 	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Public Methods
 */

/**
 * mimedir_period_new:
 *
 * Creates a new iCal period object.
 *
 * Return value: a new period object
 **/
MIMEDirPeriod *
mimedir_period_new (void)
{
	MIMEDirPeriod *recur;

	recur = g_object_new (MIMEDIR_TYPE_PERIOD, NULL);

	return recur;
}

/**
 * mimedir_period_new_parse:
 * @string: string to parse
 *
 * Creates a new iCal period object and fills it with data retrieved from
 * parsing @string.
 *
 * Return value: a new period object
 **/
MIMEDirPeriod *
mimedir_period_new_parse (const gchar *string)
{
	MIMEDirPeriod *recur;

	recur = g_object_new (MIMEDIR_TYPE_PERIOD, NULL);

	/* FIXME */

	return recur;
}

/**
 * mimedir_period_get_as_mimedir:
 * @period: a #MIMEDirPeriod object
 *
 * Returns a string suitable for use as MIME Directory period property.
 * The returned string should be freed with g_free().
 *
 * Return value: a string suitable as MIMEDir property
 **/
gchar *
mimedir_period_get_as_mimedir (MIMEDirPeriod *period)
{
	GString *s;

	g_return_val_if_fail (period != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PERIOD (period), NULL);

	s = g_string_new (NULL);

	/* FIXME */

	return g_string_free (s, FALSE);
}
