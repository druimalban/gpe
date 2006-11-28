INCLUDES = $(DEPS_CFLAGS)

EXTRA_DIST = \
	export.h \
	gpesyncd.h \
	import.h

gpesyncd_SOURCES = \
	export.c \
	main.c \
	import.c \
	export-vcal.c \
	import-vcal.c

gpesyncd_LDADD = $(DEPS_LIBS)

bin_PROGRAMS = gpesyncd
