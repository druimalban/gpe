PREFIX = /usr/local
PACKAGE = libxsettings
DEBUG = no
CVSBUILD = yes
VERSION = 0.11

.SUFFIXES: .os

LIBXSETTINGS_SO = libXsettings.so.0
XSETTINGS_MEMBERS = xsettings-common

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) `pkg-config --cflags x11`
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) `pkg-config --libs x11`

XSETTINGS_OBJS = $(patsubst %,%.os,$(XSETTINGS_MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS) $(XSETTINGS_MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS) $(XSETTINGS_MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

.c.os:;
	$(CC) -o $@ -c $^ $(CFLAGS) $(PACKAGE_CFLAGS) -fPIC

all: libXsettings.so

libXsettings.so: $(LIBXSETTINGS_SO)
	ln -sf $^ $@

$(LIBXSETTINGS_SO): $(XSETTINGS_OBJS)
	$(CC) -o $@ $^ -shared -Wl,-soname -Wl,$(LIBXSETTINGS_SO) -nostartfiles $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: $(LIBXSETTINGS_SO)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -s $(LIBXSETTINGS_SO) $(DESTDIR)$(PREFIX)/lib/$(LIBXSETTINGS_SO)

install-devel:
	install -m 644 xsettings-common.h $(DESTDIR)$(PREFIX)/include/xsettings-common.h
	ln -sf $(LIBXSETTINGS_SO) $(DESTDIR)$(PREFIX)/lib/libXsettings.so

clean:
	rm -f $(LIBXSETTINGS_SO) libXsettings.so
	rm -f $(XSETTINGS_OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
