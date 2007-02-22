bin_PROGRAMS = starling

starling_LDADD = $(DEPS_LIBS)
starling_SOURCES = main.c \
		utils.c playlist.c config.c \
		lyrics.c lastfm.c md5.c errorbox.c
		starling.h \
		utils.h playlist.h config.h \
		lyrics.h lastfm.h md5.h errorbox.h

INCLUDES = $(DEPS_CFLAGS)

nodist_starling_SOURCES = marshal.c

BUILT_SOURCES = marshal.h marshal.c

nodist_noinst_HEADERS = marshal.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = marshal.list


marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --body $^ > $@

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --header $^ > $@
