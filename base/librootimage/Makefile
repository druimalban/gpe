PREFIX = /usr/local
PACKAGE = librootimage0
CVSBUILD = yes
DEBUG = no

.SUFFIXES: .os

LIBROOTIMAGE_SO = librootimage.so.0
MEMBERS = rootpixmap

CPPFLAGS += -D_GNU_SOURCE

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.os,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

.c.os:;
	$(CC) -o $@ -c $^ $(CFLAGS) -fPIC

all: librootimage.so

librootimage.so: $(LIBROOTIMAGE_SO)
	ln -sf $^ $@

$(LIBROOTIMAGE_SO): $(OBJS)
	$(CC) -o $@ $^ -shared -Wl,-soname -Wl,$(LIBROOTIMAGE_SO) -nostartfiles -L/usr/X11R6/lib -lX11 

install-program: $(LIBROOTIMAGE_SO)
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	install $(LIBROOTIMAGE_SO) $(DESTDIR)$(PREFIX)/lib/$(LIBROOTIMAGE_SO)
	strip $(DESTDIR)$(PREFIX)/lib/$(LIBROOTIMAGE_SO)

clean:
	rm -f $(LIBROOTIMAGE_SO) librootimage.so
	rm -f $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
