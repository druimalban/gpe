/* Dump an iCalendar file
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: ical-dump.c 213 2005-09-01 15:36:27Z srittau $
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <locale.h>
#include <libintl.h>
#include <glib.h>

#include <mimedir/mimedir-datetime.h>
#include <mimedir/mimedir-init.h>
#include <mimedir/mimedir-recurrence.h>
#include <mimedir/mimedir-vcal.h>
#include <mimedir/mimedir-vcomponent.h>
#include <mimedir/mimedir-vevent.h>
#include <mimedir/mimedir-vtodo.h>


#ifndef _
#define _(x) dgettext(GETTEXT_PACKAGE, (x))
#endif


static void
usage (const char *program_name)
{
	fprintf (stderr, _("Usage: %s ICALFILE\n"), program_name);
}


static const gchar *
frequency_to_string (MIMEDirRecurrenceFrequency freq)
{
	switch (freq) {
	case RECURRENCE_SECONDLY:
		return _("secondly");
	case RECURRENCE_MINUTELY:
		return _("minutely");
	case RECURRENCE_HOURLY:
		return _("hourly");
	case RECURRENCE_DAILY:
		return _("daily");
	case RECURRENCE_WEEKLY:
		return _("weekly");
	case RECURRENCE_MONTHLY:
		return _("monthly");
	case RECURRENCE_YEARLY:
		return _("yearly");
	default:
		return _("unknown");
	}
}


static const gchar *
unit_to_string (MIMEDirRecurrenceUnit unit)
{
	switch (unit) {
	case RECURRENCE_UNIT_SECOND:
		return _("second");
	case RECURRENCE_UNIT_MINUTE:
		return _("minute");
	case RECURRENCE_UNIT_HOUR:
		return _("hour");
	case RECURRENCE_UNIT_DAY:
		return _("day");
	case RECURRENCE_UNIT_MONTHDAY:
		return _("day of month");
	case RECURRENCE_UNIT_YEARDAY:
		return _("day of year");
	case RECURRENCE_UNIT_WEEKNO:
		return _("week number");
	case RECURRENCE_UNIT_MONTH:
		return _("month");
	default:
		return _("unknown");
	}
}


static void
print_recurrence (MIMEDirRecurrence *recur)
{
	gchar *units;
	guint freq, count, interval, unit;
	MIMEDirDateTime *until;

	g_object_get (G_OBJECT (recur),
		      "frequency", &freq,
		      "until", &until,
		      "count", &count,
		      "interval", &interval,
		      "unit", &unit,
		      "units", &units,
		      NULL);

	printf (_("  Recurrence:\n"));

	printf (_("    Frequency: %s\n"), frequency_to_string (freq));
	if (until) {
		gchar *dt = mimedir_datetime_to_string (until);
		printf (_("    Until: %s\n"), dt);
		g_free (dt);
		g_object_unref (G_OBJECT (until));
	}
	if (count) {
		printf (_("    Count: %d\n"), count);
	}
	if (interval) {
		printf (_("    Interval: %d\n"), interval);
	}
	if (unit != RECURRENCE_UNIT_NONE) {
		printf (_("    Unit: %s\n"), unit_to_string (unit));
	}
	if (units) {
		printf (_("    Units: %s\n"), units);
		g_free (units);
	}
}


static void
print_component (MIMEDirVComponent *component)
{
	gchar *s, *summary, *categories, *uid;
	guint priority, seq;
	MIMEDirDateTime *dtstart, *dtend, *due;
	MIMEDirRecurrence *recur;

	if (MIMEDIR_IS_VEVENT (component))
		printf (_("Event:\n\n"));
	else if (MIMEDIR_IS_VTODO (component))
		printf (_("Todo item:\n\n"));
	else
		printf (_("Unknown component:\n\n"));

	g_object_get (G_OBJECT (component),
		      "summary",    &summary,
		      "priority",   &priority,

		      "dtstart",    &dtstart,
		      "dtend",      &dtend,
		      "due",        &due,
		      "recurrence", &recur,

		      "uid",        &uid,

		      "sequence",   &seq,

		      NULL);

	categories = mimedir_vcomponent_get_categories_as_string (component);

	if (summary && *summary) {
		s = g_locale_from_utf8 (summary, -1, NULL, NULL, NULL);
		printf (_("  Summary: %s\n"), s);
		g_free (s);
	}
	if (categories && *categories) {
		s = g_locale_from_utf8 (categories, -1, NULL, NULL, NULL);
		printf (_("  Categories: %s\n"), s);
		g_free (s);
	}
	if (priority) {
		const gchar *severity;
		if (priority <= 4)
			severity = _("high");
		else if (priority >= 6)
			severity = _("low");
		else
			severity = _("medium");
		printf (_("  Priority: %s (%d)\n"), severity, priority);
	}
	if (dtstart && mimedir_datetime_is_valid (dtstart)) {
		s = mimedir_datetime_to_string (dtstart);
		if (s) {
			printf (_("  Start date: %s\n"), s);
			g_free (s);
		}
	}
	if (dtend && mimedir_datetime_is_valid (dtend)) {
		s = mimedir_datetime_to_string (dtend);
		if (s) {
			printf (_("  End date: %s\n"), s);
			g_free (s);

		}
	}
	if (due && mimedir_datetime_is_valid (due)) {
		s = mimedir_datetime_to_string (due);
		if (s) {
			printf (_("  Due date: %s\n"), s);
			g_free (s);
		}
	}
	if (recur) {
		print_recurrence (recur);
		g_object_unref (G_OBJECT (recur));
	}
	if (uid && *uid)
		printf (_("  Unique ID: %s\n"), uid);
	printf (_("  Sequence: %u\n"), seq);

	if (dtstart)
		g_object_unref (G_OBJECT (dtstart));
	if (dtend)
		g_object_unref (G_OBJECT (dtend));
	if (due)
		g_object_unref (G_OBJECT (due));
	g_free (summary);
	g_free (categories);
	g_free (uid);

	if (mimedir_vcomponent_get_allday (component))
		printf (_("  All day event\n"));

	printf("\n");
}

int
main (int argc, char **argv)
{
	GError *error = NULL;
	GList *callist, *l;

	/* I18n setup */

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GLOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Glib setup */

	mimedir_init ();

	/* Argument handling */

	if (argc != 2) {
		fprintf (stderr, _("%s: invalid number of arguments\n"),
			 argv[0]);
		usage (argv[0]);
		return 1;
	}

	/* Read the iCalendar list */

	callist = mimedir_vcal_read_file (argv[1], &error);

	if (error) {
		fprintf (stderr, "%s: %s\n",
			 argv[0], error->message);
		g_error_free (error);
		return 1;
	}

	/* Print the cards */

	for (l = callist; l != NULL; l = g_list_next (l)) {
		MIMEDirVCal *vcal;
		GSList *subs, *i;

		g_assert (l->data != NULL && MIMEDIR_IS_VCAL (l->data));

		vcal = MIMEDIR_VCAL (l->data);

		subs = mimedir_vcal_get_component_list (vcal);
		for (i = subs; i != NULL; i = g_slist_next (i))
			print_component (MIMEDIR_VCOMPONENT (i->data));
		mimedir_vcal_free_component_list (subs);
	}

	/* Cleanup */

	mimedir_vcal_free_list (callist);

	return 0;
}
