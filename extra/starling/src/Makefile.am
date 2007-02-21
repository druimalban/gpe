bin_PROGRAMS = starling
starling_SOURCES = callbacks.c interface.c main.c \
		utils.c playlist.c config.c \
		lyrics.c lastfm.c md5.c errorbox.c

nodist_starling_SOURCES = marshal.c

starling_CFLAGS = -I. @GTK_CFLAGS@ @GSTREAMER_CFLAGS@ @GPE_CFLAGS@ \
		@SQLITE_CFLAGS@ @LIBSOUP_CFLAGS@ \
		-DPACKAGE_LOCALE_DIR=\"@starling_localedir@\"
starling_LDADD = @GTK_LIBS@ @GSTREAMER_LIBS@ @GPE_LIBS@ -lgstaudio-0.10 \
		@SQLITE_LIBS@ @LIBSOUP_LIBS@
#Let's hope libgstadio is in /usr/lib or in gstreamer ldpath

BUILT_SOURCES = marshal.h marshal.c

noinst_HEADERS = starling.h callbacks.h interface.h \
		utils.h playlist.h config.h \
		lyrics.h lastfm.h md5.h errorbox.h

nodist_noinst_HEADERS = marshal.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = marshal.list


marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --body $^ > $@

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --header $^ > $@
