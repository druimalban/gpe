bin_PROGRAMS = starling starling-catalog

# Annoyingly, the files in LIBOBJS are not compiled per target but once
# and with the system cflags.
AM_CFLAGS = -Wall $(BASE_CFLAGS)

starling_CFLAGS = $(GUI_CFLAGS)
starling_LDADD = $(GUI_LIBS) $(LIBOBJS)
starling_SOURCES = main.c \
		starling.h starling.c \
		musicdb.h musicdb.c \
		player.h player.c \
		playlist.h playlist.c \
		playlists.h playlists.c \
		utils.h utils.c \
		lyrics.h lyrics.c \
		lastfm.h lastfm.c \
		errorbox.h errorbox.c \
		cache.h cache.c \
		marshal.c marshal.h \
		config.h \
		search.y \
		caption.h caption.c \
		md5sum.h md5sum.c \
		sqlite.h

starling_catalog_CFLAGS = $(BASE_CFLAGS)
starling_catalog_LDADD = $(BASE_LIBS) $(LIBOBJS)
starling_catalog_SOURCES = starling-catalog.c \
		marshal.h marshal.c \
		cache.h cache.c \
		musicdb.h musicdb.c \
		config.h \
		search.y \
		caption.h caption.c \
		sqlite.h

BUILT_SOURCES = marshal.h marshal.c search.h search.c
AM_YFLAGS = -d

nodist_noinst_HEADERS = marshal.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = marshal.list md5.h

marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --body $^ > $@

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --header $^ > $@
