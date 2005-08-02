plugindir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror -lnsqlc $(shell pkg-config --cflags libgpevtype)  @PACKAGE_CFLAGS@
AM_LIBS = $(shell pkg-config --libs libgpevtype)

config_DATA = gpe-sync

opensyncheader_HEADERS = gpe_sync.h

plugin_LTLIBRARIES = gpe_sync.la

EXTRA_DIST = \
	gpe_sync.h \
	contacts.h \
	gpe_xml.h \
	gpe_db.h \
	gpe-sync

gpe_sync_la_SOURCES = \
	gpe_sync.c \
	contacts.c \
	gpe_db.c \
	gpe_xml.c
	
gpe_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
gpe_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

## if you plan to install format libraries: ##
#formats_LTLIBRARIES = format.la

#format_la_SOURCES = format.c
#format_la_LDFLAGS = -avoid-version -export-dynamic -module
#format_la_LIBADD = @PACKAGE_LIBS@ @LIBS@
