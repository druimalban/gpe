PREFIX = /usr/local
PACKAGE = libxsettings0
DEBUG = no
CVSBUILD = yes
VERSION = 0.10

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

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif
.c.os:;
	$(CC) -o $@ -c $^ $(CFLAGS) -fPIC

all: libXsettings.so

libXsettings.so: $(LIBXSETTINGS_SO)
	ln -sf $^ $@

$(LIBXSETTINGS_SO): $(XSETTINGS_OBJS)
	$(CC) -o $@ $^ -shared -Wl,-soname -Wl,$(LIBXSETTINGS_SO) -nostartfiles -L/usr/X11R6/lib -lX11 

install-program: $(LIBXSETTINGS_SO)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -s $(LIBXSETTINGS_SO) $(DESTDIR)$(PREFIX)/lib/$(LIBXSETTINGS_SO)

clean:
	rm -f $(LIBXSETTINGS_SO) libXsettings.so
	rm -f $(XSETTINGS_OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
