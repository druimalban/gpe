PREFIX = /usr/local
PACKAGE = libxsettings-client0
DEBUG = no
CVSBUILD = yes
VERSION = 0.13

.SUFFIXES: .os

LIBXSETTINGS_SO = libXsettings-client.so.0
XSETTINGS_MEMBERS = xsettings-client

CPPFLAGS += -D_GNU_SOURCE

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CPPFLAGS += -I../libxsettings

XSETTINGS_OBJS = $(patsubst %,%.os,$(XSETTINGS_MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS) $(XSETTINGS_MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS) $(XSETTINGS_MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

.c.os:;
	$(CC) -o $@ -c $^ $(CFLAGS) $(CPPFLAGS) -fPIC

all: libXsettings-client.so

libXsettings-client.so: $(LIBXSETTINGS_SO)
	ln -sf $^ $@

$(LIBXSETTINGS_SO): $(XSETTINGS_OBJS)
	$(CC) -o $@ $^ -shared -Wl,-soname -Wl,$(LIBXSETTINGS_SO) -nostartfiles -L/usr/X11R6/lib -lX11 -L../libxsettings -lXsettings

install-program: $(LIBXSETTINGS_SO)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -s $(LIBXSETTINGS_SO) $(DESTDIR)$(PREFIX)/lib/$(LIBXSETTINGS_SO)

install-devel:
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 644 xsettings-client.h $(DESTDIR)$(PREFIX)/include/xsettings-client.h
	install -d $(DESTDIR)$(PREFIX)/lib
	ln -sf $(LIBXSETTINGS_SO) $(DESTDIR)$(PREFIX)/lib/libXsettings-client.so
                                
clean:
	rm -f $(LIBXSETTINGS_SO) libXsettings-client.so
	rm -f $(XSETTINGS_OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
