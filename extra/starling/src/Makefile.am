bin_PROGRAMS = starling

AM_CFLAGS = -Wall -O0

starling_LDADD = $(DEPS_LIBS) $(HILDON_LIBS) 
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
		md5.h md5.c

INCLUDES = $(DEPS_CFLAGS) $(HILDON_CFLAGS)

BUILT_SOURCES = marshal.h marshal.c

nodist_noinst_HEADERS = marshal.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = marshal.list


marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --body $^ > $@

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --header $^ > $@
