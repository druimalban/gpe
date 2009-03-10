PREFIX = /usr
PACKAGE = libdotdesktop
DEBUG = no
VERSION = 0.11
CC?=gcc
STRIP=strip
BUILD = $(shell [ -e build ] && echo build || echo ../build)

LINGUAS = 

MEMBERS = dotdesktop hash

SONAME = libdotdesktop.so.0
CONTROL = control

CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

PACKAGE_CFLAGS = `pkg-config --cflags glib-2.0`
PACKAGE_LDFLAGS = `pkg-config --libs glib-2.0`

HEADERS = dotdesktop.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libdotdesktop.so

libdotdesktop.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(PACKAGE_LDFLAGS) $(LDFLAGS)

libdotdesktop.a: $(OBJS)
	ar cq $@ $^

.c.o:;
	$(CC) $(PACKAGE_CFLAGS) $(MY_CFLAGS) $(CFLAGS) -c $< -o $@

upstream: clean
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; cd ..; cp -a libdotdesktop libdotdesktop-$$VERSION.orig; tar czf libdotdesktop_$$VERSION.orig.tar.gz --exclude=libdotdesktop-$$VERSION.orig/debian --exclude=libdotdekstop-$$VERSION.orig/CVS --exclude=libdotdesktop-$$VERSION.orig/familiar libdotdesktop-$$VERSION.orig ; rm -rf libdotdesktop-$$VERSION.orig )

dpkg: upstream
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; TMP=/var/tmp/debian/$(PACKAGE); PV=$(PACKAGE)-$$VERSION; rm -rf $$TMP; mkdir -p $$TMP; cp ../$(PACKAGE)_$$VERSION.orig.tar.gz $$TMP; (cd $$TMP; tar xzf $(PACKAGE)_$$VERSION.orig.tar.gz ; mv $$PV.orig $$PV ); mkdir -p $$TMP/$$PV/debian; cp debian/* $$TMP/$$PV/debian; cd $$TMP/$$PV; fakeroot dpkg-buildpackage -sa ; cd ..; rm -rf $$PV)

install-program: $(SONAME)
	install -D $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	$(STRIP) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libdotdesktop.so

ipkg: clean
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/$(CONTROL) familiar/dist/CONTROL/control
	cp familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr DEBUG=no install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f $(OBJS) $(SONAME) libdotdesktop.so $(DEPS)


-include $(DEPS)

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
