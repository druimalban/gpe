PREFIX = /usr/local
PACKAGE = libgpepimc
#DEBUG = yes 
CVSBUILD = yes
VERSION = 0.1

LINGUAS = 

MEMBERS = ui db

SONAME = $(PACKAGE).so.0

CFLAGS = $(GPECFLAGS) -DG_DISABLE_DEPRECATED -I../libdisplaymigration
LDFLAGS = $(GPELIBS) -L../libdisplaymigration -ldisplaymigration

CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC

ifeq ($(DEBUG),yes)
CFLAGS += -g #-O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

CPPFLAGS += -I.

HEADERS = gpe/pim-categories.h gpe/pim-categories-ui.h

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
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/$$i; done
	install -m 644 -D $(PACKAGE).pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/$(PACKAGE).pc
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(PACKAGE).so

clean:
	rm -f $(OBJS) $(SONAME) $(PACKAGE).so $(DEPS) $(PACKAGE).pc $(PACKAGE)-uninstalled.pc

install-program: $(SONAME)
	mkdir -p $(DESTDIR)$(PREFIX)/lib/
	install -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
