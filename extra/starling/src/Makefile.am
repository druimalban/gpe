bin_PROGRAMS = starling starling-catalog

AM_CFLAGS = -Wall

starling_CFLAGS = $(GUI_CFLAGS) $(HILDON_CFLAGS)
starling_LDADD = $(GUI_LIBS) $(HILDON_LIBS)
starling_SOURCES = main.c \
		starling.h starling.c \
		musicdb.h musicdb.c \
		player.h player.c \
		playlist.h playlist.c \
		utils.h utils.c \
		lyrics.h lyrics.c \
		lastfm.h lastfm.c \
		errorbox.h errorbox.c \
		cache.h cache.c \
		marshal.c marshal.h \
		config.h \
		search.y \
		caption.h caption.c \
		md5.h md5.c

if HILDON
starling_SOURCES += moko-type.h \
		moko-finger-scroll.h moko-finger-scroll.c
endif

starling_catalog_CFLAGS = $(BASE_CFLAGS)
starling_catalog_LDADD = $(BASE_LIBS)
starling_catalog_SOURCES = starling-catalog.c \
		marshal.h marshal.c \
		cache.h cache.c \
		musicdb.h musicdb.c \
		config.h \
		search.y \
		caption.h caption.c

BUILT_SOURCES = marshal.h marshal.c search.h search.c
AM_YFLAGS = -d

nodist_noinst_HEADERS = marshal.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = marshal.list


marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --body $^ > $@

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --header $^ > $@
