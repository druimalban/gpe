PREFIX = /usr/local
PACKAGE = libnsqlc
#DEBUG = yes 
CVSBUILD = yes
VERSION = 0.2

LINGUAS = 

MEMBERS = nsqlc printf

SONAME = $(PACKAGE).so.0

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -I.
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) `pkg-config --cflags glib-2.0` -DG_DISABLE_DEPRECATED
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) `pkg-config --libs glib-2.0`

HEADERS = event-db.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE).so nsqlc-test

$(PACKAGE).so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS) $(PACKAGE_LDFLAGS)

nsqlc-test: nsqlc-test.c
	$(CC) $(CFLAGS) $(PACKAGE_CFLAGS) -o $@ $< -L. -lnsqlc $(LDFLAGS)

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(PACKAGE).so

clean:
	rm -f $(OBJS) $(SONAME) $(PACKAGE).so $(DEPS)

install-program: $(SONAME)
	mkdir -p $(DESTDIR)$(PREFIX)/lib/
	install $(INSTALLSTRIP) $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
