lib_LTLIBRARIES = librewrite.la

AM_CFLAGS = -D_GNU_SOURCE

EXTRA_DIST = rewrite.map

librewrite_la_LIBADD = -ldl
librewrite_la_LDFLAGS = -Wl,--version-script,rewrite.map
librewrite_la_SOURCES = rewrite.c

include gpe-dist.am
