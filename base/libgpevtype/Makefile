PREFIX = /usr/local
PACKAGE = libgpevtype
#DEBUG = yes 
CVSBUILD = yes
VERSION = 0.2

LINGUAS = 

MEMBERS = tag-db vcard vtodo vevent

SONAME = $(PACKAGE).so.0

CFLAGS = $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags mimedir-1.0) -DG_DISABLE_DEPRECATED
LDFLAGS = $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs mimedir-1.0)

CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC

ifeq ($(DEBUG),yes)
CFLAGS += -g #-O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

CPPFLAGS += -I.

HEADERS = tag-db.h vcard.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE).so $(PACKAGE).pc $(PACKAGE)-uninstalled.pc

$(PACKAGE).so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

.c.s:;
	$(CC) $(CFLAGS) -S $<

install-devel: $(PACKAGE).pc
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(PACKAGE).so

clean:
	rm -f $(OBJS) $(SONAME) $(PACKAGE).so $(DEPS) $(PACKAGE).pc $(PACKAGE)-uninstalled.pc

install-program: $(SONAME)
	mkdir -p $(DESTDIR)$(PREFIX)/lib/
	install -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
