PREFIX = /usr/local
PACKAGE = libxsettings0
DEBUG = no

.SUFFIXES: .os

LIBXSETTINGS_SO = libXsettings.so.0
XSETTINGS_MEMBERS = xsettings-common

CPPFLAGS += -D_GNU_SOURCE

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

XSETTINGS_OBJS = $(patsubst %,%.os,$(XSETTINGS_MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS) $(XSETTINGS_MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS) $(XSETTINGS_MEMBERS))

.c.os:;
	$(CC) -o $@ -c $^ $(CFLAGS) -fPIC

all: libXsettings.so

libXsettings.so: $(LIBXSETTINGS_SO)
	ln -sf $^ $@

$(LIBXSETTINGS_SO): $(XSETTINGS_OBJS)
	$(CC) -o $@ $^ -shared -Wl,-soname -Wl,$(LIBXSETTINGS_SO) -nostartfiles -L/usr/X11R6/lib -lX11 

install: $(LIBXSETTINGS_SO)
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	install $(LIBXSETTINGS_SO) $(DESTDIR)$(PREFIX)/lib/$(LIBXSETTINGS_SO)
	strip $(DESTDIR)$(PREFIX)/lib/$(LIBXSETTINGS_SO)

clean:
	rm -f $(LIBXSETTINGS_SO) libXsettings.so
	rm -f $(XSETTINGS_OBJS) $(DEPS)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
-include $(DEPS)
