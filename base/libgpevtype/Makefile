PREFIX = /usr/local
PACKAGE = libgpevtype
DEBUG = no
CVSBUILD = yes
VERSION = 0.9

LINGUAS = 

MEMBERS = tag-db vcard vtodo vevent

SONAME = $(PACKAGE).so.0

PACKAGE_CFLAGS = $(STANDARD_CFLAGS) $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags mimedir-1.0) -DG_DISABLE_DEPRECATED -fPIC
PACKAGE_LDFLAGS = $(STANDARD_LDFLAGS) $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs mimedir-1.0)
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\"

PACKAGE_CPPFLAGS += -I.

HEADERS = tag-db.h vcard.h vtodo.h vevent.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE).so $(PACKAGE).pc $(PACKAGE)-uninstalled.pc

doc: 
	make -C doc PACKAGE=$(PACKAGE)

$(PACKAGE).so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-devel: $(PACKAGE).pc
	for i in $(HEADERS); do install -m 644 -D gpe/$$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	install -d $(DESTDIR)$(PREFIX)/lib/pkgconfig
	ln -sf $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(PACKAGE).so
	install -m 644 $(PACKAGE).pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/$(PACKAGE).pc

clean:
	rm -f $(OBJS) $(SONAME) $(PACKAGE).so $(DEPS) $(PACKAGE).pc $(PACKAGE)-uninstalled.pc *.stamp

install-program: $(SONAME)
	mkdir -p $(DESTDIR)$(PREFIX)/lib/
	install $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
.PHONY: doc
