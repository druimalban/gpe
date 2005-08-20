bin_PROGRAMS = gpe-mini-browser

gpe_mini_browser_SOURCES = interface-calls.c loading-calls.c gpe-mini-browser.h misc-calls.c

if HILDON
gpe_mini_browser_SOURCES += hildon-mini-browser.c
else
gpe_mini_browser_SOURCES += gpe-mini-browser.c
endif

if BOOKMARK
gpe_mini_browser_SOURCES += db-backend.c
else
gpe_mini_browser_SOURCES +=
CFLAGS += -DNOBOOKMARKS
endif

INCLUDES = @MINIBROWSER_CFLAGS@
LDADD = @MINIBROWSER_LIBS@

include gpe-dist.am
