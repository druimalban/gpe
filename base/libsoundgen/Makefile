PREFIX = /usr/local
DEBUG = no

MEMBERS = soundgen

ifeq ($(GTK2),yes)
SONAME = libsoundgen.so.1
CONTROL = control1
else
SONAME = libsoundgen.so.0
CONTROL = control
endif

ifeq ($(GTK2),yes)
GLIBCFLAGS = `pkg-config --cflags glib-2.0`
GLIBLDFLAGS += `pkg-config --libs glib-2.0`
else
GLIBCFLAGS = `glib-config --cflags`
GLIBLDFLAGS = `glib-config --libs`
endif

CFLAGS += -Wall $(GLIBCFLAGS)
LDFLAGS += $(GLIBLDFLAGS)
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

CPPFLAGS += -I./gpe

HEADERS = soundgen.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libsoundgen.so

libsoundgen.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

libsoundgen.a: $(OBJS)
	ar cq $@ $^

.c.s:;
	$(CC) $(CFLAGS) -S $<

upstream: clean
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; cd ..; cp -a libsoundgen libsoundgen-$$VERSION.orig; tar czf libsoundgen_$$VERSION.orig.tar.gz --exclude=libsoundgen-$$VERSION.orig/debian --exclude=libsoundgen-$$VERSION.orig/CVS --exclude=libsoundgen-$$VERSION.orig/familiar libsoundgen-$$VERSION.orig ; rm -rf libsoundgen-$$VERSION.orig )

dpkg: upstream
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; TMP=/var/tmp/debian/$(PACKAGE); PV=$(PACKAGE)-$$VERSION; rm -rf $$TMP; mkdir -p $$TMP; cp ../$(PACKAGE)_$$VERSION.orig.tar.gz $$TMP; (cd $$TMP; tar xzf $(PACKAGE)_$$VERSION.orig.tar.gz ; mv $$PV.orig $$PV ); mkdir -p $$TMP/$$PV/debian; cp debian/* $$TMP/$$PV/debian; cd $$TMP/$$PV; fakeroot dpkg-buildpackage -sa ; cd ..; rm -rf $$PV)

install: $(SONAME)
	install -D $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	strip $(DESTDIR)$(PREFIX)/lib/$(SONAME)

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libsoundgen.so

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/$(CONTROL) familiar/dist/CONTROL/control
	cp familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr DEBUG=no install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f $(OBJS) $(SONAME) libsoundgen.so $(DEPS)

include ../build/Makefile.translation

-include $(DEPS)
