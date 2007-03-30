# $Id: Makefile.am,v 1.4 2002/08/24 17:06:05 srittau Exp $

bin_PROGRAMS = ical-dump vcard-dump vcard-normalize

INCLUDES = -I$(top_srcdir) $(GLIB_CFLAGS) -DGLOCALEDIR=\"$(datadir)/locale\"
LDADD = $(top_builddir)/mimedir/libmimedir.la $(GLIB_LIBS)

ical_dump_SOURCES = ical-dump.c
vcard_dump_SOURCES = vcard-dump.c
vcard_normalize_SOURCES = vcard-normalize.c
