# $Id: Makefile.am,v 1.19 2002/12/19 15:21:27 srittau Exp $

#
# The Library
#

lib_LTLIBRARIES = libmimedir.la

INCLUDES = \
	-I$(top_srcdir) -DG_DISABLE_DEPRECATED $(GLIB_CFLAGS) \
	-DLOCALEDIR=\""$(datadir)/locale"\"


libmimedir_la_LIBADD = $(GLIB_LIBS)
libmimedir_la_LDFLAGS = -release $(MIMEDIR_MAJOR_VERSION).$(MIMEDIR_MINOR_VERSION) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)


libmimedir_la_SOURCES =	\
	mimedir-marshal.c		\
	mimedir-marshal.h		\
	mimedir-attachment.c		\
	mimedir-attendee.c		\
	mimedir-attribute.c             \
	mimedir-datetime.c		\
	mimedir-init.c			\
	mimedir-period.c		\
	mimedir-profile.c		\
	mimedir-recurrence.c		\
	mimedir-utils.c			\
	mimedir-utils.h			\
	mimedir-valarm.c		\
	mimedir-vcal.c			\
	mimedir-vcard.c			\
	mimedir-vcard-address.c		\
	mimedir-vcard-email.c		\
	mimedir-vcard-phone.c		\
	mimedir-vcomponent.c		\
	mimedir-vevent.c		\
	mimedir-vfreebusy.c		\
	mimedir-vjournal.c		\
	mimedir-vtimezone.c		\
	mimedir-vtodo.c

includedir = $(prefix)/include/mimedir-1.0/mimedir
include_HEADERS = \
	mimedir.h			\
	mimedir-attachment.h		\
	mimedir-attendee.h		\
	mimedir-attribute.h		\
	mimedir-datetime.h		\
	mimedir-init.h			\
	mimedir-period.h		\
	mimedir-profile.h		\
	mimedir-recurrence.h		\
	mimedir-valarm.h		\
	mimedir-vcal.h			\
	mimedir-vcard.h			\
	mimedir-vcard-address.h		\
	mimedir-vcard-email.h		\
	mimedir-vcard-phone.h		\
	mimedir-vcomponent.h		\
	mimedir-vevent.h		\
	mimedir-vfreebusy.h		\
	mimedir-vjournal.h		\
	mimedir-vtimezone.h		\
	mimedir-vtodo.h

EXTRA_DIST = \
	mimedir-marshal.list

#
# Signal Marshallers
#

mimedir-marshal.h: mimedir-marshal.list
	( @GLIB_GENMARSHAL@ --prefix=mimedir_marshal $< --header > mimedir-marshal.tmp \
	&& mv mimedir-marshal.tmp $@ ) \
	|| ( rm -f mimedir-marshal.tmp && exit 1 )
mimedir-marshal.c: mimedir-marshal.list mimedir-marshal.h
	( @GLIB_GENMARSHAL@ --prefix=mimedir_marshal $< --body > mimedir-marshal.tmp \
	&& mv mimedir-marshal.tmp $@ ) \
	|| ( rm -f mimedir-marshal.tmp && exit 1 )
